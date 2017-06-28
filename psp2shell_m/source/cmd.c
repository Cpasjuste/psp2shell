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

#include "libmodule.h"
#include "psp2shell.h"
#include "file.h"
#include "module.h"
#include "thread.h"
#include "main.h"
#include "utility.h"

static void cmd_reset();

static void toAbsolutePath(s_FileList *fileList, char *path) {

    p2s_removeEndSlash(path);

    char *p = strrchr(path, ':');
    if (!p) { // relative path
        char np[MAX_PATH_LENGTH];
        strncpy(np, path, MAX_PATH_LENGTH);
        snprintf(path, MAX_PATH_LENGTH, "%s/%s", fileList->path, np);
    }
}

static ssize_t cmd_put(s_client *client, long size, char *name, char *dst) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: cmd_put\n");
    return 0;
#else
    char new_path[MAX_PATH_LENGTH];
    memset(new_path, 0, MAX_PATH_LENGTH);

    if (strncmp(dst, "0", 1) == 0) {
        strncpy(new_path, client->fileList.path, MAX_PATH_LENGTH);
        toAbsolutePath(&client->fileList, new_path);
    } else {
        strncpy(new_path, dst, MAX_PATH_LENGTH);
        toAbsolutePath(&client->fileList, new_path);
    }

    // if dest is a directory, append source filename
    if (s_isDir(new_path)) {
        strcat(new_path, "/");
        strcat(new_path, name);
    }

    SceUID fd = s_open(new_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) {
        p2s_cmd_send(client->cmd_sock, CMD_NOK);
        PRINT_ERR("could not open file for writing: %s\n", new_path);
        return -1;
    }

    p2s_cmd_send(client->cmd_sock, CMD_OK);

    ssize_t received = p2s_recv_file(client->cmd_sock, fd, size);
    s_close(fd);

    PRINT_OK("received `%s` to `%s` (%i)\n\n", name, new_path, received);

    return received;
#endif
}

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

#ifdef __KERNEL__
    PRINT_ERR("TODO: cmd_umount\n");
#else
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
#endif
    return 0;
}

static void cmd_title() {

#ifdef __KERNEL__
    PRINT_ERR("TODO: cmd_title\n");
#else
    char name[256];
    char id[16];

    if (p2s_get_running_app_name(name) == 0) {
        p2s_get_running_app_title_id(id);
        PRINT_OK("\n\n\tname: %s\n\tid: %s\n\tpid: 0x%08X\n\n",
                 name, id, p2s_get_running_app_pid());
    } else {
        PRINT_OK("\n\n\tname: SceShell\n\tpid: 0x%08X\n\n",
                 sceKernelGetProcessId());
    }
#endif
}

static void cmd_load(int sock, long size, const char *tid) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: cmd_load\n");
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

    char tid[16];

    if (p2s_get_running_app_title_id(tid) != 0) {
        p2s_cmd_send(sock, CMD_NOK);
        PRINT_ERR("can't reload SceShell...\n");
        return;
    }

    cmd_load(sock, size, tid);
}

static void cmd_reset() {
    if (p2s_reset_running_app() != 0) {
        PRINT_ERR("can't reset SceShell...\n");
    }
}

static void cmd_cd(s_client *client, char *path) {

    char oldPath[1024];
    memset(oldPath, 0, 1024);
    strncpy(oldPath, path, 1024);

    p2s_removeEndSlash(path);
    p2s_removeEndSlash(client->fileList.path);

    if (strcmp(path, "..") == 0) {
        char *p = strrchr(client->fileList.path, '/');
        if (p) {
            p[1] = '\0';
        } else {
            p = strrchr(client->fileList.path, ':');
            if (p) {
                if (strlen(client->fileList.path) - ((p + 1) - client->fileList.path) > 0) {
                    p[1] = '\0';
                } else {
                    strcpy(client->fileList.path, HOME_PATH);
                }
            } else {
                strcpy(client->fileList.path, HOME_PATH);
            }
        }
    } else if (s_exist(path)) {
        strcpy(client->fileList.path, path);
    } else {
        char tmp[MAX_PATH_LENGTH];
        snprintf(tmp, MAX_PATH_LENGTH, "%s/%s", client->fileList.path, path);
        if (s_exist(tmp)) {
            strncpy(client->fileList.path, tmp, MAX_PATH_LENGTH);
        }
    }

    // update files
    s_fileListEmpty(&client->fileList);
    int res = s_fileListGetEntries(&client->fileList, client->fileList.path);
    if (res < 0) {
        PRINT_ERR("could not cd to directory: %s\n", client->fileList.path);
        strncpy(client->fileList.path, oldPath, MAX_PATH_LENGTH);
        s_fileListGetEntries(&client->fileList, oldPath);
    }
}

