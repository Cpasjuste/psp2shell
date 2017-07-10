//
// Created by cpasjuste on 10/07/17.
//

#include "psp2shell_k.h"
#include "kp2s_hooks_io.h"
#include "host_driver.h"

#define MAX_HOST_FD 128
static int host_fds[MAX_HOST_FD];

// kernel hooks
static Hook hooks[HOOK_END] = {
        // psp2/io/fcntl.h
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x463B25CC, _sceIoOpen},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0xF99DD8A3, _sceIoClose},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0xE17EFC03, _sceIoRead},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x21EE91F0, _sceIoWrite},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x62090481, _sceIoLseek},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x0D7BB3E1, _sceIoRemove},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0xDC0C4997, _sceIoRename},
        // psp2/io/fcntl.h
        // psp2/io/dirent.h
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x463B25CC, _sceIoDopen},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x20CF5FC7, _sceIoDread},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x19C81DD6, _sceIoDclose},
        // psp2/io/dirent.h
};

// user hooks
/*
static Hook hooks[HOOK_END] = {
        // psp2/io/fcntl.h
        {-1, 0, 0x6C60AC61, _sceIoOpen},
        {-1, 0, 0x09CD0FC8, _sceIoOpenAsync},
        {-1, 0, 0xC70B8886, _sceIoClose},
        {-1, 0, 0x8EA3616A, _sceIoCloseAsync},
        {-1, 0, 0xFDB32293, _sceIoRead},
        {-1, 0, 0x773EBD45, _sceIoReadAsync},
        {-1, 0, 0x34EFD876, _sceIoWrite},
        {-1, 0, 0xE0D63D2A, _sceIoWriteAsync},
        {-1, 0, 0x99BA173E, _sceIoLseek},
        {-1, 0, 0x2300858E, _sceIoLseekAsync},
        {-1, 0, 0x49252B9B, _sceIoLseek32},
        //{-1, 0, 0xXXXXXXXX, _sceIoLseek32Async},          // TODO: nid ?
        {-1, 0, 0xE20ED0F3, _sceIoRemove},
        {-1, 0, 0xF737E369, _sceIoRename},
        {-1, 0, 0x98ACED6D, _sceIoSync},
        {-1, 0, 0x16512F59, _sceIoSyncByFd},
        //{-1, 0, 0xXXXXXXXX, _sceIoWaitAsync},             // TODO: nid ?
        //{-1, 0, 0xXXXXXXXX, _sceIoWaitAsyncCB},           // TODO: nid ?
        //{-1, 0, 0xXXXXXXXX, _sceIoPollAsync},             // TODO: nid ?
        //{-1, 0, 0xXXXXXXXX, _sceIoGetAsyncStat},          // TODO: nid ?
        {-1, 0, 0xCEF48835, _sceIoCancel},
        //{-1, 0, 0xXXXXXXXX, _sceIoGetDevType},            // TODO: nid ?
        //{-1, 0, 0xXXXXXXXX, _sceIoChangeAsyncPriority},   // TODO: nid ?
        //{-1, 0, 0xXXXXXXXX, _sceIoSetAsyncCallback},      // TODO: nid ?
        // psp2/io/fcntl.h
        // psp2/io/dirent.h
        {-1, 0, 0xA9283DD0, _sceIoDopen},
        {-1, 0, 0x9C8B6624, _sceIoDread},
        {-1, 0, 0x422A221A, _sceIoDclose},
        // psp2/io/dirent.h
};
*/

static int open_host_fd(const char *file, int flags, SceMode mode) {

    int uid = -1;

    for (int i = 0; i < MAX_HOST_FD; i++) {
        if (host_fds[i] < 0) {
            uid = io_open((char *) file, flags, mode);
            host_fds[i] = uid;
            break;
        }
    }

    return uid;
}

static int open_host_dfd(const char *dir) {

    int uid = -1;

    for (int i = 0; i < MAX_HOST_FD; i++) {
        if (host_fds[i] < 0) {
            uid = io_dopen(dir);
            host_fds[i] = uid;
            break;
        }
    }

    return uid;
}

static int close_host_fd(int fd) {

    int res = -1;

    for (int i = 0; i < MAX_HOST_FD; i++) {
        if (host_fds[i] == fd) {
            res = io_close(fd);
            host_fds[i] = -1;
            break;
        }
    }

    return res;
}

static int close_host_dfd(int fd) {

    int res = -1;

    for (int i = 0; i < MAX_HOST_FD; i++) {
        if (host_fds[i] == fd) {
            res = io_dclose(fd);
            host_fds[i] = -1;
            break;
        }
    }

    return res;
}

static bool is_host_fd(int fd) {

    for (int i = 0; i < MAX_HOST_FD; i++) {
        if (host_fds[i] == fd) {
            return true;
        }
    }

    return false;
}


SceUID _sceIoOpen(const char *file, int flags, SceMode mode) {

    /*
    char kfile[256];
    memset(kfile, 0, 256);

    int res = ksceKernelStrncpyUserToKernel(kfile, (uintptr_t) file, 256);
    if (res < 0) {
        printf("_sceIoOpen: ksceKernelMemcpyUserToKernel = 0x%08X\n", res);
        return TAI_CONTINUE(SceUID, hooks[HOOK_IO_OPEN].ref, file, flags, mode);
    }
    */

    if (strncmp(file, "host0", 5) != 0) {
        //printf("_sceIoOpen(%s, %i, %i): host0 not detected\n", kfile, flags, mode);
        return TAI_CONTINUE(SceUID, hooks[HOOK_IO_OPEN].ref, file, flags, mode);
    }

    int fid = open_host_fd(file, flags, mode);
    printf("_sceIoOpen(%s, %i, %i) = 0x%08X\n", file, flags, mode, fid);
    return fid;
}

