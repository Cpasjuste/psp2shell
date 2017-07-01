//
// Created by cpasjuste on 29/06/17.
//

#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>
#include <libk/string.h>
#include <libk/stdarg.h>
#include <libk/stdio.h>
#include <taihen.h>

#include "kp2s_hooks.h"
#include "kp2s_utility.h"
#include "psp2shell_k.h"

#define P2S_MSG_LEN 256

static SceUID g_hooks[MAX_HOOKS];
static tai_hook_ref_t ref_hooks[MAX_HOOKS];
static int __stdout_fd = 1073807367;

static int _kDebugPrintf(const char *fmt, ...) {

    char temp_buf[P2S_MSG_LEN];
    memset(temp_buf, 0, P2S_MSG_LEN);
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(temp_buf, P2S_MSG_LEN, fmt, args);
    va_end(args);

#ifdef __USB__
    kp2s_print_stdout(temp_buf, len);
#else
    kp2s_print("%s", temp_buf);
#endif

    return TAI_CONTINUE(int, ref_hooks[2], fmt, args);
}

static int _kDebugPrintf2(int num0, int num1, const char *fmt, ...) {

    char temp_buf[P2S_MSG_LEN];
    memset(temp_buf, 0, P2S_MSG_LEN);
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(temp_buf, P2S_MSG_LEN, fmt, args);
    va_end(args);

#ifdef __USB__
    kp2s_print_stdout(temp_buf, len);
#else
    kp2s_print("%s", temp_buf);
#endif

    return TAI_CONTINUE(int, ref_hooks[3], num0, num1, fmt, args);
}

static int _sceIoWrite(SceUID fd, const void *data, SceSize size) {

    if (ref_hooks[0] <= 0) {
        return 0;
    }

#ifdef __USB__
    if (fd == __stdout_fd) {
        kp2s_print_stdout(data, size);
    }
#else
    if (fd == __stdout_fd && size < P2S_MSG_LEN) {
        char temp_buf[size];
        memset(temp_buf, 0, size);
        ksceKernelStrncpyUserToKernel(temp_buf, (uintptr_t) data, size);
        kp2s_print_len(size, "%s", temp_buf);
    }
#endif

    return TAI_CONTINUE(int, ref_hooks[0], fd, data, size);
}

static int _sceKernelGetStdout() {

    if (ref_hooks[1] <= 0) {
        return 0;
    }

    int fd = TAI_CONTINUE(int, ref_hooks[1]);
    __stdout_fd = fd;

    return fd;
}

void set_hooks() {

    uint32_t state;
    ENTER_SYSCALL(state);

    g_hooks[0] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[0],
            "SceIofilemgr",
            0xF2FF276E,
            0x34EFD876,
            _sceIoWrite);
    //LOG("hook: sceIoWrite: 0x%08X\n", g_hooks[0]);

    g_hooks[1] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[1],
            "SceProcessmgr",
            0x2DD91812,
            0xE5AA625C,
            _sceKernelGetStdout);
    //LOG("hook: sceKernelGetStdout: 0x%08X\n", g_hooks[1]);

    g_hooks[2] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[2],
            "SceSysmem",
            0x88758561, // SceDebugForDriver
            0x391B74B7, // ksceDebugPrintf
            _kDebugPrintf);
    //LOG("hook: _printf: 0x%08X\n", g_hooks[2]);

    g_hooks[3] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[3],
            "SceSysmem",
            0x88758561, // SceDebugForDriver
            0x02B04343, // ksceDebugPrintf2
            _kDebugPrintf2);
    //LOG("hook: _printf2: 0x%08X\n", g_hooks[3]);

    EXIT_SYSCALL(state);
}

void delete_hooks() {

    for (int i = 0; i < MAX_HOOKS; i++) {
        if (g_hooks[i] >= 0)
            taiHookReleaseForKernel(g_hooks[i], ref_hooks[i]);
    }
}
