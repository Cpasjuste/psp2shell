//
// Created by cpasjuste on 03/06/17.
//

#include <vitasdk.h>
#include <taihen.h>
#include <libk/string.h>
#include <libk/stdio.h>
#include <libk/stdlib.h>

#include "psp2shell.h"
#include "module.h"
#include "thread.h"
#include "main.h"
#include "utility.h"
#include "cmd.h"

static void cmd_reset();

static void sendOK(int sock) {
    sceNetSend(sock, "1\n", 2, 0);
}

static void sendNOK(int sock) {
    sceNetSend(sock, "0\n", 2, 0);
}

static char *toAbsolutePath(s_FileList *fileList, char *path) {

    p2s_removeEndSlash(path);
    char *p = strrchr(path, '/');
    if (!p) { // relative path
        sprintf(path, "%s/%s", fileList->path, path);
    }

    return path;
}

static ssize_t cmd_put(s_client *client, long size, char *name, char *dst) {

    char *new_path;
    if (strncmp(dst, "0", 1) == 0) {
        new_path = toAbsolutePath(&client->fileList, client->fileList.path);
    } else {
        new_path = toAbsolutePath(&client->fileList, dst);
    }

    // if dest is a directory, append source filename
    if (s_isDir(new_path)) {
        strcat(new_path, "/");
        strcat(new_path, name);
    }

    SceUID fd = s_open(new_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) {
        sendNOK(client->cmd_sock);
        PRINT_ERR("could not open file for writing: %s\n", new_path);
        return -1;
    }
    sendOK(client->cmd_sock);

    ssize_t received = p2s_recv_file(client->cmd_sock, fd, size);
    s_close(fd);

    psp2shell_print_color(COL_GREEN, "sent `%s` to `%s` (%i)", name, new_path, received);

    return received;
}

static int cmd_mount(char *tid) {
#ifndef MODULE
    int res = sceAppMgrAppMount(tid);
    if (res != 0) {
        PRINT_ERR("could not mount title: %s (err=%i)\n", tid, res);
        return -1;
    }
#endif
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
        psp2shell_print("%s (%s)\n", name, id);
    } else {
        psp2shell_print("SceShell\n");
    }
}

static void cmd_load(int sock, long size, const char *tid) {

    char path[256];

    sceAppMgrDestroyOtherApp();
    sceKernelDelayThread(1000 * 1000);

    sprintf(path, "ux0:/app/%s/eboot.bin", tid);

    SceUID fd = s_open(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
    if (fd < 0) {
        sendNOK(sock);
        PRINT_ERR("could not open file for writing: %s\n", path);
        return;
    }

    sendOK(sock);

    ssize_t received = p2s_recv_file(sock, fd, size);
    if (received > 0) {
        p2s_launch_app_by_uri(tid);
    } else {
        psp2shell_print_color(COL_RED, "reload failed, received size < 0\n");
    }
    s_close(fd);
}

static void cmd_reload(int sock, long size) {

    char tid[16];

    if (p2s_get_running_app_title_id(tid) != 0) {
        sendNOK(sock);
        PRINT_ERR("can't reload SceShell...\n");
        return;
    }

    cmd_load(sock, size, tid);
}

static void cmd_reset() {
    p2s_reset_running_app();
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
        char tmp[1024];
        snprintf(tmp, 1024, "%s/%s", client->fileList.path, path);
        if (s_exist(tmp)) {
            strcpy(client->fileList.path, tmp);
        }
    }

    // update files
    s_fileListEmpty(&client->fileList);
    int res = s_fileListGetEntries(&client->fileList, client->fileList.path);
    if (res < 0) {
        psp2shell_print_color(COL_RED, "could not cd to directory: %s\n", client->fileList.path);
        strcpy(client->fileList.path, oldPath);
        s_fileListGetEntries(&client->fileList, oldPath);
    }
}

static void cmd_ls(s_client *client, char *path) {

    int res, i;
    int noPath = strcmp(path, HOME_PATH) == 0;

    s_FileList fl;
    s_FileList *fileList = noPath ? &client->fileList : &fl;
    if (!noPath) {
        memset(fileList, 0, sizeof(s_FileList));
        strcpy(fileList->path, path);
    }

    s_fileListEmpty(fileList);
    res = s_fileListGetEntries(fileList, fileList->path);

    if (res < 0) {
        psp2shell_print_color(COL_RED, "directory does not exist: %s\n", path);
        if (!noPath) {
            s_fileListEmpty(fileList);
        }
        return;
    }

    size_t msg_size = (size_t) (fileList->length * 256) + 512;
    char msg[msg_size];
    memset(msg, 0, msg_size);
    strcat(msg, "\n\n");
    for (i = 0; i < strlen(fileList->path); i++) {
        strcat(msg, "-");
    }
    sprintf(msg + strlen(msg), "\n%s\n", fileList->path);
    for (i = 0; i < strlen(fileList->path); i++) {
        strcat(msg, "-");
    }
    strcat(msg, "\n");

    s_FileListEntry *file_entry = fileList->head;
    for (i = 0; i < fileList->length; i++) {
        strncat(msg, "\t", 256);
        strncat(msg, file_entry->name, 256);
        strncat(msg, "\n", 256);
        file_entry = file_entry->next;
    }
    psp2shell_print("%s\n", msg);

    if (!noPath) {
        s_fileListEmpty(fileList);
    }
}

