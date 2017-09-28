//
// Created by cpasjuste on 26/06/17.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <usb.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <utime.h>
#include <pthread.h>
#include <sys/statvfs.h>
#include <usbhostfs/usbhostfs.h>

#include "usbhostfs.h"
#include "hostfs.h"
#include "psp_fileio.h"

char g_rootdir[PATH_MAX];
struct HostDrive g_drives[MAX_HOSTDRIVES];
struct FileHandle open_files[MAX_FILES];
struct DirHandle open_dirs[MAX_DIRS];
int g_nocase = 0;
int g_msslash = 0;
pthread_mutex_t g_drivemtx = PTHREAD_MUTEX_INITIALIZER;

int is_dir(const char *path) {
    struct stat s;
    int ret = 0;

    if (!stat(path, &s)) {
        if (S_ISDIR(s.st_mode)) {
            ret = 1;
        }
    }

    return ret;
}

int gen_path(char *path, int dir) {
    char abspath[PATH_MAX];
    const char *tokens[MAX_TOKENS];
    const char *outtokens[MAX_TOKENS];
    int count;
    int token;
    int pathpos;

    strcpy(abspath, path);
    count = 0;
    tokens[0] = strtok(abspath, "/");
    while ((tokens[count]) && (count < (MAX_TOKENS - 1))) {
        tokens[++count] = strtok(NULL, "/");
    }

    /* Remove any single . and .. */
    pathpos = 0;
    for (token = 0; token < count; token++) {
        if (strcmp(tokens[token], ".") == 0) {
            /* Do nothing */
        } else if (strcmp(tokens[token], "..") == 0) {
            /* Decrement the path position if > 0 */
            if (pathpos > 0) {
                pathpos--;
            }
        } else {
            outtokens[pathpos++] = tokens[token];
        }
    }

    strcpy(path, "/");
    for (token = 0; token < pathpos; token++) {
        strcat(path, outtokens[token]);
        if ((dir) || (token < (pathpos - 1))) {
            strcat(path, "/");
        }
    }

    return 1;
}

int calc_rating(const char *str1, const char *str2) {
    int rating = 0;

    while ((*str1) && (*str2)) {
        if (*str1 == *str2) {
            rating++;
        }
        str1++;
        str2++;
    }

    return rating;
}

/* Scan the directory, return the first name which matches case insensitive */
int find_nocase(const char *rootdir, const char *relpath, char *token) {
    DIR *dir;
    struct dirent *ent;
    char abspath[PATH_MAX];
    char match[PATH_MAX];
    int len;
    int rating = -1;
    int ret = 0;

    V_PRINTF(2, "Finding token %s\n", token);

    len = snprintf(abspath, PATH_MAX, "%s%s", rootdir, relpath);
    if ((len < 0) || (len > PATH_MAX)) {
        return 0;
    }

    V_PRINTF(2, "Checking %s\n", abspath);
    dir = opendir(abspath);
    if (dir != NULL) {
        V_PRINTF(2, "Opened directory\n");
        while ((ent = readdir(dir))) {
            V_PRINTF(2, "Got dir entry %p->%s\n", ent, ent->d_name);
            if (strcasecmp(ent->d_name, token) == 0) {
                int tmp;

                tmp = calc_rating(token, ent->d_name);
                V_PRINTF(2, "Found match %s for %s rating %d\n", ent->d_name, token, tmp);
                if (tmp > rating) {
                    strcpy(match, ent->d_name);
                    rating = tmp;
                }

                ret = 1;
            }
        }

        closedir(dir);
    } else {
        V_PRINTF(2, "Couldn't open %s\n", abspath);
    }

    if (ret) {
        strcpy(token, match);
    }

    return ret;
}

/* Make a relative path case insensitive, if we fail then leave the path as is, just in case */
void make_nocase(const char *rootdir, char *path, int dir) {
    char abspath[PATH_MAX];
    char retpath[PATH_MAX];
    char *tokens[MAX_TOKENS];
    int count;
    int token;

    strcpy(abspath, path);
    count = 0;
    tokens[0] = strtok(abspath, "/");
    while ((tokens[count]) && (count < (MAX_TOKENS - 1))) {
        tokens[++count] = strtok(NULL, "/");
    }

    strcpy(retpath, "/");
    for (token = 0; token < count; token++) {
        if (!find_nocase(rootdir, retpath, tokens[token])) {
            /* Might only be an error if this is not the last token, otherwise we could be
             * trying to create a new directory or file, if we are not then the rest of the code
             * will handle the error */
            if ((token < (count - 1))) {
                break;
            }
        }

        strcat(retpath, tokens[token]);
        if ((dir) || (token < (count - 1))) {
            strcat(retpath, "/");
        }
    }

    if (token == count) {
        strcpy(path, retpath);
    }
}

int make_path(unsigned int drive, const char *path, char *retpath, int dir) {
    char hostpath[PATH_MAX];
    int len;
    int ret = -1;

    if (drive >= MAX_HOSTDRIVES) {
        fprintf(stderr, "Host drive number is too large (%d)\n", drive);
        return -1;
    }

    if (pthread_mutex_lock(&g_drivemtx)) {
        fprintf(stderr, "Could not lock mutex (%s)\n", strerror(errno));
        return -1;
    }

    do {

        len = snprintf(hostpath, PATH_MAX, "%s%s", g_drives[drive].currdir, path);
        if ((len < 0) || (len >= PATH_MAX)) {
            fprintf(stderr, "Path length too big (%d)\n", len);
            break;
        }

        if (g_msslash) {
            int i;

            for (i = 0; i < len; i++) {
                if (hostpath[i] == '\\') {
                    hostpath[i] = '/';
                }
            }
        }

        if (gen_path(hostpath, dir) == 0) {
            break;
        }

        /* Make the relative path case insensitive if needed */
        if (g_nocase) {
            make_nocase(g_drives[drive].rootdir, hostpath, dir);
        }

        len = snprintf(retpath, PATH_MAX, "%s/%s", g_drives[drive].rootdir, hostpath);
        if ((len < 0) || (len >= PATH_MAX)) {
            fprintf(stderr, "Path length too big (%d)\n", len);
            break;
        }

        if (gen_path(retpath, dir) == 0) {
            break;
        }

        ret = 0;
    } while (0);

    pthread_mutex_unlock(&g_drivemtx);

    return ret;
}

