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

#ifndef _P2S_MSG_H_
#define _P2S_MSG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "psp2shell_k.h"

#define P2S_ERR_SOCKET      0x80000001
#define P2S_ERR_INVALID_MSG 0x80000002

typedef struct P2S_MSG {
    int color;
    char buffer[P2S_KMSG_SIZE];
} P2S_MSG;

#define P2S_SIZE_MSG        (450)

enum p2s_colors_t {
    COL_NONE = 10,
    COL_RED,
    COL_YELLOW,
    COL_GREEN,
    COL_HEX
};

int p2s_msg_receive(int sock, P2S_MSG *msg);

void p2s_msg_send(int sock, int color, const char *msg);

int p2s_msg_send_msg(int sock, P2S_MSG *msg);

int p2s_msg_to_string(char *buffer, P2S_MSG *cmd);

int p2s_msg_to_msg(P2S_MSG *msg, const char *buffer);

int p2s_msg_to_msg_advanced(P2S_MSG *msg, const char *buffer, size_t len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //_P2S_MSG_H_
