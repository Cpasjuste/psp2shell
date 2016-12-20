//
// Created by cpasjuste on 09/11/16.
//
//#ifdef MODULE
#include <taihen.h>

#include "utility.h"
#include "psp2shell.h"
#include "libmodule.h"
#include "hooks.h"

static SceUID g_hooks[1];

/*
static tai_hook_ref_t g_sceClibPrintf_hook;
int sceClibPrintf_patched(const char *fmt, ...) {
    psp2shell_print("printf_patched\n");
    return 0;
}
*/

void hooks_init() {
    /*
    g_hooks[0] = taiHookFunctionImport(&g_sceClibPrintf_hook,
                                       "SceLibKernel",
                                       0xCAE9ACE6,
                                       0xFA26BC62,
                                       sceClibPrintf_patched);
    s_debug("hook = %i\n", g_hooks[0]);
    */
}

void hooks_exit() {
    /*
    if (g_hooks[0] >= 0)
        taiHookRelease(g_hooks[0], g_sceClibPrintf_hook);
    */
}
//#endif