int open_file(int drive, const char *path, unsigned int mode, unsigned int mask) {
    char fullpath[PATH_MAX];
    unsigned int real_mode = 0;
    int fd = -1;

    if (make_path(drive, path, fullpath, 0) < 0) {
        V_PRINTF(1, "Invalid file path %s\n", path);
        return GETERROR(ENOENT);
    }

    V_PRINTF(2, "open: %s\n", fullpath);
    V_PRINTF(1, "Opening file %s\n", fullpath);

    if ((mode & PSP_O_RDWR) == PSP_O_RDWR) {
        V_PRINTF(2, "Read/Write mode\n");
        real_mode = O_RDWR;
    } else {
        if (mode & PSP_O_RDONLY) {
            V_PRINTF(2, "Read mode\n");
            real_mode = O_RDONLY;
        } else if (mode & PSP_O_WRONLY) {
            V_PRINTF(2, "Write mode\n");
            real_mode = O_WRONLY;
        } else {
            V_PRINTF(2, "Read mode\n");
            real_mode = O_RDONLY;
            /*
            fprintf(stderr, "No access mode specified\n");
            return GETERROR(EINVAL);
            */
        }
    }

    if (mode & PSP_O_APPEND) {
        real_mode |= O_APPEND;
    }

    if (mode & PSP_O_CREAT) {
        real_mode |= O_CREAT;
    }

    if (mode & PSP_O_TRUNC) {
        real_mode |= O_TRUNC;
    }

    if (mode & PSP_O_EXCL) {
        real_mode |= O_EXCL;
    }

    if (!is_dir(fullpath)) {
        fd = open(fullpath, real_mode, mask & ~0111);
        if (fd >= 0) {
            if (fd < MAX_FILES) {
                open_files[fd].opened = 1;
                open_files[fd].mode = mode;
                open_files[fd].name = strdup(fullpath);
            } else {
                close(fd);
                fprintf(stderr, "Error filedescriptor out of range\n");
                fd = GETERROR(EMFILE);
            }
        } else {
            V_PRINTF(1, "Could not open file %s\n", fullpath);
            fd = GETERROR(errno);
        }
    } else {
        fd = GETERROR(ENOENT);
    }

    return fd;
}

void fill_time(time_t t, SceDateTime *scetime) {
    struct tm *filetime;

    memset(scetime, 0, sizeof(*scetime));
    filetime = localtime(&t);
    scetime->year = LE16(filetime->tm_year + 1900);
    scetime->month = LE16(filetime->tm_mon + 1);
    scetime->day = LE16(filetime->tm_mday);
    scetime->hour = LE16(filetime->tm_hour);
    scetime->minute = LE16(filetime->tm_min);
    scetime->second = LE16(filetime->tm_sec);
}

int fill_stat(const char *dirname, const char *name, SceIoStat *scestat) {
    char path[PATH_MAX];
    struct stat st;
    int len;

    /* If dirname is NULL then name is a preconverted path */
    if (dirname != NULL) {
        if (dirname[strlen(dirname) - 1] == '/') {
            len = snprintf(path, PATH_MAX, "%s%s", dirname, name);
        } else {
            len = snprintf(path, PATH_MAX, "%s/%s", dirname, name);
        }
        if ((len < 0) || (len > PATH_MAX)) {
            fprintf(stderr, "Couldn't fill in directory name\n");
            return GETERROR(ENAMETOOLONG);
        }
    } else {
        strcpy(path, name);
    }

    if (stat(path, &st) < 0) {
        fprintf(stderr, "Couldn't stat file %s (%s)\n", path, strerror(errno));
        return GETERROR(errno);
    }

    scestat->size = (SceOff) LE64(st.st_size);
    scestat->mode = 0;
    scestat->attr = 0;
    if (S_ISLNK(st.st_mode)) {
        scestat->attr = LE32(FIO_SO_IFLNK);
        scestat->mode = LE32(FIO_S_IFLNK);
    } else if (S_ISDIR(st.st_mode)) {
        scestat->attr = LE32(FIO_SO_IFDIR);
        scestat->mode = LE32(FIO_S_IFDIR);
    } else {
        scestat->attr = LE32(FIO_SO_IFREG);
        scestat->mode = LE32(FIO_S_IFREG);
    }

    // TODO: fix mode for psp2
    scestat->mode |= 0777;//|= LE32(st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));

    fill_time(st.st_ctime, &scestat->ctime);
    fill_time(st.st_atime, &scestat->atime);
    fill_time(st.st_mtime, &scestat->mtime);

    return 0;
}

