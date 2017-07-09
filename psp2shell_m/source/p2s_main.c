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

#include "psp2shell_k.h"

static SceUID thid_k2u = -1;
static int quit = 0;

void p2s_cmd_parse(P2S_CMD *cmd);

void p2s_print_color(int color, const char *fmt, ...) {

    char buffer[P2S_KMSG_SIZE];
    memset(buffer, 0, P2S_KMSG_SIZE);

    va_list args;
    va_start(args, fmt);
    size_t len = (size_t) vsnprintf(buffer, P2S_KMSG_SIZE, fmt, args);
    va_end(args);

    kp2s_print_color_user(color, buffer, len);
}

static int thread_k2u(SceSize args, void *argp) {

    printf("psp2shellusb_m: thread_k2u: start\n");
    P2S_CMD cmd;

    kp2s_set_ready(1);

    while (!quit) {

        memset(&cmd, 0, sizeof(cmd));
        int res = kp2s_wait_cmd(&cmd);
        if (res == 0) {
            //PRINT("\nkp2s_wait_cmd(u): type=%i, arg[0]=%s\r\n", cmd.type, cmd.args[0]);
            p2s_cmd_parse(&cmd);
        } else {
            sceKernelDelayThread(1000);
        }
    }

    kp2s_set_ready(0);

    printf("psp2shellusb_m: thread_k2u: exit\n");

    sceKernelExitDeleteThread(0);
    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {

    printf("psp2shellusb_m: module_start\n");
    thid_k2u = sceKernelCreateThread("p2s_k2u", thread_k2u, 128, 0x5000, 0, 0x10000, 0);
    if (thid_k2u >= 0) {
        sceKernelStartThread(thid_k2u, 0, NULL);
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    printf("psp2shellusb_m: module_stop\n");
    quit = 1;
    kp2s_set_ready(0);

    if (thid_k2u >= 0) {
        printf("psp2shellusb_m: wait thid_k2u thread end...\n");
        sceKernelWaitThreadEnd(thid_k2u, 0, NULL);
        //sceKernelDeleteThread(thid_k2u);
        printf("psp2shellusb_m: thid_k2u thread ended\n");
    }

    return SCE_KERNEL_STOP_SUCCESS;
}

void module_exit(void) {

}
