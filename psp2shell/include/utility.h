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

#ifndef UTILITY_H
#define UTILITY_H

#include <psp2/rtc.h>

int s_launchAppByUriExit(char *titleid);

void s_netInit();

int s_bind_port(int sock, int port);

int s_get_sock(int sock);

int s_recvall(int sock, void *buffer, int size, int flags);

ssize_t s_recv_file(int sock, SceUID fd, long size);

int s_hasEndSlash(char *path);

int s_removeEndSlash(char *path);

int s_addEndSlash(char *path);

#endif // UTILITY_H