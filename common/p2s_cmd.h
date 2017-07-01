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

#ifndef _P2S_CMD_H_
#define _P2S_CMD_H_

#ifdef __cplusplus
extern "C" {
#endif

#define P2S_ERR_SOCKET      0x80000001
#define P2S_ERR_INVALID_CMD 0x80000002

#define P2S_SIZE_DATA       (8 * 1024)
#define P2S_SIZE_STRING     (128)

#define P2S_MAX_ARGS        (3)

typedef struct P2S_CMD {
    int type;
    char args[P2S_MAX_ARGS][P2S_SIZE_STRING];
} P2S_CMD;

#define P2S_SIZE_CMD        (400)

enum p2s_cmd_t {
    CMD_START = 10,

    CMD_LS,
    CMD_CD,
    CMD_RM,
    CMD_RMDIR,
    CMD_MKDIR,
    CMD_MV,
    CMD_PWD,
    CMD_GET,
    CMD_PUT,
    CMD_LAUNCH,
    CMD_LOAD,
    CMD_RELOAD,
    CMD_TEST,
    CMD_MOUNT,
    CMD_UMOUNT,
    CMD_TITLE,
    CMD_OVADD,
    CMD_OVDEL,
    CMD_OVLS,
    CMD_FIOSCP,
    CMD_MODLS,
    CMD_MODLS_PID,
    CMD_MODINFO,
    CMD_MODINFO_PID,
    CMD_MODLOADSTART,
    CMD_MODLOADSTART_PID,
    CMD_MODSTOPUNLOAD,
    CMD_MODSTOPUNLOAD_PID,
    CMD_KMODLOADSTART,
    CMD_KMODSTOPUNLOAD,
    CMD_MODDUMP,
    CMD_THLS,
    CMD_THPAUSE,
    CMD_THRESUME,
    CMD_MEMR,
    CMD_MEMW,
    CMD_RESET,
    CMD_REBOOT,
    CMD_EXIT,
    CMD_HELP,

    CMD_OK = 64,
    CMD_NOK = 65
};

int p2s_cmd_receive(int sock, P2S_CMD *cmd);

size_t p2s_cmd_receive_buffer(int sock, void *buffer, size_t size);

int p2s_cmd_wait_result(int sock);

void p2s_cmd_send(int sock, int cmdType);

void p2s_cmd_send_cmd(int sock, P2S_CMD *cmd);

void p2s_cmd_send_fmt(int sock, const char *fmt, ...);

void p2s_cmd_send_string(int sock, int cmdType, const char *value);

void p2s_cmd_send_int(int sock, int cmdType, int value);

void p2s_cmd_send_long(int sock, int cmdType, long value);

int p2s_cmd_to_string(char *buffer, P2S_CMD *c);

int p2s_cmd_to_cmd(P2S_CMD *c, const char *buffer);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _P2S_CMD_H_