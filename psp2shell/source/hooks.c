//
// Created by cpasjuste on 09/11/16.
//
#ifdef MODULE

#include <taihen.h>
#include "psp2shell.h"
#include "libmodule.h"
#include "hooks.h"

static SceUID sceClibPrintf_uid;
static tai_hook_ref_t sceClibPrintf_hook;

int sceClibPrintf_patched(const char *fmt, ...) {

    char msg[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, 256, fmt, args);
    va_end(args);
    psp2shell_print_color(0, msg);

    return TAI_CONTINUE(int, sceClibPrintf_hook, fmt, args);
}

void hooks_init() {
#ifndef __VITA_KERNEL__
    sceClibPrintf_uid = taiHookFunctionImport(&sceClibPrintf_hook,
                                              TAI_MAIN_MODULE,
                                              TAI_ANY_LIBRARY,
                                              0xFA26BC62,
                                              sceClibPrintf_patched);
#endif
}

void hooks_exit() {
#ifndef __VITA_KERNEL__
    if (sceClibPrintf_uid >= 0)
        taiHookRelease(sceClibPrintf_uid, sceClibPrintf_hook);
#endif
}

#endif