static void cmd_ls(s_client *client, char *path) {

    s_FileList fileList;
    int res, i;

    bool noPath = strcmp(path, HOME_PATH) == 0;
    if (noPath) {
        strncpy(fileList.path, client->fileList.path, MAX_PATH_LENGTH);
    } else {
        if (strcmp(client->fileList.path, HOME_PATH) == 0) {
            strncpy(fileList.path, path, MAX_PATH_LENGTH);
        } else {
            char new_path[MAX_PATH_LENGTH];
            memset(new_path, 0, MAX_PATH_LENGTH);
            strncpy(new_path, path, MAX_PATH_LENGTH);
            toAbsolutePath(&client->fileList, new_path);
            strncpy(fileList.path, new_path, MAX_PATH_LENGTH);
        }
    }

    res = s_fileListGetEntries(&fileList, fileList.path);

    if (res < 0) {
        PRINT_ERR("directory does not exist: %s\n", fileList.path);
        s_fileListEmpty(&fileList);
        return;
    }

    PRINT("\n\n-------------------\n");
    PRINT("%s:\n", fileList.path);
    PRINT("-------------------\n");
    s_FileListEntry *file_entry = fileList.head;
    for (i = 0; i < fileList.length; i++) {
        PRINT("\t%s\n", file_entry->name);
        file_entry = file_entry->next;
    }
    PRINT("\r\n");

    s_fileListEmpty(&fileList);
}

static void cmd_pwd(s_client *client) {
    PRINT_OK("\n%s\n\n", client->fileList.path);
}

static void cmd_mv(s_client *client, char *src, char *dst) {

    char new_src[MAX_PATH_LENGTH];
    char new_dst[MAX_PATH_LENGTH];

    strncpy(new_src, src, MAX_PATH_LENGTH);
    strncpy(new_dst, dst, MAX_PATH_LENGTH);

    toAbsolutePath(&client->fileList, new_src);
    toAbsolutePath(&client->fileList, new_dst);

    int res = s_movePath(new_src, new_dst, MOVE_INTEGRATE | MOVE_REPLACE, NULL);
    if (res != 1) {
        PRINT_ERR("\nerror: %i\n\n", res);
    } else {
        PRINT_OK("\nmoved `%s` to `%s`\n\n", new_src, new_dst);
    }
}

static void cmd_rm(s_client *client, char *file) {

    char new_path[MAX_PATH_LENGTH];
    strncpy(new_path, file, MAX_PATH_LENGTH);

    toAbsolutePath(&client->fileList, new_path);

    if (!s_isDir(new_path)) {
        int res = s_removePath(new_path, NULL);
        if (res != 1) {
            PRINT_ERR("\nerror: %i\n\n", res);
        } else {
            PRINT_OK("\nfile deleted: %s\n\n", new_path);
        }
    } else {
        PRINT_ERR("\nnot a file: %s\n\n", new_path);
    }
}

static void cmd_rmdir(s_client *client, char *path) {

    char new_path[MAX_PATH_LENGTH];
    strncpy(new_path, path, MAX_PATH_LENGTH);
    toAbsolutePath(&client->fileList, new_path);

    int res = s_removePath(new_path, NULL);
    if (res != 1) {
        PRINT_ERR("\nerror: %i\n\n", res);
    } else {
        PRINT_OK("\ndirectory deleted: %s\n\n", new_path);
    }
}

static void cmd_memr(const char *address_str, const char *size_str) {

    unsigned int address = strtoul(address_str, NULL, 16);
    unsigned int size = strtoul(size_str, NULL, 16);
    unsigned int max = address + size;

    unsigned int *addr = (unsigned int *) address;

    while ((unsigned int) addr < max) {

        psp2shell_print_color(COL_HEX, "0x%08X: %08X %08X %08X %08X\n",
                              addr,
                              addr[0], addr[1], addr[2], addr[3]
        );

        addr += 4;
    }
}

