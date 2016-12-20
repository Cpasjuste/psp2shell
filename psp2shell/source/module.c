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

#ifndef __VITA_KERNEL__
#include <psp2/kernel/modulemgr.h>
#include "psp2shell.h"
#include "module.h"
#ifdef MODULE
#include "libmodule.h"
#else
#include <stdlib.h>
#endif

static void printModuleInfo(SceKernelModuleInfo *moduleInfo) {
    int i;

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
    for (i = 0; i < 4; ++i) {
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

int ps_moduleList() {

    SceUID ids[256];
    int count = 256, i;

    int res = sceKernelGetModuleList(0xFF, ids, &count);
    if (res != 0) {
        psp2shell_print_color(COL_RED, "modls failed: %i\n", res);
        return res;
    } else {
        psp2shell_print("\tmodules count: %i\n\n", count);

        SceKernelModuleInfo *moduleInfo;
        moduleInfo = malloc(count * sizeof(SceKernelModuleInfo));

        for (i = 0; i < count; i++) {
            moduleInfo[i].size = sizeof(SceKernelModuleInfo);
            res = sceKernelGetModuleInfo(ids[i], &moduleInfo[i]);
            if (res != 0) {
                psp2shell_print_color(COL_RED, "getting modinfo of %i failed\n", ids[i]);
                continue;
            } else {
                printModuleInfo(&moduleInfo[i]);
            }
        }

        free(moduleInfo);
    }
    return 0;
}

int ps_moduleLoadStart(char *modulePath) {

    int res = sceKernelLoadStartModule(modulePath, 0, NULL, 0, 0, 0);
    if (res != 0) {
        psp2shell_print_color(COL_RED, "modld failed: %i\n", res);
    }
    return res;
}

#endif // __VITA_KERNEL__