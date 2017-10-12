/*
	PSP2SHELL
	Copyright (C) 2016, Cpasjuste

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/io/fcntl.h>
#include <libk/string.h>
#include <libk/stdio.h>
#include <libk/stdarg.h>
#include <taihen.h>

#include "psp2shell_k.h"

#define CHUNK_SIZE 2048
static uint8_t chunk[CHUNK_SIZE];

#define MAX_HOOKS 32
static SceUID g_hooks[MAX_HOOKS];
static tai_hook_ref_t ref_hooks[MAX_HOOKS];
static int __stdout_fd = 1073807367;

static bool ready = false;

static kp2s_msg kmsg_list[MSG_MAX];

static void kp2s_add_msg(int len, const char *msg) {

    for (int i = 0; i < MSG_MAX; i++) {
        if (kmsg_list[i].len <= 0) {
            memset(kmsg_list[i].msg, 0, P2S_MSG_LEN);
            int new_len = len > P2S_MSG_LEN ? P2S_MSG_LEN : len;
            strncpy(kmsg_list[i].msg, msg, (size_t) new_len);
            kmsg_list[i].len = new_len;
            break;
        }
    }
}

static int _kDebugPrintf(const char *fmt, ...) {

    char temp_buf[P2S_MSG_LEN];
    memset(temp_buf, 0, P2S_MSG_LEN);
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(temp_buf, P2S_MSG_LEN, fmt, args);
    va_end(args);

    if (ready) {
        kp2s_add_msg(len, temp_buf);
    }

    return TAI_CONTINUE(int, ref_hooks[2], fmt, args);
}

static int _kDebugPrintf2(int num0, int num1, const char *fmt, ...) {

    char temp_buf[P2S_MSG_LEN];
    memset(temp_buf, 0, P2S_MSG_LEN);
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(temp_buf, P2S_MSG_LEN, fmt, args);
    va_end(args);

    if (ready) {
        kp2s_add_msg(len, temp_buf);
    }

    return TAI_CONTINUE(int, ref_hooks[3], num0, num1, fmt, args);
}

static int _sceIoWrite(SceUID fd, const void *data, SceSize size) {

    if (ref_hooks[0] <= 0) {
        return 0;
    }

    if (fd == __stdout_fd && ready && size < P2S_MSG_LEN) {
        char temp_buf[size];
        memset(temp_buf, 0, size);
        ksceKernelStrncpyUserToKernel(temp_buf, (uintptr_t) data, size);
        kp2s_add_msg(size, temp_buf);
    }

    return TAI_CONTINUE(int, ref_hooks[0], fd, data, size);
}

static int _sceKernelGetStdout() {

    if (ref_hooks[1] <= 0) {
        return 0;
    }

    int fd = TAI_CONTINUE(int, ref_hooks[1]);
    __stdout_fd = fd;

    return fd;
}

SceSize kpsp2shell_wait_buffer(kp2s_msg *msg_list) {

    SceSize count = 0;
    int state = 0;

    ENTER_SYSCALL(state);

    for (int i = 0; i < MSG_MAX; i++) {
        if (kmsg_list[i].len > 0) {
            ksceKernelMemcpyKernelToUser(
                    (uintptr_t) &msg_list[count], &kmsg_list[i], sizeof(kp2s_msg));
            kmsg_list[i].len = 0;
            count++;
        }
    }

    EXIT_SYSCALL(state);

    return count;
}

void kpsp2shell_set_ready(bool rdy) {

    ready = rdy;
}

int kpsp2shell_dump(SceUID pid, const char *filename, void *addr, unsigned int size) {

    SceUID fd;
    fd = ksceIoOpen(filename, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
    if (fd <= 0) {
        return fd;
    }

    int i = 0;
    size_t bufsize = 0;

    while (i < size) {
        if (size - i > CHUNK_SIZE) {
            bufsize = CHUNK_SIZE;
        } else {
            bufsize = size - i;
        }
        ksceKernelMemcpyUserToKernelForPid(pid, chunk, (uintptr_t) (addr + i), bufsize);
        i += bufsize;

        ksceIoWrite(fd, chunk, bufsize);
    }

    ksceIoClose(fd);

    return 0;
}

int kpsp2shell_dump_module(SceUID pid, SceUID uid, const char *dst) {

    int ret;
    SceKernelModuleInfo kinfo;
    memset(&kinfo, 0, sizeof(SceKernelModuleInfo));
    kinfo.size = sizeof(SceKernelModuleInfo);

    uint32_t state;
    ENTER_SYSCALL(state);

    SceUID kid = ksceKernelKernelUidForUserUid(pid, uid);
    if (kid < 0) {
        kid = uid;
    }

    ret = ksceKernelGetModuleInfo(pid, kid, &kinfo);

    if (ret >= 0) {

        char path[128];

        for (int i = 0; i < 4; i++) {

            SceKernelSegmentInfo *seginfo = &kinfo.segments[i];

            if (seginfo->memsz <= 0 || seginfo->vaddr == NULL || seginfo->size != sizeof(*seginfo)) {
                continue;
            }

            memset(path, 0, 128);
            ksceKernelStrncpyUserToKernel(path, (uintptr_t) dst, 128);
            snprintf(path + strlen(path), 128, "/%s_0x%08X_seg%d.bin",
                     kinfo.module_name, (uintptr_t) seginfo->vaddr, i);

            _kDebugPrintf("dumping: %s\n", path);

            ret = kpsp2shell_dump(pid, path, seginfo->vaddr, seginfo->memsz);
        }
    }

    EXIT_SYSCALL(state);

    return ret;
}

int kpsp2shell_get_module_info(SceUID pid, SceUID uid, SceKernelModuleInfo *info) {

    int ret;
    SceKernelModuleInfo kinfo;
    memset(&kinfo, 0, sizeof(SceKernelModuleInfo));
    kinfo.size = sizeof(SceKernelModuleInfo);

    uint32_t state;
    ENTER_SYSCALL(state);

    SceUID kid = ksceKernelKernelUidForUserUid(pid, uid);
    if (kid < 0) {
        kid = uid;
    }

    ret = ksceKernelGetModuleInfo(pid, kid, &kinfo);
    if (ret >= 0) {
        ksceKernelMemcpyKernelToUser((uintptr_t) info, &kinfo, sizeof(SceKernelModuleInfo));
    }

    EXIT_SYSCALL(state);

    return ret;
}

int kpsp2shell_get_module_list(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num) {

    size_t count = 256;
    SceUID kmodids[256];

    uint32_t state;
    ENTER_SYSCALL(state);

    memset(kmodids, 0, sizeof(SceUID) * 256);
    int res = ksceKernelGetModuleList(pid, flags1, flags2, kmodids, &count);
    if (res >= 0) {
        ksceKernelMemcpyKernelToUser((uintptr_t) modids, &kmodids, sizeof(SceUID) * 256);
        ksceKernelMemcpyKernelToUser((uintptr_t) num, &count, sizeof(size_t));
    } else {
        ksceKernelMemcpyKernelToUser((uintptr_t) num, &count, sizeof(size_t));
    }

    EXIT_SYSCALL(state);

    return res;
}

void set_hooks() {

    uint32_t state;
    ENTER_SYSCALL(state);

    g_hooks[0] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[0],
            "SceIofilemgr",
            0xF2FF276E,
            0x34EFD876,
            _sceIoWrite);
    //LOG("hook: sceIoWrite: 0x%08X\n", g_hooks[0]);

    g_hooks[1] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[1],
            "SceProcessmgr",
            0x2DD91812,
            0xE5AA625C,
            _sceKernelGetStdout);
    //LOG("hook: sceKernelGetStdout: 0x%08X\n", g_hooks[1]);

    g_hooks[2] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[2],
            "SceSysmem",
            0x88758561, // SceDebugForDriver
            0x391B74B7, // ksceDebugPrintf
            _kDebugPrintf);
    //LOG("hook: _printf: 0x%08X\n", g_hooks[2]);

    g_hooks[3] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[3],
            "SceSysmem",
            0x88758561, // SceDebugForDriver
            0x02B04343, // ksceDebugPrintf2
            _kDebugPrintf2);
    //LOG("hook: _printf2: 0x%08X\n", g_hooks[3]);

    EXIT_SYSCALL(state);
}

void delete_hooks() {

    for (int i = 0; i < MAX_HOOKS; i++) {
        if (g_hooks[i] >= 0)
            taiHookReleaseForKernel(g_hooks[i], ref_hooks[i]);
    }
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    set_hooks();

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    delete_hooks();

    return SCE_KERNEL_STOP_SUCCESS;
}
