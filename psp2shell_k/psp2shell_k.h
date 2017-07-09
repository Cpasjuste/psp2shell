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


#ifndef _PSP2SHELL_K_H_
#define _PSP2SHELL_K_H_

#define P2S_KMSG_SIZE   400

#ifdef __PSP2__

#include "libmodule.h"
#include "kp2s_cmd_parser.h"
#include "kp2s_utility.h"
#include "kp2s_hooks.h"
#include "kp2s_io.h"
#include "p2s_cmd.h"
#include "p2s_msg.h"

typedef struct {
    int msg_sock;
    int cmd_sock;
    P2S_CMD cmd;
    P2S_MSG msg;
    char path[MAX_PATH_LENGTH];
} kp2s_client;

#ifdef __KERNEL__
int kp2s_print_stdout(const char *data, size_t size);
int kp2s_print_stdout_user(const char *data, size_t size);
int kp2s_print_color(int color, const char *fmt, ...);
#define p2s_print_color kp2s_print_color
#else
int kp2s_print_stdout_user(const char *data, size_t size);
int kp2s_print_color_user(int color, const char *data, size_t size);
void p2s_print_color(int color, const char *fmt, ...);
#endif

#define PRINT(...) p2s_print_color(COL_YELLOW, __VA_ARGS__)
#define PRINT_COL(color, ...) p2s_print_color(color, __VA_ARGS__)
#define PRINT_ERR(fmt, ...) p2s_print_color(COL_RED, "\n\nNOK: " fmt "\n\r\n", ## __VA_ARGS__)
#define PRINT_ERR_CODE(func_name, code) p2s_print_color(COL_RED, "\n\nNOK: %s = 0x%08X\n\r\n", func_name, code)
#define PRINT_PROMPT() p2s_print_color(COL_NONE, "\r\n");

int kp2s_wait_cmd(P2S_CMD *cmd);

void kp2s_set_ready(bool ready);

int kp2s_print_module_info(SceUID pid, SceUID uid);

int kp2s_print_module_list(SceUID pid, int flags1, int flags2);

int kp2s_dump_module(SceUID pid, SceUID uid, const char *dst);

#endif // __PSP2__
#endif //_PSP2SHELL_K_H_