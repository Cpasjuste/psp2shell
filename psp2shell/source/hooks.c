//
// Created by cpasjuste on 09/11/16.
//
#ifdef MODULE

#include <taihen.h>
#include "psp2shell.h"
#include "libmodule.h"
#include "hooks.h"

extern void _psp2shell_print_color(SceSize size, int color, const char *fmt, ...);

static char sceClibPrintf_msg[512];
static SceUID sceClibPrintf_uid;
static tai_hook_ref_t sceClibPrintf_hook;

int sceClibPrintf_patched(const char *fmt, ...) {

    memset(sceClibPrintf_msg, 0, 512);
    va_list args;
    va_start(args, fmt);
    vsnprintf(sceClibPrintf_msg, 512, fmt, args);
    va_end(args);
    psp2shell_print_color(0, sceClibPrintf_msg);

    return TAI_CONTINUE(int, sceClibPrintf_hook, fmt, args);
}

/*
static int __stdout_fd = 1073807367;
static SceUID g_hooks[2];

static tai_hook_ref_t ref_hook0;

int sceIoWrite_patched(SceUID fd, const void *data, SceSize size) {
    if (fd == __stdout_fd) {
        _psp2shell_print_color(size, 0, (char *) data);
    }
    return TAI_CONTINUE(int, ref_hook0, fd, data, size);
}

static tai_hook_ref_t ref_hook1;

int sceKernelGetStdout_patched() {
    //debug("sceIoOpen: %s\n",file);
    int fd = TAI_CONTINUE(int, ref_hook1);
    __stdout_fd = fd;
    return fd;
}
*/

void hooks_init() {
#ifndef __VITA_KERNEL__
    /*
    g_hooks[0] = taiHookFunctionExport(&ref_hook0, "SceIofilemgr", 0xF2FF276E,
                                       0x34EFD876, sceIoWrite_patched);
    g_hooks[1] = taiHookFunctionExport(&ref_hook1, "SceProcessmgr", 0x2DD91812,
                                       0xE5AA625C, sceKernelGetStdout_patched);
    */
    sceClibPrintf_uid = taiHookFunctionImport(&sceClibPrintf_hook,
                                              TAI_MAIN_MODULE,
                                              TAI_ANY_LIBRARY,
                                              0xFA26BC62,
                                              sceClibPrintf_patched);
#endif
}

void hooks_exit() {
#ifndef __VITA_KERNEL__
    /*
    if (g_hooks[0] >= 0) taiHookRelease(g_hooks[0], ref_hook0);
    if (g_hooks[1] >= 0) taiHookRelease(g_hooks[1], ref_hook1);
    */

    if (sceClibPrintf_uid >= 0)
        taiHookRelease(sceClibPrintf_uid, sceClibPrintf_hook);
#endif
}
#endif
