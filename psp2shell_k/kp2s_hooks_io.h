//
// Created by cpasjuste on 10/07/17.
//

#ifndef _KP2S_HOOKS_IO_H_
#define _KP2S_HOOKS_IO_H_

#include "libmodule.h"

typedef struct Hook {
    SceUID uid;
    tai_hook_ref_t ref;
    const char name[32];
    uint32_t lib;
    uint32_t nid;
    const void *func;
} Hook;

// kernel hooks
enum {
    //HOOK_IO_OPEN = 0,
    HOOK_IO_KOPEN = 0,
    HOOK_IO_KOPEN2,
    HOOK_IO_KCLOSE,
    HOOK_IO_KREAD,
    HOOK_IO_KWRITE,
    HOOK_IO_KLSEEK,
    HOOK_IO_KREMOVE,
    HOOK_IO_KRENAME,
    HOOK_IO_KDOPEN,
    HOOK_IO_KDREAD,
    HOOK_IO_KDCLOSE,
    HOOK_IO_KMKDIR,
    HOOK_IO_KRMDIR,
    HOOK_IO_KGETSTAT,
    HOOK_IO_KGETSTATBYFD,
    HOOK_IO_KCHSTAT,
    HOOK_IO_KDEVCTL,
    HOOK_END
};

// psp2/io/fcntl.h
SceUID _ksceIoOpen(const char *path, int flags, SceMode mode);

SceUID _ksceIoOpen2(const char *path, int flags, SceMode mode);

struct sceIoOpenOpt { uint32_t unk_0;uint32_t unk_4; };
SceUID _sceIoOpen(const char *path, int flags, SceMode mode, struct sceIoOpenOpt *opt);
//SceUID _sceIoOpen(const char *path, int flags, SceMode mode);

SceUID _sceIoOpenAsync(const char *path, int flags, SceMode mode);

int _ksceIoClose(SceUID fd);

int _sceIoCloseAsync(SceUID fd);

int _ksceIoRead(SceUID fd, void *data, SceSize size);

int _sceIoReadAsync(SceUID fd, void *data, SceSize size);

int _ksceIoWrite(SceUID fd, const void *data, SceSize size);

int _sceIoWriteAsync(SceUID fd, const void *data, SceSize size);

SceOff _ksceIoLseek(SceUID fd, SceOff offset, int whence);

int _sceIoLseekAsync(SceUID fd, SceOff offset, int whence);

int _sceIoLseek32(SceUID fd, int offset, int whence);

int _sceIoLseek32Async(SceUID fd, int offset, int whence);

int _ksceIoRemove(const char *file);

int _ksceIoRename(const char *oldname, const char *newname);

int _sceIoSync(const char *device, unsigned int unk);

int _sceIoSyncByFd(SceUID fd);

int _sceIoWaitAsync(SceUID fd, SceInt64 *res);

int _sceIoWaitAsyncCB(SceUID fd, SceInt64 *res);

int _sceIoPollAsync(SceUID fd, SceInt64 *res);

int _sceIoGetAsyncStat(SceUID fd, int poll, SceInt64 *res);

int _sceIoCancel(SceUID fd);

int _sceIoGetDevType(SceUID fd);

int _sceIoChangeAsyncPriority(SceUID fd, int pri);

int _sceIoSetAsyncCallback(SceUID fd, SceUID cb, void *argp);
// psp2/io/fcntl.h

// psp2/io/dirent.h
SceUID _ksceIoDopen(const char *dirname);

int _ksceIoDread(SceUID fd, SceIoDirent *dir);

int _ksceIoDclose(SceUID fd);
// psp2/io/dirent.h

// io/stat.h
int _ksceIoMkdir(const char *dir, SceMode mode);

int _ksceIoRmdir(const char *path);

int _ksceIoGetstat(const char *file, SceIoStat *stat);

int _ksceIoGetstatByFd(SceUID fd, SceIoStat *stat);

int _ksceIoChstat(const char *file, SceIoStat *stat, int bits);
// io/stat.h

// io/devctl.h
int _ksceIoDevctl(const char *dev, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen);
// io/devctl.h

void set_hooks_io();

void delete_hooks_io();

#endif //_KP2S_HOOKS_IO_H_