int fill_statbyfd(int32_t fd, SceIoStat *scestat) {

    struct stat st;

    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "Couldn't stat fd 0x%08X (%s)\n", fd, strerror(errno));
        return GETERROR(errno);
    }

    scestat->size = (SceOff) LE64(st.st_size);
    scestat->mode = 0;
    scestat->attr = 0;
    if (S_ISLNK(st.st_mode)) {
        scestat->attr = LE32(FIO_SO_IFLNK);
        scestat->mode = LE32(FIO_S_IFLNK);
    } else if (S_ISDIR(st.st_mode)) {
        scestat->attr = LE32(FIO_SO_IFDIR);
        scestat->mode = LE32(FIO_S_IFDIR);
    } else {
        scestat->attr = LE32(FIO_SO_IFREG);
        scestat->mode = LE32(FIO_S_IFREG);
    }

    // TODO: fix mode for psp2
    scestat->mode |= 0777;//LE32(st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));

    fill_time(st.st_ctime, &scestat->ctime);
    fill_time(st.st_atime, &scestat->atime);
    fill_time(st.st_mtime, &scestat->mtime);

    return 0;
}

static int filter_dots(const struct dirent *e) {
    if (strcmp(e->d_name, ".") == 0
        || strcmp(e->d_name, "..") == 0) {
        return 0;
    }
    return 1;
}

int dir_open(int drive, const char *dirname) {
    char fulldir[PATH_MAX];
    struct dirent **entries;
    int ret = -1;
    int i;
    int did;
    int dirnum;

    do {
        for (did = 0; did < MAX_DIRS; did++) {
            if (!open_dirs[did].opened) {
                break;
            }
        }

        if (did == MAX_DIRS) {
            fprintf(stderr, "Could not find free directory handle\n");
            ret = GETERROR(EMFILE);
            break;
        }

        if (make_path(drive, dirname, fulldir, 1) < 0) {
            ret = GETERROR(ENOENT);
            break;
        }

        V_PRINTF(2, "dopen: %s, fsnum %d\n", fulldir, drive);
        V_PRINTF(1, "Opening directory %s\n", fulldir);

        memset(&open_dirs[did], 0, sizeof(open_dirs[did]));

        dirnum = scandir(fulldir, &entries, filter_dots, alphasort);
        if (dirnum <= 0) {
            V_PRINTF(2, "Could not scan directory %s (%s)\n", fulldir, strerror(errno));
            ret = GETERROR(errno);
            break;
        }

        V_PRINTF(2, "Number of dir entries %d\n", dirnum);

        open_dirs[did].pDir = malloc(sizeof(SceIoDirent) * dirnum);
        if (open_dirs[did].pDir != NULL) {
            memset(open_dirs[did].pDir, 0, sizeof(SceIoDirent) * dirnum);
            for (i = 0; i < dirnum; i++) {
                strcpy(open_dirs[did].pDir[i].name, entries[i]->d_name);
                V_PRINTF(2, "Dirent %d: %s\n", i, entries[i]->d_name);
                if (fill_stat(fulldir, entries[i]->d_name, &open_dirs[did].pDir[i].stat) < 0) {
                    fprintf(stderr, "Error filling in directory structure\n");
                    break;
                }
            }

            if (i == dirnum) {
                ret = did;
                open_dirs[did].pos = 0;
                open_dirs[did].count = dirnum;
                open_dirs[did].opened = 1;
            } else {
                free(open_dirs[did].pDir);
            }
        } else {
            fprintf(stderr, "Could not allocate memory for directories\n");
        }

        if (ret < 0) {
            for (i = 0; i < dirnum; i++) {
                free(entries[i]);
            }
            free(entries);
        }
    } while (0);

    return ret;
}

int dir_close(int did) {
    int ret = -1;
    if ((did >= 0) && (did < MAX_DIRS)) {
        if (open_dirs[did].opened) {
            if (open_dirs[did].pDir) {
                free(open_dirs[did].pDir);
            }

            open_dirs[did].opened = 0;
            open_dirs[did].count = 0;
            open_dirs[did].pos = 0;
            open_dirs[did].pDir = NULL;

            ret = 0;
        }
    }

    return ret;
}

int handle_hello(struct usb_dev_handle *hDev) {
    struct HostFsHelloResp resp;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_HELLO);

    return usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
}

int handle_open(struct usb_dev_handle *hDev, struct HostFsOpenCmd *cmd, int cmdlen) {
    struct HostFsOpenResp resp;
    int ret = -1;
    char path[HOSTFS_PATHMAX];

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_OPEN);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsOpenCmd)) {
            fprintf(stderr, "Error, invalid open command size %d\n", cmdlen);
            break;
        }

        if (LE32(cmd->cmd.extralen) == 0) {
            fprintf(stderr, "Error, no filename passed with open command\n");
            break;
        }

        /* TODO: Should check that length is within a valid range */

        ret = euid_usb_bulk_read(hDev, 0x81, path, LE32(cmd->cmd.extralen), 10000);
        if (ret != LE32(cmd->cmd.extralen)) {
            fprintf(stderr, "Error reading open data cmd->extralen %ud, ret %d\n", LE32(cmd->cmd.extralen), ret);
            break;
        }

        V_PRINTF(2, "Open command mode %08X mask %08X name %s\n", LE32(cmd->mode), LE32(cmd->mask), path);
        resp.res = LE32(open_file(LE32(cmd->fsnum), path, LE32(cmd->mode), LE32(cmd->mask)));

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int handle_dopen(struct usb_dev_handle *hDev, struct HostFsDopenCmd *cmd, int cmdlen) {
    struct HostFsDopenResp resp;
    int ret = -1;
    char path[HOSTFS_PATHMAX];

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_DOPEN);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsDopenCmd)) {
            fprintf(stderr, "Error, invalid dopen command size %d\n", cmdlen);
            break;
        }

        if (LE32(cmd->cmd.extralen) == 0) {
            fprintf(stderr, "Error, no dirname passed with dopen command\n");
            break;
        }

        /* TODO: Should check that length is within a valid range */

        ret = euid_usb_bulk_read(hDev, 0x81, path, LE32(cmd->cmd.extralen), 10000);
        if (ret != LE32(cmd->cmd.extralen)) {
            fprintf(stderr, "Error reading open data cmd->extralen %d, ret %d\n", LE32(cmd->cmd.extralen), ret);
            break;
        }

        V_PRINTF(2, "Dopen command name %s\n", path);
        resp.res = LE32(dir_open(LE32(cmd->fsnum), path));

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int fixed_write(int fd, const void *data, int len) {
    int byteswrite = 0;
    while (byteswrite < len) {
        int ret;

        ret = write(fd, data + byteswrite, len - byteswrite);
        if (ret < 0) {
            if (errno != EINTR) {
                fprintf(stderr, "Error writing to file (%s)\n", strerror(errno));
                byteswrite = GETERROR(errno);
                break;
            }
        } else if (ret == 0) /* EOF? */
        {
            break;
        } else {
            byteswrite += ret;
        }
    }

    return byteswrite;
}

