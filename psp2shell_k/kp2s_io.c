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

void *p2s_data_buf = NULL;

static char *mount_points[] = {
        "app0:",
        "gro0:",
        "grw0:",
        "os0:",
        "pd0:",
        "sa0:",
        "savedata0:",
        "tm0:",
        "ud0:",
        "ur0:",
        "ux0:",
        "vd0:",
        "vs0:",
        "host0:"
};

#define N_MOUNT_POINTS (sizeof(mount_points) / sizeof(char **))

int kp2s_io_list_drives() {

    PRINT("\n----\n");
    PRINT("root\n");
    PRINT("----\n");

    for (int i = 0; i < N_MOUNT_POINTS; i++) {
        if (mount_points[i]) {
            SceIoStat stat;
            memset(&stat, 0, sizeof(SceIoStat));
            if (sceIoGetstat(mount_points[i], &stat) >= 0) {
                PRINT("\t%s\n", mount_points[i]);
            }
        }
    }

    return 0;
};

int kp2s_io_list_dir(const char *path) {

    SceUID dfd = sceIoDopen(path);
    if (dfd < 0) {
        PRINT_ERR("directory does not exist: %s", path);
        return dfd;
    }

    int len = strlen(path);

    PRINT("\n");
    for (int i = 0; i < len; i++) {
        PRINT("-");
    }
    PRINT("\n%s\n", path);
    for (int i = 0; i < len; i++) {
        PRINT("-");
    }
    PRINT("\n");

    int res = 0;
    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));
        res = sceIoDread(dfd, &dir);
        if (res > 0) {
            if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
                continue;
            if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                PRINT("\t%s/\n", dir.d_name);
            } else {
                PRINT("\t%s\n", dir.d_name);
            }
        }
    } while (res > 0);

    sceIoDclose(dfd);

    return 0;
}

bool kp2s_io_isdir(char *path) {

    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));

    int res = sceIoGetstat(path, &stat);
    if (res < 0)
        return false;

    return SCE_S_ISDIR(stat.st_mode) == 1;
}

SceOff kp2s_io_get_size(const char *file) {

    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));

    if (sceIoGetstat(file, &stat) >= 0) {
        return stat.st_size;
    }

    return 0;
}

int kp2s_io_exist(const char *file) {

    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));

    return sceIoGetstat(file, &stat) >= 0;
}

int kp2s_io_remove(const char *path) {

    SceUID dfd = sceIoDopen(path);

    if (dfd >= 0) {

        int res = 0;

        do {
            SceIoDirent dir;
            memset(&dir, 0, sizeof(SceIoDirent));

            res = sceIoDread(dfd, &dir);
            if (res > 0) {
                char new_path[strlen(path) + strlen(dir.d_name) + 2];
                snprintf(new_path, MAX_PATH_LENGTH, "%s%s%s", path, kp2s_has_slash(path) ? "" : "/", dir.d_name);

                if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                    int ret = kp2s_io_remove(new_path);
                    if (ret <= 0) {
                        PRINT_ERR_CODE("rmdir", ret);
                        sceIoDclose(dfd);
                        return ret;
                    }
                } else {
                    PRINT("rm | `%s` ", new_path);
                    int ret = sceIoRemove(new_path);
                    if (ret < 0) {
                        PRINT_ERR_CODE("sceIoRemove", ret);
                        sceIoDclose(dfd);
                        return ret;
                    } else {
                        PRINT_COL(COL_GREEN, "OK\n");
                    }
                }
            }
        } while (res > 0);

        sceIoDclose(dfd);

        PRINT("rm | `%s` ", path);
        int ret = sceIoRmdir(path);
        if (ret < 0) {
            PRINT_ERR_CODE("sceIoRmdir", ret);
            return ret;
        } else {
            PRINT_COL(COL_GREEN, "OK\n");
        }
    } else {

        PRINT("rm | `%s` ", path);
        int ret = sceIoRemove(path);
        if (ret < 0) {
            PRINT_ERR_CODE("sceIoRemove", ret);
            return ret;
        } else {
            PRINT_COL(COL_GREEN, "OK\n");
        }
    }

    return 1;
}

