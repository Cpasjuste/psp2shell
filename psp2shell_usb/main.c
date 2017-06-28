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

#include <libk/string.h>
#include <libk/stdio.h>
#include <libk/stdarg.h>
#include <libk/stdbool.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>
#include <taihen.h>

#include "usbhostfs/usbhostfs.h"
#include "usbhostfs/usbasync.h"
//#include "common/p2s_cmd.h"
//#include "common/shellcmd.h"

static bool quit = false;
static SceUID thid_wait = -1;

#define MAX_CLI 4096
#define CTX_BUF_SIZE 128

struct prnt_ctx {
    unsigned short fd;
    unsigned short len;
    unsigned char buf[CTX_BUF_SIZE];
};

int shprintf(const char *fmt, ...);

#define SHELL_PRINT(fmt, ...) shprintf("\xff" fmt "\xfe", ## __VA_ARGS__)
#define SHELL_PRINT_CMD(cmd, fmt, ...) shprintf("\xff" "%c" fmt "\xfe", cmd, ## __VA_ARGS__)

struct AsyncEndpoint g_endp;
struct AsyncEndpoint g_stdin;

int usbStdoutPrint(const char *data, int size) {
    usbAsyncWrite(ASYNC_STDOUT, data, size);

    return size;
}

int usbStderrPrint(const char *data, int size) {
    usbAsyncWrite(ASYNC_STDERR, data, size);

    return size;
}

int usbStdinRead(char *data, int size) {
    int ret = 0;

    while (1) {
        ret = usbAsyncRead(ASYNC_STDOUT, (unsigned char *) data, size);
        if (ret < 0) {
            ksceKernelDelayThread(250000);
            continue;
        }
        break;
    }

    return ret;
}

void usbPrintPrompt() {

    //SHELL_PRINT_CMD(SHELL_CMD_CWD, "%s", "host0:/");
    //SHELL_PRINT_CMD(SHELL_CMD_SUCCESS, "0x%08X", 0);
}

int usbShellInit(void) {

    int ret = usbAsyncRegister(ASYNC_SHELL, &g_endp);
    printf("usbAsyncRegister: ASYNC_SHELL = %i\n", ret);
    ret = usbAsyncRegister(ASYNC_STDOUT, &g_stdin);
    printf("usbAsyncRegister: ASYNC_STDOUT = %i\n", ret);
    //ttySetUsbHandler(usbStdoutPrint, usbStderrPrint, usbStdinRead);

    printf("usbWaitForConnect\n");
    ret = usbWaitForConnect();
    printf("usbWaitForConnect: %i\n", ret);

    usbPrintPrompt();

    return 0;
}

int usbShellReadInput(unsigned char *cli, char **argv, int max_cli, int max_arg) {

    int cli_pos = 0;
    int argc = 0;
    unsigned char *argstart = cli;

    while (1) {

        //printf("usbAsyncRead\n");
        if (usbAsyncRead(ASYNC_SHELL, &cli[cli_pos], 1) < 1) {
            //printf("usbAsyncRead failed\n");
            ksceKernelDelayThread(250000);
            continue;
        }
        //printf("usbAsyncRead: `%c`\n", cli[cli_pos]);

        if (cli[cli_pos] == 1) {
            cli[cli_pos] = 0;
            break;
        } else {
            if (cli_pos < (max_cli - 1)) {
                if (cli[cli_pos] == 0) {
                    if (argc < max_arg) {
                        argv[argc++] = (char *) argstart;
                        argstart = &cli[cli_pos + 1];
                    }
                }
                cli_pos++;
            }
        }
    }

    return argc;
}

static void cb(struct prnt_ctx *ctx, int type) {
    if (type == 0x200) {
        ctx->len = 0;
    } else if (type == 0x201) {
        usbAsyncWrite(ASYNC_SHELL, ctx->buf, ctx->len);
        ctx->len = 0;
    } else {
        if (type == '\n') {
            cb(ctx, '\r');
        }

        ctx->buf[ctx->len++] = type;
        if (ctx->len == CTX_BUF_SIZE) {
            usbAsyncWrite(ASYNC_SHELL, ctx->buf, ctx->len);
            ctx->len = 0;
        }
    }
}

int shprintf(const char *fmt, ...) {
    struct prnt_ctx ctx;
    va_list opt;

    cb(&ctx, 0x200);

    memset(ctx.buf, 0, 128);

    va_start(opt, fmt);
    int len = vsnprintf((char *) ctx.buf, 128, fmt, opt);
    va_end(opt);

    ctx.len = (unsigned short) len;
    cb(&ctx, 0x201);

    return 0;
}

static int thread_wait(SceSize args, void *argp) {

    printf("thread_wait start\n");

    int ret;
    unsigned char cli[MAX_CLI];
    int argc;
    char *argv[16];
    unsigned int vRet;

    usbShellInit();

    while (!quit) {

        argc = usbShellReadInput(cli, argv, MAX_CLI, 16);
        if (argc > 0) {

            //SHELL_PRINT("LS SUCCESS\n");
            //SHELL_PRINT_CMD(SHELL_CMD_SUCCESS, "0x%08X", 0);
            break;
        }
    }

    printf("thread_wait end\n");
    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    int res = usbhostfs_start();
    if (res != 0) {
        printf("module_start: usbhostfs_start failed\n");
        return SCE_KERNEL_START_FAILED;
    }

    thid_wait = ksceKernelCreateThread("psp2shell_wait", thread_wait, 0x64, 0x2000, 0, 0x10000, 0);
    if (thid_wait >= 0) {
        ksceKernelStartThread(thid_wait, 0, NULL);
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    quit = true;
    //ksceKernelDeleteThread(thid_wait);
    //ksceKernelWaitThreadEnd(thid_wait, 0, 0);

    printf("module_stop: usbhostfs_stop\n");
    int res = usbhostfs_stop();
    if (res != 0) {
        printf("module_stop: usbhostfs_stop failed\n");
    }

    return SCE_KERNEL_STOP_SUCCESS;
}
