//
// Created by cpasjuste on 29/06/17.
//

#include <psp2kern/types.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/modulemgr.h>
#include <libk/string.h>
#include <libk/stdio.h>

#include "utility.h"
#include "psp2shell_k.h"

bool kp2s_ready = false;
#define CHUNK_SIZE 2048
static uint8_t chunk[CHUNK_SIZE];

#ifndef __USB__
volatile static int k_buf_at = 0;
volatile static int k_buf_lock = 0;
volatile static int k_buf_len = 0;
volatile static char k_buf[P2S_KMSG_SIZE] = {0};

#define kp2s_print_len(len, fmt, args...) do { \
    if(!kp2s_ready) \
        return; \
    while (k_buf_lock); \
    k_buf_lock = 1; \
    k_buf_len = snprintf((char *)k_buf+k_buf_at, P2S_KMSG_SIZE - k_buf_at, fmt, args); \
    if (len > 0) \
        k_buf_len = len; \
    if (k_buf_at + k_buf_len <= P2S_KMSG_SIZE) \
        k_buf_at += k_buf_len; \
    k_buf_lock = 0; \
} while (0)

SceSize kp2s_wait_buffer(char *buffer) {

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

#endif

void kp2s_set_ready(bool rdy) {

    kp2s_ready = rdy;
}

int kp2s_dump(SceUID pid, const char *filename, void *addr, unsigned int size) {

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

int kp2s_dump_module(SceUID pid, SceUID uid, const char *dst) {

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
#ifdef __USB__

#else
            kp2s_print("dumping: %s\n", path);
#endif

            ret = kp2s_dump(pid, path, seginfo->vaddr, seginfo->memsz);
        }
    }

    EXIT_SYSCALL(state);

    return ret;
}

int kp2s_get_module_info(SceUID pid, SceUID uid, SceKernelModuleInfo *info) {

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

int kp2s_get_module_list(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num) {

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