int _sceIoClose(SceUID fd) {

    int res;

    if (is_host_fd(fd)) {
        res = close_host_fd(fd);
        printf("_sceIoClose(0x%08X) = 0x%08X\n", fd, res);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_CLOSE].ref, fd);
    }

    return res;
}

int _sceIoRead(SceUID fd, void *data, SceSize size) {

    int res;

    if (is_host_fd(fd)) {
        res = io_read(fd, data, size);
        printf("_sceIoRead(0x%08X, data, %i) = 0x%08X\n", fd, size, res);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_READ].ref, fd, data, size);
    }

    return res;
}

int _sceIoWrite(SceUID fd, const void *data, SceSize size) {

    int res;

    if (is_host_fd(fd)) {
        res = io_write(fd, data, size);
        printf("_sceIoWrite(0x%08X, data, %i) = 0x%08X\n", fd, size, res);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_WRITE].ref, fd, data, size);
    }

    return res;
}

SceOff _sceIoLseek(SceUID fd, SceOff offset, int whence) {

    SceOff res;

    if (is_host_fd(fd)) {
        res = io_lseek(fd, offset, whence);
        printf("_sceIoLseek(0x%08X, %lld, %i) == %lld\n", fd, offset, whence, res);
    } else {
        res = TAI_CONTINUE(SceOff, hooks[HOOK_IO_LSEEK].ref, fd, offset, whence);
    }

    return res;
}

int _sceIoRemove(const char *file) {

    /*
    char kfile[256];
    memset(kfile, 0, 256);

    int res = ksceKernelStrncpyUserToKernel(kfile, (uintptr_t) file, 256);
    if (res < 0) {
        printf("_sceIoRemove: ksceKernelMemcpyUserToKernel = 0x%08X\n", res);
        return TAI_CONTINUE(int, hooks[HOOK_IO_REMOVE].ref, file);
    }
    */

    if (strncmp(file, "host0", 5) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_REMOVE].ref, file);
    }

    int res = io_remove(file);
    printf("_sceIoRemove(%s) == 0x%08X\n", file, res);

    return res;
}

int _sceIoRename(const char *oldname, const char *newname) {

    /*
    char koldname[256];
    memset(koldname, 0, 256);

    int res = ksceKernelStrncpyUserToKernel(koldname, (uintptr_t) oldname, 256);
    if (res < 0) {
        printf("_sceIoRename: ksceKernelMemcpyUserToKernel = 0x%08X\n", res);
        return TAI_CONTINUE(int, hooks[HOOK_IO_RENAME].ref, oldname, newname);
    }
    */

    if (strncmp(oldname, "host0", 5) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_RENAME].ref, oldname, newname);
    }

    //char knewname[256];
    //memset(knewname, 0, 256);
    //ksceKernelStrncpyUserToKernel(knewname, (uintptr_t) newname, 256);
    int res = io_rename(oldname, newname);
    printf("_sceIoRename(%s, %s) == 0x%08X\n", oldname, newname, res);
    return res;
}

SceUID _sceIoDopen(const char *dirname) {

    /*
    char kfile[256];
    memset(kfile, 0, 256);

    int res = ksceKernelStrncpyUserToKernel(kfile, (uintptr_t) dirname, 256);
    if (res < 0) {
        printf("_sceIoRemove: ksceKernelMemcpyUserToKernel = 0x%08X\n", res);
        return TAI_CONTINUE(SceUID, hooks[HOOK_IO_DOPEN].ref, dirname);
    }
    */

    if (strncmp(dirname, "host0", 5) != 0) {
        return TAI_CONTINUE(SceUID, hooks[HOOK_IO_DOPEN].ref, dirname);
    }

    int res = open_host_dfd(dirname);
    printf("_sceIoDopen(%s) == 0x%08X\n", dirname, res);
    return res;
}

int _sceIoDread(SceUID fd, SceIoDirent *dir) {

    int res;

    if (is_host_fd(fd)) {
        res = io_dread(fd, dir);
        if (dir != NULL) {
            printf("_sceIoDread(0x%08X, %s) == 0x%08X\n", fd, dir->d_name, res);
        } else {
            printf("_sceIoDread(0x%08X, NULL) == 0x%08X\n", fd, res);
        }
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_DREAD].ref, fd, dir);
    }

    return res;
}

int _sceIoDclose(SceUID fd) {

    int res;

    if (is_host_fd(fd)) {
        res = close_host_dfd(fd);
        printf("_sceIoDclose(0x%08X) == 0x%08X\n", fd, res);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_DCLOSE].ref, fd);
    }

    return res;
}

void set_hooks_io() {

    for (int i = 0; i < MAX_HOST_FD; i++) {
        host_fds[i] = -1;
    }

    for (int i = 0; i < HOOK_END; i++) {
        hooks[i].uid = taiHookFunctionExportForKernel(
                KERNEL_PID,
                &hooks[i].ref,
                hooks[i].name,
                hooks[i].lib,
                hooks[i].nid,
                hooks[i].func);
    }
}

void delete_hooks_io() {

    for (int i = 0; i < HOOK_END; i++) {
        if (hooks[i].uid >= 0) {
            taiHookRelease(hooks[i].uid, hooks[i].ref);
        }
    }

    for (int i = 0; i < MAX_HOST_FD; i++) {
        if (host_fds[i] >= 0) {
            io_close(host_fds[i]);
        }
    }
}