int handle_write(struct usb_dev_handle *hDev, struct HostFsWriteCmd *cmd, int cmdlen) {
    static char write_block[HOSTFS_MAX_BLOCK];
    struct HostFsWriteResp resp;
    int fid;
    int ret = -1;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_WRITE);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsWriteCmd)) {
            fprintf(stderr, "Error, invalid write command size %d\n", cmdlen);
            break;
        }

        /* TODO: Check upper bound */
        if (LE32(cmd->cmd.extralen) <= 0) {
            fprintf(stderr, "Error extralen invalid (%d)\n", LE32(cmd->cmd.extralen));
            break;
        }

        /* TODO: Should check that length is within a valid range */

        ret = euid_usb_bulk_read(hDev, 0x81, write_block, LE32(cmd->cmd.extralen), 10000);
        if (ret != LE32(cmd->cmd.extralen)) {
            fprintf(stderr, "Error reading write data cmd->extralen %d, ret %d\n", LE32(cmd->cmd.extralen), ret);
            break;
        }

        fid = LE32(cmd->fid);

        V_PRINTF(2, "Write command fid: %d, length: %d\n", fid, LE32(cmd->cmd.extralen));

        if ((fid >= 0) && (fid < MAX_FILES)) {
            if (open_files[fid].opened) {
                resp.res = LE32(fixed_write(fid, write_block, LE32(cmd->cmd.extralen)));
            } else {
                fprintf(stderr, "Error fid not open %d\n", fid);
            }
        } else {
            fprintf(stderr, "Error invalid fid %d\n", fid);
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int fixed_read(int fd, void *data, int len) {
    int bytesread = 0;

    while (bytesread < len) {
        int ret;

        ret = read(fd, data + bytesread, len - bytesread);
        if (ret < 0) {
            if (errno != EINTR) {
                bytesread = GETERROR(errno);
                break;
            }
        } else if (ret == 0) {
            /* No more to read */
            break;
        } else {
            bytesread += ret;
        }
    }

    return bytesread;
}

int handle_read(struct usb_dev_handle *hDev, struct HostFsReadCmd *cmd, int cmdlen) {
    static char read_block[HOSTFS_MAX_BLOCK];
    struct HostFsReadResp resp;
    int fid;
    int ret = -1;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_READ);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsReadCmd)) {
            fprintf(stderr, "Error, invalid read command size %d\n", cmdlen);
            break;
        }

        /* TODO: Check upper bound */
        if (LE32(cmd->len) <= 0) {
            fprintf(stderr, "Error extralen invalid (%d)\n", LE32(cmd->len));
            break;
        }

        fid = LE32(cmd->fid);
        V_PRINTF(2, "Read command fid: %d, length: %d\n", fid, LE32(cmd->len));

        if ((fid >= 0) && (fid < MAX_FILES)) {
            if (open_files[fid].opened) {
                resp.res = LE32(fixed_read(fid, read_block, LE32(cmd->len)));
                if (LE32(resp.res) >= 0) {
                    resp.cmd.extralen = resp.res;
                }
            } else {
                fprintf(stderr, "Error fid not open %d\n", fid);
            }
        } else {
            fprintf(stderr, "Error invalid fid %d\n", fid);
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
        if (ret < 0) {
            fprintf(stderr, "Error writing read response (%d)\n", ret);
            break;
        }

        if (LE32(resp.cmd.extralen) > 0) {
            ret = euid_usb_bulk_write(hDev, 0x2, read_block, LE32(resp.cmd.extralen), 10000);
        }
    } while (0);

    return ret;
}

int handle_close(struct usb_dev_handle *hDev, struct HostFsCloseCmd *cmd, int cmdlen) {
    struct HostFsCloseResp resp;
    int ret = -1;
    int fid;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_CLOSE);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsCloseCmd)) {
            fprintf(stderr, "Error, invalid close command size %d\n", cmdlen);
            break;
        }

        fid = LE32(cmd->fid);
        V_PRINTF(2, "Close command fid: %d\n", fid);
        if ((fid > STDERR_FILENO) && (fid < MAX_FILES) && (open_files[fid].opened)) {
            if (close(fid) < 0) {
                resp.res = LE32(GETERROR(errno));
            } else {
                resp.res = LE32(0);
            }

            open_files[fid].opened = 0;
            if (open_files[fid].name) {
                free(open_files[fid].name);
                open_files[fid].name = NULL;
            }
        } else {
            fprintf(stderr, "Error invalid file id in close command (%d)\n", fid);
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int handle_dclose(struct usb_dev_handle *hDev, struct HostFsDcloseCmd *cmd, int cmdlen) {
    struct HostFsDcloseResp resp;
    int ret = -1;
    int did;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_DCLOSE);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsDcloseCmd)) {
            fprintf(stderr, "Error, invalid close command size %d\n", cmdlen);
            break;
        }

        did = LE32(cmd->did);
        V_PRINTF(2, "Dclose command did: %d\n", did);
        resp.res = dir_close(did);

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);


    return ret;
}

