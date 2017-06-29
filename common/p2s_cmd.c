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

#ifdef DEBUG
#ifdef __KERNEL__
#define printf ksceDebugPrintf
#else
int sceClibPrintf(const char *, ...);
#define printf sceClibPrintf
#endif
#endif

#include <libk/stdio.h>
#include <libk/string.h>
#include <libk/stdlib.h>
#include <libk/stdbool.h>
#include <libk/stdarg.h>
#include <sys/types.h>

#ifdef __KERNEL__
#include <psp2kern/kernel/threadmgr.h>
#include "usbasync.h"
#include "usbhostfs.h"
#define send(a, b, c, d) usbShellWrite((unsigned int)a, b, c)
#elif __USB__
#define send(a, b, c, d) printf("send: not implemented\n")
#define recv(a, b, c, d) printf("recv: not implemented\n")
#else // WIFI
#include <psp2/net/net.h>
#define send sceNetSend
#define recv sceNetRecv
#endif
#else
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/socket.h>

#ifdef PC_SIDE
#define send(a, b, c, d) usbShellWrite((unsigned int)a, b, c)
#include <usb.h>
#include "usbasync.h"
#include "usbhostfs.h"

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

#endif

#include "p2s_cmd.h"

#if defined(__KERNEL__) || defined(PC_SIDE)

static int usbShellWrite(unsigned int chan, const char *data, int size) {

    int ret = 0;

    //printf("usbWrite.usbAsyncWrite: %i\n", size);
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
    //printf("usbWrite.usbAsyncWrite: %i\n", ret);

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

int p2s_cmd_receive(int sock, P2S_CMD *cmd) {

    char buffer[P2S_SIZE_CMD];
    memset(buffer, 0, P2S_SIZE_CMD);

#if defined(__KERNEL__) || defined(PC_SIDE)
    int read = usbShellRead((unsigned int) sock, buffer, P2S_SIZE_CMD);
    if (read < 0) {
        return read;
    }
#else
    ssize_t read = recv(sock, buffer, P2S_SIZE_CMD, 0);
    if (read < 2) {
        return read <= 0 ? P2S_ERR_SOCKET : P2S_ERR_INVALID_CMD;
    }
#endif
    bool is_cmd = p2s_cmd_to_cmd(cmd, buffer) == 0;
    if (!is_cmd) {
        return P2S_ERR_INVALID_CMD;
    }

    return 0;
}

int p2s_cmd_wait_result(int sock) {

    char buffer[P2S_SIZE_CMD];
    memset(buffer, 0, P2S_SIZE_CMD);

#if defined(__KERNEL__) || defined(PC_SIDE)
    int read = usbShellRead((unsigned int) sock, buffer, P2S_SIZE_CMD);
    if (read < 0) {
        return read;
    }
#else
    ssize_t read = recv(sock, buffer, P2S_SIZE_CMD, 0);
    if (read < 2) {
        return -1;
    }
#endif
    P2S_CMD cmd;
    if (p2s_cmd_to_cmd(&cmd, buffer) != 0) {
        return -1;
    }

    if (cmd.type != CMD_OK) {
        return -1;
    }

    return 0;
}

size_t p2s_cmd_receive_buffer(int sock, void *buffer, size_t size) {

#ifndef __USB__
    ssize_t len;
    size_t left = size;

    while (left) {
        len = recv(sock, buffer, left, 0);
        if (len == 0) {
            size = 0;
            break;
        };
        if (len == -1) {
            break;
        } else {
            left -= len;
            buffer += len;
        }
    }
#endif
    return size;
}

void p2s_cmd_send(int sock, int cmdType) {

    char buffer[4];
    memset(buffer, 0, 4);
    snprintf(buffer, 4, "%i", cmdType);
    send(sock, buffer, 4, 0);
}

void p2s_cmd_send_cmd(int sock, P2S_CMD *cmd) {

    char buffer[P2S_SIZE_CMD];
    memset(buffer, 0, P2S_SIZE_CMD);

    if (p2s_cmd_to_string(buffer, cmd) == 0) {
        send(sock, buffer, strlen(buffer), 0);
    }
}

void p2s_cmd_send_fmt(int sock, const char *fmt, ...) {

    char buffer[P2S_SIZE_CMD];
    memset(buffer, 0, P2S_SIZE_CMD);
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, P2S_SIZE_CMD, fmt, args);
    va_end(args);

    send(sock, buffer, strlen(buffer), 0);
}

void p2s_cmd_send_string(int sock, int cmdType, const char *value) {

    char buffer[P2S_SIZE_STRING + 6];
    memset(buffer, 0, P2S_SIZE_STRING + 6);
    snprintf(buffer, P2S_SIZE_STRING + 6, "%i\"%s\"", cmdType, value);
    send(sock, buffer, strlen(buffer), 0);
}

void p2s_cmd_send_int(int sock, int cmdType, int value) {

    char buffer[16];
    memset(buffer, 0, 16);
    snprintf(buffer, 16, "%i\"%i\"", cmdType, value);
    send(sock, buffer, strlen(buffer), 0);
}

void p2s_cmd_send_long(int sock, int cmdType, long value) {

    char buffer[32];
    memset(buffer, 0, 32);
    snprintf(buffer, 32, "%i\"%ld\"", cmdType, value);
    send(sock, buffer, strlen(buffer), 0);
}

int p2s_cmd_to_string(char *buffer, P2S_CMD *cmd) {

    if (!buffer || !cmd) {
        return -1;
    }

    memset(buffer, 0, P2S_SIZE_CMD);
    sprintf(buffer, "%i", cmd->type);
    for (int i = 0; i < P2S_MAX_ARGS; i++) {
        snprintf(buffer + strlen(buffer), P2S_SIZE_CMD, "\"%s", cmd->args[i]);
    }
    strncat(buffer, "\"", P2S_SIZE_CMD);

    return 0;
}

int p2s_cmd_to_cmd(P2S_CMD *cmd, const char *buffer) {

    if (!cmd || !buffer) {
        return -1;
    }

    if (strlen(buffer) < 2) {
        return -1;
    }

    memset(cmd, 0, sizeof(P2S_CMD));

    // type
    char tmp[2];
    strncpy(tmp, buffer, 2);
    cmd->type = atoi(tmp);
    if (cmd->type < CMD_START) {
        cmd->type = 0;
        return -1;
    }

    const char *start = NULL, *end = buffer;

    for (int i = 0; i < P2S_MAX_ARGS; i++) {

        start = strstr(end, "\"");
        if (!start) {
            break;
        }
        start += 1;

        end = strstr(start, "\"");
        if (!end) {
            break;
        }

        strncpy(cmd->args[i], start, end - start);
    }

    return 0;
}
