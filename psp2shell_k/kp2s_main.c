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

#include "psp2shell_k.h"

#include "usbhostfs/usbasync.h"
#include "usbhostfs/usbhostfs.h"
#include "kp2s_hooks_io.h"

static bool quit = false;
static SceUID u_sema = -1;
static SceUID k_sema = -1;
static SceUID thid_wait = -1;
static P2S_CMD kp2s_cmd;
static int kp2s_busy = 0;
extern bool kp2s_ready;
extern void *p2s_data_buf;

static struct AsyncEndpoint g_endpoint;
static struct AsyncEndpoint g_stdout;
static struct AsyncEndpoint g_stdgdb;
static struct AsyncEndpoint g_stderr;

int kp2s_cmd_parse(kp2s_client *client, P2S_CMD *cmd);

static int usbShellInit(void) {

    // shell print
    int ret = usbAsyncRegister(ASYNC_SHELL, &g_endpoint);
    printf("usbAsyncRegister: ASYNC_SHELL = %i\n", ret);

    // stdout print
    ret = usbAsyncRegister(ASYNC_STDOUT, &g_stdout);
    printf("usbAsyncRegister: ASYNC_STDOUT = %i\n", ret);

    // kDebugPrintf print
    ret = usbAsyncRegister(ASYNC_GDB, &g_stdgdb);
    printf("usbAsyncRegister: ASYNC_GDB = %i\n", ret);

    // kDebugPrintf2 print
    ret = usbAsyncRegister(ASYNC_STDERR, &g_stderr);
    printf("usbAsyncRegister: ASYNC_STDERR = %i\n", ret);

    printf("usbWaitForConnect\n");
    ret = usbWaitForConnect();
    printf("usbWaitForConnect: %i\n", ret);

    return ret;
}

static void welcome() {

    PRINT("\n\n                     ________         .__           .__  .__   \n");
    PRINT("______  ____________ \\_____  \\   _____|  |__   ____ |  | |  |  \n");
    PRINT("\\____ \\/  ___/\\____ \\ /  ____/  /  ___/  |  \\_/ __ \\|  | |  |  \n");
    PRINT("|  |_> >___ \\ |  |_> >       \\  \\___ \\|   Y  \\  ___/|  |_|  |__\n");
    PRINT("|   __/____  >|   __/\\_______ \\/____  >___|  /\\___  >____/____/\n");
    PRINT("|__|       \\/ |__|           \\/     \\/     \\/     \\/ %s\n\n", __DATE__);
    PRINT("\r\n");
}

int kp2s_print_stdout(const char *data, size_t size) {

    if (!usbhostfs_connected()) {
        return -1;
    }

    //while (kp2s_busy) { sceKernelDelayThread(100); };
    //kp2s_busy = 1;
    int ret = usbAsyncWrite(ASYNC_STDOUT, data, size);
    //kp2s_busy = 0;

    return ret;
}

int kp2s_print_stdout_user(const char *data, size_t size) {

    int state = 0;
    ENTER_SYSCALL(state);

    if (!usbhostfs_connected()) {
        EXIT_SYSCALL(state);
        return -1;
    }

    char kbuf[size];
    memset(kbuf, 0, size);
    ksceKernelMemcpyUserToKernel(kbuf, (uintptr_t) data, size);

    //while (kp2s_busy) { sceKernelDelayThread(100); };
    //kp2s_busy = 1;
    int ret = usbAsyncWrite(ASYNC_STDOUT, kbuf, size);
    //kp2s_busy = 0;

    EXIT_SYSCALL(state);
    return ret;
}

int kp2s_print_stderr(const char *data, size_t size) {

    if (!usbhostfs_connected()) {
        return -1;
    }

    //while (kp2s_busy) { sceKernelDelayThread(100); };
    //kp2s_busy = 1;
    int ret = usbAsyncWrite(ASYNC_STDERR, data, size);
    //kp2s_busy = 0;

    return ret;
}

int kp2s_print_stdgdb(const char *data, size_t size) {

    if (!usbhostfs_connected()) {
        return -1;
    }

    //while (kp2s_busy) { sceKernelDelayThread(100); };
    //kp2s_busy = 1;
    int ret = usbAsyncWrite(ASYNC_GDB, data, size);
    //kp2s_busy = 0;

    return ret;
}

int kp2s_print_color(int color, const char *fmt, ...) {

    if (!usbhostfs_connected()) {
        return -1;
    }

    char buffer[P2S_KMSG_SIZE];
    memset(buffer, 0, P2S_KMSG_SIZE);
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, P2S_KMSG_SIZE, fmt, args);
    va_end(args);

    while (kp2s_busy) { sceKernelDelayThread(100); };
    kp2s_busy = 1;
    p2s_msg_send(ASYNC_SHELL, color, buffer);
    kp2s_busy = 0;

    return len;
}