int handle_dread(struct usb_dev_handle *hDev, struct HostFsDreadCmd *cmd, int cmdlen) {
    struct HostFsDreadResp resp;
    SceIoDirent *dir = NULL;
    int ret = -1;
    int did;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_READ);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsDreadCmd)) {
            fprintf(stderr, "Error, invalid dread command size %d\n", cmdlen);
            break;
        }

        did = LE32(cmd->did);
        V_PRINTF(2, "Dread command did: %d\n", did);

        if ((did >= 0) && (did < MAX_FILES)) {
            if (open_dirs[did].opened) {
                if (open_dirs[did].pos < open_dirs[did].count) {
                    dir = &open_dirs[did].pDir[open_dirs[did].pos++];
                    resp.cmd.extralen = LE32(sizeof(SceIoDirent));
                    resp.res = LE32(open_dirs[did].count - open_dirs[did].pos + 1);
                } else {
                    resp.res = LE32(0);
                }
            } else {
                fprintf(stderr, "Error did not open %d\n", did);
            }
        } else {
            fprintf(stderr, "Error invalid did %d\n", did);
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
        if (ret < 0) {
            fprintf(stderr, "Error writing dread response (%d)\n", ret);
            break;
        }

        if (LE32(resp.cmd.extralen) > 0) {
            ret = euid_usb_bulk_write(hDev, 0x2, (char *) dir, LE32(resp.cmd.extralen), 10000);
        }
    } while (0);

    return ret;
}

