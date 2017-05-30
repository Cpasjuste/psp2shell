#include <libk/stdio.h>
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>

#include "include/psp2shell_k.h"

static SceUID g_hooks[2];
static tai_hook_ref_t ref_hook0;
static int __stdout_fd = 1073807367;

static StdoutCallback stdoutCb = NULL;

void setStdoutCb(StdoutCallback cb) {
    stdoutCb = cb;
}

int sceIoWrite_patched(SceUID fd, const void *data, SceSize size) {

    if (fd == __stdout_fd
        && size < 513
        && stdoutCb != NULL) {
        stdoutCb(0, size);
    }

    return TAI_CONTINUE(int, ref_hook0, fd, data, size);
}

static tai_hook_ref_t ref_hook1;

int sceKernelGetStdout_patched() {

    int fd = TAI_CONTINUE(int, ref_hook1);

    __stdout_fd = fd;

    return fd;
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    g_hooks[0] = taiHookFunctionExportForKernel(KERNEL_PID,
                                                &ref_hook0, "SceIofilemgr", 0xF2FF276E,
                                                0x34EFD876, sceIoWrite_patched);
    g_hooks[1] = taiHookFunctionExportForKernel(KERNEL_PID,
                                                &ref_hook1, "SceProcessmgr", 0x2DD91812,
                                                0xE5AA625C, sceKernelGetStdout_patched);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    if (g_hooks[0] >= 0)
        taiHookReleaseForKernel(g_hooks[0], ref_hook0);

    if (g_hooks[1] >= 0)
        taiHookReleaseForKernel(g_hooks[1], ref_hook1);

    return SCE_KERNEL_STOP_SUCCESS;
}
