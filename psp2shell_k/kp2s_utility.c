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

#include "psp2shell_k.h"

bool kp2s_ready = false;
#define CHUNK_SIZE 2048
static uint8_t chunk[CHUNK_SIZE];

int kp2s_has_slash(const char *path) {
    return path[strlen(path) - 1] == '/';
}

int kp2s_add_slash(char *path) {
    int len = strlen(path);
    if (len < MAX_PATH_LENGTH - 2) {
        if (path[len - 1] != '/') {
            strcat(path, "/");
            return 1;
        }
    }

    return 0;
}

int kp2s_remove_slash(char *path) {

    int len = strlen(path);

    if (path[len - 1] == '/') {
        path[len - 1] = '\0';
        return 1;
    }

    return 0;
}

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

int kp2s_get_module_info(bool userMode, SceUID pid, SceUID uid, SceKernelModuleInfo *info) {

    int ret;
    SceKernelModuleInfo kinfo;
    memset(&kinfo, 0, sizeof(SceKernelModuleInfo));
    kinfo.size = sizeof(SceKernelModuleInfo);

    if (userMode) {
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
    } else {
        ret = ksceKernelGetModuleInfo(pid, pid, info);
    }

    return ret;
}

int kp2s_get_module_list(bool userMode, SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num) {

    int res;

    if (userMode) {
        size_t count = 256;
        SceUID kmodids[256];
        uint32_t state;
        ENTER_SYSCALL(state);
        memset(kmodids, 0, sizeof(SceUID) * 256);
        res = ksceKernelGetModuleList(pid, flags1, flags2, kmodids, &count);
        if (res >= 0) {
            ksceKernelMemcpyKernelToUser((uintptr_t) modids, &kmodids, sizeof(SceUID) * 256);
            ksceKernelMemcpyKernelToUser((uintptr_t) num, &count, sizeof(size_t));
        } else {
            ksceKernelMemcpyKernelToUser((uintptr_t) num, &count, sizeof(size_t));
        }
        EXIT_SYSCALL(state);
    } else {
        res = ksceKernelGetModuleList(pid, flags1, flags2, modids, num);
    }

    return res;
}

