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

#include <psp2/kernel/threadmgr.h>
#include <psp2/sysmodule.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>
#include <psp2/appmgr.h>

#endif

#ifdef MODULE

#include "libmodule.h"

#else

#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/errno.h>
#include <psp2/kernel/processmgr.h>

#endif

#include "main.h"
#include "psp2cmd.h"
#include "pool.h"

int s_launchAppByUriExit(char *titleid) {
#ifndef __VITA_KERNEL__
    char uri[32];
    sprintf(uri, "psgm:play?titleid=%s", titleid);
    sceKernelDelayThread(100000);
    sceAppMgrLaunchAppByUri(0xFFFFF, uri);
    sceKernelDelayThread(10000);
    sceAppMgrLaunchAppByUri(0xFFFFF, uri);
#ifndef MODULE //TODO:
    sceKernelExitProcess(0);
#endif
#endif
    return 0;
}

void s_netInit() {
#ifndef __VITA_KERNEL__
    int loaded = sceSysmoduleIsLoaded(SCE_SYSMODULE_NET);
    if (loaded != 0) {
        sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
        int ret = sceNetShowNetstat();
        if (ret == SCE_NET_ERROR_ENOTINIT) {
            SceNetInitParam netInitParam;
            netInitParam.memory = malloc(0x4000);
            netInitParam.size = 0x4000;
            netInitParam.flags = 0;
            sceNetInit(&netInitParam);
        }
        sceNetCtlInit();
    }
#endif
}

int s_bind_port(int sock, int port) {

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
    if (sceNetListen(sock, MAX_CLIENT) < 0) {
        printf("sceNetListen failed\n");
        return -1;
    }

    return sock;
}

int s_get_sock(int sock) {

    SceNetSockaddrIn clientAddress;
    unsigned int c = sizeof(clientAddress);
    return sceNetAccept(sock, (SceNetSockaddr *) &clientAddress, &c);
}

int s_recvall(int sock, void *buffer, int size, int flags) {
    int len;
    size_t sizeLeft = (size_t) size;

    while (sizeLeft) {
        len = sceNetRecv(sock, buffer, sizeLeft, flags);
        if (len == 0) {
            size = 0;
            break;
        };
        if (len == -1) {
            // TODO: nostdlib
#ifdef MODULE
            break;
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                break;
            }
#endif
        } else {
            sizeLeft -= len;
            buffer += len;
        }
    }
    return size;
}

static unsigned char *rcv_buffer = NULL;

size_t s_recv_file(int sock, SceUID fd, long size) {

    size_t len, received = 0, left = (size_t) size;
    int bufSize = SIZE_DATA;

    //unsigned char *buffer = pool_data_malloc(SIZE_DATA);
    if (rcv_buffer == NULL) {
        rcv_buffer = pool_data_malloc(SIZE_DATA);
    }

    if (rcv_buffer == NULL) {
        return 0;
    }

    memset(rcv_buffer, 0, SIZE_DATA);

    while (left > 0) {
        if (left < bufSize) bufSize = left;
        len = (size_t) s_recvall(sock, rcv_buffer, bufSize, 0);
        sceIoWrite(fd, rcv_buffer, (SceSize) len);
        left -= len;
        received += len;
    }

    return received;
}

int s_hasEndSlash(char *path) {
    return path[strlen(path) - 1] == '/';
}

int s_removeEndSlash(char *path) {
    int len = strlen(path);

    if (path[len - 1] == '/') {
        path[len - 1] = '\0';
        return 1;
    }

    return 0;
}

int s_addEndSlash(char *path) {
    int len = strlen(path);
    if (len < MAX_PATH_LENGTH - 2) {
        if (path[len - 1] != '/') {
            strcat(path, "/");
            return 1;
        }
    }

    return 0;
}

void s_log_write(const char *msg) {

    sceIoMkdir("ux0:/tai/", 6);

    SceUID fd = sceIoOpen("ux0:/tai/psp2shell.log",
                          SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
    if (fd < 0)
        return;

    sceIoWrite(fd, msg, strlen(msg));
    sceIoClose(fd);
}
