#include <vitasdkkern.h>
#include <libk/string.h>
#include <libk/stdio.h>
#include <taihen.h>
#include <libk/stdarg.h>

#include "psp2shell_k.h"
#include "kutility.h"

static char kbuf[BUF_SIZE];

#define MAX_HOOKS 64
static SceUID g_hooks[MAX_HOOKS];
static tai_hook_ref_t ref_hooks[MAX_HOOKS];
static int __stdout_fd = 1073807367;

static SceUID k_mutex, u_mutex;

static int ready = 0;

void set_hooks();

void delete_hooks();

void update_kbuf(const char *buffer, unsigned int size);

/*
static int _kDebugPrintf(const char *fmt, ...) {

    char temp_buf[512];
    memset(temp_buf, 0, 512);
    va_list args;
    va_start(args, fmt);
    vsnprintf(temp_buf, 512, fmt, args);
    va_end(args);

    //LOG("_printf: %s\n", temp_buf);
    if (ready) {
        update_kbuf(temp_buf, strlen(temp_buf));
    }

    return TAI_CONTINUE(int, ref_hooks[2], fmt, args);
}
*/

static int _kDebugPrintf2(int num0, int num1, const char *fmt, ...) {

    char temp_buf[512];
    memset(temp_buf, 0, 512);
    va_list args;
    va_start(args, fmt);
    vsnprintf(temp_buf, 512, fmt, args);
    va_end(args);

    //LOG("_printf2: %s\n", temp_buf);
    if (ready) {
        update_kbuf(temp_buf, strlen(temp_buf));
    }

    return TAI_CONTINUE(int, ref_hooks[3], num0, num1, fmt, args);
}

int _sceIoWrite(SceUID fd, const void *data, SceSize size) {

    if (ref_hooks[0] <= 0) {
        return 0;
    }

    if (fd == __stdout_fd && ready) {
        update_kbuf(data, size);
    }

    return TAI_CONTINUE(int, ref_hooks[0], fd, data, size);
}

int _sceKernelGetStdout() {

    if (ref_hooks[1] <= 0) {
        return 0;
    }

    int fd = TAI_CONTINUE(int, ref_hooks[1]);
    __stdout_fd = fd;
    //LOG("hook: __stdout_fd: 0x%08X\n", __stdout_fd);

    return fd;
}

void update_kbuf(const char *buffer, unsigned int size) {

    if (size > BUF_SIZE) {
        return;
    }

    uint32_t state;
    ENTER_SYSCALL(state);

    memset(kbuf, 0, BUF_SIZE);
    ksceKernelStrncpyUserToKernel(kbuf, (uintptr_t) buffer, size);

    ksceKernelSignalSema(k_mutex, 1);
    ksceKernelWaitSema(u_mutex, 1, NULL);

    EXIT_SYSCALL(state);
}

void kpsp2shell_wait_buffer(char *buffer, unsigned int size) {

    uint32_t state;
    ENTER_SYSCALL(state);

    ksceKernelWaitSema(k_mutex, 1, NULL);
    if (ready) {
        ksceKernelStrncpyKernelToUser((uintptr_t) buffer, kbuf, size);
        ksceKernelSignalSema(u_mutex, 1);
    }

    EXIT_SYSCALL(state);
}

void kpsp2shell_set_ready(int rdy) {

    ready = rdy;
    // "unblock" update_kbuf
    if (ready == 0) {
        ksceKernelSignalSema(k_mutex, 1);
        ksceKernelSignalSema(u_mutex, 1);
    }
}

int kpsp2shell_get_module_info(SceUID pid, SceUID modid, SceKernelModuleInfo *info) {

    SceKernelModuleInfo kinfo;
    memset(&kinfo, 0, sizeof(SceKernelModuleInfo));
    kinfo.size = sizeof(SceKernelModuleInfo);

    uint32_t state;
    ENTER_SYSCALL(state);

    int ret = ksceKernelGetModuleInfo(pid, modid, &kinfo);
    if (ret >= 0) {
        ksceKernelMemcpyKernelToUser((uintptr_t) info, &kinfo, sizeof(SceKernelModuleInfo));
    }

    EXIT_SYSCALL(state);

    return ret;
}

int kpsp2shell_get_module_list(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num) {

    size_t count = 256;
    SceUID kmodids[256];

    uint32_t state;
    ENTER_SYSCALL(state);

    memset(kmodids, 0, sizeof(SceUID) * 256);
    int res = ksceKernelGetModuleList(pid, flags1, flags2, kmodids, &count);
    if (res >= 0) {
        ksceKernelMemcpyKernelToUser((uintptr_t) modids, &kmodids, sizeof(SceUID) * 256);
        ksceKernelMemcpyKernelToUser((uintptr_t) num, &count, sizeof(size_t));
    } else {
        ksceKernelMemcpyKernelToUser((uintptr_t) num, &count, sizeof(size_t));
    }

    EXIT_SYSCALL(state);

    return res;
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

    /*
    g_hooks[2] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[2],
            "SceSysmem",
            0x88758561, // SceDebugForDriver
            0x391B74B7, // printf
            _kDebugPrintf);
    //LOG("hook: _printf: 0x%08X\n", g_hooks[2]);
    */

    g_hooks[3] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[3],
            "SceSysmem",
            0x88758561, // SceDebugForDriver
            0x02B04343, // printf2
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

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    k_mutex = ksceKernelCreateSema("k_mutex", 0, 0, 1, NULL);
    u_mutex = ksceKernelCreateSema("u_mutex", 0, 0, 1, NULL);

    set_hooks();

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    ksceKernelDeleteMutex(k_mutex);
    ksceKernelDeleteMutex(u_mutex);

    delete_hooks();

    return SCE_KERNEL_STOP_SUCCESS;
}
