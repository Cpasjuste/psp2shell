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

#ifndef __VITA_KERNEL__

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/net/net.h>
#include <psp2/appmgr.h>
#include <taihen.h>

#endif

#include "main.h"
#include "utility.h"
#include "psp2cmd.h"
#include "psp2shell.h"
#include "module.h"
#include "thread.h"
#include "pool.h"

#ifndef MODULE

#include <psp2/power.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef DEBUG

extern int psvDebugScreenPrintf(const char *format, ...);

#define printf psvDebugScreenPrintf
#else
#define printf(...)
#endif
#else

#include "libmodule.h"
#include "hooks.h"
#include "../../psp2shell_k/include/psp2shell_k.h"

#endif


#define PRINT_ERR(...) psp2shell_print_color(COL_RED, __VA_ARGS__)

#ifndef MODULE //TODO

int sceAppMgrAppMount(char *tid);

#endif

//#define printf LOG

static SceUID thid_wait = -1;
static int listen_port = 3333;
static int quit = 0;
static s_client client;
static int server_sock_msg;
static int server_sock_cmd;

#ifndef __VITA_KERNEL__

static void cmd_reset();

static void sendOK(int sock) {
    sceNetSend(sock, "1\n", 2, 0);
}

static void sendNOK(int sock) {
    sceNetSend(sock, "0\n", 2, 0);
}

#endif