static void cmd_memw(const char *address_str, const char *data_str) {

#ifdef __KERNEL__
    PRINT_ERR("TODO: cmd_memw\n");
#else
    unsigned int address = strtoul(address_str, NULL, 16);
    unsigned int size = strlen(data_str) / 8;

    // try to find module segment by address
    int segment = -1;
    unsigned int offset = 0;
    unsigned int uid = 0;

    SceUID ids[256];
    int count = 256;
    SceKernelModuleInfo moduleInfo;

    int res = sceKernelGetModuleList(0xFF, ids, &count);
    if (res != 0) {
        return;
    } else {
        for (int i = 0; i < count; i++) {
            memset(&moduleInfo, 0, sizeof(SceKernelModuleInfo));
            moduleInfo.size = sizeof(SceKernelModuleInfo);
            res = sceKernelGetModuleInfo(ids[i], &moduleInfo);
            if (res >= 0) {
                for (int j = 0; j < 4; j++) {
                    if (moduleInfo.segments[j].memsz <= 0) {
                        continue;
                    }
                    if (address >= (unsigned int) moduleInfo.segments[j].vaddr
                        && address <
                           (unsigned int) moduleInfo.segments[j].vaddr + (unsigned int) moduleInfo.segments[j].memsz) {
                        uid = moduleInfo.handle;
                        segment = j;
                        offset = address - (unsigned int) moduleInfo.segments[j].vaddr;
                        break;
                    }
                }
            }
        }
    }

    if (uid <= 0) {
        return;
    }

    for (unsigned int i = 0; i < size; i++) {
        char tmp[8];
        strncpy(tmp, data_str + (i * 8), 8);
        unsigned int data = strtoul(tmp, NULL, 16);
        taiInjectData(uid, segment, offset, &data, 4);
        offset += 4;
    }
#endif
}

static void cmd_reboot() {
#ifdef __KERNEL__
    PRINT_ERR("TODO: cmd_reboot\n");
#else
    psp2shell_exit();
    scePowerRequestColdReset();
#endif
}

void p2s_cmd_parse(s_client *client, P2S_CMD *cmd) {

    switch (cmd->type) {

        case CMD_CD:
            cmd_cd(client, cmd->args[0]);
            break;

        case CMD_LS:
            cmd_ls(client, cmd->args[0]);
            break;

        case CMD_PWD:
            cmd_pwd(client);
            break;

        case CMD_RM:
            cmd_rm(client, cmd->args[0]);
            break;

        case CMD_RMDIR:
            cmd_rmdir(client, cmd->args[0]);
            break;

        case CMD_MV:
            cmd_mv(client, cmd->args[0], cmd->args[1]);
            break;

        case CMD_PUT:
            cmd_put(client, strtoul(cmd->args[2], NULL, 0), cmd->args[0], cmd->args[1]);
            break;

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
            cmd_load(client->cmd_sock, strtoul(cmd->args[1], NULL, 0), cmd->args[0]);
            break;

        case CMD_RELOAD:
            cmd_reload(client->cmd_sock, strtoul(cmd->args[0], NULL, 0));
            break;

        case CMD_RESET:
            cmd_reset();
            break;

        case CMD_REBOOT:
            cmd_reboot();
            break;

        case CMD_MEMR:
            cmd_memr(cmd->args[0], cmd->args[1]);
            break;

        case CMD_MEMW:
            cmd_memw(cmd->args[0], cmd->args[1]);
            break;

        case CMD_MODLS:
            p2s_moduleList();
            break;

        case CMD_MODLS_PID:
            p2s_moduleListForPid((SceUID) strtoul(cmd->args[0], NULL, 16));
            break;

        case CMD_MODINFO:
            p2s_moduleInfo((SceUID) strtoul(cmd->args[0], NULL, 16));
            break;

        case CMD_MODINFO_PID:
            p2s_moduleInfoForPid((SceUID) strtoul(cmd->args[0], NULL, 16), (SceUID) strtoul(cmd->args[1], NULL, 16));
            break;

        case CMD_MODLOADSTART:
            p2s_moduleLoadStart(cmd->args[0]);
            break;

        case CMD_MODLOADSTART_PID:
            p2s_moduleLoadStartForPid((SceUID) strtoul(cmd->args[0], NULL, 16), cmd->args[1]);
            break;

        case CMD_MODSTOPUNLOAD:
            p2s_moduleStopUnload((SceUID) strtoul(cmd->args[0], NULL, 16));
            break;

        case CMD_MODSTOPUNLOAD_PID:
            p2s_moduleStopUnloadForPid((SceUID) strtoul(cmd->args[0], NULL, 16),
                                       (SceUID) strtoul(cmd->args[1], NULL, 16));
            break;

        case CMD_KMODLOADSTART:
            p2s_kmoduleLoadStart(cmd->args[0]);
            break;

        case CMD_KMODSTOPUNLOAD:
            p2s_kmoduleStopUnload((SceUID) strtoul(cmd->args[0], NULL, 16));
            break;

        case CMD_MODDUMP:
            p2s_moduleDumpForPid((SceUID) strtoul(cmd->args[0], NULL, 16),
                                 (SceUID) strtoul(cmd->args[1], NULL, 16),
                                 cmd->args[2]);
            break;

        case CMD_THLS:
            ps_threadList();
            break;

        default:
            PRINT_ERR("Unrecognized command\n");
            break;
    }
}
