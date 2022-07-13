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

#include <string.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include <psp2kern/kernel/sysmem.h>
//#include <psp2kern/appmgr.h>
#include <taihen.h>
//#include <psp2/appmgr.h>

#include "psp2shell_k.h"

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);

#define get_export(modname, lib_nid, func_nid, func) \
    module_get_export_func(KERNEL_PID, modname, lib_nid, func_nid, (uintptr_t *)func)

#define SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW 0x1020D006

int (*sceKernelSysrootGetShellPidForDriver)(void);

int (*sceAppMgrGetRunningAppIdListForShell)(SceInt32 *appIds, int count);

SceUID (*sceAppMgrGetProcessIdByAppIdForShell)(SceInt32 appId);

void *p2s_malloc(size_t size);

void p2s_free(void *p);

SceUID p2s_get_running_app_pid() {
#ifdef __VITA_KERNEL__TEMP
    get_export("SceSysmem", 0x2ED7F97A, 0x05093E7B, &sceKernelSysrootGetShellPidForDriver);
    SceUID pid = sceKernelSysrootGetShellPidForDriver();
    printf("sceKernelSysrootGetShellPidForDriver: 0x%08X\n", pid);
    return pid;
#else
    SceUID pid = -1;
    SceUID ids[20];

    get_export("SceAppMgr", 0x8AF17416, 0xDA66AE0E, &sceAppMgrGetRunningAppIdListForShell);
    //get_export("SceAppMgrUser", 0xA6605D6F, 0x63FAC2A9, &sceAppMgrGetProcessIdByAppIdForShell);
    //ksceKernelStrncpyKernelToUser(temp_buf, data, size);

    int count = sceAppMgrGetRunningAppIdListForShell(ids, 20);
    printf("sceAppMgrGetRunningAppIdListForShell: %i\n", count);
    if (count > 0) {
        //pid = sceAppMgrGetProcessIdByAppIdForShell(ids[0]);
    }

    return pid;
#endif
}

SceUID p2s_get_running_app_id() {
#ifndef __VITA_KERNEL__
    SceUID ids[20];

    int count = sceAppMgrGetRunningAppIdListForShell(ids, 20);
    if (count > 0) {
        return ids[0];
    }
#endif
    return 0;
}

int p2s_get_running_app_name(char *name) {
    int ret = -1;

#ifndef __VITA_KERNEL__
    SceUID pid = p2s_get_running_app_pid();
    if (pid > 0) {
        ret = sceAppMgrAppParamGetString(pid, 9, name, 256);
        return ret;
    }
#endif

    return ret;
}

int p2s_get_running_app_title_id(char *title_id) {
    int ret = -1;

#ifndef __VITA_KERNEL__
    SceUID pid = p2s_get_running_app_pid();
    if (pid > 0) {
        ret = sceAppMgrAppParamGetString(pid, 12, title_id, 256);
        return ret;
    }
#endif

    return ret;
}

int p2s_launch_app_by_uri(const char *tid) {
#ifndef __VITA_KERNEL__
    char uri[32];

    sceAppMgrDestroyOtherApp();
    sceKernelDelayThread(1000 * 1000);

    snprintf(uri, 32, "psgm:play?titleid=%s", tid);

    for (int i = 0; i < 40; i++) {
        if (sceAppMgrLaunchAppByUri(0xFFFFF, uri) != 0) {
            break;
        }
        sceKernelDelayThread(10000);
    }
#endif

    return 0;
}

int p2s_reset_running_app() {
#ifndef __VITA_KERNEL__
    char name[256];
    char id[16];
    char uri[32];

    if (p2s_get_running_app_title_id(id) != 0) {
        return -1;
    }

    if (p2s_get_running_app_name(name) != 0) {
        return -1;
    }

    sceAppMgrDestroyOtherApp();
    sceKernelDelayThread(1000 * 1000);

    snprintf(uri, 32, "psgm:play?titleid=%s", id);

    for (int i = 0; i < 40; i++) {
        sceAppMgrLaunchAppByUri(0xFFFFF, uri);
        sceKernelDelayThread(10000);
    }
#endif

    return 0;
}


int p2s_hasEndSlash(char *path) {
    return path[strlen(path) - 1] == '/';
}

int p2s_removeEndSlash(char *path) {
    size_t len = strlen(path);
    if (path[len - 1] == '/') {
        path[len - 1] = '\0';
        return 1;
    }

    return 0;
}

int p2s_addEndSlash(char *path) {
    size_t len = strlen(path);
    if (len < MAX_PATH_LENGTH - 2) {
        if (path[len - 1] != '/') {
            strcat(path, "/");
            return 1;
        }
    }

    return 0;
}

void p2s_log_write(const char *msg) {
    ksceIoMkdir("ux0:/tai/", 6);
    SceUID fd = ksceIoOpen("ux0:/tai/psp2shell.log", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
    if (fd < 0) return;
    ksceIoWrite(fd, msg, strlen(msg));
    ksceIoClose(fd);
}

void *p2s_malloc(size_t size) {
    void *p = NULL;
    SceUID uid = ksceKernelAllocMemBlock(
            "p2s", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, (size + 0xFFF) & (~0xFFF), 0);
    if (uid >= 0) {
        ksceKernelGetMemBlockBase(uid, &p);
    }

    return p;
}

void p2s_free(void *p) {
    SceUID uid = ksceKernelFindMemBlockByAddr(p, 1);
    if (uid >= 0) {
        ksceKernelFreeMemBlock(uid);
    }
}
