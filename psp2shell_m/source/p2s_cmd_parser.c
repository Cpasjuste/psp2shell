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
#include "p2s_utility.h"
#include "p2s_module.h"
#include "p2s_thread.h"

static void cmd_reset();

static int cmd_mount(char *tid) {
/*
    int res = sceAppMgrAppMount(tid);
    if (res != 0) {
        PRINT_ERR("could not mount title: %s (err=%i)\n", tid, res);
        return -1;
    }
*/
    return 0;
}

static int cmd_umount(char *device) {

    // hack to close all open files descriptors before umount
    // from 0x40010000 to 0x43ff00ff
    int i, j;
    for (j = 0x10000; j < 0x4000000; j += 0x10000) {
        for (i = 0x01; i < 0x100; i += 2) {
            sceIoClose(0x40000000 + j + i);
            sceIoDclose(0x40000000 + j + i);
        };
    }

    int res = sceAppMgrUmount(device);
    // 0x80800002: Not mounted
    if (res != 0x80800002 && res != 0) {
        PRINT_ERR("could not umount device: %s (err=%x)\n", device, res);
        return -1;
    }

    return 0;
}

static void cmd_title() {

    char name[256];
    char id[16];

    if (p2s_get_running_app_name(name) == 0) {
        p2s_get_running_app_title_id(id);
        PRINT("\n\tname: %s\n\tid: %s\n\tpid: 0x%08X\n",
              name, id, p2s_get_running_app_pid());
    } else {
        PRINT("\n\tname: SceShell\n\tpid: 0x%08X\n",
              sceKernelGetProcessId());
    }

    PRINT_PROMPT();
}

static void cmd_load(int sock, long size, const char *tid) {

#if defined(__KERNEL__) || defined(__USB__)
    PRINT_ERR("TODO: cmd_load");
#else
    char path[256];

    sceAppMgrDestroyOtherApp();
    sceKernelDelayThread(1000 * 1000);

    sprintf(path, "ux0:/app/%s/eboot.bin", tid);

    SceUID fd = s_open(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
    if (fd < 0) {
        p2s_cmd_send(sock, CMD_NOK);
        PRINT_ERR("could not open file for writing: %s\n", path);
        return;
    }

    p2s_cmd_send(sock, CMD_OK);

    ssize_t received = p2s_recv_file(sock, fd, size);
    s_close(fd);

    if (received > 0) {
        p2s_launch_app_by_uri(tid);
    } else {
        PRINT_ERR("reload failed, received size < 0\n");
    }
#endif
}

static void cmd_reload(int sock, long size) {

#if defined(__KERNEL__) || defined(__USB__)
    PRINT_ERR("TODO: cmd_reload");
#else
    char tid[16];

    if (p2s_get_running_app_title_id(tid) != 0) {
        p2s_cmd_send(sock, CMD_NOK);
        PRINT_ERR("can't reload SceShell...");
        return;
    }

    cmd_load(sock, size, tid);
#endif
}

static void cmd_reset() {
    if (p2s_reset_running_app() != 0) {
        PRINT_ERR("can't reset SceShell...");
    }
}

void p2s_cmd_parse(P2S_CMD *cmd) {

    switch (cmd->type) {

        case CMD_LAUNCH:
            p2s_launch_app_by_uri(cmd->args[0]);
            break;

        case CMD_TITLE:
            cmd_title();

        case CMD_MOUNT:
            cmd_mount(cmd->args[0]);
            break;

        case CMD_UMOUNT:
            cmd_umount(cmd->args[0]);
            break;

        case CMD_LOAD:
            cmd_load(0, strtoul(cmd->args[1], NULL, 0), cmd->args[0]);
            break;

        case CMD_RELOAD:
            cmd_reload(0, strtoul(cmd->args[0], NULL, 0));
            break;

        case CMD_RESET:
            cmd_reset();
            break;

        case CMD_MODLS:
            p2s_moduleList();
            PRINT_PROMPT();
            break;

        case CMD_MODINFO:
            p2s_moduleInfo((SceUID) strtoul(cmd->args[0], NULL, 16));
            PRINT_PROMPT();
            break;

        case CMD_MODLOADSTART:
            p2s_moduleLoadStart(cmd->args[0]);
            PRINT_PROMPT();
            break;

        case CMD_MODSTOPUNLOAD:
            p2s_moduleStopUnload((SceUID) strtoul(cmd->args[0], NULL, 16));
            PRINT_PROMPT();
            break;

        case CMD_THLS:
            ps_threadList();
            PRINT_PROMPT();
            break;

        default:
            PRINT_ERR("Unrecognized command\n");
            break;
    }
}
