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

#include <libk/string.h>
#include <libk/stdio.h>
#include <libk/stdbool.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>

#include "libmodule.h"
#include "psp2shell.h"
#include "utility.h"
#include "file.h"
#include "usbhostfs.h"
#include "usbasync.h"

typedef struct {
    int msg_sock;
    int cmd_sock;
    P2S_CMD cmd;
    P2S_MSG msg;
    s_FileList fileList;
} s_client;

static s_client *client;
static bool quit = false;
static SceUID thid_wait = -1;

struct AsyncEndpoint g_endp;
struct AsyncEndpoint g_stdout;

void p2s_cmd_parse(s_client *client, P2S_CMD *cmd);

int usbShellPrint(const char *data, int size) {
    usbAsyncWrite(ASYNC_SHELL, data, size);
    return size;
}

int usbStdoutPrint(const char *data, int size) {
    usbAsyncWrite(ASYNC_STDOUT, data, size);
    return size;
}

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

void psp2shell_print_color(int color, const char *fmt, ...) {

    client->msg.color = color;
    memset(client->msg.buffer, 0, P2S_KMSG_SIZE);
    va_list args;
    va_start(args, fmt);
    vsnprintf(client->msg.buffer, P2S_KMSG_SIZE, fmt, args);
    va_end(args);

    printf("psp2shell_print_color: %s\n", client->msg.buffer);
    p2s_msg_send(ASYNC_SHELL, color, client->msg.buffer);
    //p2s_msg_send_msg(ASYNC_SHELL, &client->msg);
}

static void welcome() {

    PRINT("                     ________         .__           .__  .__   \n");
    PRINT("______  ____________ \\_____  \\   _____|  |__   ____ |  | |  |  \n");
    PRINT("\\____ \\/  ___/\\____ \\ /  ____/  /  ___/  |  \\_/ __ \\|  | |  |  \n");
    PRINT("|  |_> >___ \\ |  |_> >       \\  \\___ \\|   Y  \\  ___/|  |_|  |__\n");
    PRINT("|   __/____  >|   __/\\_______ \\/____  >___|  /\\___  >____/____/\n");
    PRINT("|__|       \\/ |__|           \\/     \\/     \\/     \\/ %s\n\n", "0.0");
    PRINT("\r\n");
}

static int thread_wait(SceSize args, void *argp) {

    printf("thread_wait start\n");

    usbShellInit();
    //welcome();

    // setup clients data
    client = p2s_malloc(sizeof(s_client));
    if (client == NULL) { // crap
        printf("client alloc failed\n");
        sceKernelExitDeleteThread(0);
        return -1;
    }
    memset(client, 0, sizeof(s_client));
    client->msg_sock = -1;
    client->cmd_sock = -1;

    // init client file listing memory/**/
    memset(&client->fileList, 0, sizeof(s_FileList));
    strcpy(client->fileList.path, HOME_PATH);
    s_fileListGetEntries(&client->fileList, HOME_PATH);

    while (!quit) {

        int res = p2s_cmd_receive(ASYNC_SHELL, &client->cmd);
        if (res != 0) {
            printf("p2s_cmd_receive failed\n");
            break;
        } else {
            p2s_cmd_parse(client, &client->cmd);
        }
    }

    printf("thread_wait end\n");
    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    int res = usbhostfs_start();
    if (res != 0) {
        printf("module_start: usbhostfs_start failed\n");
        return SCE_KERNEL_START_FAILED;
    }

    thid_wait = ksceKernelCreateThread("psp2shell_wait", thread_wait, 0x64, 0x5000, 0, 0x10000, 0);
    if (thid_wait >= 0) {
        ksceKernelStartThread(thid_wait, 0, NULL);
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    quit = true;
    //ksceKernelDeleteThread(thid_wait);
    //ksceKernelWaitThreadEnd(thid_wait, 0, 0);

    printf("module_stop: usbhostfs_stop\n");
    int res = usbhostfs_stop();
    if (res != 0) {
        printf("module_stop: usbhostfs_stop failed\n");
    }

    p2s_free(client);

    return SCE_KERNEL_STOP_SUCCESS;
}
