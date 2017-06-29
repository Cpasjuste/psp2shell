/*
	PSP2SHELL
	Copyright (C) 2016, Cpasjuste

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/modulemgr.h>
#include <libk/stdio.h>
#include <libk/stdarg.h>
#include <libk/string.h>
#include <libk/stdbool.h>

#include "hooks.h"

#ifdef __USB__

#include "../psp2shell_m/include/psp2shell.h"
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>
#include "usbhostfs/usbasync.h"
#include "usbhostfs/usbhostfs.h"
#include "../common/p2s_msg.h"
#include "../common/p2s_cmd.h"

static bool quit = false;
static SceUID thid_wait = -1;
static P2S_CMD kp2s_cmd;
volatile static int k_buf_lock = 0;
extern bool kp2s_ready;

static struct AsyncEndpoint g_endp;
static struct AsyncEndpoint g_stdout;

static int usbShellInit(void) {

    int ret = usbAsyncRegister(ASYNC_SHELL, &g_endp);
    printf("usbAsyncRegister: ASYNC_SHELL = %i\n", ret);
    ret = usbAsyncRegister(ASYNC_STDOUT, &g_stdout);
    printf("usbAsyncRegister: ASYNC_STDOUT = %i\n", ret);

    printf("usbWaitForConnect\n");
    ret = usbWaitForConnect();
    printf("usbWaitForConnect: %i\n", ret);

    return 0;
}

static void welcome() {

    PRINT("\n\n                     ________         .__           .__  .__   \n");
    PRINT("______  ____________ \\_____  \\   _____|  |__   ____ |  | |  |  \n");
    PRINT("\\____ \\/  ___/\\____ \\ /  ____/  /  ___/  |  \\_/ __ \\|  | |  |  \n");
    PRINT("|  |_> >___ \\ |  |_> >       \\  \\___ \\|   Y  \\  ___/|  |_|  |__\n");
    PRINT("|   __/____  >|   __/\\_______ \\/____  >___|  /\\___  >____/____/\n");
    PRINT("|__|       \\/ |__|           \\/     \\/     \\/     \\/ %s\n\n", "0.0");
    PRINT("\r\n");
}

int kp2s_print_stdout(const char *data, int size) {
    usbAsyncWrite(ASYNC_STDOUT, data, size);
    return size;
}

int kp2s_print_stdout_user(const char *data, int size) {

    char kbuf[P2S_KMSG_SIZE];
    memset(kbuf, 0, P2S_KMSG_SIZE);
    ksceKernelMemcpyUserToKernel(kbuf, (uintptr_t) data, sizeof(size));
    usbAsyncWrite(ASYNC_STDOUT, kbuf, size);
    return size;
}

void kp2s_print_color(int color, const char *fmt, ...) {

    if (!usbhostfs_connected()) {
        return;
    }

    char buffer[P2S_KMSG_SIZE];
    memset(buffer, 0, P2S_KMSG_SIZE);

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, P2S_KMSG_SIZE, fmt, args);
    va_end(args);

    p2s_msg_send(ASYNC_SHELL, color, buffer);
}

void kp2s_print_color_user(int color, const char *fmt, ...) {

    int state = 0;
    ENTER_SYSCALL(state);

    if (!usbhostfs_connected()) {
        return;
    }

    char buffer[P2S_KMSG_SIZE];
    memset(buffer, 0, P2S_KMSG_SIZE);

    char kfmt[P2S_KMSG_SIZE];
    memset(kfmt, 0, P2S_KMSG_SIZE);
    ksceKernelMemcpyUserToKernel(kfmt, (uintptr_t) fmt, sizeof(fmt));

    va_list args;
    va_start(args, kfmt);
    vsnprintf(buffer, P2S_KMSG_SIZE, kfmt, args);
    va_end(args);

    p2s_msg_send(ASYNC_SHELL, color, buffer);

    EXIT_SYSCALL(state);
}

int kp2s_wait_cmd(P2S_CMD *cmd) {

    int state = 0;
    int ret = -1;

    ENTER_SYSCALL(state);

    while (k_buf_lock);
    k_buf_lock = 1;

    if (kp2s_cmd.type > CMD_START) {
        ksceKernelMemcpyKernelToUser((uintptr_t) cmd, &kp2s_cmd, sizeof(P2S_CMD));
        kp2s_cmd.type = 0;
        ret = 0;
    }

    k_buf_lock = 0;

    EXIT_SYSCALL(state);

    return ret;
}

static int thread_wait_cmd(SceSize args, void *argp) {

    printf("thread_wait_cmd start\n");

    usbShellInit();
    welcome();

    while (!quit) {

        while (k_buf_lock);
        k_buf_lock = 1;
        int res = p2s_cmd_receive(ASYNC_SHELL, &kp2s_cmd);
        k_buf_lock = 0;

        if (res != 0) {
            if (!usbhostfs_connected()) {
                printf("p2s_cmd_receive failed, waiting for usb...\n");
                usbWaitForConnect();
            }
        } else {
            if (!kp2s_ready) {
                PRINT_ERR("psp2shell main user module not loaded \n");
            }
        }
    }

    printf("thread_wait_cmd end\n");
    return 0;
}

#endif

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

#ifdef __USB__
    int res = usbhostfs_start();
    if (res != 0) {
        printf("module_start: usbhostfs_start failed\n");
        return SCE_KERNEL_START_FAILED;
    }

    thid_wait = ksceKernelCreateThread("kp2s_wait_cmd", thread_wait_cmd, 0x64, 0x5000, 0, 0x10000, 0);
    if (thid_wait >= 0) {
        ksceKernelStartThread(thid_wait, 0, NULL);
    }
#endif
    set_hooks();

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    delete_hooks();

#ifdef __USB__
    quit = true;
    //ksceKernelDeleteThread(thid_wait);
    //ksceKernelWaitThreadEnd(thid_wait, 0, 0);

    printf("module_stop: usbhostfs_stop\n");
    int res = usbhostfs_stop();
    if (res != 0) {
        printf("module_stop: usbhostfs_stop failed\n");
    }
#endif

    return SCE_KERNEL_STOP_SUCCESS;
}
