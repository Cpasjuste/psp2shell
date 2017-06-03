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

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>

#include <psp2/net/net.h>
#include <taihen.h>

#include "psp2cmd.h"
#include "main.h"
#include "utility.h"
#include "psp2shell.h"
#include "module.h"
#include "thread.h"
#include "taipool.h"

#include "libmodule.h"
#include "../../psp2shell_k/include/psp2shell_k.h"
#include "cmd.h"

static SceUID thid_wait, thid_kbuf;
static int listen_port = 3333;
static int quit = 0;
static s_client *client;
static int server_sock_msg;
static int server_sock_cmd;

static void close_client() {

    kpsp2shell_set_ready(0);

    if (client != NULL) {

        if (client->msg_sock >= 0) {
            sceNetSocketClose(client->msg_sock);
            client->msg_sock = -1;
        }

        if (client->cmd_sock >= 0) {
            sceNetSocketClose(client->cmd_sock);
            client->cmd_sock = -1;
        }

        s_fileListEmpty(&client->fileList);
    }
}

static void close_server() {

    if (server_sock_msg >= 0) {
        sceNetSocketClose(server_sock_msg);
        server_sock_msg = -1;
    }
    if (server_sock_cmd >= 0) {
        sceNetSocketClose(server_sock_cmd);
        server_sock_cmd = -1;
    }
}

static int open_server() {

    close_server();

    server_sock_msg = p2s_bind_port(server_sock_msg, listen_port);
    if (server_sock_msg <= 0) {
        return -1;
    }
    server_sock_cmd = p2s_bind_port(server_sock_cmd, listen_port + 1);
    if (server_sock_cmd <= 0) {
        return -1;
    }

    return 0;
}

void psp2shell_print_color_advanced(SceSize size, int color, const char *fmt, ...) {

    char msg[size];
    memset(msg, 0, size);
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, size, fmt, args);
    va_end(args);

    snprintf(msg + strlen(msg), size, "%i", color);

    if (client->msg_sock > 0) {
        sceNetSend(client->msg_sock, msg, size, 0);
        int ret = sceNetRecv(client->msg_sock, msg, 1, 0);
        if (ret < 0) { // wait for answer
            printf("psp2shell_print: sceNetRecv failed: %i\n", ret);
        }
    }
}

static void welcome() {

    char msg[1024];
    memset(msg, 0, 1024);
    strcpy(msg, "\n");
    strcat(msg, "                     ________         .__           .__  .__   \n");
    strcat(msg, "______  ____________ \\_____  \\   _____|  |__   ____ |  | |  |  \n");
    strcat(msg, "\\____ \\/  ___/\\____ \\ /  ____/  /  ___/  |  \\_/ __ \\|  | |  |  \n");
    strcat(msg, "|  |_> >___ \\ |  |_> >       \\  \\___ \\|   Y  \\  ___/|  |_|  |__\n");
    strcat(msg, "|   __/____  >|   __/\\_______ \\/____  >___|  /\\___  >____/____/\n");
    sprintf(msg + strlen(msg), "|__|       \\/ |__|           \\/     \\/     \\/     \\/ %s\n\n", VERSION);
    psp2shell_print_color(COL_GREEN, msg);
}

int cmd_thread(SceSize args, void *argp) {

    printf("cmd_thread\n");

    // init client file listing memory
    printf("client->fileList malloc\n");
    memset(&client->fileList, 0, sizeof(s_FileList));
    strcpy(client->fileList.path, HOME_PATH);
    s_fileListGetEntries(&client->fileList, HOME_PATH);

    // Welcome!
    printf("welcome client\n");
    welcome();

    // get data sock
    printf("get data sock\n");
    client->cmd_sock = p2s_get_sock(server_sock_cmd);
    printf("got data sock: %i\n", client->cmd_sock);

    kpsp2shell_set_ready(1);

    while (!quit) {

        memset(client->cmd_buffer, 0, SIZE_CMD);

        int read_size = sceNetRecv(
                client->cmd_sock,
                client->cmd_buffer, SIZE_CMD, 0);

        if (read_size <= 0) {
            printf("sceNetRecv failed: %i\n", read_size);
            break;
        } else if (read_size > 0) {
            printf("sceNetRecv ok: %i\n", read_size);
            cmd_parse(client);
        }
    }

    printf("closing connection\n");

    close_client();

    sceKernelExitDeleteThread(0);

    return 0;
}

static int thread_wait(SceSize args, void *argp) {

    // setup clients data
    memset(client, 0, sizeof(s_client));
    client->msg_sock = -1;
    client->cmd_sock = -1;
    memset(client->cmd_buffer, 0, SIZE_CMD);

    // setup sockets
    if (open_server() != 0) {
        psp2shell_exit();
        sceKernelExitDeleteThread(0);
        return -1;
    }

    // accept incoming connections
    printf("Waiting for connections...\n");
    while (!quit) {

        int client_sock = p2s_get_sock(server_sock_msg);
        if (client_sock < 0) {
            if (quit) {
                break;
            }
            printf("network disconnected: reset (%i)\n", client_sock);
            sceKernelDelayThread(1000 * 1000);
            open_server();
            continue;
        } else if (client_sock == 0) {
            printf("client_sock == 0\n");
        }

        printf("new connexion on port %i (sock=%i)\n", listen_port, client_sock);

        // max client/socket count reached
        if (client->msg_sock > 0) {
            printf("Connection refused, max client reached (1)\n");
            sceNetSocketClose(client_sock);
            continue;
        }

        printf("Connection accepted\n");

        client->msg_sock = client_sock;
        client->thid = sceKernelCreateThread("psp2shell_cmd", cmd_thread, 64, 0x4000, 0, 0x10000, 0);
        if (client->thid >= 0)
            sceKernelStartThread(client->thid, 0, NULL);
    }

    psp2shell_exit();

    sceKernelExitDeleteThread(0);
    return 0;
}

static int thread_kbuf(SceSize args, void *argp) {

    char buffer[512];
    memset(buffer, 0, 512);

    while (!quit) {

        memset(buffer, 0, 512);
        kpsp2shell_wait_buffer(buffer, 512);
        if (client->msg_sock > 0) {
            psp2shell_print(buffer);
        }
    }

    sceKernelExitDeleteThread(0);
    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    listen_port = 3333;

    // init pool
    taipool_init(0x100000); // 1M

    // load network modules
    p2s_netInit();

    client = taipool_alloc(sizeof(s_client));

    thid_kbuf = sceKernelCreateThread("psp2shell_kbuf", thread_kbuf, 64, 0x1000, 0, 0x10000, 0);
    if (thid_kbuf >= 0) {
        sceKernelStartThread(thid_kbuf, 0, NULL);
    }

    thid_wait = sceKernelCreateThread("psp2shell_wait", thread_wait, 64, 0x1000, 0, 0x10000, 0);
    if (thid_wait >= 0) {
        sceKernelStartThread(thid_wait, 0, NULL);
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    psp2shell_exit();
    return SCE_KERNEL_STOP_SUCCESS;
}

void module_exit(void) {

}

void psp2shell_exit() {

    quit = 1;

    close_client();

    close_server();

    if (thid_kbuf >= 0) {
        sceKernelDeleteThread(thid_kbuf);
    }

    taipool_term();
}
