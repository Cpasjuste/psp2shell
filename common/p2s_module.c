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

int p2s_moduleInfo(SceUID uid) {

#ifdef __KERNEL__
    PRINT_ERR("use modinfop (provide pid)");
    return 0;
#else
    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleInfoForPid(pid, uid);
#endif
}

int p2s_moduleInfoForPid(SceUID pid, SceUID uid) {

    SceKernelModuleInfo moduleInfo;
    memset(&moduleInfo, 0, sizeof(SceKernelModuleInfo));
    moduleInfo.size = sizeof(SceKernelModuleInfo);

#ifdef __KERNEL__
    int res = kp2s_get_module_info(false, pid, uid, &moduleInfo);
#else
    int res = kp2s_get_module_info(true, pid, uid, &moduleInfo);
#endif
    if (res == 0) {
        printModuleInfoFull(&moduleInfo);
    } else {
        PRINT_ERR("getting module info failed: 0x%08X", res);
    }

    return res;
}

int p2s_moduleList() {

#ifdef __KERNEL__
    PRINT_ERR("use modlistp (provide pid), or launch an application");
    return 0;
#else
    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleListForPid(pid);
#endif
}

int p2s_moduleListForPid(SceUID pid) {

    SceUID ids[256];
    size_t count = 256;

#ifdef __KERNEL__
    int res = kp2s_get_module_list(false, pid, 0xFF, 1, ids, &count);
#else
    int res = kp2s_get_module_list(true, pid, 0xFF, 1, ids, &count);
#endif
    if (res != 0) {
        PRINT_ERR("module list failed: 0x%08X", res);
        return res;
    } else {
        PRINT("kp2s_get_module_list: module count=%i\n", count);
        PRINT("\n");
        SceKernelModuleInfo moduleInfo;
        for (int i = 0; i < count; i++) {
            if (ids[i] > 0) {
                memset(&moduleInfo, 0, sizeof(SceKernelModuleInfo));
#ifdef __KERNEL__
                res = kp2s_get_module_info(false, pid, ids[i], &moduleInfo);
#else
                res = kp2s_get_module_info(true, pid, ids[i], &moduleInfo);
#endif
                if (res == 0) {
                    PRINT("\t%s (uid: 0x%08X)\n",
                          moduleInfo.module_name, moduleInfo.handle);
                }
            }
        }
    }

    return 0;
}

SceUID p2s_moduleLoadStart(char *modulePath) {

#ifdef __KERNEL__
    PRINT_ERR("use modloadp (provide pid), or launch an application");
    return 0;
#else
    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleLoadStartForPid(pid, modulePath);
#endif
}

SceUID p2s_moduleLoadStartForPid(SceUID pid, char *modulePath) {

    SceUID uid = taiLoadStartModuleForPid(pid, modulePath, 0, NULL, 0);
    if (uid < 0) {
        PRINT_ERR("module load/start failed: 0x%08X", uid);
    } else {
        PRINT_COL(COL_GREEN, "module loaded/started: uid = 0x%08X\n", uid);
    }

    return uid;
}

int p2s_moduleStopUnload(SceUID uid) {

#ifdef __KERNEL__
    PRINT_ERR("use modstopp(provide pid), or launch an application");
    return 0;
#else
    SceUID pid = p2s_get_running_app_pid();
    if (pid < 0) {
        pid = sceKernelGetProcessId();
    }

    return p2s_moduleStopUnloadForPid(pid, uid);
#endif
}

int p2s_moduleStopUnloadForPid(SceUID pid, SceUID uid) {

    int status;

    int res = taiStopUnloadModuleForPid(pid, uid, 0, NULL, 0, NULL, &status);
    if (res != 0) {
        PRINT_ERR("module stop/unload failed: 0x%08X", status);
    } else {
        PRINT_COL(COL_GREEN, "module stopped/unloaded\n");
    }

    return res;
}

SceUID p2s_kmoduleLoadStart(char *modulePath) {

    SceUID uid = taiLoadStartKernelModule(modulePath, 0, NULL, 0);
    if (uid < 0) {
        PRINT_ERR("module load/start failed: 0x%08X", uid);
    } else {
        PRINT_COL(COL_GREEN, "module loaded/started: uid = 0x%08X\n", uid);
    }

    return uid;
}

int p2s_kmoduleStopUnload(SceUID uid) {

    int status;

    int res = taiStopUnloadKernelModule(uid, 0, NULL, 0, NULL, &status);
    if (res != 0) {
        PRINT_ERR("module stop/unload failed: 0x%08X", status);
    } else {
        PRINT_COL(COL_GREEN, "module stopped/unloaded\n");
    }

    return res;
}

int p2s_moduleDumpForPid(SceUID pid, SceUID uid, const char *dst) {

    int res = kp2s_dump_module(pid, uid, dst);
    if (res != 0) {
        PRINT_ERR("module dump failed: 0x%08X", res);
    } else {
        PRINT_COL(COL_GREEN, "module dump success\n");
    }

    return res;
}
