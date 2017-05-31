/*
	VitaShell
	Copyright (C) 2015-2016, TheFloW

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

#ifndef __VITA_KERNEL__ // TODO

#ifndef __VITA_KERNEL__

#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/devctl.h>

#endif
#ifndef MODULE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#else
#include "libmodule.h"
#endif

#include "psp2cmd.h"
#include "file.h"
#include "utility.h"
#include "pool.h"

#define SCE_ERROR_ERRNO_EEXIST 0x80010011
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

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
};

#define N_MOUNT_POINTS (sizeof(mount_points) / sizeof(char **))

BOOL s_isDir(char *path) {
#ifdef __VITA_KERNEL__
    return FALSE;
#else
    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));
    int res = sceIoGetstat(path, &stat);
    if (res < 0)
        return FALSE;

    return SCE_S_ISDIR(stat.st_mode) == 1;
#endif
}

BOOL s_exist(char *path) {
#ifdef __VITA_KERNEL__
    return FALSE;
#else
    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));
    int res = sceIoGetstat(path, &stat);
    if (res < 0)
        return FALSE;
    return TRUE;
#endif
}

SceUID s_open(const char *file, int flags, SceMode mode) {
    return sceIoOpen(file, flags, mode);
}

void s_close(SceUID fd) {
    sceIoClose(fd);
}

int s_getFileSize(char *pInputFileName) {
    SceUID fd = sceIoOpen(pInputFileName, SCE_O_RDONLY, 0);
    if (fd < 0)
        return fd;

    int fileSize = (int) sceIoLseek(fd, 0, SCE_SEEK_END);

    sceIoClose(fd);
    return fileSize;
}

int s_getPathInfo(char *path, uint64_t *size, uint32_t *folders, uint32_t *files) {
#ifdef __VITA_KERNEL__
    return -1;
#else
    SceUID dfd = sceIoDopen(path);
    if (dfd >= 0) {
        int res = 0;

        do {
            SceIoDirent dir;
            memset(&dir, 0, sizeof(SceIoDirent));

            res = sceIoDread(dfd, &dir);
            if (res > 0) {
                if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
                    continue;

                size_t len = strlen(path) + strlen(dir.d_name) + 2;
                char new_path[len];
                memset(new_path, 0, len);
                snprintf(new_path, MAX_PATH_LENGTH, "%s%s%s", path, p2s_hasEndSlash(path) ? "" : "/", dir.d_name);

                if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                    int ret = s_getPathInfo(new_path, size, folders, files);
                    if (ret <= 0) {
                        sceIoDclose(dfd);
                        return ret;
                    }
                } else {
                    if (size)
                        (*size) += dir.d_stat.st_size;

                    if (files)
                        (*files)++;
                }
            }
        } while (res > 0);

        sceIoDclose(dfd);

        if (folders)
            (*folders)++;
    } else {
        if (size) {
            SceIoStat stat;
            memset(&stat, 0, sizeof(SceIoStat));

            int res = sceIoGetstat(path, &stat);
            if (res < 0)
                return res;

            (*size) += stat.st_size;
        }

        if (files)
            (*files)++;
    }

    return 1;
#endif
}

int s_removePath(char *path, s_FileProcessParam *param) {
#ifdef __VITA_KERNEL__
    return -1;
#else
    SceUID dfd = sceIoDopen(path);
    if (dfd >= 0) {
        int res = 0;

        do {
            SceIoDirent dir;
            memset(&dir, 0, sizeof(SceIoDirent));

            res = sceIoDread(dfd, &dir);
            if (res > 0) {
                if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
                    continue;

                size_t len = strlen(path) + strlen(dir.d_name) + 2;
                char new_path[len];
                memset(new_path, 0, len);
                snprintf(new_path, MAX_PATH_LENGTH, "%s%s%s", path, p2s_hasEndSlash(path) ? "" : "/", dir.d_name);

                if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                    int ret = s_removePath(new_path, param);
                    if (ret <= 0) {
                        sceIoDclose(dfd);
                        return ret;
                    }
                } else {
                    int ret = sceIoRemove(new_path);
                    if (ret < 0) {
                        sceIoDclose(dfd);
                        return ret;
                    }

                    if (param) {
                        if (param->value)
                            (*param->value)++;

                        if (param->SetProgress)
                            param->SetProgress(param->value ? *param->value : 0, param->max);

                        if (param->cancelHandler && param->cancelHandler()) {
                            sceIoDclose(dfd);
                            return 0;
                        }
                    }
                }
            }
        } while (res > 0);

        sceIoDclose(dfd);

        int ret = sceIoRmdir(path);
        if (ret < 0)
            return ret;

        if (param) {
            if (param->value)
                (*param->value)++;

            if (param->SetProgress)
                param->SetProgress(param->value ? *param->value : 0, param->max);

            if (param->cancelHandler && param->cancelHandler()) {
                return 0;
            }
        }
    } else {
        int ret = sceIoRemove(path);
        if (ret < 0)
            return ret;

        if (param) {
            if (param->value)
                (*param->value)++;

            if (param->SetProgress)
                param->SetProgress(param->value ? *param->value : 0, param->max);

            if (param->cancelHandler && param->cancelHandler()) {
                return 0;
            }
        }
    }

    return 1;
#endif
}

int s_copyFile(char *src_path, char *dst_path, s_FileProcessParam *param) {
    // The source and destination paths are identical
    if (strcasecmp(src_path, dst_path) == 0) {
        return -1;
    }

    // The destination is a subfolder of the source folder
    size_t len = strlen(src_path);
    if (strncasecmp(src_path, dst_path, len) == 0 && (dst_path[len] == '/' || dst_path[len - 1] == '/')) {
        return -2;
    }

    SceUID fdsrc = sceIoOpen(src_path, SCE_O_RDONLY, 0);
    if (fdsrc < 0)
        return fdsrc;

    SceUID fddst = sceIoOpen(dst_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fddst < 0) {
        sceIoClose(fdsrc);
        return fddst;
    }

    //char buf[TRANSFER_SIZE];
    unsigned char *buf = pool_data_malloc(SIZE_DATA);
    if (buf == NULL) {
        return -3;
    }
    memset(buf, 0, SIZE_DATA);

    int read;
    while ((read = sceIoRead(fdsrc, buf, SIZE_DATA)) > 0) {
        int res = sceIoWrite(fddst, buf, (SceSize) read);
        if (res < 0) {

            sceIoClose(fddst);
            sceIoClose(fdsrc);

            return res;
        }

        if (param) {
            if (param->value)
                (*param->value) += read;

            if (param->SetProgress)
                param->SetProgress(param->value ? *param->value : 0, param->max);

            if (param->cancelHandler && param->cancelHandler()) {

                sceIoClose(fddst);
                sceIoClose(fdsrc);

                return 0;
            }
        }
    }

    sceIoClose(fddst);
    sceIoClose(fdsrc);

    return 1;
}

int s_copyPath(char *src_path, char *dst_path, s_FileProcessParam *param) {
#ifdef __VITA_KERNEL__
    return -1;
#else
    // The source and destination paths are identical
    if (strcasecmp(src_path, dst_path) == 0) {
        return -1;
    }

    // The destination is a subfolder of the source folder
    size_t len = strlen(src_path);
    if (strncasecmp(src_path, dst_path, len) == 0 && (dst_path[len] == '/' || dst_path[len - 1] == '/')) {
        return -2;
    }

    SceUID dfd = sceIoDopen(src_path);
    if (dfd >= 0) {
        int ret = sceIoMkdir(dst_path, 0777);
        if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
            sceIoDclose(dfd);
            return ret;
        }

        if (param) {
            if (param->value)
                (*param->value)++;

            if (param->SetProgress)
                param->SetProgress(param->value ? *param->value : 0, param->max);

            if (param->cancelHandler && param->cancelHandler()) {
                sceIoDclose(dfd);
                return 0;
            }
        }

        int res = 0;

        do {
            SceIoDirent dir;
            memset(&dir, 0, sizeof(SceIoDirent));

            res = sceIoDread(dfd, &dir);
            if (res > 0) {
                if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
                    continue;

                len = strlen(src_path) + strlen(dir.d_name) + 2;
                char new_src_path[len];
                memset(new_src_path, 0, len);
                snprintf(new_src_path, MAX_PATH_LENGTH, "%s%s%s",
                         src_path, p2s_hasEndSlash(src_path) ? "" : "/", dir.d_name);

                len = strlen(dst_path) + strlen(dir.d_name) + 2;
                char new_dst_path[len];
                memset(new_dst_path, 0, len);
                snprintf(new_dst_path, MAX_PATH_LENGTH, "%s%s%s",
                         dst_path, p2s_hasEndSlash(dst_path) ? "" : "/", dir.d_name);

                if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                    ret = s_copyPath(new_src_path, new_dst_path, param);
                } else {
                    ret = s_copyFile(new_src_path, new_dst_path, param);
                }

                if (ret <= 0) {
                    sceIoDclose(dfd);
                    return ret;
                }
            }
        } while (res > 0);

        sceIoDclose(dfd);
    } else {
        return s_copyFile(src_path, dst_path, param);
    }

    return 1;
#endif
}

int s_movePath(char *src_path, char *dst_path, int flags, s_FileProcessParam *param) {
#ifdef __VITA_KERNEL__
    return -1;
#else
    // The source and destination paths are identical
    if (strcasecmp(src_path, dst_path) == 0) {
        return -1;
    }

    // The destination is a subfolder of the source folder
    size_t len = strlen(src_path);
    if (strncasecmp(src_path, dst_path, len) == 0 && (dst_path[len] == '/' || dst_path[len - 1] == '/')) {
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
        if (src_is_dir != dst_is_dir)
            return -3;

        // Replace file
        if (!src_is_dir && !dst_is_dir && flags & MOVE_REPLACE) {
            sceIoRemove(dst_path);

            res = sceIoRename(src_path, dst_path);
            if (res < 0)
                return res;

            return 1;
        }

        // Integrate directory
        if (src_is_dir && dst_is_dir && flags & MOVE_INTEGRATE) {
            SceUID dfd = sceIoDopen(src_path);
            if (dfd < 0)
                return dfd;

            do {
                SceIoDirent dir;
                memset(&dir, 0, sizeof(SceIoDirent));

                res = sceIoDread(dfd, &dir);
                if (res > 0) {
                    if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
                        continue;

                    len = strlen(src_path) + strlen(dir.d_name) + 2;
                    char new_src_path[len];
                    memset(new_src_path, 0, len);
                    snprintf(new_src_path, MAX_PATH_LENGTH, "%s%s%s", src_path, p2s_hasEndSlash(src_path) ? "" : "/",
                             dir.d_name);

                    len = strlen(dst_path) + strlen(dir.d_name) + 2;
                    char new_dst_path[len];
                    memset(new_dst_path, 0, len);
                    snprintf(new_dst_path, MAX_PATH_LENGTH, "%s%s%s", dst_path, p2s_hasEndSlash(dst_path) ? "" : "/",
                             dir.d_name);

                    // Recursive move
                    int ret = s_movePath(new_src_path, new_dst_path, flags, param);

                    if (ret <= 0) {
                        sceIoDclose(dfd);
                        return ret;
                    }
                }
            } while (res > 0);

            sceIoDclose(dfd);

            // Integrated, now remove this directory
            sceIoRmdir(src_path);
        }
    }

    return 1;
#endif
}

typedef struct {
    char *extension;
    int type;
} s_ExtensionType;

static s_ExtensionType s_extension_types[] = {
        {".BMP",  FILE_TYPE_BMP},
        {".INI",  FILE_TYPE_INI},
        {".JPG",  FILE_TYPE_JPEG},
        {".JPEG", FILE_TYPE_JPEG},
        {".MP3",  FILE_TYPE_MP3},
        {".PNG",  FILE_TYPE_PNG},
        {".SFO",  FILE_TYPE_SFO},
        {".TXT",  FILE_TYPE_TXT},
        {".VPK",  FILE_TYPE_VPK},
        {".XML",  FILE_TYPE_XML},
        {".ZIP",  FILE_TYPE_ZIP},
};

int s_getFileType(char *file) {
    char *p = strrchr(file, '.');
    if (p) {
        int i;
        for (i = 0; i < (sizeof(s_extension_types) / sizeof(s_ExtensionType)); i++) {
            if (strcasecmp(p, s_extension_types[i].extension) == 0) {
                return s_extension_types[i].type;
            }
        }
    }

    return FILE_TYPE_UNKNOWN;
}

void s_fileListAddEntry(s_FileList *list, s_FileListEntry *entry, int sort) {
    entry->next = NULL;
    entry->previous = NULL;

    if (list->head == NULL) {
        list->head = entry;
        list->tail = entry;
    } else {
        if (sort != SORT_NONE) {
            s_FileListEntry *p = list->head;
            s_FileListEntry *previous = NULL;

            while (p) {
                // Sort by type
                if (entry->is_folder > p->is_folder)
                    break;

                // '..' is always at first
                if (strcmp(entry->name, "..") == 0)
                    break;

                if (strcmp(p->name, "..") != 0) {
                    // Get the minimum length without /
                    int len = MIN(entry->name_length, p->name_length);
                    if (entry->name[len - 1] == '/' || p->name[len - 1] == '/')
                        len--;

                    // Sort by name
                    if (entry->is_folder == p->is_folder) {
                        int diff = strncasecmp(entry->name, p->name, (size_t) len);
                        if (diff < 0 || (diff == 0 && entry->name_length < p->name_length)) {
                            break;
                        }
                    }
                }

                previous = p;
                p = p->next;
            }

            if (previous == NULL) { // Order: entry (new head) -> p (old head)
                entry->next = p;
                p->previous = entry;
                list->head = entry;
            } else if (previous->next == NULL) { // Order: p (old tail) -> entry (new tail)
                s_FileListEntry *tail = list->tail;
                tail->next = entry;
                entry->previous = tail;
                list->tail = entry;
            } else { // Order: previous -> entry -> p
                previous->next = entry;
                entry->previous = previous;
                entry->next = p;
                p->previous = entry;
            }
        } else {
            s_FileListEntry *tail = list->tail;
            tail->next = entry;
            entry->previous = tail;
            list->tail = entry;
        }
    }

    list->length++;
}

void s_fileListEmpty(s_FileList *list) {
    s_FileListEntry *entry = list->head;

    while (entry) {
        s_FileListEntry *next = entry->next;
        free(entry);
        entry = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    list->files = 0;
    list->folders = 0;
}

int s_fileListGetMountPointEntries(s_FileList *list) {
#ifdef __VITA_KERNEL__
    return -1;
#else
    int i;

    for (i = 0; i < N_MOUNT_POINTS; i++) {
        if (mount_points[i]) {
            SceIoStat stat;
            memset(&stat, 0, sizeof(SceIoStat));
            if (sceIoGetstat(mount_points[i], &stat) >= 0) {
                s_FileListEntry *entry = malloc(sizeof(s_FileListEntry));
                strcpy(entry->name, mount_points[i]);
                entry->name_length = strlen(entry->name);
                entry->is_folder = 1;
                entry->type = FILE_TYPE_UNKNOWN;

                SceIoDevInfo info;
                memset(&info, 0, sizeof(SceIoDevInfo));
                int res = sceIoDevctl(entry->name, 0x3001, 0, 0, &info, sizeof(SceIoDevInfo));
                if (res >= 0) {
                    entry->size = info.free_size;
                    entry->size2 = info.max_size;
                } else {
                    entry->size = 0;
                    entry->size2 = 0;
                }

                memcpy(&entry->time, &stat.st_ctime, sizeof(SceDateTime));

                s_fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);

                list->folders++;
            }
        }
    }

    return 0;
#endif
}

int s_fileListGetDirectoryEntries(s_FileList *list, char *path) {
#ifdef __VITA_KERNEL__
    return -1;
#else
    SceUID dfd = sceIoDopen(path);
    if (dfd < 0)
        return dfd;

    s_FileListEntry *entry = malloc(sizeof(s_FileListEntry));
    strcpy(entry->name, DIR_UP);
    entry->name_length = strlen(entry->name);
    entry->is_folder = 1;
    entry->type = FILE_TYPE_UNKNOWN;
    s_fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);

    int res = 0;

    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = sceIoDread(dfd, &dir);
        if (res > 0) {
            if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
                continue;

            entry = malloc(sizeof(s_FileListEntry));
            strcpy(entry->name, dir.d_name);

            entry->is_folder = SCE_S_ISDIR(dir.d_stat.st_mode);
            if (entry->is_folder) {
                p2s_addEndSlash(entry->name);
                entry->type = FILE_TYPE_UNKNOWN;
                list->folders++;
            } else {
                entry->type = s_getFileType(entry->name);
                list->files++;
            }

            entry->name_length = strlen(entry->name);
            entry->size = dir.d_stat.st_size;

            memcpy(&entry->time, &dir.d_stat.st_mtime, sizeof(SceDateTime));

            s_fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);
        }
    } while (res > 0);

    sceIoDclose(dfd);

    return 0;
#endif
}

int s_fileListGetEntries(s_FileList *list, char *path) {
    if (strcasecmp(path, HOME_PATH) == 0) {
        return s_fileListGetMountPointEntries(list);
    }
    return s_fileListGetDirectoryEntries(list, path);
}

#endif