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

#ifndef LIBMODULE_H
#define LIBMODULE_H
#ifdef MODULE

#include <libk/string.h>
#include <libk/stdlib.h>
#include <libk/stdio.h>
#include <libk/stdarg.h>

#ifdef __VITA_KERNEL__

#include <psp2kern/types.h>
#include <psp2kern/net/net.h>
#include <psp2kern/kernel/sysmem.h>

#else

#include <psp2/kernel/sysmem.h>
#include <psp2/io/fcntl.h>
#include <psp2/types.h>
#include <psp2/power.h>
#include <psp2/appmgr.h>

int sceKernelStartModule(SceUID modid, SceSize args, void *argp, int flags, void *option, int *status);

#endif

#ifdef DEBUG
int sceClibPrintf(const char *, ...);
#define printf sceClibPrintf
#endif

#ifdef __VITA_KERNEL__

#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/processmgr.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>

#define sceKernelStartModule ksceKernelStartModule

SceUID ksceKernelFindMemBlockByAddr(const void *addr, SceSize size);

#define sceKernelAllocMemBlock ksceKernelAllocMemBlock
#define sceKernelGetMemBlockBase ksceKernelGetMemBlockBase
#define sceKernelFindMemBlockByAddr ksceKernelFindMemBlockByAddr
#define sceKernelFreeMemBlock ksceKernelFreeMemBlock

#define sceKernelWaitThreadEnd ksceKernelWaitThreadEnd
#define sceKernelDeleteThread ksceKernelDeleteThread
#define sceKernelCreateThread ksceKernelCreateThread
#define sceKernelStartThread ksceKernelStartThread
#define sceKernelDelayThread ksceKernelDelayThread
#define sceKernelExitDeleteThread ksceKernelExitDeleteThread
#define sceIoOpen ksceIoOpen
#define sceIoRead ksceIoRead
#define sceIoWrite ksceIoWrite
#define sceIoLseek ksceIoLseek
#define sceIoClose ksceIoClose
#define sceIoRename ksceIoRename
#define sceIoRemove ksceIoRemove
#define sceIoMkdir ksceIoMkdir

#define sceNetSocketClose ksceNetSocketClose
#define sceNetSocket ksceNetSocket
#define sceNetBind ksceNetBind
#define sceNetListen ksceNetListen
#define sceNetAccept ksceNetAccept
#define sceNetRecv(a, b, c, d) ksceNetRecvfrom(a, b, c, d, NULL, NULL)
#define sceNetSend(a, b, c, d) ksceNetSendto (a, b, c, d, NULL, 0)
#define sceNetHtons ksceNetHtons
#define sceNetHtonl ksceNetHtonl

#else

int sceKernelStopModule(SceUID modid, SceSize args, void *argp, int flags, void *option, int *status);

SceUID sceAppMgrGetProcessIdByAppIdForShell(SceUID appId);

int sceAppMgrGetRunningAppIdListForShell(SceUID *ids, int count);

// return AppId ?
SceUID sceAppMgrLaunchAppByName2ForShell(const char *name, const char *param, SceAppMgrLaunchAppOptParam *optParam);

int sceAppMgrDestroyOtherAppByAppIdForShell(SceUID appId, void *a, void *b);

int sceAppMgrDestroyAppByAppId(SceUID aid);

#endif


void *malloc(size_t size);

void free(void *p);

#define strcasecmp strcmp
#define strncasecmp strncmp

#endif // MODULE
#endif //LIBMODULE_H
