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
        // io/fcntl.h
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x75192972, _sceIoOpen},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0xF99DD8A3, _sceIoClose},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0xE17EFC03, _sceIoRead},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x21EE91F0, _sceIoWrite},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x62090481, _sceIoLseek},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x0D7BB3E1, _sceIoRemove},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0xDC0C4997, _sceIoRename},
        // io/fcntl.h
        // io/dirent.h
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x463B25CC, _sceIoDopen},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x20CF5FC7, _sceIoDread},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x19C81DD6, _sceIoDclose},
        // io/dirent.h
        // io/stat.h
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x7F710B25, _sceIoMkdir},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x1CC9C634, _sceIoRmdir},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x75C96D25, _sceIoGetstat},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x462F059B, _sceIoGetstatByFd},
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x7D42B8DC, _sceIoChstat},
        // io/stat.h
        // io/devctl.h
        {-1, 0, "SceIofilemgr", 0x40FD29C7, 0x16882FC4, _sceIoDevctl},
        // io/devctl.h
};

static const char *path_to_host(const char *path, char *buffer) {

    size_t len = strlen(path);
    if (len > 5) {
        strncpy(buffer, path + 6, len - 5);
        return buffer;
    }

    return path;
}

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
            char buf[256];
            uid = io_dopen(path_to_host(dir, buf));
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

    if (strncmp(file, "host0", 5) != 0) {
        //printf("_sceIoOpen(%s, %i, %i): host0 not detected\n", kfile, flags, mode);
        return TAI_CONTINUE(SceUID, hooks[HOOK_IO_OPEN].ref, file, flags, mode);
    }

    int fid = open_host_fd(file, flags, mode);
    printf("_sceIoOpen(%s, 0x%08X, 0x%08X) = 0x%08X\n", file, flags, mode, fid);
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

    if (strncmp(file, "host0", 5) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_REMOVE].ref, file);
    }
    char buf[256];
    int res = io_remove(path_to_host(file, buf));
    printf("_sceIoRemove(%s) == 0x%08X\n", file, res);

    return res;
}

int _sceIoRename(const char *oldname, const char *newname) {

    if (strncmp(oldname, "host0", 5) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_RENAME].ref, oldname, newname);
    }

    char buf0[256], buf1[256];
    int res = io_rename(path_to_host(oldname, buf0), path_to_host(newname, buf1));
    printf("_sceIoRename(%s, %s) == 0x%08X\n", oldname, newname, res);
    return res;
}

SceUID _sceIoDopen(const char *dirname) {

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

int _sceIoMkdir(const char *dir, SceMode mode) {

    if (strncmp(dir, "host0", 5) != 0) {
        int res = TAI_CONTINUE(int, hooks[HOOK_IO_MKDIR].ref, dir, mode);
        printf("_sceIoMkdir(%s, 0x%08X) == 0x%08X\n", dir, mode, res);
        return res;
    }

    char buf[256];
    const char *path = path_to_host(dir, buf);
    int res = io_mkdir(path, mode);
    printf("_sceIoMkdir(%s, 0x%08X) == 0x%08X\n", path, mode, res);
    return res;
}

int _sceIoRmdir(const char *path) {

    if (strncmp(path, "host0", 5) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_RMDIR].ref, path);
    }

    char buf[256];
    int res = io_rmdir(path_to_host(path, buf));
    printf("_sceIoRmdir(%s) == 0x%08X\n", path, res);
    return res;
}

int _sceIoGetstat(const char *file, SceIoStat *stat) {

    if (strncmp(file, "host0", 5) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_GETSTAT].ref, file, stat);
    }

    char buf[256];
    int res = io_getstat(path_to_host(file, buf), stat);
    printf("_sceIoGetstat(%s) == 0x%08X\n", file, res);
    return res;
}

int _sceIoGetstatByFd(SceUID fd, SceIoStat *stat) {

    int res;

    if (is_host_fd(fd)) {
        res = io_getstatbyfd(fd, stat);
        printf("_sceIoGetstatByFd(0x%08X) == 0x%08X\n", fd, res);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_GETSTATBYFD].ref, fd, stat);
    }

    return res;
}

int _sceIoChstat(const char *file, SceIoStat *stat, int bits) {

    if (strncmp(file, "host0", 5) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_GETSTAT].ref, file, stat, bits);
    }

    char buf[256];
    int res = io_chstat(path_to_host(file, buf), stat, bits);
    printf("_sceIoChstat(%s) == 0x%08X\n", file, res);
    return res;
}

int _sceIoDevctl(const char *dev, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen) {

    if (strncmp(dev, "host0", 5) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_DEVCTL].ref, dev, cmd, indata, inlen, outdata, outlen);
    }

    int res = io_devctl(dev, cmd, indata, inlen, outdata, outlen);
    printf("_sceIoDevctl(%s) == 0x%08X\n", dev, res);
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
