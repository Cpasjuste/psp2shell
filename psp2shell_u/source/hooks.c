//
// Created by cpasjuste on 09/11/16.
//

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <libk/string.h>
#include <libk/stdarg.h>
#include <libk/stdio.h>
#include <taihen.h>

#include "hooks.h"
#include "../../psp2shell_k/include/psp2shell_k.h"

static char temp_buf[512];

#define HOOK_MAX 32
static SceUID sceClibPrintf_uid[HOOK_MAX];
static tai_hook_ref_t sceClibPrintf_ref[HOOK_MAX];

static tai_hook_ref_t module_start_ref;
SceUID module_start_uid;

static int _sceClibPrintf(const char *fmt, ...) {

    memset(temp_buf, 0, 512);
    va_list args;
    va_start(args, fmt);
    vsnprintf(temp_buf, 512, fmt, args);
    va_end(args);

    kpsp2shell_print(strlen(temp_buf), temp_buf);

    return 0;
}

static int module_start_hook(SceSize argc, const void *args) {

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

void ps2_hooks_init() {

    module_start_uid = taiHookFunctionExport(&module_start_ref,
                                             TAI_MAIN_MODULE,
                                             TAI_ANY_LIBRARY,
                                             0x935CD196,
                                             module_start_hook);
}

void ps2_hooks_exit() {

    if (module_start_uid >= 0) {
        taiHookRelease(module_start_uid, module_start_ref);
    }

    for (int i = 0; i < HOOK_MAX; i++) {
        if (sceClibPrintf_uid[i] > 0) {
            taiHookRelease(sceClibPrintf_uid[i], sceClibPrintf_ref[i]);
        }
    }
}
