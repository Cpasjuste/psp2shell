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

#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/errno.h>
#include <psp2/sysmodule.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/io/fcntl.h>
#include <psp2/appmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <main.h>
#include <psp2cmd.h>

int s_launchAppByUriExit(char *titleid) {
    char uri[32];
    sprintf(uri, "psgm:play?titleid=%s", titleid);
    sceKernelDelayThread(100000);
    sceAppMgrLaunchAppByUri(0xFFFFF, uri);
    sceKernelDelayThread(10000);
    sceAppMgrLaunchAppByUri(0xFFFFF, uri);
    sceKernelExitProcess(0);
    return 0;
}

void s_netInit() {

    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

    int ret = sceNetShowNetstat();
    if (ret == SCE_NET_ERROR_ENOTINIT) {
        SceNetInitParam netInitParam;
        netInitParam.memory = malloc(1048576);
        netInitParam.size = 1048576;
        netInitParam.flags = 0;
        sceNetInit(&netInitParam);
    }

    sceNetCtlInit();
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
        printf("bind failed\n");
        return -1;
    }

    // listen
    sceNetListen(sock, MAX_CLIENT);

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

ssize_t s_recv_file(int sock, SceUID fd, long size) {

    ssize_t len, received = 0, left = size;
    unsigned char *buffer = malloc(SIZE_BUFFER);
    int bufSize = SIZE_BUFFER;

    while (left > 0) {
        if (left < bufSize) bufSize = left;
        len = s_recvall(sock, buffer, bufSize, 0);
        sceIoWrite(fd, buffer, (SceSize) len);
        left -= len;
        received += len;
    }
    free(buffer);

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