int handle_lseek(struct usb_dev_handle *hDev, struct HostFsLseekCmd *cmd, int cmdlen) {
    struct HostFsLseekResp resp;
    int ret = -1;
    int fid;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_LSEEK);
    resp.res = LE32(-1);
    resp.ofs = LE32(0);

    do {
        if (cmdlen != sizeof(struct HostFsLseekCmd)) {
            fprintf(stderr, "Error, invalid lseek command size %d\n", cmdlen);
            break;
        }

        fid = LE32(cmd->fid);
        V_PRINTF(2, "Lseek command fid: %d, ofs: %ld, whence: %d\n", fid, LE64(cmd->ofs), LE32(cmd->whence));
        if ((fid > STDERR_FILENO) && (fid < MAX_FILES) && (open_files[fid].opened)) {
            /* TODO: Probably should ensure whence is mapped across, just in case */
            resp.ofs = LE64((int64_t) lseek(fid, (off_t) LE64(cmd->ofs), LE32(cmd->whence)));
            if (LE64(resp.ofs) < 0) {
                resp.res = LE32(-1);
            } else {
                resp.res = LE32(0);
            }
        } else {
            fprintf(stderr, "Error invalid file id in close command (%d)\n", fid);
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int handle_remove(struct usb_dev_handle *hDev, struct HostFsRemoveCmd *cmd, int cmdlen) {
    struct HostFsRemoveResp resp;
    int ret = -1;
    char path[HOSTFS_PATHMAX];
    char fullpath[PATH_MAX];

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_REMOVE);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsRemoveCmd)) {
            fprintf(stderr, "Error, invalid remove command size %d\n", cmdlen);
            break;
        }

        if (LE32(cmd->cmd.extralen) == 0) {
            fprintf(stderr, "Error, no filename passed with remove command\n");
            break;
        }

        /* TODO: Should check that length is within a valid range */

        ret = euid_usb_bulk_read(hDev, 0x81, path, LE32(cmd->cmd.extralen), 10000);
        if (ret != LE32(cmd->cmd.extralen)) {
            fprintf(stderr, "Error reading remove data cmd->extralen %d, ret %d\n", LE32(cmd->cmd.extralen), ret);
            break;
        }

        V_PRINTF(2, "Remove command name %s\n", path);
        if (make_path(LE32(cmd->fsnum), path, fullpath, 0) == 0) {
            if (unlink(fullpath) < 0) {
                resp.res = LE32(GETERROR(errno));
            } else {
                resp.res = LE32(0);
            }
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int handle_rmdir(struct usb_dev_handle *hDev, struct HostFsRmdirCmd *cmd, int cmdlen) {
    struct HostFsRmdirResp resp;
    int ret = -1;
    char path[HOSTFS_PATHMAX];
    char fullpath[PATH_MAX];

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_RMDIR);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsRmdirCmd)) {
            fprintf(stderr, "Error, invalid rmdir command size %d\n", cmdlen);
            break;
        }

        if (LE32(cmd->cmd.extralen) == 0) {
            fprintf(stderr, "Error, no filename passed with rmdir command\n");
            break;
        }

        /* TODO: Should check that length is within a valid range */

        ret = euid_usb_bulk_read(hDev, 0x81, path, LE32(cmd->cmd.extralen), 10000);
        if (ret != LE32(cmd->cmd.extralen)) {
            fprintf(stderr, "Error reading rmdir data cmd->extralen %d, ret %d\n", LE32(cmd->cmd.extralen), ret);
            break;
        }

        V_PRINTF(2, "Rmdir command name %s\n", path);
        if (make_path(LE32(cmd->fsnum), path, fullpath, 0) == 0) {
            if (rmdir(fullpath) < 0) {
                resp.res = LE32(GETERROR(errno));
            } else {
                resp.res = LE32(0);
            }
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int handle_mkdir(struct usb_dev_handle *hDev, struct HostFsMkdirCmd *cmd, int cmdlen) {
    struct HostFsMkdirResp resp;
    int ret = -1;
    char path[HOSTFS_PATHMAX];
    char fullpath[PATH_MAX];

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_MKDIR);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsMkdirCmd)) {
            fprintf(stderr, "Error, invalid mkdir command size %d\n", cmdlen);
            break;
        }

        if (LE32(cmd->cmd.extralen) == 0) {
            fprintf(stderr, "Error, no filename passed with mkdir command\n");
            break;
        }

        /* TODO: Should check that length is within a valid range */

        ret = euid_usb_bulk_read(hDev, 0x81, path, LE32(cmd->cmd.extralen), 10000);
        if (ret != LE32(cmd->cmd.extralen)) {
            fprintf(stderr, "Error reading mkdir data cmd->extralen %d, ret %d\n", LE32(cmd->cmd.extralen), ret);
            break;
        }

        V_PRINTF(2, "Mkdir command mode %08X, name %s\n", LE32(cmd->mode), path);
        if (make_path(LE32(cmd->fsnum), path, fullpath, 0) == 0) {
            if (mkdir(fullpath, LE32(cmd->mode)) < 0) {
                resp.res = LE32(GETERROR(errno));
            } else {
                resp.res = LE32(0);
            }
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int handle_getstat(struct usb_dev_handle *hDev, struct HostFsGetstatCmd *cmd, int cmdlen) {
    struct HostFsGetstatResp resp;
    SceIoStat st;
    int ret = -1;
    char path[HOSTFS_PATHMAX];
    char fullpath[PATH_MAX];

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_GETSTAT);
    resp.res = LE32(-1);
    memset(&st, 0, sizeof(st));

    do {
        if (cmdlen != sizeof(struct HostFsGetstatCmd)) {
            fprintf(stderr, "Error, invalid getstat command size %d\n", cmdlen);
            break;
        }

        if (LE32(cmd->cmd.extralen) == 0) {
            fprintf(stderr, "Error, no filename passed with getstat command\n");
            break;
        }

        /* TODO: Should check that length is within a valid range */

        ret = euid_usb_bulk_read(hDev, 0x81, path, LE32(cmd->cmd.extralen), 10000);
        if (ret != LE32(cmd->cmd.extralen)) {
            fprintf(stderr, "Error reading getstat data cmd->extralen %d, ret %d\n", LE32(cmd->cmd.extralen), ret);
            break;
        }

        V_PRINTF(2, "Getstat command name %s\n", path);
        if (make_path(LE32(cmd->fsnum), path, fullpath, 0) == 0) {
            resp.res = LE32(fill_stat(NULL, fullpath, &st));
            if (LE32(resp.res) == 0) {
                resp.cmd.extralen = LE32(sizeof(st));
                //printf("Getstat(%s): m=%i a=%i s=%li\n", fullpath, st.mode, st.attr, (long) st.size);
            }
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
        if (ret < 0) {
            fprintf(stderr, "Error writing getstat response (%d)\n", ret);
            break;
        }

        if (LE32(resp.cmd.extralen) > 0) {
            ret = euid_usb_bulk_write(hDev, 0x2, (char *) &st, sizeof(st), 10000);
        }
    } while (0);

    return ret;
}

int handle_getstatbyfd(struct usb_dev_handle *hDev, struct HostFsGetstatByFdCmd *cmd, int cmdlen) {

    struct HostFsGetstatByFdResp resp;
    SceIoStat st;
    int ret = -1;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_GETSTATBYFD);
    resp.res = LE32(-1);
    memset(&st, 0, sizeof(st));

    do {
        if (cmdlen != sizeof(struct HostFsGetstatByFdCmd)) {
            fprintf(stderr, "Error, invalid getstatbyfd command size %d\n", cmdlen);
            break;
        }

        int32_t fid = LE32(cmd->fid);
        V_PRINTF(2, "GetstatByFd fd: 0x%08X\n", fid);

        resp.res = LE32(fill_statbyfd(fid, &st));
        if (LE32(resp.res) == 0) {
            resp.mode = st.mode;
            resp.attr = st.attr;
            resp.size = st.size;
            resp.ctime = st.ctime;
            resp.atime = st.atime;
            resp.mtime = st.mtime;
            //printf("GetstatByFd: m=%i a=%i s=%li\n", st.mode, st.attr, (long) st.size);
            //resp.cmd.extralen = LE32(sizeof(st));
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
        if (ret < 0) {
            fprintf(stderr, "Error writing getstatbyfd response (%d)\n", ret);
            break;
        }

    } while (0);

    return ret;
}

int psp_settime(const char *path, const struct HostFsTimeStamp *ts, int set) {
    time_t convtime;
    struct tm stime;
    struct utimbuf tbuf;
    struct stat st;

    stime.tm_year = LE16(ts->year) - 1900;
    stime.tm_mon = LE16(ts->month) - 1;
    stime.tm_mday = LE16(ts->day);
    stime.tm_hour = LE16(ts->hour);
    stime.tm_min = LE16(ts->minute);
    stime.tm_sec = LE16(ts->second);

    if (stat(path, &st) < 0) {
        return -1;
    }

    tbuf.actime = st.st_atime;
    tbuf.modtime = st.st_mtime;

    convtime = mktime(&stime);
    if (convtime == (time_t) -1) {
        return -1;
    }

    if (set == PSP_CHSTAT_ATIME) {
        tbuf.actime = convtime;
    } else if (set == PSP_CHSTAT_MTIME) {
        tbuf.modtime = convtime;
    } else {
        return -1;
    }

    return utime(path, &tbuf);

}

int psp_chstat(const char *path, struct HostFsChstatCmd *cmd) {
    int ret = 0;

    if (LE32(cmd->bits) & PSP_CHSTAT_MODE) {
        int mask;

        mask = LE32(cmd->mode) & (FIO_S_IRWXU | FIO_S_IRWXG | FIO_S_IRWXO);
        ret = chmod(path, mask);
        if (ret < 0) {
            V_PRINTF(2, "Could not set file mask\n");
            return GETERROR(errno);
        }
    }

    if (LE32(cmd->bits) & PSP_CHSTAT_SIZE) {
        /* Do a truncate */
    }

    if (LE32(cmd->bits) & PSP_CHSTAT_ATIME) {
        if (psp_settime(path, &cmd->atime, PSP_CHSTAT_ATIME) < 0) {
            V_PRINTF(2, "Could not set access time\n");
            return -1;
        }
    }

    if (LE32(cmd->bits) & PSP_CHSTAT_MTIME) {
        if (psp_settime(path, &cmd->mtime, PSP_CHSTAT_MTIME) < 0) {
            V_PRINTF(2, "Could not set modification time\n");
            return -1;
        }
    }

    return 0;
}

int handle_chstat(struct usb_dev_handle *hDev, struct HostFsChstatCmd *cmd, int cmdlen) {
    struct HostFsChstatResp resp;
    int ret = -1;
    char path[HOSTFS_PATHMAX];
    char fullpath[PATH_MAX];

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_CHSTAT);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsChstatCmd)) {
            fprintf(stderr, "Error, invalid chstat command size %d\n", cmdlen);
            break;
        }

        if (LE32(cmd->cmd.extralen) == 0) {
            fprintf(stderr, "Error, no filename passed with chstat command\n");
            break;
        }

        /* TODO: Should check that length is within a valid range */

        ret = euid_usb_bulk_read(hDev, 0x81, path, LE32(cmd->cmd.extralen), 10000);
        if (ret != LE32(cmd->cmd.extralen)) {
            fprintf(stderr, "Error reading chstat data cmd->extralen %d, ret %d\n", LE32(cmd->cmd.extralen), ret);
            break;
        }

        V_PRINTF(2, "Chstat command name %s, bits %08X\n", path, LE32(cmd->bits));
        if (make_path(LE32(cmd->fsnum), path, fullpath, 0) == 0) {
            resp.res = LE32(psp_chstat(fullpath, cmd));
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int handle_rename(struct usb_dev_handle *hDev, struct HostFsRenameCmd *cmd, int cmdlen) {
    struct HostFsRenameResp resp;
    int ret = -1;
    char path[HOSTFS_PATHMAX];
    char oldpath[PATH_MAX];
    char newpath[PATH_MAX];
    char destpath[PATH_MAX];
    int oldpathlen;
    int newpathlen;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_RENAME);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsRenameCmd)) {
            fprintf(stderr, "Error, invalid rename command size %d\n", cmdlen);
            break;
        }

        if (LE32(cmd->cmd.extralen) == 0) {
            fprintf(stderr, "Error, no filenames passed with rename command\n");
            break;
        }

        /* TODO: Should check that length is within a valid range */

        memset(path, 0, sizeof(path));
        ret = euid_usb_bulk_read(hDev, 0x81, path, LE32(cmd->cmd.extralen), 10000);
        if (ret != LE32(cmd->cmd.extralen)) {
            fprintf(stderr, "Error reading rename data cmd->extralen %d, ret %d\n", LE32(cmd->cmd.extralen), ret);
            break;
        }

        /* Really should check this better ;) */
        oldpathlen = strlen(path);
        newpathlen = strlen(path + oldpathlen + 1);

        /* If the old path is absolute and the new path is relative then rebase newpath */
        if ((*path == '/') && (*(path + oldpathlen + 1) != '/')) {
            char *slash;

            strcpy(destpath, path);
            /* No need to check, should at least stop on the first slash */
            slash = strrchr(destpath, '/');
            /* Nul terminate after slash */
            *(slash + 1) = 0;
            strcat(destpath, path + oldpathlen + 1);
        } else {
            /* Just copy in oldpath */
            strcpy(destpath, path + oldpathlen + 1);
        }

        V_PRINTF(2, "Rename command oldname %s, newname %s\n", path, destpath);

        if (!make_path(LE32(cmd->fsnum), path, oldpath, 0) && !make_path(LE32(cmd->fsnum), destpath, newpath, 0)) {
            if (rename(oldpath, newpath) < 0) {
                resp.res = LE32(GETERROR(errno));
            } else {
                resp.res = LE32(0);
            }
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int handle_chdir(struct usb_dev_handle *hDev, struct HostFsChdirCmd *cmd, int cmdlen) {
    struct HostFsChdirResp resp;
    int ret = -1;
    char path[HOSTFS_PATHMAX];
    int fsnum;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_CHDIR);
    resp.res = -1;

    do {
        if (cmdlen != sizeof(struct HostFsChdirCmd)) {
            fprintf(stderr, "Error, invalid chdir command size %d\n", cmdlen);
            break;
        }

        if (LE32(cmd->cmd.extralen) == 0) {
            fprintf(stderr, "Error, no filename passed with mkdir command\n");
            break;
        }

        /* TODO: Should check that length is within a valid range */

        ret = euid_usb_bulk_read(hDev, 0x81, path, LE32(cmd->cmd.extralen), 10000);
        if (ret != LE32(cmd->cmd.extralen)) {
            fprintf(stderr, "Error reading chdir data cmd->extralen %d, ret %d\n", LE32(cmd->cmd.extralen), ret);
            break;
        }

        V_PRINTF(2, "Chdir command name %s\n", path);

        fsnum = LE32(cmd->fsnum);
        if ((fsnum >= 0) && (fsnum < MAX_HOSTDRIVES)) {
            strcpy(g_drives[fsnum].currdir, path);
            resp.res = 0;
        }

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
    } while (0);

    return ret;
}

