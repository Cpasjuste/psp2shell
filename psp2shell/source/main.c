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

#include <psp2/appmgr.h>
#include "binn.h"
#include "main.h"
#include "utility.h"
#include "psp2cmd.h"
#include "psp2shell.h"
#include "module.h"
#include "thread.h"

#ifndef __VITA_KERNEL__

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/net/net.h>

#endif
#ifndef MODULE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <taihen.h>

#ifdef DEBUG
extern int psvDebugScreenPrintf(const char *format, ...);
#define printf psvDebugScreenPrintf
#else
#define printf(...)
#endif
#else
#include <taihen.h>
#include "libmodule.h"
#include "hooks.h"
#endif

#define PRINT_ERR(...) psp2shell_print_color(COL_RED, __VA_ARGS__)

static void cmd_reset();

#ifndef MODULE //TODO

int sceAppMgrAppMount(char *tid);

#endif
static SceUID thid_wait = -1;
static int listen_port = 3333;
static int quit = 0;
static s_client clients[MAX_CLIENT];
static int server_sock_msg;
static int server_sock_cmd;

static void sendOK(int sock) {
    sceNetSend(sock, "1\n", 2, 0);
}

static void sendNOK(int sock) {
    sceNetSend(sock, "0\n", 2, 0);
}

void psp2shell_print_color(int color, const char *fmt, ...) {

    char msg[SIZE_PRINT];
    memset(msg, 0, SIZE_PRINT);
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, SIZE_PRINT, fmt, args);
    va_end(args);

    binn *bin = binn_object();
    binn_object_set_int32(bin, "c", color);
    binn_object_set_str(bin, "0", msg);

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].msg_sock > 0) {
            sceNetSend(clients[i].msg_sock, binn_ptr(bin), (size_t) binn_size(bin), 0);
            int ret = sceNetRecv(clients[i].msg_sock, msg, 1, 0);
            if(ret < 0) { // wait for answer
                printf("psp2shell_print: sceNetRecv failed: %i\n", ret);
            } else {
                printf("psp2shell_print: sceNetRecv ok: %i\n", ret);
            }
        }
    }

    binn_free(bin);
}

void psp2shell_print(const char *fmt, ...) {

    char msg[SIZE_PRINT];
    memset(msg, 0, SIZE_PRINT);
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, SIZE_PRINT, fmt, args);
    va_end(args);
    psp2shell_print_color(0, msg);
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

#ifndef __VITA_KERNEL__ // TODO

static char *toAbsolutePath(s_FileList *fileList, char *path) {

    char *new_path;

    s_removeEndSlash(path);
    char *p = strrchr(path, '/');
    if (!p) { // relative path
        new_path = malloc(strlen(fileList->path) + strlen(path) + 1);
        sprintf(new_path, "%s/%s", fileList->path, path);
    } else {
        new_path = malloc(strlen(path));
        strcpy(new_path, path);
    }

    return new_path;
}

static ssize_t cmd_put(s_client *client, long size, char *name, char *dst) {

    char *new_path;
    if (strncmp(dst, "0", 1) == 0) {
        new_path = toAbsolutePath(client->fileList, client->fileList->path);
    } else {
        new_path = toAbsolutePath(client->fileList, dst);
    }

    // if dest is a directory, append source filename
    if (s_isDir(new_path)) {
        strcat(new_path, "/");
        strcat(new_path, name);
    }

    SceUID fd = s_open(new_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) {
        sendNOK(client->cmd_sock);
        PRINT_ERR("could not open file for writing: %s\n", new_path);
        free(new_path);
        return -1;
    }
    sendOK(client->cmd_sock);

    ssize_t received = s_recv_file(client->cmd_sock, fd, size);
    s_close(fd);

    psp2shell_print_color(COL_GREEN, "sent `%s` to `%s` (%i)", name, new_path, received);

    free(new_path);

    return received;
}

static int cmd_mount(char *tid) {
#ifndef __VITA_KERNEL__
#ifndef MODULE
    int res = sceAppMgrAppMount(tid);
    if (res != 0) {
        psp2shell_print_color(COL_RED, "could not mount title: %s (err=%i)\n", tid, res);
        return -1;
    }
#endif
#endif
    return 0;
}

static int cmd_umount(char *device) {
#ifndef __VITA_KERNEL__
    // hack to close all open files descriptors before umount
    // from 0x40010000 to 0x43ff00ff
    int i, j;
    for (j = 0x10000; j < 0x4000000; j += 0x10000) {
        for (i = 0x01; i < 0x100; i += 2) {
            sceIoClose(0x40000000 + j + i);
            sceIoDclose(0x40000000 + j + i);
        };
    }

    int res = sceAppMgrUmount(device);
    if (res != 0) {
        psp2shell_print_color(COL_RED, "could not umount device: %s (err=%i)\n", device, res);
        return -1;
    }
#endif
    return 0;
}

