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
#ifdef __PSP2__

#include "psp2shell_k.h"

#ifdef __KERNEL__

#include "usbasync.h"

#define send(a, b, c, d) usbShellWrite((unsigned int)a, b, c)
#else
#define send(a, b, c, d) printf("send: not implemented\n")
#define recv(a, b, c, d) printf("recv: not implemented\n")
#endif
#endif

#ifdef PC_SIDE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <usb.h>
#include "usbhostfs.h"
#include "p2s_msg.h"

#define send(a, b, c, d) usbShellWrite((unsigned int)a, b, c)

#if defined BUILD_BIGENDIAN || defined _BIG_ENDIAN
uint32_t swap32(uint32_t i)
{
    uint8_t *p = (uint8_t *) &i;
    uint32_t ret;

    ret = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];

    return ret;
}
#define LE32(x) swap32(x)
#else
#define LE16(x) (x)
#define LE32(x) (x)
#define LE64(x) (x)
#endif

int euid_usb_bulk_write(
        usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);

extern usb_dev_handle *g_hDev;
#endif

#if defined(__KERNEL__) || defined(PC_SIDE)

static int usbShellWrite(unsigned int chan, const char *data, int size) {

    int ret = 0;

    //printf("usbWrite.usbAsyncWrite: sending = %i\n", size);
#ifdef PC_SIDE
    char buffer[sizeof(struct AsyncCommand) + size];
    struct AsyncCommand *cmd;
    cmd = (struct AsyncCommand *) buffer;
    cmd->magic = LE32(ASYNC_MAGIC);
    cmd->channel = ASYNC_SHELL;
    memcpy(&buffer[sizeof(struct AsyncCommand)], data, (size_t) size);

    ret = euid_usb_bulk_write(g_hDev, 0x4, buffer, sizeof(buffer), 10000);
    if (ret < 0) {
        printf("usbShellWrite.euid_usb_bulk_write: %s\n", usb_strerror());
    }
#else
    ret = usbAsyncWrite(chan, data, size);
#endif
    //printf("usbWrite.usbAsyncWrite: sent = %i\n", ret);

    return ret;
}

static int usbShellRead(unsigned int chan, char *data, int size) {

    int ret = 0;

#ifndef PC_SIDE
    //printf("usbRead.usbAsyncRead: %i\n", size);
    while (1) {
        ret = usbAsyncRead(chan, (unsigned char *) data, size);
        if (ret < 0) {
            //ksceKernelDelayThread(250000);
            //continue;
            break;
        }
        break;
    }
    //printf("usbRead.usbAsyncRead: %i\n", size);
#endif
    return ret;
}

#endif

int p2s_msg_receive(int sock, P2S_MSG *msg) {

    char buffer[P2S_SIZE_MSG];
    memset(buffer, 0, P2S_SIZE_MSG);

#if defined(__KERNEL__) || defined(PC_SIDE)
    int read = usbShellRead((unsigned int) sock, buffer, P2S_SIZE_MSG);
    if (read < 0) {
        memset(msg->buffer, 0, P2S_KMSG_SIZE);
        strncpy(msg->buffer, buffer, P2S_KMSG_SIZE);
        return read;
    }
#else
        ssize_t read = recv(sock, buffer, P2S_SIZE_MSG, 0);
        if (read < 3) {
            memset(msg->buffer, 0, P2S_KMSG_SIZE);
            strncpy(msg->buffer, buffer, P2S_KMSG_SIZE);
            return read <= 0 ? P2S_ERR_SOCKET : P2S_ERR_INVALID_MSG;
        }
#endif

    bool is_msg = p2s_msg_to_msg(msg, buffer) == 0;
    if (!is_msg) {
        memset(msg->buffer, 0, P2S_KMSG_SIZE);
        strncpy(msg->buffer, buffer, P2S_KMSG_SIZE);
        return P2S_ERR_INVALID_MSG;
    }

    return 0;
}

void p2s_msg_send(int sock, int color, const char *msg) {

    size_t len = strlen(msg) + 2;
    char buffer[len];
    memset(buffer, 0, len);
    snprintf(buffer, len, "%i%s", color, msg);
    send(sock, buffer, len, 0);
}

int p2s_msg_send_msg(int sock, P2S_MSG *msg) {

    char buffer[P2S_SIZE_MSG];

    if (p2s_msg_to_string(buffer, msg) == 0) {
        send(sock, buffer, strlen(buffer), 0);
        return 0;
    }

    return -1;
}

int p2s_msg_to_string(char *buffer, P2S_MSG *cmd) {

    if (!buffer || !cmd) {
        return -1;
    }

    memset(buffer, 0, P2S_SIZE_MSG);
    sprintf(buffer, "%i", cmd->color);
    snprintf(buffer + 2, P2S_SIZE_MSG, "%s", cmd->buffer);

    return 0;
}

int p2s_msg_to_msg_advanced(P2S_MSG *msg, const char *buffer, size_t len) {

    if (!msg || !buffer) {
        return -1;
    }

    if (len < 0) {
        return -1;
    }

    memset(msg, 0, sizeof(P2S_MSG));

    // type
    char tmp[2];
    strncpy(tmp, buffer, 2);
    msg->color = atoi(tmp);
    if (msg->color < COL_NONE) {
        msg->color = 0;
        return -1;
    }

    if (len > 0) {
        strncpy(msg->buffer, buffer + 2, len);
    }

    return 0;
}

int p2s_msg_to_msg(P2S_MSG *msg, const char *buffer) {

    return p2s_msg_to_msg_advanced(msg, buffer, strlen(buffer) - 2);
}
