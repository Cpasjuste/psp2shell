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

#ifndef __KERNEL__
#include "p2s_module.h"
#include "../psp2shell_m/include/p2s_utility.h"
#endif

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

    int res = kp2s_print_module_info(pid, uid);

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

    int res = kp2s_print_module_list(pid, 0xFF, 1);

    return res;
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
