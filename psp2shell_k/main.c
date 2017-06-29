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
#include "hooks.h"

#define CHUNK_SIZE 2048
static uint8_t chunk[CHUNK_SIZE];

volatile static int k_buf_at = 0;
volatile static int k_buf_lock = 0;
volatile static int k_buf_len = 0;
volatile static char k_buf[P2S_KMSG_SIZE] = {0};

#define p2s_print_len(len, fmt, args...) do { \
  while (k_buf_lock); \
  k_buf_lock = 1; \
  k_buf_len = snprintf((char *)k_buf+k_buf_at, P2S_KMSG_SIZE - k_buf_at, fmt, args); \
  if (len > 0) \
    k_buf_len = len; \
  if (k_buf_at + k_buf_len <= P2S_KMSG_SIZE) \
    k_buf_at += k_buf_len; \
  k_buf_lock = 0; \
} while (0)

#define p2s_print(fmt, args...) p2s_print_len(0, fmt, args)

SceSize kpsp2shell_wait_buffer(char *buffer) {

    if (k_buf_at <= 0) {
        return 0;
    }

    int state = 0;
    int count = 0;

    ENTER_SYSCALL(state);

    while (k_buf_lock);
    k_buf_lock = 1;

    count = k_buf_at;
    ksceKernelStrncpyKernelToUser((uintptr_t) buffer, (char *) k_buf, k_buf_at + 1);
    k_buf_at = 0;
    k_buf_lock = 0;

    EXIT_SYSCALL(state);

    return (SceSize) count;
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

            p2s_print("dumping: %s\n", path);

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

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    set_hooks();

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    delete_hooks();

    return SCE_KERNEL_STOP_SUCCESS;
}
