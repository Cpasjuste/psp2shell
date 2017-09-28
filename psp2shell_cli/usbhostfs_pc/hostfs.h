//
// Created by cpasjuste on 26/06/17.
//

#ifndef _HOSTFS_H_
#define _HOSTFS_H_

#include "psp_fileio.h"

#define MAX_FILES 256
#define MAX_DIRS  256
#define MAX_TOKENS 256
#define MAX_HOSTDRIVES 1

#define GETERROR(x) (0x80010000 | (x))

/* Contains the paths for a single hist drive */
struct HostDrive {
    char rootdir[PATH_MAX];
    char currdir[PATH_MAX];
};

struct FileHandle {
    int opened;
    int mode;
    char *name;
};

struct DirHandle {
    int opened;
    /* Current count of entries left */
    int count;
    /* Current position in the directory entries */
    int pos;
    /* Head of list, each entry will be freed when read */
    SceIoDirent *pDir;
};

typedef struct SceIoDevInfo {
    SceOff max_size;
    SceOff free_size;
    unsigned int cluster_size;
    void *unk;
} SceIoDevInfo;

extern char g_rootdir[PATH_MAX];
extern struct HostDrive g_drives[MAX_HOSTDRIVES];
extern struct FileHandle open_files[MAX_FILES];
extern struct DirHandle open_dirs[MAX_DIRS];
extern int g_nocase;
extern int g_msslash;
extern pthread_mutex_t g_drivemtx;

extern int euid_usb_bulk_write(
        usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);

extern int euid_usb_bulk_read(
        usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);

extern int g_verbose;

#define V_PRINTF(level, fmt, ...) { if(g_verbose >= level) { fprintf(stderr, fmt, ## __VA_ARGS__); } }

#if defined BUILD_BIGENDIAN || defined _BIG_ENDIAN
uint16_t swap16(uint16_t i)
{
    uint8_t *p = (uint8_t *) &i;
    uint16_t ret;

    ret = (p[1] << 8) | p[0];

    return ret;
}

uint32_t swap32(uint32_t i)
{
    uint8_t *p = (uint8_t *) &i;
    uint32_t ret;

    ret = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];

    return ret;
}

uint64_t swap64(uint64_t i)
{
    uint8_t *p = (uint8_t *) &i;
    uint64_t ret;

    ret = (uint64_t) p[0] | ((uint64_t) p[1] << 8) | ((uint64_t) p[2] << 16) | ((uint64_t) p[3] << 24)
        | ((uint64_t) p[4] << 32) | ((uint64_t) p[5] << 40) | ((uint64_t) p[6] << 48) | ((uint64_t) p[7] << 56);

    return ret;
}
#define LE16(x) swap16(x)
#define LE32(x) swap32(x)
#define LE64(x) swap64(x)
#else
#define LE16(x) (x)
#define LE32(x) (x)
#define LE64(x) (x)
#endif

int init_hostfs(void);

void close_hostfs(void);

int gen_path(char *path, int dir);

int handle_hello(struct usb_dev_handle *hDev);

int handle_open(struct usb_dev_handle *hDev, struct HostFsOpenCmd *cmd, int cmdlen);

int handle_dopen(struct usb_dev_handle *hDev, struct HostFsDopenCmd *cmd, int cmdlen);

int fixed_write(int fd, const void *data, int len);

int handle_write(struct usb_dev_handle *hDev, struct HostFsWriteCmd *cmd, int cmdlen);

int fixed_read(int fd, void *data, int len);

int handle_read(struct usb_dev_handle *hDev, struct HostFsReadCmd *cmd, int cmdlen);

int handle_close(struct usb_dev_handle *hDev, struct HostFsCloseCmd *cmd, int cmdlen);

int handle_dclose(struct usb_dev_handle *hDev, struct HostFsDcloseCmd *cmd, int cmdlen);

int handle_dread(struct usb_dev_handle *hDev, struct HostFsDreadCmd *cmd, int cmdlen);

int handle_lseek(struct usb_dev_handle *hDev, struct HostFsLseekCmd *cmd, int cmdlen);

int handle_remove(struct usb_dev_handle *hDev, struct HostFsRemoveCmd *cmd, int cmdlen);

int handle_rmdir(struct usb_dev_handle *hDev, struct HostFsRmdirCmd *cmd, int cmdlen);

int handle_mkdir(struct usb_dev_handle *hDev, struct HostFsMkdirCmd *cmd, int cmdlen);

int handle_getstat(struct usb_dev_handle *hDev, struct HostFsGetstatCmd *cmd, int cmdlen);

int handle_getstatbyfd(struct usb_dev_handle *hDev, struct HostFsGetstatByFdCmd *cmd, int cmdlen);

int handle_chstat(struct usb_dev_handle *hDev, struct HostFsChstatCmd *cmd, int cmdlen);

int handle_rename(struct usb_dev_handle *hDev, struct HostFsRenameCmd *cmd, int cmdlen);

int handle_chdir(struct usb_dev_handle *hDev, struct HostFsChdirCmd *cmd, int cmdlen);

int handle_ioctl(struct usb_dev_handle *hDev, struct HostFsIoctlCmd *cmd, int cmdlen);

int handle_devctl(struct usb_dev_handle *hDev, struct HostFsDevctlCmd *cmd, int cmdlen);

#endif //_HOSTFS_H_
