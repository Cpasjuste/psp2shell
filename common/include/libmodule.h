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
#include <psp2kern/net/net.h>

#ifdef __VITA_KERNEL__
#include <psp2kern/types.h>
#else
#include <psp2/types.h>
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
#define sceKernelWaitThreadEnd sceKernelWaitThreadEndForKernel
#define sceKernelDeleteThread sceKernelDeleteThreadForKernel
#define sceKernelCreateThread sceKernelCreateThreadForKernel
#define sceKernelStartThread sceKernelStartThreadForKernel
#define sceKernelDelayThread sceKernelDelayThreadForKernel
#define sceIoOpen sceIoOpenForDriver
#define sceIoRead sceIoReadForDriver
#define sceIoWrite sceIoWriteForDriver
#define sceIoLseek sceIoLseekForDriver
#define sceIoClose sceIoCloseForDriver
#define sceIoRename sceIoRenameForDriver
#define sceIoRemove sceIoRemoveForDriver

#define sceNetSocketClose sceNetSocketCloseForDriver
#define sceNetSocket sceNetSocketForDriver
#define sceNetBind sceNetBindForDriver
#define sceNetListen sceNetListenForDriver
#define sceNetAccept sceNetAcceptForDriver
#define sceNetSend sceNetSendForDriver
#define sceNetRecv sceNetRecvForDriver
#define sceNetHtons sceNetHtonsForDriver
#define sceNetHtonl sceNetHtonlForDriver
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
