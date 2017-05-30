//
// Created by cpasjuste on 09/11/16.
//
#ifdef MODULE

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <taihen.h>
#include "psp2shell.h"
#include "libmodule.h"
#include "hooks.h"

/*
extern void _psp2shell_print_color(SceSize size, int color, const char *fmt, ...);

static char buffer[256];

#define HOOK_MAX 32
static SceUID sceClibPrintf_uid[HOOK_MAX];
static tai_hook_ref_t sceClibPrintf_ref[HOOK_MAX];

static tai_hook_ref_t module_start_ref;
SceUID module_start_uid;

int _sceClibPrintf(const char *fmt, ...) {

    memset(buffer, 0, 256);
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, 256, fmt, args);
    va_end(args);
    _psp2shell_print_color(256, 0, buffer);

    return 0;
    //return TAI_CONTINUE(int, sceClibPrintf_ref, fmt, args);
}

int module_start_hook(SceSize argc, const void *args) {

    // hook all user modules import (sceClibPrintf)

    SceUID ids[256];
    int mod_count = 256;
    int hook_count = 0;
    SceKernelModuleInfo moduleInfo;

    int res = sceKernelGetModuleList(0xFF, ids, &mod_count);
    if (res == 0) {
        for (int i = 0; i < mod_count; i++) {

            memset(&moduleInfo, 0, sizeof(SceKernelModuleInfo));
            moduleInfo.size = sizeof(SceKernelModuleInfo);
            res = sceKernelGetModuleInfo(ids[i], &moduleInfo);

            if (res >= 0) {

                if (strcmp(moduleInfo.module_name, "psp2shell") == 0
                    || strcmp(moduleInfo.module_name, "SceLibPgf") == 0
                    || strcmp(moduleInfo.module_name, "SceLibNetCtl") == 0
                    || strcmp(moduleInfo.module_name, "SceNet") == 0
                    || strcmp(moduleInfo.module_name, "SceAppUtil") == 0
                    || strcmp(moduleInfo.module_name, "SceLibPvf") == 0
                    || strcmp(moduleInfo.module_name, "SceLibft2") == 0
                    || strcmp(moduleInfo.module_name, "SceLibDbg") == 0
                    || strcmp(moduleInfo.module_name, "SceCommonDialog") == 0
                    || strcmp(moduleInfo.module_name, "SceShellSvc") == 0
                    || strcmp(moduleInfo.module_name, "SceGxm") == 0
                    || strcmp(moduleInfo.module_name, "SceGpuEs4User") == 0
                    || strcmp(moduleInfo.module_name, "SceAvcodecUser") == 0
                    || strcmp(moduleInfo.module_name, "SceDriverUser") == 0
                    || strcmp(moduleInfo.module_name, "SceLibKernel") == 0) {
                    //psp2shell_print("skip hook: %s\n", moduleInfo.module_name);
                    continue;
                }

                //psp2shell_print("apply hook: %s\n", moduleInfo.module_name);
                sceClibPrintf_uid[hook_count] =
                        taiHookFunctionImport(&sceClibPrintf_ref[i],
                                              moduleInfo.module_name,
                                              TAI_ANY_LIBRARY,
                                              0xFA26BC62,
                                              _sceClibPrintf);
                hook_count++;

                if (hook_count >= HOOK_MAX) {
                    break;
                }
            }
        }
    }

    return TAI_CONTINUE(int, module_start_ref, argc, args);
}
*/

void hooks_init() {
    /*
#ifndef __VITA_KERNEL__
    module_start_uid = taiHookFunctionExport(&module_start_ref,  // Output a reference
                                             TAI_MAIN_MODULE,       // Name of module being hooked
                                             TAI_ANY_LIBRARY, // If there's multiple libs exporting this
                                             0x935CD196,      // Special NID specifying module_start
                                             module_start_hook); // Name of the hook function
#endif
     */
}

void hooks_exit() {
    /*
#ifndef __VITA_KERNEL__
    for (int i = 0; i < HOOK_MAX; i++) {
        if (sceClibPrintf_uid[i] > 0) {
            taiHookRelease(sceClibPrintf_uid[i], sceClibPrintf_ref[i]);
        }
    }

    if (module_start_uid >= 0)
        taiHookRelease(module_start_uid, module_start_ref);
#endif
     */
}

#endif
