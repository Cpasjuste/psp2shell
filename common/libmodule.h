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

#include <libk/string.h>
#include <libk/stdlib.h>
#include <libk/stdio.h>
#include <libk/stdarg.h>
#include <libk/stdbool.h>
#include <sys/types.h>
#include <taihen.h>

#ifdef __KERNEL__

#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/processmgr.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/io/dirent.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include <psp2kern/io/devctl.h>
#include <psp2kern/types.h>

#ifdef DEBUG
int kp2s_print_stdout(const char *data, size_t size);
#define printf(...) \
do { \
    char buffer[256]; \
    snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
    kp2s_print_stdout(buffer, strlen(buffer)); \
} while (0)

#endif
#else

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/devctl.h>
#include <psp2/sysmodule.h>
#include <psp2/types.h>
#include <psp2/power.h>
#include <psp2/appmgr.h>
#include <psp2/rtc.h>
#include <errno.h>

#ifdef DEBUG
int sceClibPrintf(const char *, ...);
#define printf sceClibPrintf
#endif // DEBUG
#endif // __KERNEL__

#ifdef __KERNEL__

#define taiHookRelease taiHookReleaseForKernel
#define taiHookFunctionImport taiHookFunctionImportForKernel
#define taiLoadStartKernelModule(a, b, c, d) ksceKernelLoadStartModule(a, b, c, d, NULL, 0)
#define taiStopUnloadKernelModule ksceKernelStopUnloadModule
#define taiLoadStartModuleForPid(a, b, c, d, e) ksceKernelLoadStartModuleForPid(a, b, c, d, e, NULL, 0)
#define taiStopUnloadModuleForPid ksceKernelStopUnloadModuleForPid
#define sceKernelStartModule ksceKernelStartModule
#define sceKernelGetModuleInfo ksceKernelGetModuleInfo
#define kpsp2shell_get_module_info ksceKernelGetModuleInfo
#define kpsp2shell_get_module_list ksceKernelGetModuleList

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
#define sceIoRmdir ksceIoRmdir
#define sceIoDopen ksceIoDopen
#define sceIoDread ksceIoDread
#define sceIoDclose ksceIoDclose
#define sceIoGetstat ksceIoGetstat
#define sceIoDevctl ksceIoDevctl


#define sceNetSocketClose ksceNetSocketClose
#define sceNetSocket ksceNetSocket
#define sceNetBind ksceNetBind
#define sceNetListen ksceNetListen
#define sceNetAccept ksceNetAccept
#define sceNetRecv(a, b, c, d) ksceNetRecvfrom(a, b, c, d, NULL, NULL)
#define sceNetSend(a, b, c, d) ksceNetSendto (a, b, c, d, NULL, 0)
#define sceNetHtons ksceNetHtons
#define sceNetHtonl ksceNetHtonl

#endif

void *p2s_malloc(size_t size);

void p2s_free(void *p);

#define strcasecmp strcmp
#define strncasecmp strncmp

#endif //LIBMODULE_H