int kp2s_print_color_user(int color, const char *data, size_t size) {

    int state = 0;
    ENTER_SYSCALL(state);

    if (!usbhostfs_connected()) {
        PRINT_ERR("kp2s_print_color_user(k): not connected\n");
        EXIT_SYSCALL(state);
        return -1;
    }

    char buffer[size + 1];
    memset(buffer, 0, size + 1);
    ksceKernelMemcpyUserToKernel(buffer, (uintptr_t) data, size);
    buffer[size + 1] = '\0';

    while (kp2s_busy) { sceKernelDelayThread(100); };
    kp2s_busy = 1;
    p2s_msg_send(ASYNC_SHELL, color, buffer);
    kp2s_busy = 0;

    EXIT_SYSCALL(state);

    return size;
}

int kp2s_wait_cmd(P2S_CMD *cmd) {

    int state = 0;
    int ret = -1;

    ENTER_SYSCALL(state);

    //PRINT_ERR("ksceKernelWaitSema: u_sema\n");
    ksceKernelWaitSema(u_sema, 1, NULL);
    //PRINT_ERR("ksceKernelWaitSema: u_sema received\n");

    if (kp2s_ready && kp2s_cmd.type > CMD_START) {
        ksceKernelMemcpyKernelToUser((uintptr_t) cmd, &kp2s_cmd, sizeof(P2S_CMD));
        kp2s_cmd.type = 0;
        ret = 0;
    }

    ksceKernelSignalSema(k_sema, 1);

    EXIT_SYSCALL(state);

    return ret;
}

void kp2s_set_ready(bool rdy) {

    if (kp2s_ready != rdy) {
        kp2s_ready = rdy;
        if (kp2s_ready <= 0) {
            // unlock user thread
            ksceKernelSignalSema(u_sema, 1);
        }
    }
}

static int thread_wait_cmd(SceSize args, void *argp) {

    printf("thread_wait_cmd start\n");
    ksceKernelDelayThread(1000 * 1000 * 5);

    int res = usbhostfs_start();
    if (res != 0) {
        printf("thread_wait_cmd: usbhostfs_start failed\n");
        return -1;
    }

    usbShellInit();
    welcome();

    set_hooks();
    set_hooks_io();

    kp2s_client client;
    memset(&client, 0, sizeof(kp2s_client));
    strcpy(client.path, HOME_PATH);

    while (!quit) {

        res = p2s_cmd_receive(ASYNC_SHELL, &kp2s_cmd);
        //PRINT_ERR("p2s_cmd_receive: %i", kp2s_cmd.type);

        if (res != 0) {
            if (!usbhostfs_connected()) {
                printf("p2s_cmd_receive failed, waiting for usb...\n");
                usbWaitForConnect();
            }
        } else {
            res = kp2s_cmd_parse(&client, &kp2s_cmd);
            if (res != 0) {
                if (!kp2s_ready) {
                    PRINT_ERR("psp2shell main user module not loaded");
                } else {
                    // send cmd to user module
                    ksceKernelSignalSema(u_sema, 1);
                    ksceKernelWaitSema(k_sema, 0, NULL);
                }
            }
        }
    }

    printf("thread_wait_cmd end\n");
    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    u_sema = ksceKernelCreateSema("p2s_sem_u", 0, 0, 1, NULL);
    k_sema = ksceKernelCreateSema("p2s_sem_k", 0, 0, 1, NULL);

    thid_wait = ksceKernelCreateThread("kp2s_wait_cmd", thread_wait_cmd, 64, 0x10000, 0, 0x10000, 0);
    if (thid_wait >= 0) {
        ksceKernelStartThread(thid_wait, 0, NULL);
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    delete_hooks_io();
    delete_hooks();

    quit = true;

    if (u_sema >= 0) {
        ksceKernelDeleteSema(u_sema);
    }
    if (k_sema >= 0) {
        ksceKernelDeleteSema(k_sema);
    }
    //ksceKernelDeleteThread(thid_wait);
    //ksceKernelWaitThreadEnd(thid_wait, 0, 0);

    printf("module_stop: usbhostfs_stop...\n");
    int res = usbhostfs_stop();
    if (res != 0) {
        printf("module_stop: usbhostfs_stop failed\n");
    }

    if (p2s_data_buf != NULL) {
        p2s_free(p2s_data_buf);
    }

    return SCE_KERNEL_STOP_SUCCESS;
}
