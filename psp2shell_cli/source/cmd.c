//
// Created by cpasjuste on 13/09/16.
//

#include <stdlib.h>
#include <libgen.h>

#include "main.h"
#include "utility.h"
#include "p2s_cmd.h"
#include "cmd.h"

extern int psp2sell_cli_exit();

ssize_t send_file(int sock, FILE *file, long size) {

#ifdef __USB__
    return 0;
#else
    ssize_t len, progress = 0;
    char *buf = (char *) malloc(P2S_SIZE_DATA);
    memset(buf, 0, P2S_SIZE_DATA);

    while ((len = fread(buf, sizeof(char), P2S_SIZE_DATA, file)) > 0) {
        if (send(sock, buf, (size_t) len, 0) < 0) {
            printf("ERROR: Failed to send file. (errno = %d)\n", errno);
            break;
        }
        progress += len;
        printf("\t[%lu/%lu]\n", progress, size);
        memset(buf, 0, P2S_SIZE_DATA);
    }

    free(buf);
    return progress;
#endif
}

int cmd_cd(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_string(cmd_sock, CMD_CD, argv[1]);

    return 0;
}

int cmd_ls(int argc, char **argv) {

    p2s_cmd_send_string(cmd_sock, CMD_LS, argc < 2 ? "root" : argv[1]);

    return 0;
}

int cmd_pwd(int argc, char **argv) {

    p2s_cmd_send(cmd_sock, CMD_PWD);

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
        p2s_cmd_send_string(cmd_sock, CMD_RM, argv[1]);
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
        p2s_cmd_send_string(cmd_sock, CMD_RMDIR, argv[1]);
    }

    return 0;
}

int cmd_mv(int argc, char **argv) {

    if (argc < 3) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_fmt(cmd_sock, "%i\"%s\"%s\"", CMD_MV, argv[1], argv[2]);

    return 0;
}

int cmd_cp(int argc, char **argv) {

    if (argc < 3) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_fmt(cmd_sock, "%i\"%s\"%s\"", CMD_CP, argv[1], argv[2]);

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

    p2s_cmd_send_fmt(cmd_sock, "%i\"%s\"%s\"%ld\"",
                     CMD_PUT, basename(argv[1]), argc < 3 ? "0" : argv[2], size);

    if (p2s_cmd_wait_result(cmd_sock) == 0) {
        send_file(cmd_sock, fp, size);
    }

    fclose(fp);

    return 0;
}

int cmd_launch(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_string(cmd_sock, CMD_LAUNCH, argv[1]);

    return 0;
}

int cmd_reset(int argc, char **argv) {

    p2s_cmd_send(cmd_sock, CMD_RESET);

    return 0;
}