static void cmd_reload(int sock, long size) {
#ifndef __VITA_KERNEL__
    char eboot_path[256];
    char title_id[16];

    sceAppMgrAppParamGetString(0, 12, title_id, 256);
    sprintf(eboot_path, "ux0:/app/%s/eboot.bin", title_id);

    if (cmd_umount("app0:") != 0) {
        psp2shell_print_color(COL_RED, "reload failed: can't umount app0\n");
        return;
    }

    SceUID fd = s_open(eboot_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) {
        sendNOK(sock);
        PRINT_ERR("could not open file for writing: %s\n", eboot_path);
        return;
    }
    sendOK(sock);

    ssize_t received = s_recv_file(sock, fd, size);
    if (received > 0) {
        cmd_reset();
    } else {
        psp2shell_print_color(COL_RED, "reload failed, received size < 0\n");
    }
    s_close(fd);
#endif
}

static void cmd_reset() {
#ifndef __VITA_KERNEL__
    int res = sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL);
    if (res != 0) {
        psp2shell_print_color(COL_RED, "could not reset: %i\n", res);
    }
#endif
}

static void cmd_cd(s_client *client, char *path) {

    char oldPath[1024];
    memset(oldPath, 0, 1024);
    strncpy(oldPath, path, 1024);

    s_removeEndSlash(path);
    s_removeEndSlash(client->fileList->path);

    if (strcmp(path, "..") == 0) {
        char *p = strrchr(client->fileList->path, '/');
        if (p) {
            p[1] = '\0';
        } else {
            p = strrchr(client->fileList->path, ':');
            if (p) {
                if (strlen(client->fileList->path) - ((p + 1) - client->fileList->path) > 0) {
                    p[1] = '\0';
                } else {
                    strcpy(client->fileList->path, HOME_PATH);
                }
            } else {
                strcpy(client->fileList->path, HOME_PATH);
            }
        }
    } else if (s_exist(path)) {
        strcpy(client->fileList->path, path);
    } else {
        char tmp[1024];
        snprintf(tmp, 1024, "%s/%s", client->fileList->path, path);
        if (s_exist(tmp)) {
            strcpy(client->fileList->path, tmp);
        }
    }

    // update files
    s_fileListEmpty(client->fileList);
    int res = s_fileListGetEntries(client->fileList, client->fileList->path);
    if (res < 0) {
        psp2shell_print_color(COL_RED, "could not cd to directory: %s\n", client->fileList->path);
        strcpy(client->fileList->path, oldPath);
        s_fileListGetEntries(client->fileList, oldPath);
    }
}

static void cmd_ls(s_client *client, char *path) {

    int res, i;
    int noPath = strcmp(path, HOME_PATH) == 0;

    s_FileList *fileList = noPath ? client->fileList : malloc(sizeof(s_FileList));
    if (!noPath) {
        memset(fileList, 0, sizeof(s_FileList));
        strcpy(fileList->path, path);
    }

    s_fileListEmpty(fileList);
    res = s_fileListGetEntries(fileList, fileList->path);

    if (res < 0) {
        psp2shell_print_color(COL_RED, "directory does not exist: %s\n", path);
        if (!noPath) {
            s_fileListEmpty(fileList);
            free(fileList);
        }
        return;
    }

    size_t msg_size = (size_t) (fileList->length * 256) + 512;
    char *msg = malloc(msg_size);
    memset(msg, 0, msg_size);
    strcat(msg, "\n\n");
    for (i = 0; i < strlen(fileList->path); i++) {
        strcat(msg, "-");
    }
    sprintf(msg + strlen(msg), "\n%s\n", fileList->path);
    for (i = 0; i < strlen(fileList->path); i++) {
        strcat(msg, "-");
    }
    strcat(msg, "\n");

    s_FileListEntry *file_entry = fileList->head;
    for (i = 0; i < fileList->length; i++) {
        strncat(msg, "\t", 256);
        strncat(msg, file_entry->name, 256);
        strncat(msg, "\n", 256);
        file_entry = file_entry->next;
    }
    psp2shell_print("%s\n", msg);

    free(msg);
    if (!noPath) {
        s_fileListEmpty(fileList);
        free(fileList);
    }
}

static void cmd_pwd(s_client *client) {
    psp2shell_print("\n\n%s\n\n", client->fileList->path);
}

static void cmd_mv(s_client *client, char *src, char *dst) {

    char *new_src = toAbsolutePath(client->fileList, src);
    char *new_dst = toAbsolutePath(client->fileList, dst);

    int res = s_movePath(new_src, new_dst, MOVE_INTEGRATE | MOVE_REPLACE, NULL);
    if (res != 1) {
        psp2shell_print_color(COL_RED, "error: %i\n", res);
    } else {
        psp2shell_print_color(COL_GREEN, "moved `%s` to `%s`\n", new_src, new_dst);
    }

    free(new_src);
    free(new_dst);
}

