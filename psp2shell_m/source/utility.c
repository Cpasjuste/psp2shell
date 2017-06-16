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

#include <psp2/kernel/threadmgr.h>
#include <psp2/sysmodule.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/appmgr.h>
#include <errno.h>

#include "../include/psp2shell.h"
#include "p2s_cmd.h"
#include "../include/main.h"
#include "../include/libmodule.h"
#include "../include/taipool.h"

#define NET_STACK_SIZE 0x4000
static unsigned char net_stack[NET_STACK_SIZE];

SceUID p2s_get_running_app_pid() {

    SceUID pid = -1;
    SceUID ids[20];

    int count = sceAppMgrGetRunningAppIdListForShell(ids, 20);
    if (count > 0) {
        pid = sceAppMgrGetProcessIdByAppIdForShell(ids[0]);
    }

    return pid;
}

SceUID p2s_get_running_app_id() {

    SceUID ids[20];

    int count = sceAppMgrGetRunningAppIdListForShell(ids, 20);
    if (count > 0) {
        return ids[0];
    }

    return 0;
}

int p2s_get_running_app_name(char *name) {

    int ret = -1;

    SceUID pid = p2s_get_running_app_pid();
    if (pid > 0) {
        ret = sceAppMgrAppParamGetString(pid, 9, name, 256);
        return ret;
    }

    return ret;
}

int p2s_get_running_app_title_id(char *title_id) {

    int ret = -1;

    SceUID pid = p2s_get_running_app_pid();
    if (pid > 0) {
        ret = sceAppMgrAppParamGetString(pid, 12, title_id, 256);
        return ret;
    }

    return ret;
}

int p2s_launch_app_by_uri(const char *tid) {

    char uri[32];

    sceAppMgrDestroyOtherApp();
    sceKernelDelayThread(1000 * 1000);

    snprintf(uri, 32, "psgm:play?titleid=%s", tid);

    for (int i = 0; i < 40; i++) {
        sceAppMgrLaunchAppByUri(0xFFFFF, uri);
        sceKernelDelayThread(10000);
    }

    return 0;
}

int p2s_reset_running_app() {

    char name[256];
    char id[16];
    char uri[32];

    if (p2s_get_running_app_title_id(id) != 0) {
        return -1;
    }

    if (p2s_get_running_app_name(name) != 0) {
        return -1;
    }

    sceAppMgrDestroyOtherApp();
    sceKernelDelayThread(1000 * 1000);

    snprintf(uri, 32, "psgm:play?titleid=%s", id);

    for (int i = 0; i < 40; i++) {
        sceAppMgrLaunchAppByUri(0xFFFFF, uri);
        sceKernelDelayThread(10000);
    }

    return 0;
}

void p2s_netInit() {

    int loaded = sceSysmoduleIsLoaded(SCE_SYSMODULE_NET);
    if (loaded != 0) {
        sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
        int ret = sceNetShowNetstat();
        if (ret == SCE_NET_ERROR_ENOTINIT) {
            SceNetInitParam netInitParam;
            netInitParam.memory = net_stack;
            netInitParam.size = NET_STACK_SIZE;
            netInitParam.flags = 0;
            sceNetInit(&netInitParam);
        }
        sceNetCtlInit();
    }
}

int p2s_bind_port(int sock, int port) {

    SceNetSockaddrIn serverAddress;

    // create server socket
    char sName[32];
    snprintf(sName, 32, "PSP2SHELLSOCK_%i", port);

    sock = sceNetSocket(sName,
                        SCE_NET_AF_INET,
                        SCE_NET_SOCK_STREAM, 0);

    // prepare the sockaddr structure
    serverAddress.sin_family = SCE_NET_AF_INET;
    serverAddress.sin_port = sceNetHtons((unsigned short) port);
    serverAddress.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);

    // bind
    if (sceNetBind(sock, (SceNetSockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        printf("sceNetBind failed\n");
        return -1;
    }

    // listen
    if (sceNetListen(sock, 64) < 0) {
        printf("sceNetListen failed\n");
        return -1;
    }

    return sock;
}

int p2s_get_sock(int sock) {

    SceNetSockaddrIn clientAddress;
    unsigned int c = sizeof(clientAddress);
    return sceNetAccept(sock, (SceNetSockaddr *) &clientAddress, &c);
}

int p2s_recvall(int sock, void *buffer, int size, int flags) {
    int len;
    size_t sizeLeft = (size_t) size;

    while (sizeLeft) {
        len = sceNetRecv(sock, buffer, sizeLeft, flags);
        if (len == 0) {
            size = 0;
            break;
        };
        if (len == -1) {
            break;
        } else {
            sizeLeft -= len;
            buffer += len;
        }
    }
    return size;
}

size_t p2s_recv_file(int sock, SceUID fd, long size) {

    size_t len, received = 0, left = (size_t) size;
    int bufSize = P2S_SIZE_DATA;

    unsigned char *buffer = taipool_alloc(P2S_SIZE_DATA);
    if (buffer == NULL) {
        return 0;
    }

    memset(buffer, 0, P2S_SIZE_DATA);

    while (left > 0) {
        if (left < bufSize) bufSize = left;
        len = (size_t) p2s_recvall(sock, buffer, bufSize, 0);
        sceIoWrite(fd, buffer, (SceSize) len);
        left -= len;
        received += len;
    }

    taipool_free(buffer);

    return received;
}

int p2s_hasEndSlash(char *path) {
    return path[strlen(path) - 1] == '/';
}

int p2s_removeEndSlash(char *path) {
    int len = strlen(path);

    if (path[len - 1] == '/') {
        path[len - 1] = '\0';
        return 1;
    }

    return 0;
}

int p2s_addEndSlash(char *path) {
    int len = strlen(path);
    if (len < MAX_PATH_LENGTH - 2) {
        if (path[len - 1] != '/') {
            strcat(path, "/");
            return 1;
        }
    }

    return 0;
}

void p2s_log_write(const char *msg) {

    sceIoMkdir("ux0:/tai/", 6);

    SceUID fd = sceIoOpen("ux0:/tai/psp2shell.log",
                          SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
    if (fd < 0)
        return;

    sceIoWrite(fd, msg, strlen(msg));
    sceIoClose(fd);
}
