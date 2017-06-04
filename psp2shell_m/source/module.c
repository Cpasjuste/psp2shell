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

#include "utility.h"
#include "psp2shell.h"
#include "module.h"
#include "libmodule.h"
#include "../../psp2shell_k/psp2shell_k.h"

static void printModuleInfoFull(SceKernelModuleInfo *moduleInfo) {

    psp2shell_print_color(COL_GREEN, "module_name: %s\n", moduleInfo->module_name);
    psp2shell_print("\tpath: %s\n", moduleInfo->path);
    psp2shell_print("\thandle: 0x%08X\n", moduleInfo->handle);
    psp2shell_print("\tflags: 0x%08X\n", moduleInfo->flags);
    psp2shell_print("\tmodule_start: 0x%08X\n", moduleInfo->module_start);
    psp2shell_print("\tmodule_stop: 0x%08X\n", moduleInfo->module_stop);
    psp2shell_print("\texidxTop: 0x%08X\n", moduleInfo->exidxTop);
    psp2shell_print("\texidxBtm: 0x%08X\n", moduleInfo->exidxBtm);
    psp2shell_print("\ttlsInit: 0x%08X\n", moduleInfo->tlsInit);
    psp2shell_print("\ttlsInitSize: 0x%08X\n", moduleInfo->tlsInitSize);
    psp2shell_print("\ttlsAreaSize: 0x%08X\n", moduleInfo->tlsAreaSize);
    psp2shell_print("\ttype: %i\n", moduleInfo->type);
    psp2shell_print("\tunk28: 0x%08X\n", moduleInfo->unk28);
    psp2shell_print("\tunk30: 0x%08X\n", moduleInfo->unk30);
    psp2shell_print("\tunk40: 0x%08X\n", moduleInfo->unk40);
    psp2shell_print("\tunk44: 0x%08X\n", moduleInfo->unk44);
    for (int i = 0; i < 4; ++i) {
        if (moduleInfo->segments[i].memsz <= 0) {
            continue;
        }
        psp2shell_print("\tsegment[%i].perms: 0x%08X\n", i, moduleInfo->segments[i].perms);
        psp2shell_print("\tsegment[%i].vaddr: 0x%08X\n", i, moduleInfo->segments[i].vaddr);
        psp2shell_print("\tsegment[%i].memsz: 0x%08X\n", i, moduleInfo->segments[i].memsz);
        psp2shell_print("\tsegment[%i].flags: 0x%08X\n", i, moduleInfo->segments[i].flags);
        psp2shell_print("\tsegment[%i].res: %i\n", i, moduleInfo->segments[i].res);
    }
    psp2shell_print("\n\n");
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

    int res = kpsp2shell_get_module_info(pid, uid, &moduleInfo);
    if (res == 0) {
        printModuleInfoFull(&moduleInfo);
    } else {
        psp2shell_print_color(COL_RED, "getting module info failed: 0x%08X\n", res);
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
    size_t count = 256;

    int res = kpsp2shell_get_module_list(pid, 0xFF, 1, ids, &count);
    if (res != 0) {
        psp2shell_print_color(COL_RED, "module list failed: 0x%08X\n", res);
        return res;
    } else {
        SceKernelModuleInfo moduleInfo;
        for (int i = 0; i < count; i++) {
            if (ids[i] > 0) {
                memset(&moduleInfo, 0, sizeof(SceKernelModuleInfo));
                res = kpsp2shell_get_module_info(pid, ids[i], &moduleInfo);
                if (res == 0) {
                    psp2shell_print_color(COL_GREEN, "%s\t\t\tuid: 0x%08X\n",
                                          moduleInfo.module_name, moduleInfo.handle);
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
        psp2shell_print_color(COL_RED, "module load/start failed: 0x%08X\n", uid);
    } else {
        psp2shell_print_color(COL_GREEN, "module loaded/started: uid = 0x%08X\n", uid);
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
        psp2shell_print_color(COL_RED, "module stop/unload failed: 0x%08X\n", status);
    } else {
        psp2shell_print_color(COL_GREEN, "module stopped/unloaded\n");
    }

    return res;
}
