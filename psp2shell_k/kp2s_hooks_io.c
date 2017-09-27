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
        //{-1, 0, "SceIofilemgr", TAI_ANY_LIBRARY, 0xCC67B6FD, _sceIoOpen},
        //{-1, 0, "SceIofilemgr", 0xCAE9ACE6, 0x6C60AC61, _sceIoOpen},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x75192972, _ksceIoOpen},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0xC3D34965, _ksceIoOpen2},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0xF99DD8A3, _ksceIoClose},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0xE17EFC03, _ksceIoRead},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x21EE91F0, _ksceIoWrite},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x62090481, _ksceIoLseek},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x0D7BB3E1, _ksceIoRemove},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0xDC0C4997, _ksceIoRename},
        // io/fcntl.h
        // io/dirent.h
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x463B25CC, _ksceIoDopen},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x20CF5FC7, _ksceIoDread},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x19C81DD6, _ksceIoDclose},
        // io/dirent.h
        // io/stat.h
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x7F710B25, _ksceIoMkdir},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x1CC9C634, _ksceIoRmdir},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x75C96D25, _ksceIoGetstat},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x462F059B, _ksceIoGetstatByFd},
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x7D42B8DC, _ksceIoChstat},
        // io/stat.h
        // io/devctl.h
        {-1, 0, "SceIofilemgr", 0x40FD29C7,      0x16882FC4, _ksceIoDevctl},
        // io/devctl.h
};

static const char *path_to_host(const char *path, char *buffer) {

    size_t len = strlen(path);
    if (len > 4) {
        strncpy(buffer, path + 5, len - 4);
        return buffer;
    }

    return path;
}

static int open_host_fd(const char *file, int flags, SceMode mode) {

    int uid = -1;

    for (int i = 0; i < MAX_HOST_FD; i++) {
        if (host_fds[i] < 0) {
            char buf[256];
            uid = io_open((char *) path_to_host(file, buf), flags, mode);
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

/*
SceUID _sceIoOpen(const char *file, int flags, SceMode mode, struct sceIoOpenOpt *opt) {
//SceUID _sceIoOpen(const char *file, int flags, SceMode mode) {

    TODO:
    if (strncmp(file, "p2s0", 4) != 0) {
        //return TAI_CONTINUE(SceUID, hooks[HOOK_IO_OPEN].ref, file, flags, mode);
        return TAI_CONTINUE(SceUID, hooks[HOOK_IO_OPEN].ref, file, flags, mode, opt);
    }

    int fid = open_host_fd(file, flags, mode);
    //printf("_sceIoOpen(%s, 0x%08X, 0x%08X) = 0x%08X\n", file, flags, mode, fid);
    return fid;
}
*/

SceUID _ksceIoOpen(const char *file, int flags, SceMode mode) {

    if (strncmp(file, "p2s0", 4) != 0) {
        return TAI_CONTINUE(SceUID, hooks[HOOK_IO_KOPEN].ref, file, flags, mode);
    }

    // TODO:
    // SCE_KERNEL_ERROR_INVALID_UID when called from user "sceIoOpen" :/
    int fid = open_host_fd(file, flags, mode);

    SceUID pid = ksceKernelGetProcessId();
    SceUID kid = ksceKernelKernelUidForUserUid(pid, fid);
    if (kid >= 0) {
        printf("_ksceIoOpen(%s, 0x%08X, 0x%08X) = 0x%08X\n", file, flags, mode, kid);
    } else {
        printf("_ksceIoOpen error: 0x%08X\n", kid);
    }

    return kid;
}

SceUID _ksceIoOpen2(const char *file, int flags, SceMode mode) {

    if (strncmp(file, "p2s0", 4) != 0) {
        return TAI_CONTINUE(SceUID, hooks[HOOK_IO_KOPEN2].ref, file, flags, mode);
    }

    // TODO:
    // SCE_KERNEL_ERROR_INVALID_UID when called from user "sceIoOpen" :/
    int fid = open_host_fd(file, flags, mode);
    printf("_ksceIoOpen2(%s, 0x%08X, 0x%08X) = 0x%08X\n", file, flags, mode, fid);
    return fid;
}

int _ksceIoClose(SceUID fd) {

    int res;

    if (is_host_fd(fd)) {
        res = close_host_fd(fd);
        printf("_ksceIoClose(0x%08X) = 0x%08X\n", fd, res);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_KCLOSE].ref, fd);
    }

    return res;
}

int _ksceIoRead(SceUID fd, void *data, SceSize size) {

    int res;

    if (is_host_fd(fd)) {
        res = io_read(fd, data, size);
        printf("_ksceIoRead(0x%08X, data, %i) = 0x%08X\n", fd, size, res);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_KREAD].ref, fd, data, size);
    }

    return res;
}

int _ksceIoWrite(SceUID fd, const void *data, SceSize size) {

    int res;

    if (is_host_fd(fd)) {
        res = io_write(fd, data, size);
        printf("_ksceIoWrite(0x%08X, data, %i) = 0x%08X\n", fd, size, res);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_KWRITE].ref, fd, data, size);
    }

    return res;
}

