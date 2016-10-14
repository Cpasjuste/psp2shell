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

#include <psp2/kernel/modulemgr.h>
#include <stdlib.h>
#include "psp2shell.h"
#include "module.h"

#define printf psp2shell_print

static void printModuleInfo(SceKernelModuleInfo *moduleInfo) {
    int i;

    psp2shell_print_color(COL_GREEN, "module_name: %s\n", moduleInfo->module_name);
    printf("\tpath: %s\n", moduleInfo->path);
    printf("\thandle: 0x%08X\n", moduleInfo->handle);
    printf("\tflags: 0x%08X\n", moduleInfo->flags);
    printf("\tmodule_start: 0x%08X\n", moduleInfo->module_start);
    printf("\tmodule_stop: 0x%08X\n", moduleInfo->module_stop);
    printf("\texidxTop: 0x%08X\n", moduleInfo->exidxTop);
    printf("\texidxBtm: 0x%08X\n", moduleInfo->exidxBtm);
    printf("\ttlsInit: 0x%08X\n", moduleInfo->tlsInit);
    printf("\ttlsInitSize: 0x%08X\n", moduleInfo->tlsInitSize);
    printf("\ttlsAreaSize: 0x%08X\n", moduleInfo->tlsAreaSize);
    printf("\ttype: %i\n", moduleInfo->type);
    printf("\tunk28: 0x%08X\n", moduleInfo->unk28);
    printf("\tunk30: 0x%08X\n", moduleInfo->unk30);
    printf("\tunk40: 0x%08X\n", moduleInfo->unk40);
    printf("\tunk44: 0x%08X\n", moduleInfo->unk44);
    for (i = 0; i < 4; ++i) {
        if (moduleInfo->segments[i].memsz <= 0) {
            continue;
        }
        printf("\tsegment[%i].perms: 0x%08X\n", i, moduleInfo->segments[i].perms);
        printf("\tsegment[%i].vaddr: 0x%08X\n", i, moduleInfo->segments[i].vaddr);
        printf("\tsegment[%i].memsz: 0x%08X\n", i, moduleInfo->segments[i].memsz);
        printf("\tsegment[%i].flags: 0x%08X\n", i, moduleInfo->segments[i].flags);
        printf("\tsegment[%i].res: %i\n", i, moduleInfo->segments[i].res);
    }
    printf("\n\n");
}

int ps_moduleList() {

    SceUID ids[256];
    int count = 256, i;

    int res = sceKernelGetModuleList(0xFF, ids, &count);
    if (res != 0) {
        psp2shell_print_color(COL_RED, "modls failed: %i\n", res);
        return res;
    } else {
        printf("\tmodules count: %i\n\n", count);

        SceKernelModuleInfo *moduleInfo;
        moduleInfo = malloc(count * sizeof(SceKernelModuleInfo));

        for (i = 0; i < count; i++) {
            moduleInfo[i].size = sizeof(SceKernelModuleInfo);
            res = sceKernelGetModuleInfo(ids[i], &moduleInfo[i]);
            if (res != 0) {
                printf("getting modinfo of %i failed\n", ids[i]);
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
        return res;
    }
}
