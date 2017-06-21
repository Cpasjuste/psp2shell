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

#include <psp2/net/net.h>
#include <libk/stdio.h>
#include <libk/string.h>
#include <libk/stdlib.h>
#include <libk/stdbool.h>
#include <libk/stdarg.h>
#include <sys/types.h>

#define send sceNetSend
#define recv sceNetRecv

#ifdef DEBUG

int sceClibPrintf(const char *, ...);

#define printf sceClibPrintf
#endif

#else

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/socket.h>

#endif

#include "p2s_cmd.h"

int p2s_cmd_receive(int sock, P2S_CMD *cmd) {

    char buffer[P2S_SIZE_CMD];
    memset(buffer, 0, P2S_SIZE_CMD);

    ssize_t read = recv(sock, buffer, P2S_SIZE_CMD, 0);
    if (read < 2) {
        return read <= 0 ? P2S_ERR_SOCKET : P2S_ERR_INVALID_CMD;
    }

    bool is_cmd = p2s_cmd_to_cmd(cmd, buffer) == 0;
    if (!is_cmd) {
        return P2S_ERR_INVALID_CMD;
    }

    return 0;
}

int p2s_cmd_wait_result(int sock) {

    char buffer[P2S_SIZE_CMD];
    memset(buffer, 0, P2S_SIZE_CMD);

    ssize_t read = recv(sock, buffer, P2S_SIZE_CMD, 0);
    if (read < 2) {
        return -1;
    }

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