int kp2s_io_copy_file(const char *src_path, const char *dst_path) {

    PRINT("cp | `%s` => `%s` ", src_path, dst_path);

    // The source and destination paths are identical
    if (strcasecmp(src_path, dst_path) == 0) {
        PRINT_ERR("src = dst");
        return -1;
    }

    // The destination is a subfolder of the source folder
    int len = strlen(src_path);
    if (strncasecmp(src_path, dst_path, (size_t) len) == 0
        && (dst_path[len] == '/' || dst_path[len - 1] == '/')) {
        PRINT_ERR("dst inside src");
        return -2;
    }

    SceUID fdsrc = sceIoOpen(src_path, SCE_O_RDONLY, 0);
    if (fdsrc < 0) {
        PRINT_ERR_CODE("sceIoOpen(src)", fdsrc);
        return fdsrc;
    }

    SceUID fddst = sceIoOpen(dst_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fddst < 0) {
        PRINT_ERR_CODE("sceIoOpen(dst)", fddst);
        sceIoClose(fdsrc);
        return fddst;
    }

    int progress_bar[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    SceOff size = kp2s_io_get_size(src_path);
    SceOff progress = 0;

    if (p2s_data_buf == NULL) {
        p2s_data_buf = p2s_malloc(TRANSFER_SIZE);
    }

    while (1) {

        int read = sceIoRead(fdsrc, p2s_data_buf, TRANSFER_SIZE);
        if (read < 0) {
            sceIoClose(fddst);
            sceIoClose(fdsrc);
            sceIoRemove(dst_path);
            PRINT_ERR_CODE("read(src)", read);
            return read;
        }

        if (read == 0) {
            break;
        }

        int write = sceIoWrite(fddst, p2s_data_buf, (SceSize) read);
        if (write < 0) {
            sceIoClose(fddst);
            sceIoClose(fdsrc);
            sceIoRemove(dst_path);
            PRINT_ERR_CODE("write(dst)", write);
            return write;
        }

        progress += write;
        int percent = (int) (((float) progress / (float) size) * 100);
        int index = percent / 10;
        if (percent > index * 10 && progress_bar[index] <= 0) {
            PRINT("#");
            progress_bar[index] = 1;
        }
    }

    // Inherit file stat
    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));
    ksceIoGetstatByFd(fdsrc, &stat);
    ksceIoChstatByFd(fddst, &stat, 0x3B);

    sceIoClose(fddst);
    sceIoClose(fdsrc);

    PRINT_COL(COL_GREEN, " OK\n");

    return 1;
}

int kp2s_io_copy_path(const char *src_path, const char *dst_path) {

    // The source and destination paths are identical
    if (strcasecmp(src_path, dst_path) == 0) {
        PRINT_ERR("src = dst");
        return -1;
    }

    // The destination is a subfolder of the source folder
    int len = strlen(src_path);
    if (strncasecmp(src_path, dst_path, (size_t) len) == 0
        && (dst_path[len] == '/' || dst_path[len - 1] == '/')) {
        PRINT_ERR("dst inside src");
        return -2;
    }

    SceUID dfd = sceIoDopen(src_path);
    if (dfd >= 0) {

        SceIoStat stat;
        memset(&stat, 0, sizeof(SceIoStat));
        ksceIoGetstatByFd(dfd, &stat);

        int ret = sceIoMkdir(dst_path, stat.st_mode & 0xFFF);
        if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
            PRINT_ERR_CODE("sceIoMkdir", ret);
            sceIoDclose(dfd);
            return ret;
        }

        int res = 0;

        do {

            SceIoDirent dir;
            memset(&dir, 0, sizeof(SceIoDirent));

            res = sceIoDread(dfd, &dir);

            if (res > 0) {
                char *new_src_path = p2s_malloc(strlen(src_path) + strlen(dir.d_name) + 2);
                snprintf(new_src_path, MAX_PATH_LENGTH, "%s%s%s", src_path, kp2s_has_slash(src_path) ? "" : "/",
                         dir.d_name);

                char *new_dst_path = p2s_malloc(strlen(dst_path) + strlen(dir.d_name) + 2);
                snprintf(new_dst_path, MAX_PATH_LENGTH, "%s%s%s", dst_path, kp2s_has_slash(dst_path) ? "" : "/",
                         dir.d_name);

                if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                    //PRINT("cp | `%s` => `%s`\n", new_src_path, new_dst_path);
                    ret = kp2s_io_copy_path(new_src_path, new_dst_path);
                    if (ret <= 0) {
                        PRINT_ERR_CODE("kp2s_io_copy_path", ret);
                    }
                } else {
                    ret = kp2s_io_copy_file(new_src_path, new_dst_path);
                }

                p2s_free(new_dst_path);
                p2s_free(new_src_path);

                if (ret <= 0) {
                    PRINT_ERR_CODE("cp", ret);
                    sceIoDclose(dfd);
                    return ret;
                }
            }
        } while (res > 0);
        sceIoDclose(dfd);
    } else {
        return kp2s_io_copy_file(src_path, dst_path);
    }

    return 1;
}

