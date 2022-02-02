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

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <taihen.h>

#include "../include/utility.h"
#include "../include/psp2shell.h"
#include "../include/module.h"
#include "../include/libmodule.h"

#ifndef DEBUG
#include "../../psp2shell_k/psp2shell_k.h"
#endif

static void printModuleInfoFull(SceKernelModuleInfo *moduleInfo) {
    PRINT_OK("module_name: %s\n", moduleInfo->module_name);
    PRINT_OK("\tpath: %s\n", moduleInfo->path);
    PRINT_OK("\thandle: 0x%08X\n", moduleInfo->modid);
    //PRINT_OK("\tflags: 0x%08X\n", moduleInfo->flags);
    PRINT_OK("\tmodule_start: 0x%08X\n", moduleInfo->start_entry);
    PRINT_OK("\tmodule_stop: 0x%08X\n", moduleInfo->stop_entry);
    PRINT_OK("\texidxTop: 0x%08X\n", moduleInfo->exidx_top);
    PRINT_OK("\texidxBtm: 0x%08X\n", moduleInfo->exidx_btm);
    PRINT_OK("\ttlsInit: 0x%08X\n", moduleInfo->tlsInit);
    PRINT_OK("\ttlsInitSize: 0x%08X\n", moduleInfo->tlsInitSize);
    PRINT_OK("\ttlsAreaSize: 0x%08X\n", moduleInfo->tlsAreaSize);
    // TODO: update
    //PRINT_OK("\ttype: %i\n", moduleInfo->type);
    //PRINT_OK("\tunk28: 0x%08X\n", moduleInfo->unk28);
    //PRINT_OK("\tunk30: 0x%08X\n", moduleInfo->unk30);
    //PRINT_OK("\tunk40: 0x%08X\n", moduleInfo->unk40);
    //PRINT_OK("\tunk44: 0x%08X\n", moduleInfo->unk44);
    for (int i = 0; i < 4; ++i) {
        if (moduleInfo->segments[i].memsz <= 0) {
            continue;
        }
        PRINT_OK("\tsegment[%i].perms: 0x%08X\n", i, moduleInfo->segments[i].perms);
        PRINT_OK("\tsegment[%i].vaddr: 0x%08X\n", i, moduleInfo->segments[i].vaddr);
        PRINT_OK("\tsegment[%i].memsz: 0x%08X\n", i, moduleInfo->segments[i].memsz);
        //PRINT_OK("\tsegment[%i].flags: 0x%08X\n", i, moduleInfo->segments[i].flags);
        PRINT_OK("\tsegment[%i].res: %i\n", i, moduleInfo->segments[i].res);
    }
    PRINT_OK("\n\n");
}

int p2s_moduleInfo(SceUID uid) {

    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleInfoForPid(pid, uid);
}

int p2s_moduleInfoForPid(SceUID pid, SceUID uid) {

    SceKernelModuleInfo moduleInfo;
    memset(&moduleInfo, 0, sizeof(SceKernelModuleInfo));
    moduleInfo.size = sizeof(SceKernelModuleInfo);

#ifdef DEBUG
    int res = sceKernelGetModuleInfo(uid, &moduleInfo);
#else
    int res = kpsp2shell_get_module_info(pid, uid, &moduleInfo);
#endif
    if (res == 0) {
        printModuleInfoFull(&moduleInfo);
    } else {
        PRINT_ERR("\ngetting module info failed: 0x%08X\n", res);
    }

    return res;
}

int p2s_moduleList() {

    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleListForPid(pid);
}

int p2s_moduleListForPid(SceUID pid) {

    SceUID ids[256];
    SceSize count = 256;

#ifdef DEBUG
    int res = sceKernelGetModuleList(0xFF, ids, &count);
#else
    int res = kpsp2shell_get_module_list(pid, 0xFF, 1, ids, &count);
#endif
    if (res != 0) {
        PRINT_ERR("module list failed: 0x%08X\n", res);
        return res;
    } else {
        SceKernelModuleInfo moduleInfo;
        for (int i = 0; i < count; i++) {
            if (ids[i] > 0) {
                memset(&moduleInfo, 0, sizeof(SceKernelModuleInfo));
#ifdef DEBUG
                moduleInfo.size = sizeof(SceKernelModuleInfo);
                res = sceKernelGetModuleInfo(ids[i], &moduleInfo);
#else
                res = kpsp2shell_get_module_info(pid, ids[i], &moduleInfo);
#endif
                if (res == 0) {
                    PRINT_OK("%s (uid: 0x%08X)\n",
                             moduleInfo.module_name, moduleInfo.modid);
                }
            }
        }
    }

    return 0;
}

SceUID p2s_moduleLoadStart(char *modulePath) {

    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleLoadStartForPid(pid, modulePath);
}

SceUID p2s_moduleLoadStartForPid(SceUID pid, char *modulePath) {

    SceUID uid = taiLoadStartModuleForPid(pid, modulePath, 0, NULL, 0);
    if (uid < 0) {
        PRINT_ERR("\nmodule load/start failed: 0x%08X\n\n", uid);
    } else {
        PRINT_OK("\nmodule loaded/started: uid = 0x%08X\n\n", uid);
    }

    return uid;
}

int p2s_moduleStopUnload(SceUID uid) {

    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleStopUnloadForPid(pid, uid);
}

int p2s_moduleStopUnloadForPid(SceUID pid, SceUID uid) {

    int status;

    int res = taiStopUnloadModuleForPid(pid, uid, 0, NULL, 0, NULL, &status);
    if (res != 0) {
        PRINT_ERR("\nmodule stop/unload failed: 0x%08X\n\n", status);
    } else {
        PRINT_OK("\nmodule stopped/unloaded\n\n");
    }

    return res;
}

SceUID p2s_kmoduleLoadStart(char *modulePath) {

    SceUID uid = taiLoadStartKernelModule(modulePath, 0, NULL, 0);
    if (uid < 0) {
        PRINT_ERR("\nmodule load/start failed: 0x%08X\n\n", uid);
    } else {
        PRINT_OK("\nmodule loaded/started: uid = 0x%08X\n\n", uid);
    }

    return uid;
}

int p2s_kmoduleStopUnload(SceUID uid) {

    int status;

    int res = taiStopUnloadKernelModule(uid, 0, NULL, 0, NULL, &status);
    if (res != 0) {
        PRINT_ERR("\nmodule stop/unload failed: 0x%08X\n\n", status);
    } else {
        PRINT_OK("\nmodule stopped/unloaded\n\n");
    }

    return res;
}

int p2s_moduleDumpForPid(SceUID pid, SceUID uid, const char *dst) {

#ifdef DEBUG
    PRINT_ERR("\nmodule dump not enabled\n\n");
    return -1;
#else
    int res = kpsp2shell_dump_module(pid, uid, dst);
    if (res != 0) {
        PRINT_ERR("\nmodule dump failed: 0x%08X\n\n", res);
    } else {
        PRINT_OK("\nmodule dump success\n\n");
    }

    return res;
#endif
}
