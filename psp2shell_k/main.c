#include <vitasdkkern.h>
#include <libk/stdio.h>
#include <taihen.h>

#include "include/psp2shell_k.h"

static int sock = 0;

static SceUID g_hooks[2];
static tai_hook_ref_t ref_hooks[2];
static int __stdout_fd = 1073807367;

int _sceIoWrite(SceUID fd, const void *data, SceSize size) {

    if (ref_hooks[0] <= 0) {
        return 0;
    }

    if (fd == __stdout_fd && sock > 0) {
        kpsp2shell_print(sock, size, data);
    }

    return TAI_CONTINUE(int, ref_hooks[0], fd, data, size);
}

int _sceKernelGetStdout() {

    if (ref_hooks[1] <= 0) {
        return 0;
    }

    int fd = TAI_CONTINUE(int, ref_hooks[1]);
    __stdout_fd = fd;

    return fd;
}

void kpsp2shell_set_sock(int s) {
    sock = s;
}

void kpsp2shell_print(int s, unsigned int size, const char *msg) {

    char buf[size];

    uint32_t state;
    ENTER_SYSCALL(state);

    ksceKernelStrncpyUserToKernel(buf, (uintptr_t) msg, size);
    ksceNetSendto(s, buf, size, 0, NULL, 0);
    ksceNetRecvfrom(s, buf, 2, 0x1000, NULL, 0);

    EXIT_SYSCALL(state);
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    g_hooks[0] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[0],
            "SceIofilemgr",
            0xF2FF276E,
            0x34EFD876,
            _sceIoWrite);

    g_hooks[1] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[1],
            "SceProcessmgr",
            0x2DD91812,
            0xE5AA625C,
            _sceKernelGetStdout);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    if (g_hooks[0] >= 0)
        taiHookReleaseForKernel(g_hooks[0], ref_hooks[0]);

    if (g_hooks[1] >= 0)
        taiHookReleaseForKernel(g_hooks[1], ref_hooks[1]);

    return SCE_KERNEL_STOP_SUCCESS;
}
