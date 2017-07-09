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

#include <vitasdkkern.h>
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

static void printModuleInfoFull(SceKernelModuleInfo *moduleInfo) {

    PRINT("\nmodule_name: %s\n", moduleInfo->module_name);
    PRINT("\tpath: %s\n", moduleInfo->path);
    PRINT("\thandle: 0x%08X\n", moduleInfo->handle);
    PRINT("\tflags: 0x%08X\n", moduleInfo->flags);
    PRINT("\tmodule_start: 0x%08X\n", moduleInfo->module_start);
    PRINT("\tmodule_stop: 0x%08X\n", moduleInfo->module_stop);
    PRINT("\texidxTop: 0x%08X\n", moduleInfo->exidxTop);
    PRINT("\texidxBtm: 0x%08X\n", moduleInfo->exidxBtm);
    PRINT("\ttlsInit: 0x%08X\n", moduleInfo->tlsInit);
    PRINT("\ttlsInitSize: 0x%08X\n", moduleInfo->tlsInitSize);
    PRINT("\ttlsAreaSize: 0x%08X\n", moduleInfo->tlsAreaSize);
    PRINT("\ttype: %i\n", moduleInfo->type);
    PRINT("\tunk28: 0x%08X\n", moduleInfo->unk28);
    PRINT("\tunk30: 0x%08X\n", moduleInfo->unk30);
    PRINT("\tunk40: 0x%08X\n", moduleInfo->unk40);
    PRINT("\tunk44: 0x%08X\n", moduleInfo->unk44);
    for (int i = 0; i < 4; ++i) {
        if (moduleInfo->segments[i].memsz <= 0) {
            continue;
        }
        PRINT("\tsegment[%i].perms: 0x%08X\n", i, moduleInfo->segments[i].perms);
        PRINT("\tsegment[%i].vaddr: 0x%08X\n", i, moduleInfo->segments[i].vaddr);
        PRINT("\tsegment[%i].memsz: 0x%08X\n", i, moduleInfo->segments[i].memsz);
        PRINT("\tsegment[%i].flags: 0x%08X\n", i, moduleInfo->segments[i].flags);
        PRINT("\tsegment[%i].res: %i\n", i, moduleInfo->segments[i].res);
    }
}

int kp2s_print_module_info(SceUID pid, SceUID uid) {

    uint32_t state;
    ENTER_SYSCALL(state);

    SceKernelModuleInfo info;
    memset(&info, 0, sizeof(SceKernelModuleInfo));
    info.size = sizeof(SceKernelModuleInfo);
    SceUID kuid = ksceKernelKernelUidForUserUid(pid, uid);
    if (kuid < 0) {
        kuid = uid;
    }

    int res = ksceKernelGetModuleInfo(pid, kuid, &info);
    if (res >= 0) {
        printModuleInfoFull(&info);
    } else {
        PRINT_ERR("getting module info failed: 0x%08X", res);
    }

    EXIT_SYSCALL(state);

    return res;
}

int kp2s_print_module_list(SceUID pid, int flags1, int flags2) {

    uint32_t state;
    ENTER_SYSCALL(state);

    size_t count = 256;
    SceUID modids[256];
    memset(modids, 0, sizeof(SceUID) * 256);

    int res = ksceKernelGetModuleList(pid, flags1, flags2, modids, &count);
    if (res >= 0) {
        PRINT("\n");
        SceKernelModuleInfo info;
        for (int i = 0; i < count; i++) {
            memset(&info, 0, sizeof(SceKernelModuleInfo));
            info.size = sizeof(SceKernelModuleInfo);
            res = ksceKernelGetModuleInfo(pid, modids[i], &info);
            if (res == 0) {
                PRINT("\t%s (uid: 0x%08X)\n",
                      info.module_name, info.handle);
            }
        }
    } else {
        PRINT_ERR("module list failed: 0x%08X", res);
    }

    EXIT_SYSCALL(state);

    return res;
}
