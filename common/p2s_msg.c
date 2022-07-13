//
// Created by cpasjuste on 21/06/17.
//

#ifdef __PSP2__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <psp2kern/net/net.h>

#define send ksceNetSend
#define recv ksceNetRecv

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

#include "p2s_msg.h"

int p2s_msg_receive(int sock, P2S_MSG *msg) {
    char buffer[P2S_SIZE_MSG];
    memset(buffer, 0, P2S_SIZE_MSG);

    ssize_t read = recv(sock, buffer, P2S_SIZE_MSG, 0);
    if (read < 3) {
        memset(msg->buffer, 0, P2S_KMSG_SIZE);
        strncpy(msg->buffer, buffer, P2S_KMSG_SIZE);
        return read <= 0 ? P2S_ERR_SOCKET : P2S_ERR_INVALID_MSG;
    }

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
    snprintf(buffer, P2S_SIZE_MSG, "%i", cmd->color);
    snprintf(buffer + 2, P2S_SIZE_MSG - 2, "%s", cmd->buffer);

    return 0;
}

int p2s_msg_to_msg(P2S_MSG *msg, const char *buffer) {
    if (!msg || !buffer) {
        return -1;
    }

    if (strlen(buffer) < 3) {
        return -1;
    }

    memset(msg, 0, sizeof(P2S_MSG));

    // msg color
    char tmp[3];
    strncpy(tmp, buffer, 2);
    tmp[2] = '\0';
    msg->color = strtol(tmp, NULL, 10);
    if (msg->color < COL_NONE) {
        msg->color = 0;
        return -1;
    }

    strncpy(msg->buffer, buffer + 2, P2S_KMSG_SIZE);

    return 0;
}
