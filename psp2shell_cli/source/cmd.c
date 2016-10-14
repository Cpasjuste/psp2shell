//
// Created by cpasjuste on 13/09/16.
//

#include <stdlib.h>
#include <sys/socket.h>
#include <libgen.h>
#include <errno.h>
#include <stdbool.h>

#include "main.h"
#include "utility.h"
#include "psp2cmd.h"
#include "cmd.h"
#include "binn.h"

extern void close_terminal();

bool response_ok(int sock) {
    char buf[2];
    recv(sock, buf, 2, 0);
    return buf[0] == '1';
}

ssize_t send_file(FILE *file, long size) {

    ssize_t len, progress = 0;
    char *buf = malloc(SIZE_BUFFER);
    memset(buf, 0, SIZE_BUFFER);

    while ((len = fread(buf, sizeof(char), SIZE_BUFFER, file)) > 0) {
        if (send(data_sock, buf, (size_t) len, 0) < 0) {
            printf("ERROR: Failed to send file. (errno = %d)\n", errno);
            break;
        }
        progress += len;
        printf("\t[%lu/%lu]\n", progress, size);
        memset(buf, 0, SIZE_BUFFER);
    }
    free(buf);
    return progress;
}

int cmd_cd(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_CD);
    binn_object_set_str(obj, "0", argv[1]);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    return 0;
}

int cmd_ls(int argc, char **argv) {

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_LS);
    binn_object_set_str(obj, "0", argc < 2 ? "root" : argv[1]);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    return 0;
}

int cmd_pwd(int argc, char **argv) {

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_PWD);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    return 0;
}

int cmd_rm(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    printf("remove `%s` ? (y/N)\n", argv[1]);
    char c;
    scanf("%c", &c);
    if (c == 'y') {
        binn *obj = binn_object();
        binn_object_set_int32(obj, "t", CMD_RM);
        binn_object_set_str(obj, "0", argv[1]);
        send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
        binn_free(obj);
    }
    return 0;
}

int cmd_rmdir(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    printf("remove `%s` ? (y/N)\n", argv[1]);
    char c;
    scanf("%c", &c);
    if (c == 'y') {
        binn *obj = binn_object();
        binn_object_set_int32(obj, "t", CMD_RMDIR);
        binn_object_set_str(obj, "0", argv[1]);
        send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
        binn_free(obj);
    }

    return 0;
}

int cmd_mv(int argc, char **argv) {

    if (argc < 3) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_MV);
    binn_object_set_str(obj, "0", argv[1]);
    binn_object_set_str(obj, "1", argv[2]);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    return 0;
}

int cmd_put(int argc, char **argv) {

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("ERROR: file does not exist: \"%s\"\n", argv[1]);
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_PUT);
    binn_object_set_int64(obj, "0", size);
    binn_object_set_str(obj, "1", basename(argv[1]));
    binn_object_set_str(obj, "2", argc < 3 ? "0" : argv[2]);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    if (response_ok(data_sock)) {
        send_file(fp, size);
    }

    fclose(fp);
    printf("done\n");

    return 0;
}

int cmd_launch(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_LAUNCH);
    binn_object_set_str(obj, "0", argv[1]);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    return 0;
}

int cmd_reset(int argc, char **argv) {

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_RESET);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    return 0;
}

int cmd_reload(int argc, char **argv) {

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("ERROR: file does not exist: \"%s\"\n", argv[1]);
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_RELOAD);
    binn_object_set_int64(obj, "0", size);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    if (response_ok(data_sock)) {
        send_file(fp, size);
    }

    fclose(fp);
    printf("done\n");

    return 0;
}

int cmd_mount(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_MOUNT);
    binn_object_set_str(obj, "0", argv[1]);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    return 0;
}

int cmd_umount(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_UMOUNT);
    binn_object_set_str(obj, "0", argv[1]);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    return 0;
}

int cmd_modls(int argc, char **argv) {

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_MODLS);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    return 0;
}

int cmd_thls(int argc, char **argv) {

    binn *obj = binn_object();
    binn_object_set_int32(obj, "t", CMD_THLS);
    send(data_sock, binn_ptr(obj), (size_t) binn_size(obj), 0);
    binn_free(obj);

    return 0;
}

int cmd_help(int argc, char **argv) {

    int i = 0;
    printf("psp2shell commands: \n\n");
    printf("-----------------------------------------------------\n");
    while (cmd[i].name != NULL) {
        printf("%s %s\n%s\n", cmd[i].name, cmd[i].args, cmd[i].desc);
        printf("-----------------------------------------------------\n");
        i++;
    }

    return 0;
}

int cmd_exit(int argc, char **argv) {
    close_terminal();
    exit(0);
}

COMMAND cmd[] = {
        {"cd",      "<remote_path>",              "Enter a directory.",                            cmd_cd},
        {"ls",      "<remote_path>",              "List a directory.",                             cmd_ls},
        {"pwd",     "",                           "Get working directory.",                        cmd_pwd},
        {"rm",      "<remote_file>",              "Remove a file",                                 cmd_rm},
        {"rmdir",   "<local_path> <remote_path>", "Remove a directory",                            cmd_rmdir},
        {"mv",      "<local_path> <remote_path>", "Move a file/directory",                         cmd_mv},
        {"put",     "<local_path> <remote_path>", "Upload a file.",                                cmd_put},
        {"reset",   "",                           "Restart the application.",                      cmd_reset},
        {"reload",  "<eboot.bin>",                "Send (eboot.bin) and restart the application.", cmd_reload},
        {"launch",  "<titleid>",                  "Launch title",                                  cmd_launch},
//        {"mount",   "<titleid>",                  "Mount titleid",                                 cmd_mount},
        {"umount",  "<dev:>",                     "Umount device.",                                cmd_umount},
        {"modlist", "",                           "List all loaded modules.",                      cmd_modls},
        {"thlist",  "",                           "List (own) running threads.",                   cmd_thls},
        {"?",       "",                           "Display the help.",                             cmd_help},
        {"help",    "",                           "Display the help.",                             cmd_help},
        {"exit",    "",                           "Exit the shell.",                               cmd_exit},
        {NULL, NULL, NULL}
};

COMMAND *cmd_find(char *name) {
    int i;

    for (i = 0; cmd[i].name; i++)
        if (strcmp(name, cmd[i].name) == 0)
            return (&cmd[i]);

    return NULL;
}
