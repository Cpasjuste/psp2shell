//
// Created by cpasjuste on 12/07/22.
//

#include <string.h>
#include <stdio.h>
#include <psp2kern/net/net.h>
#include <psp2kern/io/fcntl.h>

#include "psp2shell_k.h"
#include "utility.h"
#include "net.h"

int p2s_netInit() {
#ifndef __VITA_KERNEL__
    int loaded = sceSysmoduleIsLoaded(SCE_SYSMODULE_NET);
    if (loaded != 0) {
        sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
        loaded = sceSysmoduleIsLoaded(SCE_SYSMODULE_NET);
        if (loaded != 0) {
            return -1;
        }

        int ret = sceNetShowNetstat();
        if (ret == SCE_NET_ERROR_ENOTINIT) {
            SceNetInitParam netInitParam;
            netInitParam.memory = net_stack;
            netInitParam.size = NET_STACK_SIZE;
            netInitParam.flags = 0;
            if (sceNetInit(&netInitParam) != 0) {
                return -1;
            }
        }

        if (sceNetCtlInit() != 0) {
            return -1;
        }
    }
#endif

    return 0;
}

int p2s_bind_port(int sock, int port) {
    SceNetSockaddrIn serverAddress;

    // create server socket
    char sName[32];
    snprintf(sName, 32, "p2s_%i", port);
    sock = ksceNetSocket(sName, SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);

    // prepare the sockaddr structure
    serverAddress.sin_family = SCE_NET_AF_INET;
    serverAddress.sin_port = ksceNetHtons((unsigned short) port);
    serverAddress.sin_addr.s_addr = ksceNetHtonl(SCE_NET_INADDR_ANY);

    // bind
    if (ksceNetBind(sock, (SceNetSockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        //printf("sceNetBind failed\n");
        return -1;
    }

    // listen
    if (ksceNetListen(sock, 64) < 0) {
        //printf("sceNetListen failed\n");
        return -1;
    }

    return sock;
}

int p2s_get_sock(int sock) {
    SceNetSockaddrIn clientAddress;
    unsigned int c = sizeof(clientAddress);
    return ksceNetAccept(sock, (SceNetSockaddr *) &clientAddress, &c);
}

int p2s_close_sock(int sock) {
    if (sock >= 0) {
        ksceNetSocketClose(sock);
    }

    return -1;
}

int p2s_recv_all(int sock, void *buffer, int size, int flags) {
    int len;
    size_t sizeLeft = (size_t) size;

    while (sizeLeft) {
        len = ksceNetRecv(sock, buffer, sizeLeft, flags);
        if (len == 0) {
            size = 0;
            break;
        }
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
    size_t bufSize = P2S_SIZE_DATA;

    unsigned char *buffer = p2s_malloc(P2S_SIZE_DATA);
    if (buffer == NULL) {
        return 0;
    }

    memset(buffer, 0, P2S_SIZE_DATA);

    while (left > 0) {
        if (left < bufSize) bufSize = left;
        len = (size_t) p2s_recv_all(sock, buffer, (int) bufSize, 0);
        ksceIoWrite(fd, buffer, (SceSize) len);
        left -= len;
        received += len;
    }

    p2s_free(buffer);

    return received;
}
