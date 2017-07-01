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

static bool is_root_path(const char *path) {
    return strncmp(path, HOME_PATH, MAX_PATH_LENGTH) == 0;
}

static void to_abs_path(const char *home, char *path) {

    kp2s_remove_slash(path);

    char *p = strrchr(path, ':');
    if (!p) { // relative path
        char new_path[MAX_PATH_LENGTH];
        strncpy(new_path, path, MAX_PATH_LENGTH);
        snprintf(path, MAX_PATH_LENGTH, "%s/%s", home, new_path);
    }
}


static void cmd_cd(kp2s_client *client, char *path) {

    char old_path[MAX_PATH_LENGTH];
    memset(old_path, 0, MAX_PATH_LENGTH);
    strncpy(old_path, client->path, MAX_PATH_LENGTH);

    kp2s_remove_slash(path);
    kp2s_remove_slash(client->path);

    if (strcmp(path, "..") == 0) {
        char *p = strrchr(client->path, '/');
        if (p) {
            p[1] = '\0';
        } else {
            p = strrchr(client->path, ':');
            if (p) {
                if (strlen(client->path) - ((p + 1) - client->path) > 0) {
                    p[1] = '\0';
                } else {
                    strcpy(client->path, HOME_PATH);
                    return;
                }
            } else {
                strcpy(client->path, HOME_PATH);
                return;
            }
        }
    } else if (kp2s_io_exist(path)) {
        strncpy(client->path, path, MAX_PATH_LENGTH);
    } else {
        char tmp[MAX_PATH_LENGTH];
        snprintf(tmp, MAX_PATH_LENGTH, "%s/%s", client->path, path);
        if (kp2s_io_exist(tmp)) {
            strncpy(client->path, tmp, MAX_PATH_LENGTH);
            return;
        }
    }

    if (strncmp(client->path, HOME_PATH, MAX_PATH_LENGTH) != 0
        && !kp2s_io_exist(client->path)) {
        PRINT_ERR_PROMPT("could not cd to directory: %s", client->path);
        strncpy(client->path, old_path, MAX_PATH_LENGTH);
    }
}

static void cmd_ls(kp2s_client *client, char *path) {

    bool relative = is_root_path(path);
    if (relative) {
        if (is_root_path(client->path)) {
            kp2s_io_list_drives();
        } else {
            kp2s_io_list_dir(client->path);
        }
    } else {
        if (kp2s_io_exist(path)) {
            kp2s_io_list_dir(path);
        } else {
            char new_path[MAX_PATH_LENGTH];
            memset(new_path, 0, MAX_PATH_LENGTH);
            snprintf(new_path, MAX_PATH_LENGTH, "%s/%s", client->path, path);
            kp2s_io_list_dir(new_path);
        }
    }
}

static void cmd_pwd(kp2s_client *client) {
    PRINT_OK_PROMPT("%s", client->path);
}

static void cmd_mv(kp2s_client *client, char *src, char *dst) {

    char new_src[MAX_PATH_LENGTH];
    char new_dst[MAX_PATH_LENGTH];

    strncpy(new_src, src, MAX_PATH_LENGTH);
    strncpy(new_dst, dst, MAX_PATH_LENGTH);
    to_abs_path(client->path, new_src);
    to_abs_path(client->path, new_dst);

    kp2s_io_move(new_src, new_dst, MOVE_INTEGRATE | MOVE_REPLACE);
}

static void cmd_rm(kp2s_client *client, char *file) {

    char new_path[MAX_PATH_LENGTH];

    strncpy(new_path, file, MAX_PATH_LENGTH);
    to_abs_path(client->path, new_path);

    if (!kp2s_io_isdir(new_path)) {
        kp2s_io_remove(new_path);
    } else {
        PRINT_ERR_PROMPT("not a file: `%s`", new_path);
    }
}

static void cmd_rmdir(kp2s_client *client, char *path) {

    char new_path[MAX_PATH_LENGTH];

    strncpy(new_path, path, MAX_PATH_LENGTH);
    to_abs_path(client->path, new_path);
    kp2s_io_remove(new_path);
}

int kp2s_cmd_parse(kp2s_client *client, P2S_CMD *cmd) {

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

        default: // not handled by kernel
            return -1;
    }

    return 0;
}