int cmd_load(int argc, char **argv) {

    if (argc < 3) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    FILE *fp = fopen(argv[2], "r");
    if (fp == NULL) {
        printf("ERROR: file does not exist: \"%s\"\n", argv[2]);
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    p2s_cmd_send_fmt(cmd_sock, "%i\"%s\"%ld\"",
                     CMD_LOAD, argv[1], size);

    if (p2s_cmd_wait_result(cmd_sock) == 0) {
        send_file(cmd_sock, fp, size);
    }

    fclose(fp);

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

    p2s_cmd_send_long(cmd_sock, CMD_RELOAD, size);

    if (p2s_cmd_wait_result(cmd_sock) == 0) {
        send_file(cmd_sock, fp, size);
    }

    fclose(fp);

    return 0;
}

int cmd_title(int argc, char **argv) {

    p2s_cmd_send(cmd_sock, CMD_TITLE);

    return 0;
}

int cmd_reboot(int argc, char **argv) {

    p2s_cmd_send(cmd_sock, CMD_REBOOT);

    return 0;
}

int cmd_mount(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_string(cmd_sock, CMD_MOUNT, argv[1]);

    return 0;
}

int cmd_umount(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_string(cmd_sock, CMD_UMOUNT, argv[1]);

    return 0;
}

int cmd_modls(int argc, char **argv) {

    p2s_cmd_send(cmd_sock, CMD_MODLS);

    return 0;
}

int cmd_modlsp(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_string(cmd_sock, CMD_MODLS_PID, argv[1]);

    return 0;
}

int cmd_modinfo(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_string(cmd_sock, CMD_MODINFO, argv[1]);

    return 0;
}

int cmd_modinfop(int argc, char **argv) {

    if (argc < 3) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_fmt(cmd_sock, "%i\"%s\"%s\"", CMD_MODINFO_PID, argv[1], argv[2]);

    return 0;
}

int cmd_modloadstart(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_string(cmd_sock, CMD_MODLOADSTART, argv[1]);

    return 0;
}

int cmd_modloadstartp(int argc, char **argv) {

    if (argc < 3) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_fmt(cmd_sock, "%i\"%s\"%s\"", CMD_MODLOADSTART_PID, argv[1], argv[2]);

    return 0;
}

int cmd_modstopunload(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_string(cmd_sock, CMD_MODSTOPUNLOAD, argv[1]);

    return 0;
}

int cmd_modstopunloadp(int argc, char **argv) {

    if (argc < 3) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_fmt(cmd_sock, "%i\"%s\"%s\"", CMD_MODSTOPUNLOAD_PID, argv[1], argv[2]);

    return 0;
}

int cmd_kmodloadstart(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_string(cmd_sock, CMD_KMODLOADSTART, argv[1]);

    return 0;
}

int cmd_kmodstopunload(int argc, char **argv) {

    if (argc < 2) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_string(cmd_sock, CMD_KMODSTOPUNLOAD, argv[1]);

    return 0;
}

int cmd_moddump(int argc, char **argv) {

    if (argc < 4) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_fmt(cmd_sock, "%i\"%s\"%s\"%s\"", CMD_MODDUMP, argv[1], argv[2], argv[3]);

    return 0;
}

int cmd_thls(int argc, char **argv) {

    p2s_cmd_send(cmd_sock, CMD_THLS);

    return 0;
}

int cmd_memr(int argc, char **argv) {

    if (argc < 3) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_fmt(cmd_sock, "%i\"%s\"%s\"", CMD_MEMR, argv[1], argv[2]);

    return 0;
}

int cmd_memw(int argc, char **argv) {

    if (argc < 3) {
        printf("incorrect number of arguments\n");
        return -1;
    }

    p2s_cmd_send_fmt(cmd_sock, "%i\"%s\"%s\"", CMD_MEMW, argv[1], argv[2]);

    return 0;
}

int cmd_help(int argc, char **argv) {

    int i = 0;
    while (cmd[i].name != NULL) {
        printf(GRN
        "%s %s"
        RES, cmd[i].name, cmd[i].args);
        printf(" -- %s\n", cmd[i].desc);
        i++;
    }

    return 0;
}

int cmd_exit(int argc, char **argv) {

    psp2sell_cli_exit();
    exit(0);
}

COMMAND cmd[] = {
        {"cd",        "<remote_path>",              "Enter a directory.",                            cmd_cd},
        {"ls",        "<remote_path>",              "List a directory.",                             cmd_ls},
        {"pwd",       "",                           "Get working directory.",                        cmd_pwd},
        {"rm",        "<remote_file>",              "Remove a file",                                 cmd_rm},
        {"rmdir",     "<remote_path>",              "Remove a directory",                            cmd_rmdir},
        {"mv",        "<remote_src> <remote_dst>",  "Move a file/directory",                         cmd_mv},
        {"cp",        "<local_src> <remote_dst>",   "Copy a file/directory",                         cmd_cp},
        {"put",       "<local_path> <remote_path>", "Upload a file.",                                cmd_put},
        {"reset",     "",                           "Restart the application.",                      cmd_reset},
        {"load",      "<title_id> <eboot.bin>",     "Send (eboot.bin) and restart the application.", cmd_load},
        {"reload",    "<eboot.bin>",                "Send (eboot.bin) and restart the application.", cmd_reload},
        {"launch",    "<titleid>",                  "Launch title",                                  cmd_launch},
        {"reboot",    "",                           "Reboot.",                                       cmd_reboot},
//        {"mount",   "<titleid>",                "Mount titleid",                                 cmd_mount},
//        {"umount",    "<dev:>",                     "Umount device.",                                cmd_umount},
        {"title",     "",                           "Get running title (name, id, pid)",             cmd_title},
        {"modlist",   "",                           "List loaded modules (for running process).",    cmd_modls},
        {"modlistp",  "<pid>",                      "List loaded modules for pid.",                  cmd_modlsp},
        {"modinfo",   "<uid>",                      "Get module information (for running process).", cmd_modinfo},
        {"modinfop",  "<pid> <uid>",                "Get module information for pid.",               cmd_modinfop},
        {"modstart",  "<path>",                     "Load/Start module (for running process).",      cmd_modloadstart},
        {"modstartp", "<pid> <path>",               "Load/Start module for pid.",                    cmd_modloadstartp},
        {"modstop",   "<uid>",                      "Stop/Unload module (for running process).",     cmd_modstopunload},
        {"modstopp",  "<pid> <uid>",                "Stop/Unload module for pid.",                   cmd_modstopunloadp},
        {"kmodstart", "<path>",                     "Load/Start kernel module.",                     cmd_kmodloadstart},
        {"kmodstop",  "<uid>",                      "Stop/Unload kernel module.",                    cmd_kmodstopunload},
        {"moddump",   "<pid> <uid> <path>",         "Dump module segments.",                         cmd_moddump},
        {"thlist",    "",                           "List (own) running threads.",                   cmd_thls},
        {"memr",      "<hex_address> <hex_size>",   "Read memory.",                                  cmd_memr},
        {"memw",      "<hex_address> <hex_data>",   "Write memory.",                                 cmd_memw},
        {"?",         "",                           "Display the help.",                             cmd_help},
        {"help",      "",                           "Display the help.",                             cmd_help},
        {"exit",      "",                           "Exit the shell.",                               cmd_exit},
        {NULL,        NULL,                         NULL}
};

COMMAND *cmd_find(char *name) {
    int i;

    for (i = 0; cmd[i].name; i++)
        if (strcmp(name, cmd[i].name) == 0)
            return (&cmd[i]);

    return NULL;
}
