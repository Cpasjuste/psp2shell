//
// Created by cpasjuste on 07/11/16.
//

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
#endif

#ifdef DEBUG
void mdebug(const char *fmt, ...);
#define printf mdebug
#endif

#ifdef __VITA_KERNEL__
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/processmgr.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/io/fcntl.h>
#include <psp2/io/stat.h>
#define sceKernelWaitThreadEnd ksceKernelWaitThreadEnd
#define sceKernelDeleteThread ksceKernelDeleteThread
#define sceKernelCreateThread ksceKernelCreateThread
#define sceKernelStartThread ksceKernelStartThread
#define sceKernelDelayThread ksceKernelDelayThread
#define sceIoOpen ksceIoOpen
#define sceIoRead ksceIoRead
#define sceIoWrite ksceIoWrite
#define sceIoLseek ksceIoLseek
#define sceIoClose ksceIoClose
#define sceIoRename ksceIoRename
#define sceIoRemove ksceIoRemove

#define sceNetSocketClose ksceNetSocketClose
#define sceNetSocket ksceNetSocket
#define sceNetBind ksceNetBind
#define sceNetListen ksceNetListen
#define sceNetAccept ksceNetAccept
#define sceNetSend ksceNetSend
#define sceNetRecv ksceNetRecv
#define sceNetHtons ksceNetHtons
#define sceNetHtonl ksceNetHtonl

#define sceKernelMemPoolAlloc ksceKernelAllocHeapMemory
#define sceKernelMemPoolCreate ksceKernelCreateHeap
#define sceKernelMemPoolFree ksceKernelFreeHeapMemory

#endif

void *malloc(size_t size);
void *_realloc(void *ptr, size_t old_size, size_t new_size);
void free(void *p);

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
double atof(const char *str);
char *strdup(const char *s);

#endif // MODULE
#endif //LIBMODULE_H