static void cmd_rm(s_client *client, char *file) {

    char *new_path = toAbsolutePath(client->fileList, file);

    if (!s_isDir(new_path)) {
        int res = s_removePath(new_path, NULL);
        if (res != 1) {
            psp2shell_print_color(COL_RED, "error: %i\n", res);
        } else {
            psp2shell_print_color(COL_GREEN, "file deleted: %s\n", new_path);
        }
    } else {
        psp2shell_print_color(COL_RED, "not a file: %s\n", new_path);
    }

    free(new_path);
}

static void cmd_rmdir(s_client *client, char *path) {

    char *new_path = toAbsolutePath(client->fileList, path);

    int res = s_removePath(new_path, NULL);
    if (res != 1) {
        psp2shell_print_color(COL_RED, "error: %i\n", res);
    } else {
        psp2shell_print_color(COL_GREEN, "directory deleted: %s\n", new_path);
    }

    free(new_path);
}

#endif //__VITA_KERNEL__

static void cmd_parse(int client_id, char *buffer) {

    int type, count, size;
    BOOL is_cmd = binn_is_valid(binn_ptr(buffer), &type, &count, &size);

    if (is_cmd) {

        switch (binn_object_int32(buffer, "t")) {

#ifndef __VITA_KERNEL__ // TODO
            case CMD_CD:
                cmd_cd(&clients[client_id], binn_object_str(buffer, "0"));
                break;

            case CMD_LS:
                cmd_ls(&clients[client_id], binn_object_str(buffer, "0"));
                break;

            case CMD_PWD:
                cmd_pwd(&clients[client_id]);
                break;

            case CMD_RM:
                cmd_rm(&clients[client_id], binn_object_str(buffer, "0"));
                break;

            case CMD_RMDIR:
                cmd_rmdir(&clients[client_id], binn_object_str(buffer, "0"));
                break;

            case CMD_MV:
                cmd_mv(&clients[client_id],
                       binn_object_str(buffer, "0"),
                       binn_object_str(buffer, "1"));
                break;

            case CMD_PUT:
                cmd_put(&clients[client_id],
                        (long) binn_object_int64(buffer, "0"),
                        binn_object_str(buffer, "1"),
                        binn_object_str(buffer, "2"));
                break;

            case CMD_LAUNCH:
                s_launchAppByUriExit(binn_object_str(buffer, "0"));
                break;

            case CMD_RELOAD:
                cmd_reload(clients[client_id].cmd_sock, (long) binn_object_int64(buffer, "0"));
                break;

            case CMD_RESET:
                cmd_reset();
                break;

            case CMD_MOUNT:
                cmd_mount(binn_object_str(buffer, "0"));
                break;

            case CMD_UMOUNT:
                cmd_umount(binn_object_str(buffer, "0"));
                break;

//TODO:
#ifndef __VITA_KERNEL__
            case CMD_MODLS:
                ps_moduleList();
                break;

            case CMD_MODLD:
                ps_moduleLoadStart(binn_object_str(buffer, 0));
                break;

            case CMD_THLS:
                ps_threadList();
                break;
#endif
#endif //__VITA_KERNEL__
            default:
                PRINT_ERR("Unrecognized command\n");
                break;
        }
    }
}

int cmd_thread(SceSize args, void *argp) {

    printf("cmd_thread\n");

    int client_id = *((int *) argp);
    char *buf = malloc(SIZE_CMD);

    // init client file listing memory
#ifndef __VITA_KERNEL__
    printf("client->fileList malloc\n");
    clients[client_id].fileList = malloc(sizeof(s_FileList));
    memset(clients[client_id].fileList, 0, sizeof(s_FileList));
    strcpy(clients[client_id].fileList->path, HOME_PATH);
    s_fileListGetEntries(clients[client_id].fileList, HOME_PATH);
#endif

    // Welcome!
    printf("welcome client %i\n", client_id);
    welcome();

    // get data sock
    printf("get data sock\n");
    clients[client_id].cmd_sock = s_get_sock(server_sock_cmd);
    printf("got data sock: %i\n", clients[client_id].cmd_sock);

    while (!quit) {

        //char buf[SIZE_CMD];
        memset(buf, 0, SIZE_CMD);
        int read_size = sceNetRecv(clients[client_id].cmd_sock, buf, SIZE_CMD, 0);
        if (read_size < 0) {
            printf("sceNetRecv failed: %x\n", read_size);
            break;
        } else if(read_size > 0) {
            printf("sceNetRecv ok: %x\n", read_size);
            cmd_parse(client_id, buf);
        }
    }

    printf("closing connection\n");
    free(buf);
#ifndef __VITA_KERNEL__
    s_fileListEmpty(clients[client_id].fileList);
    free(clients[client_id].fileList);
#endif

    sceNetSocketClose(clients[client_id].cmd_sock);
    clients[client_id].cmd_sock = -1;
    sceNetSocketClose(clients[client_id].msg_sock);
    clients[client_id].msg_sock = -1;

#ifndef __VITA_KERNEL__ // TODO
    sceKernelExitDeleteThread(0);
#endif

    return 0;
}

