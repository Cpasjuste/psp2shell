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

#include <libk/string.h>
#include <libk/stdio.h>
#include <libk/stdarg.h>
#include <vitasdkkern.h>
#include <taihen.h>

#include "psp2shell_k.h"

volatile static int at = 0;
volatile static int lock = 0;
volatile static char k_buf[P2S_KMSG_SIZE] = {0};

#define P2S_MSG_LEN 256

#define p2s_print_len(len, fmt, args...) do { \
  while (lock); \
  lock = 1; \
  snprintf((char *)k_buf+at, P2S_KMSG_SIZE-at, fmt, args); \
  if (at + len <= P2S_KMSG_SIZE) \
    at += len; \
  lock = 0; \
} while (0)

#define p2s_print(fmt, args...) p2s_print_len(strlen(fmt), fmt, args)

#define MAX_HOOKS 32
static SceUID g_hooks[MAX_HOOKS];
static tai_hook_ref_t ref_hooks[MAX_HOOKS];
static int __stdout_fd = 1073807367;

static bool ready = false;

void set_hooks();

void delete_hooks();

static int _kDebugPrintf(const char *fmt, ...) {

    char temp_buf[P2S_MSG_LEN];
    memset(temp_buf, 0, P2S_MSG_LEN);
    va_list args;
    va_start(args, fmt);
    vsnprintf(temp_buf, P2S_MSG_LEN, fmt, args);
    va_end(args);

    if (ready) {
        p2s_print("%s", temp_buf);
    }

    return TAI_CONTINUE(int, ref_hooks[2], fmt, args);
}

static int _kDebugPrintf2(int num0, int num1, const char *fmt, ...) {

    char temp_buf[P2S_MSG_LEN];
    memset(temp_buf, 0, P2S_MSG_LEN);
    va_list args;
    va_start(args, fmt);
    vsnprintf(temp_buf, P2S_MSG_LEN, fmt, args);
    va_end(args);

    if (ready) {
        p2s_print("%s", temp_buf);
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
        p2s_print_len(size, "%s", temp_buf);
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

SceSize kpsp2shell_wait_buffer(char *buffer) {

    int state = 0;
    int count = 0;

    ENTER_SYSCALL(state);

    while (lock);
    lock = 1;

    count = at;
    ksceKernelStrncpyKernelToUser((uintptr_t) buffer, (char *) k_buf, at + 1);
    at = 0;
    lock = 0;

    EXIT_SYSCALL(state);

    return (SceSize) count;
}

void kpsp2shell_set_ready(bool rdy) {

    ready = rdy;
}

int kpsp2shell_get_module_info(SceUID pid, SceUID uid, SceKernelModuleInfo *info) {

    int ret = -1;
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

/*
int unload_allowed_patched(void) {
    TAI_CONTINUE(int, ref_hooks[4]);
    return 1; // always allowed
}
*/

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
            0x391B74B7, // printf
            _kDebugPrintf);
    //LOG("hook: _printf: 0x%08X\n", g_hooks[2]);

    g_hooks[3] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[3],
            "SceSysmem",
            0x88758561, // SceDebugForDriver
            0x02B04343, // printf2
            _kDebugPrintf2);
    //LOG("hook: _printf2: 0x%08X\n", g_hooks[3]);

    /*
    g_hooks[4] = taiHookFunctionImportForKernel(
            KERNEL_PID,
            &ref_hooks[4],     // Output a reference
            "SceKernelModulemgr",
            0x11F9B314,
            0xBBA13D9C,
            unload_allowed_patched);
    */

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

    //u_mutex = ksceKernelCreateSema("u_mutex", 0, 0, 1, NULL);

    set_hooks();

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    //ksceKernelDeleteMutex(u_mutex);

    delete_hooks();

    return SCE_KERNEL_STOP_SUCCESS;
}