static void cmd_pwd(s_client *client) {
    psp2shell_print("\n\n%s\n\n", client->fileList.path);
}

static void cmd_mv(s_client *client, char *src, char *dst) {

    char *new_src = toAbsolutePath(&client->fileList, src);
    char *new_dst = toAbsolutePath(&client->fileList, dst);

    int res = s_movePath(new_src, new_dst, MOVE_INTEGRATE | MOVE_REPLACE, NULL);
    if (res != 1) {
        psp2shell_print_color(COL_RED, "error: %i\n", res);
    } else {
        psp2shell_print_color(COL_GREEN, "moved `%s` to `%s`\n", new_src, new_dst);
    }
}

static void cmd_rm(s_client *client, char *file) {

    char *new_path = toAbsolutePath(&client->fileList, file);

    if (!s_isDir(new_path)) {
        int res = s_removePath(new_path, NULL);
        if (res != 1) {
            psp2shell_print_color(COL_RED, "error: %i\n", res);
        } else {
            psp2shell_print_color(COL_GREEN, "file deleted: %s\n", new_path);
        }
    } else {
        psp2shell_print_color(COL_RED, "not a file: %s\n", new_path);
    }
}

static void cmd_rmdir(s_client *client, char *path) {

    char *new_path = toAbsolutePath(&client->fileList, path);

    int res = s_removePath(new_path, NULL);
    if (res != 1) {
        psp2shell_print_color(COL_RED, "error: %i\n", res);
    } else {
        psp2shell_print_color(COL_GREEN, "directory deleted: %s\n", new_path);
    }
}

static void cmd_memr(const char *address_str, const char *size_str) {

    unsigned int address = strtoul(address_str, NULL, 16);
    unsigned int size = strtoul(size_str, NULL, 16);
    unsigned int max = address + size;

    unsigned int *addr = (unsigned int *) address;

    while ((unsigned int) addr < max) {

        psp2shell_print_color_advanced(64, COL_HEX,
                                       "0x%08X: %08X %08X %08X %08X\n",
                                       addr,
                                       addr[0], addr[1], addr[2], addr[3]
        );

        addr += 4;
    }
}

static void cmd_memw(const char *address_str, const char *data_str) {

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
}

static void cmd_reboot() {
    psp2shell_exit();
    scePowerRequestColdReset();
}

void cmd_parse(s_client *client) {

    S_CMD cmd;
    BOOL is_cmd = s_string_to_cmd(&cmd, client->cmd_buffer) == 0;

    if (is_cmd) {

        switch (cmd.type) {

            case CMD_CD:
                cmd_cd(client, cmd.arg0);
                break;

            case CMD_LS:
                cmd_ls(client, cmd.arg0);
                break;

            case CMD_PWD:
                cmd_pwd(client);
                break;

            case CMD_RM:
                cmd_rm(client, cmd.arg0);
                break;

            case CMD_RMDIR:
                cmd_rmdir(client, cmd.arg0);
                break;

            case CMD_MV:
                cmd_mv(client, cmd.arg0, cmd.arg1);
                break;

            case CMD_PUT:
                cmd_put(client, cmd.arg2, cmd.arg0, cmd.arg1);
                break;

            case CMD_LAUNCH:
                p2s_launch_app_by_uri(cmd.arg0);
                break;

            case CMD_TITLE:
                cmd_title();

            case CMD_MOUNT:
                cmd_mount(cmd.arg0);
                break;

            case CMD_UMOUNT:
                cmd_umount(cmd.arg0);
                break;

            case CMD_LOAD:
                cmd_load(client->cmd_sock, cmd.arg2, cmd.arg0);
                break;

            case CMD_RELOAD:
                cmd_reload(client->cmd_sock, cmd.arg2);
                break;

            case CMD_RESET:
                cmd_reset();
                break;

            case CMD_REBOOT:
                cmd_reboot();
                break;

            case CMD_MEMR:
                cmd_memr(cmd.arg0, cmd.arg1);
                break;

            case CMD_MEMW:
                cmd_memw(cmd.arg0, cmd.arg1);
                break;

            case CMD_MODLS:
                p2s_moduleList();
                break;

            case CMD_MODINFO:
                p2s_moduleInfo((SceUID) strtoul(cmd.arg0, NULL, 16));
                break;

            case CMD_MODLOAD:
                p2s_moduleLoad(cmd.arg0);
                break;

            case CMD_MODSTART:
                p2s_moduleStart((SceUID) strtoul(cmd.arg0, NULL, 16));
                break;

            case CMD_MODLOADSTART:
                p2s_moduleLoadStart(cmd.arg0);
                break;

            case CMD_MODSTOP:
                p2s_moduleStop((SceUID) strtoul(cmd.arg0, NULL, 16));
                break;

            case CMD_MODUNLOAD:
                p2s_moduleUnload((SceUID) strtoul(cmd.arg0, NULL, 16));
                break;

            case CMD_MODSTOPUNLOAD:
                p2s_moduleStopUnload((SceUID) strtoul(cmd.arg0, NULL, 16));
                break;

            case CMD_THLS:
                ps_threadList();
                break;

            default:
                PRINT_ERR("Unrecognized command\n");
                break;
        }
    }
}