static void close_con() {

    for (int i = 0; i < MAX_CLIENT; i++) {
        if (clients[i].msg_sock >= 0) {
            sceNetSocketClose(clients[i].msg_sock);
            clients[i].msg_sock = -1;
        }
        if (clients[i].cmd_sock >= 0) {
            sceNetSocketClose(clients[i].cmd_sock);
            clients[i].cmd_sock = -1;
        }
        //sceKernelWaitThreadEnd(clients[i].thid, &i, NULL);
        //sceKernelDeleteThread(clients[i].thid);
    }
    if (server_sock_msg >= 0) {
        sceNetSocketClose(server_sock_msg);
        server_sock_msg = -1;
    }
    if (server_sock_cmd >= 0) {
        sceNetSocketClose(server_sock_cmd);
        server_sock_cmd = -1;
    }
}

static int open_con() {

    close_con();

    server_sock_msg = s_bind_port(server_sock_msg, listen_port);
    if (server_sock_msg <= 0) {
        return -1;
    }
    server_sock_cmd = s_bind_port(server_sock_cmd, listen_port + 1);
    if (server_sock_cmd <= 0) {
        return -1;
    }

    return 0;
}

static int thread_wait(SceSize args, void *argp) {

    // setup clients data
    memset(clients, 0, MAX_CLIENT * sizeof(s_client));
    for (int i = 0; i < MAX_CLIENT; i++) {
        clients[i].msg_sock = -1;
        clients[i].cmd_sock = -1;
    }

    // setup sockets
    if (open_con() != 0) {
        psp2shell_exit();
#ifndef __VITA_KERNEL__ // TODO
        sceKernelExitDeleteThread(0);
#endif
        return -1;
    }

    // accept incoming connections
    printf("Waiting for connections...\n");
    while (!quit) {

        int client_sock = s_get_sock(server_sock_msg);
        if (client_sock < 0) {
            if(quit) {
                break;
            }
            printf("network disconnected: reset (%x)\n", client_sock);
#ifndef __VITA_KERNEL__
            sceKernelDelayThread(1000 * 1000);
#endif
            open_con();
            continue;
        } else if(client_sock == 0) {
            printf("client_sock == 0\n");
        }

        printf("new connexion on port %i (sock=%i)\n", listen_port, client_sock);
        // find a free client/socket
        int client_id= -1;
        for (int i = 0; i < MAX_CLIENT; i++) {
            if (clients[i].msg_sock < 0) {
                client_id = i;
                break;
            }
        }

        // max client/socket count reached
        if (client_id < 0) {
            printf("Connection refused, max client reached (%i)\n", MAX_CLIENT);
            sceNetSocketClose(client_sock);
            continue;
        }

        printf("Connection accepted\n");
        clients[client_id].msg_sock = client_sock;
        clients[client_id].thid = sceKernelCreateThread("cmd_thread", cmd_thread, 64, 0x10000, 0, 0x10000, 0);
        if (clients[client_id].thid >= 0)
            sceKernelStartThread(clients[client_id].thid, sizeof(int), (void *) &client_id);
    }

    psp2shell_exit();

#ifndef __VITA_KERNEL__ // TODO
    sceKernelExitDeleteThread(0);
#endif
    return 0;
}

#ifdef MODULE
int module_start(SceSize argc, const void *args) {
    listen_port = 3333;
    hooks_init();
#else

int psp2shell_init(int port, int delay) {
    listen_port = port;
#endif

    // load network modules
    s_netInit();

    thid_wait = sceKernelCreateThread("thread_wait_client", thread_wait, 64, 0x4000, 0, 0x10000, 0);
    if (thid_wait >= 0) {
        sceKernelStartThread(thid_wait, 0, NULL);
    }
#ifndef MODULE
    // give time to client if asked
    sceKernelDelayThread((SceUInt) (1000 * 1000 * delay));
    return 0;
#else
    return SCE_KERNEL_START_SUCCESS;
#endif
}

#ifdef MODULE
int module_stop(SceSize argc, const void *args) {
    psp2shell_exit();
    hooks_exit();
    return SCE_KERNEL_STOP_SUCCESS;
}
#endif

void psp2shell_exit() {
    quit = TRUE;
    close_con();
}