int handle_ioctl(struct usb_dev_handle *hDev, struct HostFsIoctlCmd *cmd, int cmdlen) {
    static char inbuf[64 * 1024];
    static char outbuf[64 * 1024];
    int inlen;
    struct HostFsIoctlResp resp;
    int ret = -1;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_IOCTL);
    resp.res = LE32(-1);

    do {
        if (cmdlen != sizeof(struct HostFsIoctlCmd)) {
            fprintf(stderr, "Error, invalid ioctl command size %d\n", cmdlen);
            break;
        }

        inlen = LE32(cmd->cmd.extralen);
        if (inlen > 0) {
            /* TODO: Should check that length is within a valid range */

            ret = euid_usb_bulk_read(hDev, 0x81, inbuf, inlen, 10000);
            if (ret != inlen) {
                fprintf(stderr, "Error reading ioctl data cmd->extralen %d, ret %d\n", inlen, ret);
                break;
            }
        }

        V_PRINTF(2, "Ioctl command fid %d, cmdno %d, inlen %d\n", LE32(cmd->fid), LE32(cmd->cmdno), inlen);

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
        if (ret < 0) {
            fprintf(stderr, "Error writing ioctl response (%d)\n", ret);
            break;
        }

        if (LE32(resp.cmd.extralen) > 0) {
            ret = euid_usb_bulk_write(hDev, 0x2, (char *) outbuf, LE32(resp.cmd.extralen), 10000);
        }
    } while (0);

    return ret;
}