void psp2shell_print_color_advanced(SceSize size, int color, const char *fmt, ...) {

    char msg[size];
    memset(msg, 0, size);
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, size, fmt, args);
    va_end(args);

    snprintf(msg + strlen(msg), size, "%i", color);

    if (client.msg_sock > 0) {
        kpsp2shell_print_sock(client.msg_sock, strlen(msg), msg);
        /*
        sceNetSend(clients[i].msg_sock, msg, size, 0);
        int ret = sceNetRecv(clients[i].msg_sock, msg, 1, 0);
        if (ret < 0) { // wait for answer
            printf("psp2shell_print: sceNetRecv failed: %i\n", ret);
        }
        */
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

#ifndef __VITA_KERNEL__ // TODO

static char *toAbsolutePath(s_FileList *fileList, char *path) {

    p2s_removeEndSlash(path);
    char *p = strrchr(path, '/');
    if (!p) { // relative path
        sprintf(path, "%s/%s", fileList->path, path);
    }

    return path;
}

static ssize_t cmd_put(s_client *client, long size, char *name, char *dst) {

    char *new_path;
    if (strncmp(dst, "0", 1) == 0) {
        new_path = toAbsolutePath(&client->fileList, client->fileList.path);
    } else {
        new_path = toAbsolutePath(&client->fileList, dst);
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
        return -1;
    }
    sendOK(client->cmd_sock);

    ssize_t received = p2s_recv_file(client->cmd_sock, fd, size);
    s_close(fd);

    psp2shell_print_color(COL_GREEN, "sent `%s` to `%s` (%i)", name, new_path, received);

    return received;
}

static int cmd_mount(char *tid) {
#ifndef __VITA_KERNEL__
#ifndef MODULE
    int res = sceAppMgrAppMount(tid);
    if (res != 0) {
        PRINT_ERR("could not mount title: %s (err=%i)\n", tid, res);
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
    // 0x80800002: Not mounted
    if (res != 0x80800002 && res != 0) {
        PRINT_ERR("could not umount device: %s (err=%x)\n", device, res);
        return -1;
    }
#endif
    return 0;
}

static void cmd_title() {

    char name[256];
    char id[16];

    if (p2s_get_running_app_name(name) == 0) {
        p2s_get_running_app_title_id(id);
        psp2shell_print("%s (%s)\n", name, id);
    } else {
        psp2shell_print("SceShell\n");
    }
}

static void cmd_reload(int sock, long size) {
#ifndef __VITA_KERNEL__

    char path[256];
    char tid[16];

    if (p2s_get_running_app_title_id(tid) != 0) {
        sendNOK(sock);
        PRINT_ERR("can't reload SceShell...\n");
        return;
    }

    sceAppMgrDestroyOtherApp();
    sceKernelDelayThread(1000 * 1000);

    sprintf(path, "ux0:/app/%s/eboot.bin", tid);

    SceUID fd = s_open(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
    if (fd < 0) {
        sendNOK(sock);
        PRINT_ERR("could not open file for writing: %s\n", path);
        return;
    }

    sendOK(sock);

    ssize_t received = p2s_recv_file(sock, fd, size);
    if (received > 0) {
        p2s_launch_app_by_uri(tid);
    } else {
        psp2shell_print_color(COL_RED, "reload failed, received size < 0\n");
    }
    s_close(fd);
#endif
}

static void cmd_reset() {
#ifndef __VITA_KERNEL__
    p2s_reset_running_app();
#endif
}

static void cmd_cd(s_client *client, char *path) {

    char oldPath[1024];
    memset(oldPath, 0, 1024);
    strncpy(oldPath, path, 1024);

    p2s_removeEndSlash(path);
    p2s_removeEndSlash(client->fileList.path);

    if (strcmp(path, "..") == 0) {
        char *p = strrchr(client->fileList.path, '/');
        if (p) {
            p[1] = '\0';
        } else {
            p = strrchr(client->fileList.path, ':');
            if (p) {
                if (strlen(client->fileList.path) - ((p + 1) - client->fileList.path) > 0) {
                    p[1] = '\0';
                } else {
                    strcpy(client->fileList.path, HOME_PATH);
                }
            } else {
                strcpy(client->fileList.path, HOME_PATH);
            }
        }
    } else if (s_exist(path)) {
        strcpy(client->fileList.path, path);
    } else {
        char tmp[1024];
        snprintf(tmp, 1024, "%s/%s", client->fileList.path, path);
        if (s_exist(tmp)) {
            strcpy(client->fileList.path, tmp);
        }
    }

    // update files
    s_fileListEmpty(&client->fileList);
    int res = s_fileListGetEntries(&client->fileList, client->fileList.path);
    if (res < 0) {
        psp2shell_print_color(COL_RED, "could not cd to directory: %s\n", client->fileList.path);
        strcpy(client->fileList.path, oldPath);
        s_fileListGetEntries(&client->fileList, oldPath);
    }
}

static void cmd_ls(s_client *client, char *path) {

    int res, i;
    int noPath = strcmp(path, HOME_PATH) == 0;

    s_FileList fl;
    s_FileList *fileList = noPath ? &client->fileList : &fl;
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
        }
        return;
    }

    size_t msg_size = (size_t) (fileList->length * 256) + 512;
    char msg[msg_size];
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

    if (!noPath) {
        s_fileListEmpty(fileList);
    }
}

static void cmd_pwd(s_client *client) {
    psp2shell_print("\n\n%s\n\n", client->fileList.path);
}

static void cmd_mv(s_client *client, char *src, char *dst) {

    char *new_src = toAbsolutePath(&client->fileList, src);
    char *new_dst = toAbsolutePath(&client->fileList, dst);

    int res = s_movePath(new_src, new_dst, MOVE_INTEGRATE | MOVE_REPLACE, NULL);
    if (res != 1) {
        psp2shell_print_color(COL_RED, "error: %i\n", res);
    } else {
        psp2shell_print_color(COL_GREEN, "moved `%s` to `%s`\n", new_src, new_dst);
    }
}

static void cmd_rm(s_client *client, char *file) {

    char *new_path = toAbsolutePath(&client->fileList, file);

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
}

static void cmd_rmdir(s_client *client, char *path) {

    char *new_path = toAbsolutePath(&client->fileList, path);

    int res = s_removePath(new_path, NULL);
    if (res != 1) {
        psp2shell_print_color(COL_RED, "error: %i\n", res);
    } else {
        psp2shell_print_color(COL_GREEN, "directory deleted: %s\n", new_path);
    }
}

static void cmd_memr(const char *address_str, const char *size_str) {

    unsigned int address = strtoul(address_str, NULL, 16);
    unsigned int size = strtoul(size_str, NULL, 16);
    unsigned int max = address + size;

    unsigned int *addr = (unsigned int *) address;

    while ((unsigned int) addr < max) {

        psp2shell_print_color_advanced(64, COL_HEX,
                                       "0x%08X: %08X %08X %08X %08X\n",
                                       addr,
                                       addr[0], addr[1], addr[2], addr[3]
        );

        addr += 4;
    }
}

static void cmd_memw(const char *address_str, const char *data_str) {

    unsigned int address = strtoul(address_str, NULL, 16);
    unsigned int size = strlen(data_str) / 8;

    // try to find module segment by address
    int segment = -1;
    unsigned int offset = 0;
    unsigned int uid = 0;

    SceUID ids[256];
    int count = 256;
    SceKernelModuleInfo moduleInfo;

    int res = sceKernelGetModuleList(0xFF, ids, &count);
    if (res != 0) {
        return;
    } else {
        for (int i = 0; i < count; i++) {
            memset(&moduleInfo, 0, sizeof(SceKernelModuleInfo));
            moduleInfo.size = sizeof(SceKernelModuleInfo);
            res = sceKernelGetModuleInfo(ids[i], &moduleInfo);
            if (res >= 0) {
                for (int j = 0; j < 4; j++) {
                    if (moduleInfo.segments[j].memsz <= 0) {
                        continue;
                    }
                    if (address >= (unsigned int) moduleInfo.segments[j].vaddr
                        && address <
                           (unsigned int) moduleInfo.segments[j].vaddr + (unsigned int) moduleInfo.segments[j].memsz) {
                        uid = moduleInfo.handle;
                        segment = j;
                        offset = address - (unsigned int) moduleInfo.segments[j].vaddr;
                        break;
                    }
                }
            }
        }
    }

    if (uid <= 0) {
        return;
    }

    for (unsigned int i = 0; i < size; i++) {
        char tmp[8];
        strncpy(tmp, data_str + (i * 8), 8);
        unsigned int data = strtoul(tmp, NULL, 16);
        taiInjectData(uid, segment, offset, &data, 4);
        offset += 4;
    }
}

static void cmd_reboot() {
#ifndef __VITA_KERNEL__
    psp2shell_exit();
    scePowerRequestColdReset();
#endif
}

#endif //__VITA_KERNEL__

static void cmd_parse() {

    S_CMD cmd;
    BOOL is_cmd = s_string_to_cmd(&cmd, client.cmd_buffer) == 0;

    if (is_cmd) {

        switch (cmd.type) {

#ifndef __VITA_KERNEL__ // TODO
            case CMD_CD:
                cmd_cd(&client, cmd.arg0);
                break;

            case CMD_LS:
                cmd_ls(&client, cmd.arg0);
                break;

            case CMD_PWD:
                cmd_pwd(&client);
                break;

            case CMD_RM:
                cmd_rm(&client, cmd.arg0);
                break;

            case CMD_RMDIR:
                cmd_rmdir(&client, cmd.arg0);
                break;

            case CMD_MV:
                cmd_mv(&client, cmd.arg0, cmd.arg1);
                break;

            case CMD_PUT:
                cmd_put(&client, cmd.arg2, cmd.arg0, cmd.arg1);
                break;

            case CMD_LAUNCH:
                p2s_launch_app_by_uri(cmd.arg0);
                break;

            case CMD_TITLE:
                cmd_title();

            case CMD_MOUNT:
                cmd_mount(cmd.arg0);
                break;

            case CMD_UMOUNT:
                cmd_umount(cmd.arg0);
                break;

            case CMD_RELOAD:
                cmd_reload(client.cmd_sock, cmd.arg2);
                break;

            case CMD_RESET:
                cmd_reset();
                break;

            case CMD_REBOOT:
                cmd_reboot();
                break;

            case CMD_MEMR:
                cmd_memr(cmd.arg0, cmd.arg1);
                break;

            case CMD_MEMW:
                cmd_memw(cmd.arg0, cmd.arg1);
                break;
#ifndef __VITA_KERNEL__
            case CMD_MODLS:
                p2s_moduleList();
                break;

            case CMD_MODINFO:
                p2s_moduleInfo((SceUID) strtoul(cmd.arg0, NULL, 16));
                break;

            case CMD_MODLOAD:
                p2s_moduleLoad(cmd.arg0);
                break;

            case CMD_MODSTART:
                p2s_moduleStart((SceUID) strtoul(cmd.arg0, NULL, 16));
                break;

            case CMD_MODLOADSTART:
                p2s_moduleLoadStart(cmd.arg0);
                break;

            case CMD_MODSTOP:
                p2s_moduleStop((SceUID) strtoul(cmd.arg0, NULL, 16));
                break;

            case CMD_MODUNLOAD:
                p2s_moduleUnload((SceUID) strtoul(cmd.arg0, NULL, 16));
                break;

            case CMD_MODSTOPUNLOAD:
                p2s_moduleStopUnload((SceUID) strtoul(cmd.arg0, NULL, 16));
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

    // init client file listing memory
#ifndef __VITA_KERNEL__
    printf("client->fileList malloc\n");
    memset(&client.fileList, 0, sizeof(s_FileList));
    strcpy(client.fileList.path, HOME_PATH);
    s_fileListGetEntries(&client.fileList, HOME_PATH);
#endif

    // Welcome!
    printf("welcome client\n");
    welcome();

    // get data sock
    printf("get data sock\n");
    client.cmd_sock = p2s_get_sock(server_sock_cmd);
    printf("got data sock: %i\n", client.cmd_sock);

    while (!quit) {

        memset(client.cmd_buffer, 0, SIZE_CMD);

        int read_size = sceNetRecv(
                client.cmd_sock,
                client.cmd_buffer, SIZE_CMD, 0);

        if (read_size <= 0) {
            printf("sceNetRecv failed: %i\n", read_size);
            break;
        } else if (read_size > 0) {
            printf("sceNetRecv ok: %i\n", read_size);
            cmd_parse();
        }
    }

    printf("closing connection\n");

#ifndef __VITA_KERNEL__
    s_fileListEmpty(&client.fileList);
#endif
    if (client.cmd_sock >= 0) {
        sceNetSocketClose(client.cmd_sock);
        client.cmd_sock = -1;
    }
    if (client.msg_sock >= 0) {
        sceNetSocketClose(client.msg_sock);
        client.msg_sock = -1;
    }

    sceKernelExitDeleteThread(0);

    return 0;
}

static void close_con() {

    if (client.msg_sock >= 0) {
        sceNetSocketClose(client.msg_sock);
        client.msg_sock = -1;
        kpsp2shell_set_sock(-1);
    }
    if (client.cmd_sock >= 0) {
        sceNetSocketClose(client.cmd_sock);
        client.cmd_sock = -1;
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

static int thread_wait(SceSize args, void *argp) {

    // setup clients data
    memset(&client, 0, sizeof(s_client));
    client.msg_sock = -1;
    client.cmd_sock = -1;
    client.cmd_buffer = pool_data_malloc(SIZE_CMD);
    memset(client.cmd_buffer, 0, SIZE_CMD);

    // setup sockets
    if (open_con() != 0) {
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
            open_con();
            continue;
        } else if (client_sock == 0) {
            printf("client_sock == 0\n");
        }

        printf("new connexion on port %i (sock=%i)\n", listen_port, client_sock);

        // max client/socket count reached
        if (client.msg_sock > 0) {
            printf("Connection refused, max client reached (1)\n");
            sceNetSocketClose(client_sock);
            continue;
        }

        printf("Connection accepted\n");

        client.msg_sock = client_sock;
        kpsp2shell_set_sock(client.msg_sock);

        client.thid = sceKernelCreateThread("psp2shell_cmd", cmd_thread, 64, 0x4000, 0, 0x10000, 0);
        if (client.thid >= 0)
            sceKernelStartThread(client.thid, 0, NULL);
    }

    psp2shell_exit();

    sceKernelExitDeleteThread(0);

    return 0;
}

#ifdef MODULE

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    listen_port = 3333;
    ps2_hooks_init();

#else
    int psp2shell_init(int port, int delay) {
        listen_port = port;
#endif

    // init pool
    pool_create();

    // load network modules
    p2s_netInit();

    thid_wait = sceKernelCreateThread("psp2shell_wait", thread_wait, 64, 0x1000, 0, 0x10000, 0);
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
    ps2_hooks_exit();
    psp2shell_exit();
    return SCE_KERNEL_STOP_SUCCESS;
}

void module_exit(void) {

}

#endif

void psp2shell_exit() {
    quit = TRUE;
    close_con();
    pool_destroy();
}
