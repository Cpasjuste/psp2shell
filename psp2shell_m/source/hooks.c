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

static int _sceClibPrintf(const char *fmt, ...) {

    memset(buffer, 0, 256);
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, 256, fmt, args);
    va_end(args);
    psp2shell_print_color_advanced(256, 0, buffer);

    return 0;
    //return TAI_CONTINUE(int, sceClibPrintf_ref, fmt, args);
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
*/

int sceAppMgrGetProcessIdByAppIdForShell(SceUID pid);

static tai_hook_ref_t h_ref[2];
static SceUID h_uid[2];

int sceAppMgrGetRunningAppIdListForShell_h(int *a, int b) {

    int ret = TAI_CONTINUE(int, h_ref[0], a, b);

    if (a == NULL) {
        psp2shell_print("sceAppMgrGetRunningAppIdListForShell_h: a == NULL\n");
    } else {
        psp2shell_print("sceAppMgrGetRunningAppIdListForShell_h: a == 0x%08X\n", a[0]);
        if(ret > 0) {
            int res = sceAppMgrGetProcessIdByAppIdForShell(a[0]);
            psp2shell_print("sceAppMgrGetProcessIdByAppIdForShell: res == 0x%08X\n", res);
            if(res > 0) {

            }
        }
    }

    if (b == NULL) {
        psp2shell_print("sceAppMgrGetRunningAppIdListForShell_h: b == NULL\n");
    } else {
        psp2shell_print("sceAppMgrGetRunningAppIdListForShell_h: b == 0x%08X\n", b);
    }

    psp2shell_print("sceAppMgrGetRunningAppIdListForShell_h: ret == 0x%08X\n", ret);

    return ret;
}

/*
int sceAppMgrGetProcessIdByAppIdForShell_h(int a, void *b, void *c, void *d) {

    int ret = TAI_CONTINUE(int, h_ref[1], a, b, c, d);

    if (a == NULL) {
        psp2shell_print("sceAppMgrGetProcessIdByAppIdForShell_h: a == NULL\n");
    } else {
        psp2shell_print("sceAppMgrGetProcessIdByAppIdForShell_h: a == 0x%08X\n", a);

    }

    if (b == NULL) {
        psp2shell_print("sceAppMgrGetProcessIdByAppIdForShell_h: b == NULL\n");
    } else {
        psp2shell_print("sceAppMgrGetProcessIdByAppIdForShell_h: b == 0x%08X\n", b);
    }

    if (c == NULL) {
        psp2shell_print("sceAppMgrGetProcessIdByAppIdForShell_h: c == NULL\n");
    } else {
        psp2shell_print("sceAppMgrGetProcessIdByAppIdForShell_h: c == 0x%08X\n", c);
    }

    if (d == NULL) {
        psp2shell_print("sceAppMgrGetProcessIdByAppIdForShell_h: d == NULL\n");
    } else {
        psp2shell_print("sceAppMgrGetProcessIdByAppIdForShell_h: d == 0x%08X\n", d);
    }

    psp2shell_print("sceAppMgrGetProcessIdByAppIdForShell_h: 0x%08X\n", ret);

    return ret;
}
*/

void ps2_hooks_init() {

    //sceAppMgrGetRunningAppIdListForShell

    /*
    h_uid[0] = taiHookFunctionImport(&h_ref[0],
                                     TAI_MAIN_MODULE,
                                     TAI_ANY_LIBRARY,
                                     0x613A70E2,
                                     sceAppMgrGetRunningAppIdListForShell_h);
    */

/*
    h_uid[1] = taiHookFunctionImport(&h_ref[1],
                                     TAI_MAIN_MODULE,
                                     TAI_ANY_LIBRARY,
                                     0x63FAC2A9,
                                     sceAppMgrGetProcessIdByAppIdForShell_h);
*/

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

void ps2_hooks_exit() {

    /*
    if (h_uid[0] > 0) {
        taiHookRelease(h_uid[0], h_ref[0]);
    }
    */

    /*
    if (h_uid[1] > 0) {
        taiHookRelease(h_uid[1], h_ref[1]);
    }
    */

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