SceOff _ksceIoLseek(SceUID fd, SceOff offset, int whence) {

    SceOff res;

    if (is_host_fd(fd)) {
        res = io_lseek(fd, offset, whence);
        printf("_ksceIoLseek(0x%08X, %lld, %i) == %lld\n", fd, offset, whence, res);
    } else {
        res = TAI_CONTINUE(SceOff, hooks[HOOK_IO_KLSEEK].ref, fd, offset, whence);
    }

    return res;
}

int _ksceIoRemove(const char *file) {

    if (strncmp(file, "p2s0", 4) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_KREMOVE].ref, file);
    }
    char buf[256];
    int res = io_remove(path_to_host(file, buf));
    printf("_ksceIoRemove(%s) == 0x%08X\n", file, res);

    return res;
}

int _ksceIoRename(const char *oldname, const char *newname) {

    if (strncmp(oldname, "p2s0", 4) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_KRENAME].ref, oldname, newname);
    }

    char buf0[256], buf1[256];
    int res = io_rename(path_to_host(oldname, buf0), path_to_host(newname, buf1));
    printf("_ksceIoRename(%s, %s) == 0x%08X\n", oldname, newname, res);
    return res;
}

SceUID _ksceIoDopen(const char *dirname) {

    if (strncmp(dirname, "p2s0", 4) != 0) {
        return TAI_CONTINUE(SceUID, hooks[HOOK_IO_KDOPEN].ref, dirname);
    }

    int res = open_host_dfd(dirname);
    printf("_ksceIoDopen(%s) == 0x%08X\n", dirname, res);
    return res;
}

int _ksceIoDread(SceUID fd, SceIoDirent *dir) {

    int res;

    if (is_host_fd(fd)) {
        res = io_dread(fd, dir);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_KDREAD].ref, fd, dir);
    }

    return res;
}

int _ksceIoDclose(SceUID fd) {

    int res;

    if (is_host_fd(fd)) {
        res = close_host_dfd(fd);
        printf("_ksceIoDclose(0x%08X) == 0x%08X\n", fd, res);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_KDCLOSE].ref, fd);
    }

    return res;
}

int _ksceIoMkdir(const char *dir, SceMode mode) {

    if (strncmp(dir, "p2s0", 4) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_KMKDIR].ref, dir, mode);
    }

    char buf[256];
    const char *path = path_to_host(dir, buf);
    // int res = io_mkdir(path, mode); TODO: fix mode ?
    int res = io_mkdir(path, 6);
    printf("_ksceIoMkdir(%s, 0x%08X) == 0x%08X\n", path, mode, res);
    return res;
}

int _ksceIoRmdir(const char *path) {

    if (strncmp(path, "p2s0", 4) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_KRMDIR].ref, path);
    }

    char buf[256];
    int res = io_rmdir(path_to_host(path, buf));
    printf("_ksceIoRmdir(%s) == 0x%08X\n", path, res);
    return res;
}

int _ksceIoGetstat(const char *file, SceIoStat *stat) {

    if (strncmp(file, "p2s0", 4) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_KGETSTAT].ref, file, stat);
    }

    char buf[256];
    const char *path = path_to_host(file, buf);
    int res = io_getstat(path, stat);
    printf("_ksceIoGetstat(%s) == 0x%08X (m=0x%08X a=0x%08X s=%i)\n",
           path, res, stat->st_mode, stat->st_attr, (int) stat->st_size);
    return res;
}

int _ksceIoGetstatByFd(SceUID fd, SceIoStat *stat) {

    int res;

    if (is_host_fd(fd)) {
        res = io_getstatbyfd(fd, stat);
        printf("_ksceIoGetstatByFd(0x%08X) == 0x%08X (m=0x%08X a=0x%08X s=%i)\n",
               fd, res, stat->st_mode, stat->st_attr, (int) stat->st_size);
    } else {
        res = TAI_CONTINUE(int, hooks[HOOK_IO_KGETSTATBYFD].ref, fd, stat);
    }

    return res;
}

int _ksceIoChstat(const char *file, SceIoStat *stat, int bits) {

    if (strncmp(file, "p2s0", 4) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_KCHSTAT].ref, file, stat, bits);
    }

    char buf[256];
    int res = io_chstat(path_to_host(file, buf), stat, bits);
    printf("_ksceIoChstat(%s) == 0x%08X\n", file, res);
    return res;
}

int _ksceIoDevctl(const char *dev, unsigned int cmd, void *indata, int inlen, void *outdata, int outlen) {

    if (strncmp(dev, "p2s0", 4) != 0) {
        return TAI_CONTINUE(int, hooks[HOOK_IO_KDEVCTL].ref, dev, cmd, indata, inlen, outdata, outlen);
    }

    int res = io_devctl(dev, cmd, indata, inlen, outdata, outlen);
    printf("_ksceIoDevctl(%s, 0x%08X) == 0x%08X\n", dev, cmd, res);
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
