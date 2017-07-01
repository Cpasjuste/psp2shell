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

#ifndef PROJECT_KP2S_IO_H
#define PROJECT_KP2S_IO_H

#include <libk/stdbool.h>
#include <psp2kern/types.h>

#define SCE_ERROR_ERRNO_EEXIST 0x80010011

#define MAX_FILES 128
#define MAX_PATH_LENGTH 256
#define MAX_NAME_LENGTH 256
#define MAX_SHORT_NAME_LENGTH 64
#define MAX_MOUNT_POINT_LENGTH 16

#define TRANSFER_SIZE (64 * 1024)

#define HOME_PATH "root"

enum FileMoveFlags {
    MOVE_INTEGRATE = 0x1, // Integrate directories
    MOVE_REPLACE = 0x2, // Replace files
};

int kp2s_io_list_drives();

int kp2s_io_list_dir(const char *path);

bool kp2s_io_isdir(char *path);

SceOff kp2s_io_get_size(const char *file);

int kp2s_io_exist(const char *file);

int kp2s_io_remove(const char *path);

int kp2s_io_copy_file(const char *src_path, const char *dst_path);

int kp2s_io_copy_path(const char *src_path, const char *dst_path);

int kp2s_io_move(const char *src_path, const char *dst_path, int flags);

#endif //PROJECT_KP2S_IO_H
