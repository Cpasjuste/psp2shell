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
    HOOK_IO_OPEN = 0,
    HOOK_IO_CLOSE,
    HOOK_IO_READ,
    HOOK_IO_WRITE,
    HOOK_IO_LSEEK,
    HOOK_IO_REMOVE,
    HOOK_IO_RENAME,
    HOOK_IO_DOPEN,
    HOOK_IO_DREAD,
    HOOK_IO_DCLOSE,
    HOOK_END
};

// user hooks
/*
enum {
    HOOK_IO_OPEN = 0,
    HOOK_IO_OPEN_ASYNC,
    HOOK_IO_CLOSE,
    HOOK_IO_CLOSE_ASYNC,
    HOOK_IO_READ,
    HOOK_IO_READ_ASYNC,
    HOOK_IO_WRITE,
    HOOK_IO_WRITE_ASYNC,
    HOOK_IO_LSEEK,
    HOOK_IO_LSEEK_ASYNC,
    HOOK_IO_LSEEK32,
    //HOOK_IO_LSEEK32_ASYNC,        // TODO: nid ?
            HOOK_IO_REMOVE,
    HOOK_IO_RENAME,
    HOOK_IO_SYNC,
    HOOK_IO_SYNC_BY_FD,
    //HOOK_IO_WAIT_ASYNC,           // TODO: nid ?
    //HOOK_IO_WAIT_ASYNC_CB,        // TODO: nid ?
    //HOOK_IO_POLL_ASYNC,           // TODO: nid ?
    //HOOK_IO_GET_ASYNC_STAT,       // TODO: nid ?
            HOOK_IO_CANCEL,
    //HOOK_IO_GET_DEV_TYPE,         // TODO: nid ?
    //HOOK_IO_CHANGE_ASYNC_PRIORITY,// TODO: nid ?
    //HOOK_IO_SET_ASYNC_CALLBACK,   // TODO: nid ?
            HOOK_IO_DOPEN,
    HOOK_IO_DREAD,
    HOOK_IO_DCLOSE,
    HOOK_END
};
*/

// psp2/io/fcntl.h
SceUID _sceIoOpen(const char *file, int flags, SceMode mode);

SceUID _sceIoOpenAsync(const char *file, int flags, SceMode mode);

int _sceIoClose(SceUID fd);

int _sceIoCloseAsync(SceUID fd);

int _sceIoRead(SceUID fd, void *data, SceSize size);

int _sceIoReadAsync(SceUID fd, void *data, SceSize size);

int _sceIoWrite(SceUID fd, const void *data, SceSize size);

int _sceIoWriteAsync(SceUID fd, const void *data, SceSize size);

SceOff _sceIoLseek(SceUID fd, SceOff offset, int whence);

int _sceIoLseekAsync(SceUID fd, SceOff offset, int whence);

int _sceIoLseek32(SceUID fd, int offset, int whence);

int _sceIoLseek32Async(SceUID fd, int offset, int whence);

int _sceIoRemove(const char *file);

int _sceIoRename(const char *oldname, const char *newname);

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
SceUID _sceIoDopen(const char *dirname);

int _sceIoDread(SceUID fd, SceIoDirent *dir);

int _sceIoDclose(SceUID fd);
// psp2/io/dirent.h

void set_hooks_io();

void delete_hooks_io();

#endif //_KP2S_HOOKS_IO_H_