int kp2s_io_move(const char *src_path, const char *dst_path, int flags) {


    PRINT("mv | `%s` => `%s` ", src_path, dst_path);

    // The source and destination paths are identical
    if (strcasecmp(src_path, dst_path) == 0) {
        PRINT_ERR("src = dst");
        return -1;
    }

    // The destination is a subfolder of the source folder
    int len = strlen(src_path);
    if (strncasecmp(src_path, dst_path, (size_t) len) == 0
        && (dst_path[len] == '/' || dst_path[len - 1] == '/')) {
        PRINT_ERR("dst inside src");
        return -2;
    }

    int res = sceIoRename(src_path, dst_path);
    if (res == SCE_ERROR_ERRNO_EEXIST && flags & (MOVE_INTEGRATE | MOVE_REPLACE)) {

        // Src stat
        SceIoStat src_stat;
        memset(&src_stat, 0, sizeof(SceIoStat));
        res = sceIoGetstat(src_path, &src_stat);
        if (res < 0)
            return res;

        // Dst stat
        SceIoStat dst_stat;
        memset(&dst_stat, 0, sizeof(SceIoStat));
        res = sceIoGetstat(dst_path, &dst_stat);
        if (res < 0)
            return res;

        // Is dir
        int src_is_dir = SCE_S_ISDIR(src_stat.st_mode);
        int dst_is_dir = SCE_S_ISDIR(dst_stat.st_mode);

        // One of them is a file and the other a directory, no replacement or integration possible
        if (src_is_dir != dst_is_dir) {
            PRINT_ERR("file type is different");
            return -3;
        }

        // Replace file
        if (!src_is_dir && !dst_is_dir && flags & MOVE_REPLACE) {

            sceIoRemove(dst_path);
            res = sceIoRename(src_path, dst_path);
            if (res < 0) {
                PRINT_ERR_CODE("sceIoRename", res);
                return res;
            }

            PRINT_COL(COL_GREEN, "OK\n");
            return 1;
        }

        // Integrate directory
        if (src_is_dir && dst_is_dir && flags & MOVE_INTEGRATE) {

            SceUID dfd = sceIoDopen(src_path);
            if (dfd < 0) {
                return dfd;
                PRINT_ERR_CODE("sceIoDopen", dfd);
            }

            do {
                SceIoDirent dir;
                memset(&dir, 0, sizeof(SceIoDirent));

                res = sceIoDread(dfd, &dir);
                if (res > 0) {
                    char *new_src_path = p2s_malloc(strlen(src_path) + strlen(dir.d_name) + 2);
                    snprintf(new_src_path, MAX_PATH_LENGTH, "%s%s%s",
                             src_path, kp2s_has_slash(src_path) ? "" : "/", dir.d_name);

                    char *new_dst_path = p2s_malloc(strlen(dst_path) + strlen(dir.d_name) + 2);
                    snprintf(new_dst_path, MAX_PATH_LENGTH, "%s%s%s",
                             dst_path, kp2s_has_slash(dst_path) ? "" : "/", dir.d_name);

                    // Recursive move
                    int ret = kp2s_io_move(new_src_path, new_dst_path, flags);

                    p2s_free(new_dst_path);
                    p2s_free(new_src_path);

                    if (ret <= 0) {
                        PRINT_ERR_CODE("mv", ret);
                        sceIoDclose(dfd);
                        return ret;
                    }
                }
            } while (res > 0);

            sceIoDclose(dfd);

            // Integrated, now remove this directory
            sceIoRmdir(src_path);
        }
    } else if (res < 0) {
        PRINT_ERR_CODE("sceIoRename", res);
        return -1;
    }

    PRINT_COL(COL_GREEN, "OK\n");

    return 1;
}
