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

#ifndef __FILE_H__
#define __FILE_H__

#ifdef __VITA_KERNEL__
#include <psp2kern/types.h>
#endif

#define BOOL int
#define TRUE 1
#define FALSE 0

#define MAX_PATH_LENGTH 1024
#define MAX_NAME_LENGTH 256
#define MAX_SHORT_NAME_LENGTH 64
#define MAX_MOUNT_POINT_LENGTH 16

#define TRANSFER_SIZE 64 * 1024

#define HOME_PATH "root"
#define DIR_UP ".."

typedef struct {
    uint64_t max_size;
    uint64_t free_size;
    uint32_t cluster_size;
    void *unk;
} SceIoDevInfo;

enum FileTypes {
    FILE_TYPE_UNKNOWN,
    FILE_TYPE_BMP,
    FILE_TYPE_INI,
    FILE_TYPE_JPEG,
    FILE_TYPE_MP3,
    FILE_TYPE_PNG,
    FILE_TYPE_SFO,
    FILE_TYPE_TXT,
    FILE_TYPE_VPK,
    FILE_TYPE_XML,
    FILE_TYPE_ZIP,
};

enum FileSortFlags {
    SORT_NONE,
    SORT_BY_NAME_AND_FOLDER,
};

enum FileMoveFlags {
    MOVE_INTEGRATE = 0x1, // Integrate directories
    MOVE_REPLACE = 0x2, // Replace files
};

typedef struct {
    uint64_t *value;
    uint64_t max;

    void (*SetProgress)(uint64_t value, uint64_t max);

    int (*cancelHandler)();
} s_FileProcessParam;

typedef struct s_FileListEntry {
    struct s_FileListEntry *next;
    struct s_FileListEntry *previous;
    char name[MAX_NAME_LENGTH];
    int name_length;
    int is_folder;
    int type;
    SceOff size;
    SceOff size2;
    SceDateTime time;
    int reserved[16];
} s_FileListEntry;

typedef struct {
    s_FileListEntry *head;
    s_FileListEntry *tail;
    int length;
    char path[MAX_PATH_LENGTH];
    int files;
    int folders;
} s_FileList;

BOOL s_exist(char *path);

BOOL s_isDir(char *path);

SceUID s_open(const char *file, int flags, SceMode mode);

void s_close(SceUID fd);

int s_getFileSize(char *pInputFileName);

int s_getFileSha1(char *pInputFileName, uint8_t *pSha1Out, s_FileProcessParam *param);

int s_getPathInfo(char *path, uint64_t *size, uint32_t *folders, uint32_t *files);

int s_removePath(char *path, s_FileProcessParam *param);

int s_copyFile(char *src_path, char *dst_path, s_FileProcessParam *param);

int s_copyPath(char *src_path, char *dst_path, s_FileProcessParam *param);

int s_movePath(char *src_path, char *dst_path, int flags, s_FileProcessParam *param);

int s_getFileType(char *file);

void s_fileListAddEntry(s_FileList *list, s_FileListEntry *entry, int sort);

void s_fileListEmpty(s_FileList *list);

int s_fileListGetEntries(s_FileList *list, char *path);

int s_fileListGetMountPointEntries(s_FileList *list);

#endif