int get_drive_info(SceIoDevInfo *info, unsigned int drive) {
    int ret = -1;

    if (drive >= MAX_HOSTDRIVES) {
        fprintf(stderr, "Host drive number is too large (%d)\n", drive);
        return -1;
    }

    if (pthread_mutex_lock(&g_drivemtx)) {
        fprintf(stderr, "Could not lock mutex (%s)\n", strerror(errno));
        return -1;
    }

    do {
#ifdef __CYGWIN__
        struct statfs st;

        if(statfs(g_drives[drive].rootdir, &st) < 0)
        {
            fprintf(stderr, "Could not stat %s (%s)\n", g_drives[drive].rootdir, strerror(errno));
            break;
        }

        info->cluster_size = (uint32_t) (st.f_frsize);
        info->max_size = st.f_blocks * info->cluster_size;
        info->free_size = st.f_bfree * info->cluster_size;
        info->unk = 0;
#else
        struct statvfs st;

        if (statvfs(g_drives[drive].rootdir, &st) < 0) {
            fprintf(stderr, "Could not stat %s (%s)\n", g_drives[drive].rootdir, strerror(errno));
            break;
        }

        info->cluster_size = (uint32_t) (st.f_frsize);
        info->max_size = st.f_blocks * info->cluster_size;
        info->free_size = st.f_bfree * info->cluster_size;
        info->unk = 0;
#endif

        ret = 0;
    } while (0);

    pthread_mutex_unlock(&g_drivemtx);

    return ret;
}

int handle_devctl(struct usb_dev_handle *hDev, struct HostFsDevctlCmd *cmd, int cmdlen) {
    static char inbuf[64 * 1024];
    static char outbuf[64 * 1024];
    int inlen;
    struct HostFsDevctlResp resp;
    int ret = -1;
    unsigned int cmdno;

    memset(&resp, 0, sizeof(resp));
    resp.cmd.magic = LE32(HOSTFS_MAGIC);
    resp.cmd.command = LE32(HOSTFS_CMD_DEVCTL);
    resp.res = LE32(-1);

    do {

        if (cmdlen != sizeof(struct HostFsDevctlCmd)) {
            fprintf(stderr, "Error, invalid devctl command size %d\n", cmdlen);
            break;
        }

        inlen = LE32(cmd->cmd.extralen);
        if (inlen > 0) {
            /* TODO: Should check that length is within a valid range */

            ret = euid_usb_bulk_read(hDev, 0x81, inbuf, inlen, 10000);
            if (ret != inlen) {
                fprintf(stderr, "Error reading devctl data cmd->extralen %d, ret %d\n", inlen, ret);
                break;
            }
        }

        cmdno = LE32(cmd->cmdno);
        V_PRINTF(2, "Devctl command cmdno %d, inlen %d\n", cmdno, inlen);

        switch (cmdno) {
            case DEVCTL_GET_INFO:
                resp.res = LE32(get_drive_info((SceIoDevInfo *) outbuf, LE32(cmd->fsnum)));
                if (LE32(resp.res) == 0) {
                    resp.cmd.extralen = LE32(sizeof(SceIoDevInfo));
                }
                break;
            default:
                break;
        };

        ret = euid_usb_bulk_write(hDev, 0x2, (char *) &resp, sizeof(resp), 10000);
        if (ret < 0) {
            fprintf(stderr, "Error writing devctl response (%d)\n", ret);
            break;
        }

        if (LE32(resp.cmd.extralen) > 0) {
            ret = euid_usb_bulk_write(hDev, 0x2, (char *) outbuf, LE32(resp.cmd.extralen), 10000);
        }
    } while (0);

    return ret;
}

int init_hostfs(void) {
    int i;

    memset(open_files, 0, sizeof(open_files));
    memset(open_dirs, 0, sizeof(open_dirs));
    for (i = 0; i < MAX_HOSTDRIVES; i++) {
        strcpy(g_drives[i].currdir, "/");
    }

    return 0;
}

void close_hostfs(void) {
    int i;

    for (i = 3; i < MAX_FILES; i++) {
        if (open_files[i].opened) {
            close(i);
            open_files[i].opened = 0;
            if (open_files[i].name) {
                free(open_files[i].name);
                open_files[i].name = NULL;
            }
        }
    }

    for (i = 0; i < MAX_DIRS; i++) {
        if (open_dirs[i].opened) {
            dir_close(i);
        }
    }
}
