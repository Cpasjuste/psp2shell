/*
 * PSPLINK
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPLINK root for details.
 *
 * main.c - Main code for PC side of USB HostFS
 *
 * Copyright (c) 2006 James F <tyranid@gmail.com>
 *
 * $HeadURL: svn://svn.pspdev.org/psp/branches/psplinkusb/usbhostfs_pc/main.c $
 * $Id: main.c 2177 2007-02-15 17:28:02Z tyranid $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <usb.h>
#include <fcntl.h>
#include <usbhostfs.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#ifdef __CYGWIN__
#include <sys/vfs.h>
#define NO_UID_CHECK
/* Define out set* and get* calls for cygwin as they are unnecessary and can cause issues */
#define seteuid(x)
#define setegid(x)
#define getuid()
#define getgid()
#else

#include <sys/statvfs.h>

#endif

#ifdef READLINE_SHELL

#include <readline/readline.h>
#include <readline/history.h>
#include "p2s_msg.h"

#endif

#include "psp_fileio.h"
#include "hostfs.h"

#define BASE_PORT 10000

#ifdef __CYGWIN__
#define USB_TIMEOUT 1000
#else
#define USB_TIMEOUT 0
#endif

#ifndef SOL_TCP
#define SOL_TCP getprotobyname("TCP")->p_proto
#endif

int psp2sell_cli_init();

int msg_parse(P2S_MSG *msg);

//static usb_dev_handle *g_hDev = NULL;
usb_dev_handle *g_hDev = NULL;

static int g_servsocks[MAX_ASYNC_CHANNELS];
static int g_clientsocks[MAX_ASYNC_CHANNELS];
static const char *g_mapfile = NULL;

int g_verbose = 0;
int g_gdbdebug = 0;
int g_pid = HOSTFSDRIVER_PID;
int g_timeout = USB_TIMEOUT;
int g_globalbind = 0;
int g_daemon = 0;
unsigned short g_baseport = BASE_PORT;

void print_gdbdebug(int dir, const uint8_t *data, int len) {
    int i;

    if (dir) {
        printf("HOST->GDB (");
    } else {
        printf("GDB->HOST (");
    }

    for (i = 0; i < len; i++) {
        if (data[i] >= 32) {
            putchar(data[i]);
        } else {
            printf("\\%02x", data[i]);
        }
    }

    printf(")\n");
}

/* Define wrappers for the usb functions we use which can set euid */
int euid_usb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int size,
                        int timeout) {
    int ret;

    V_PRINTF(2, "Bulk Write dev %p, ep 0x%x, bytes %p, size %d, timeout %d\n",
             dev, ep, bytes, size, timeout);

    //V_PRINTF(2, "Last usb error: %s\n", usb_strerror());

    seteuid(0);
    setegid(0);
    ret = usb_bulk_write(dev, ep, bytes, size, timeout);
    seteuid(getuid());
    setegid(getgid());

    V_PRINTF(2, "Bulk Write returned %d\n", ret);
    //V_PRINTF(2, "Last usb error: %s\n", usb_strerror());

    return ret;
}

int euid_usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size,
                       int timeout) {
    int ret;

    V_PRINTF(2, "Bulk Read dev %p, ep 0x%x, bytes %p, size %d, timeout %d\n",
             dev, ep, bytes, size, timeout);
    seteuid(0);
    setegid(0);
    ret = usb_bulk_read(dev, ep, bytes, size, timeout);
    seteuid(getuid());
    setegid(getgid());

    V_PRINTF(2, "Bulk Read returned %d\n", ret);

    return ret;
}

usb_dev_handle *open_device(struct usb_bus *busses) {
    struct usb_bus *bus = NULL;
    struct usb_dev_handle *hDev = NULL;

    seteuid(0);
    setegid(0);

    for (bus = busses; bus; bus = bus->next) {
        struct usb_device *dev;

        for (dev = bus->devices; dev; dev = dev->next) {
            if ((dev->descriptor.idVendor == SONY_VID)
                && (dev->descriptor.idProduct == g_pid)) {
                hDev = usb_open(dev);
                if (hDev != NULL) {
                    int ret;
                    ret = usb_set_configuration(hDev, 1);
#ifndef NO_UID_CHECK
                    if ((ret < 0) && (errno == EPERM) && geteuid()) {
                        fprintf(stderr,
                                "Permission error while opening the USB device.\n"
                                        "Fix device permissions or run as root.\n");
                        usb_close(hDev);
                        exit(1);
                    }
#endif
                    if (ret == 0) {
                        ret = usb_claim_interface(hDev, 0);
                        if (ret == 0) {
                            seteuid(getuid());
                            setegid(getgid());
                            return hDev;
                        } else {
                            usb_close(hDev);
                            hDev = NULL;
                        }
                    } else {
                        usb_close(hDev);
                        hDev = NULL;
                    }
                }
            }
        }
    }

    if (hDev) {
        usb_close(hDev);
    }

    seteuid(getuid());
    setegid(getgid());

    return NULL;
}

void close_device(struct usb_dev_handle *hDev) {
    seteuid(0);
    setegid(0);
    if (hDev) {
        usb_release_interface(hDev, 0);
        usb_reset(hDev);
        usb_close(hDev);
    }
    seteuid(getuid());
    setegid(getgid());
}

usb_dev_handle *wait_for_device(void) {
    usb_dev_handle *hDev = NULL;

    while (hDev == NULL) {
        usb_find_busses();
        usb_find_devices();

        hDev = open_device(usb_get_busses());
        if (hDev) {
            fprintf(stderr, "Connected to device\n");
            rl_refresh_line(0, 0);
            break;
        }

        /* Sleep for one second */
        sleep(1);
    }

    return hDev;
}

void do_hostfs(struct HostFsCmd *cmd, int readlen) {
    V_PRINTF(2, "Magic: %08X\n", LE32(cmd->magic));
    V_PRINTF(2, "Command Num: %08X\n", LE32(cmd->command));
    V_PRINTF(2, "Extra Len: %d\n", LE32(cmd->extralen));

    switch (LE32(cmd->command)) {
        case HOSTFS_CMD_HELLO:
            if (handle_hello(g_hDev) < 0) {
                fprintf(stderr, "Error sending hello response\n");
            }
            break;
        case HOSTFS_CMD_OPEN:
            if (handle_open(g_hDev, (struct HostFsOpenCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in open command\n");
            }
            break;
        case HOSTFS_CMD_CLOSE:
            if (handle_close(g_hDev, (struct HostFsCloseCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in close command\n");
            }
            break;
        case HOSTFS_CMD_WRITE:
            if (handle_write(g_hDev, (struct HostFsWriteCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in write command\n");
            }
            break;
        case HOSTFS_CMD_READ:
            if (handle_read(g_hDev, (struct HostFsReadCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in read command\n");
            }
            break;
        case HOSTFS_CMD_LSEEK:
            if (handle_lseek(g_hDev, (struct HostFsLseekCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in lseek command\n");
            }
            break;
        case HOSTFS_CMD_DOPEN:
            if (handle_dopen(g_hDev, (struct HostFsDopenCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in dopen command\n");
            }
            break;
        case HOSTFS_CMD_DCLOSE:
            if (handle_dclose(g_hDev, (struct HostFsDcloseCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in dclose command\n");
            }
            break;
        case HOSTFS_CMD_DREAD:
            if (handle_dread(g_hDev, (struct HostFsDreadCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in dread command\n");
            }
            break;
        case HOSTFS_CMD_REMOVE:
            if (handle_remove(g_hDev, (struct HostFsRemoveCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in remove command\n");
            }
            break;
        case HOSTFS_CMD_RMDIR:
            if (handle_rmdir(g_hDev, (struct HostFsRmdirCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in rmdir command\n");
            }
            break;
        case HOSTFS_CMD_MKDIR:
            if (handle_mkdir(g_hDev, (struct HostFsMkdirCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in mkdir command\n");
            }
            break;
        case HOSTFS_CMD_CHDIR:
            if (handle_chdir(g_hDev, (struct HostFsChdirCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in chdir command\n");
            }
            break;
        case HOSTFS_CMD_RENAME:
            if (handle_rename(g_hDev, (struct HostFsRenameCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in rename command\n");
            }
            break;
        case HOSTFS_CMD_GETSTAT:
            if (handle_getstat(g_hDev, (struct HostFsGetstatCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in getstat command\n");
            }
            break;
        case HOSTFS_CMD_GETSTATBYFD:
            if (handle_getstatbyfd(g_hDev, (struct HostFsGetstatByFdCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in getstatbyfd command\n");
            }
            break;
        case HOSTFS_CMD_CHSTAT:
            if (handle_chstat(g_hDev, (struct HostFsChstatCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in chstat command\n");
            }
            break;
        case HOSTFS_CMD_IOCTL:
            if (handle_ioctl(g_hDev, (struct HostFsIoctlCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in ioctl command\n");
            }
            break;
        case HOSTFS_CMD_DEVCTL:
            if (handle_devctl(g_hDev, (struct HostFsDevctlCmd *) cmd, readlen) < 0) {
                fprintf(stderr, "Error in devctl command\n");
            }
            break;
        default:
            fprintf(stderr, "Error, unknown command %08X\n", cmd->command);
            break;
    };
}

void do_async(struct AsyncCommand *cmd, int readlen) {

    uint8_t *data;
    unsigned long data_len = readlen - sizeof(struct AsyncCommand);
    V_PRINTF(2, "Async Magic: %08X\n", LE32(cmd->magic));
    V_PRINTF(2, "Async Channel: %08X\n", LE32(cmd->channel));
    V_PRINTF(2, "Async Extra Len: %lu\n", data_len);

    if (readlen > sizeof(struct AsyncCommand)) {
        data = (uint8_t *) cmd + sizeof(struct AsyncCommand);
        unsigned int chan = LE32(cmd->channel);

        if (chan == ASYNC_SHELL) {

            P2S_MSG msg;
            int res = p2s_msg_to_msg_advanced(&msg, (const char *) data, data_len - 2);
            //printf("\nlen=%i, data=`%s`\n", (int) (data_len - 2), msg.buffer);
            if (res >= 0) {
                msg_parse(&msg);
            }

        } else if (chan == ASYNC_STDOUT || chan == ASYNC_STDERR || chan == ASYNC_GDB) {
            char msg[data_len + 1];
            strncpy(msg, (const char *) data, data_len);
            msg[data_len] = '\0';
            printf("%s", msg);
        }

        // TODO: add file transfer ? (should be handled by hostfs)
        /*
        if ((chan < MAX_ASYNC_CHANNELS) && (g_clientsocks[chan] >= 0)) {
            write(g_clientsocks[chan], data, readlen - sizeof(struct AsyncCommand));
            if ((chan == ASYNC_GDB) && (g_gdbdebug)) {
                print_gdbdebug(0, data, readlen - sizeof(struct AsyncCommand));
            }
        }
        */
    }

}

void do_bulk(struct BulkCommand *cmd, int readlen) {
    static char block[HOSTFS_BULK_MAXWRITE];
    int read = 0;
    int len = 0;
    unsigned int chan = 0;
    int ret = -1;

    chan = LE32(cmd->channel);
    len = LE32(cmd->size);

    V_PRINTF(2, "Bulk write command length: %d channel %d\n", len, chan);

    while (read < len) {
        int readsize;

        readsize = (len - read) > HOSTFS_MAX_BLOCK ? HOSTFS_MAX_BLOCK : (len - read);
        ret = euid_usb_bulk_read(g_hDev, 0x81, &block[read], readsize, 10000);
        if (ret != readsize) {
            fprintf(stderr, "Error reading write data readsize %d, ret %d\n", readsize, ret);
            break;
        }
        read += readsize;
    }

    if (read >= len) {
        if ((chan < MAX_ASYNC_CHANNELS) && (g_clientsocks[chan] >= 0)) {
            fixed_write(g_clientsocks[chan], block, len);
        }
    }
}

int start_hostfs(void) {

    uint32_t data[512 / sizeof(uint32_t)];
    int readlen;
    int quit = 0;

    while (!quit) {

        init_hostfs();

        g_hDev = wait_for_device();

        if (g_hDev) {

            uint32_t magic;

            magic = LE32(HOSTFS_MAGIC);

            if (euid_usb_bulk_write(g_hDev, 0x2, (char *) &magic, sizeof(magic), 1000) == sizeof(magic)) {
                while (1) {
                    readlen = euid_usb_bulk_read(g_hDev, 0x81, (char *) data, 512, g_timeout);
                    if (readlen == 0) {
                        fprintf(stderr, "Read cancelled (remote disconnected)\n");
                        rl_refresh_line(0, 0);
                        break;
                    } else if (readlen == -ETIMEDOUT) {
                        continue;
                    } else if (readlen < 0) {
                        fprintf(stderr, "Read cancelled (readlen < 0)\n");
                        rl_refresh_line(0, 0);
                        break;
                    }

                    if (readlen < sizeof(uint32_t)) {
                        fprintf(stderr, "Error could not read magic\n");
                        rl_refresh_line(0, 0);
                        break;
                    }

                    if (LE32(data[0]) == HOSTFS_MAGIC) {
                        if (readlen < sizeof(struct HostFsCmd)) {
                            fprintf(stderr, "Error reading command header %d\n", readlen);
                            rl_refresh_line(0, 0);
                            break;
                        }
                        do_hostfs((struct HostFsCmd *) data, readlen);
                    } else if (LE32(data[0]) == ASYNC_MAGIC) {
                        if (readlen < sizeof(struct AsyncCommand)) {
                            fprintf(stderr, "Error reading async header %d\n", readlen);
                            rl_refresh_line(0, 0);
                            break;
                        }
                        do_async((struct AsyncCommand *) data, readlen);
                    } else if (LE32(data[0]) == BULK_MAGIC) {
                        if (readlen < sizeof(struct BulkCommand)) {
                            fprintf(stderr, "Error reading bulk header %d\n", readlen);
                            rl_refresh_line(0, 0);
                            break;
                        }
                        do_bulk((struct BulkCommand *) data, readlen);
                    } else {
                        fprintf(stderr, "Error, invalid magic %08X\n", LE32(data[0]));
                        rl_refresh_line(0, 0);
                    }
                }
            }

            close_device(g_hDev);
            g_hDev = NULL;
        }

        close_hostfs();
    }

    return 0;
}

int parse_args(int argc, char **argv) {
    int i;

    if (getcwd(g_rootdir, PATH_MAX) < 0) {
        fprintf(stderr, "Could not get current path\n");
        return 0;
    }

    for (i = 0; i < MAX_HOSTDRIVES; i++) {
        strcpy(g_drives[i].rootdir, g_rootdir);
    }

    // TODO: REMOVE
    strcpy(g_drives[0].rootdir, "/home/cpasjuste/dev/psvita/psp2shell/cmake-build-debug/psp2shell_k/");

    while (1) {
        int ch;

        ch = getopt(argc, argv, "vghndcmb:p:f:t:");
        if (ch == -1) {
            break;
        }

        switch (ch) {
            case 'v':
                g_verbose++;
                break;
            case 'b':
                g_baseport = atoi(optarg);
                break;
            case 'g':
                g_globalbind = 1;
                break;
            case 'p':
                g_pid = strtoul(optarg, NULL, 0);
                break;
            case 'd':
                g_gdbdebug = 1;
                break;
            case 'c':
                g_nocase = 1;
                break;
            case 'm':
                g_msslash = 1;
                break;
            case 'f':
                g_mapfile = optarg;
                break;
            case 't':
                g_timeout = atoi(optarg);
                break;
            case 'n':
                g_daemon = 1;
                break;
            case 'h':
                return 0;
            default:
                printf("Unknown option\n");
                return 0;
        };
    }

    argc -= optind;
    argv += optind;

    if (argc > 0) {
        if (argc > MAX_HOSTDRIVES) {
            argc = MAX_HOSTDRIVES;
        }

        for (i = 0; i < argc; i++) {
            if (argv[i][0] != '/') {
                char tmpdir[PATH_MAX];
                snprintf(tmpdir, PATH_MAX, "%s/%s", g_rootdir, argv[i]);
                strcpy(g_drives[i].rootdir, tmpdir);
            } else {
                strcpy(g_drives[i].rootdir, argv[i]);
            }
            gen_path(g_drives[i].rootdir, 0);
            V_PRINTF(2, "Root %d: %s\n", i, g_drives[i].rootdir);
        }
    } else {
        V_PRINTF(2, "Root directory: %s\n", g_rootdir);
    }

    return 1;
}

void print_help(void) {
    fprintf(stderr, "Usage: usbhostfs_pc [options] [rootdir0..rootdir%d]\n", MAX_HOSTDRIVES - 1);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "-v                : Set verbose mode\n");
    fprintf(stderr, "-vv               : More verbose\n");
    fprintf(stderr, "-b port           : Specify the base async port (default %d)\n", BASE_PORT);
    fprintf(stderr, "-g                : Specify global bind for the sockets, as opposed to just localhost\n");
    fprintf(stderr, "-p pid            : Specify the product ID of the PSP device\n");
    fprintf(stderr, "-d                : Print GDB transfers\n");
    fprintf(stderr, "-f filename       : Load the host drive mappings from a file\n");
    fprintf(stderr, "-c                : Enable case-insensitive filenames\n");
    fprintf(stderr, "-m                : Convert backslashes to forward slashes\n");
    fprintf(stderr, "-t timeout        : Specify the USB timeout (default %d)\n", USB_TIMEOUT);
    fprintf(stderr, "-n                : Daemon mode, the shell is accessed through pcterm\n");
    fprintf(stderr, "-h                : Print this help\n");
}

void shutdown_socket(void) {
    int i;

    for (i = 0; i < MAX_ASYNC_CHANNELS; i++) {
        if (g_clientsocks[i] >= 0) {
            close(g_clientsocks[i]);
            g_clientsocks[i] = -1;
        }

        if (g_servsocks[i] >= 0) {
            close(g_servsocks[i]);
            g_servsocks[i] = -1;
        }
    }
}

int exit_app(void) {
    printf("Exiting\n");
    shutdown_socket();
    if (g_hDev) {
        /* Nuke the connection */
        seteuid(0);
        setegid(0);
        close_device(g_hDev);
    }
    //exit(1);
    return 0;
}

void signal_handler(int sig) {
    exit_app();
}

int make_socket(unsigned short port) {
    int sock;
    int on = 1;
    struct sockaddr_in name;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    if (g_globalbind) {
        name.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }

    if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }

    if (listen(sock, 1) < 0) {
        perror("listen");
        close(sock);
        return -1;
    }

    return sock;
}

int add_drive(int num, const char *dir) {
    char path[PATH_MAX];
    DIR *pDir;

    if ((num < 0) || (num >= MAX_HOSTDRIVES)) {
        printf("Invalid host driver number '%d'\n", num);
        return 0;
    }

    /* Make path */
    if (dir[0] != '/') {
        snprintf(path, PATH_MAX, "%s/%s", g_rootdir, dir);
    } else {
        strcpy(path, dir);
    }
    gen_path(path, 0);

    pDir = opendir(path);
    if (pDir) {
        closedir(pDir);
        if (pthread_mutex_lock(&g_drivemtx)) {
            printf("Couldn't lock mutex\n");
            return 0;
        }

        strcpy(g_drives[num].rootdir, path);
        strcpy(g_drives[num].currdir, "/");

        pthread_mutex_unlock(&g_drivemtx);
    } else {
        printf("Invalid directory '%s'\n", path);
        return 0;
    }

    return 1;
}

#define COMMAND_OK   0
#define COMMAND_ERR  1
#define COMMAND_HELP 2

struct ShellCmd {
    const char *name;
    const char *help;

    int (*fn)(void);
};

int nocase_set(void) {
    char *set;

    set = strtok(NULL, " \t");
    if (set) {
        if (strcmp(set, "on") == 0) {
            g_nocase = 1;
        } else if (strcmp(set, "off") == 0) {
            g_nocase = 0;
        } else {
            printf("Error setting nocase, invalid option '%s'\n", set);
        }
    } else {
        printf("nocase: %s\n", g_nocase ? "on" : "off");
    }

    return COMMAND_OK;
}

int msslash_set(void) {
    char *set;

    set = strtok(NULL, " \t");
    if (set) {
        if (strcmp(set, "on") == 0) {
            g_msslash = 1;
        } else if (strcmp(set, "off") == 0) {
            g_msslash = 0;
        } else {
            printf("Error setting msslash, invalid option '%s'\n", set);
        }
    } else {
        printf("msslash: %s\n", g_msslash ? "on" : "off");
    }

    return COMMAND_OK;
}

int gdbdebug_set(void) {
    char *set;

    set = strtok(NULL, " \t");
    if (set) {
        if (strcmp(set, "on") == 0) {
            g_gdbdebug = 1;
        } else if (strcmp(set, "off") == 0) {
            g_gdbdebug = 0;
        } else {
            printf("Error setting nocase, invalid option '%s'\n", set);
        }
    } else {
        printf("gdbdebug: %s\n", g_gdbdebug ? "on" : "off");
    }

    return COMMAND_OK;
}

int verbose_set(void) {
    char *set;

    set = strtok(NULL, " \t");
    if (set) {
        g_verbose = atoi(set);
    } else {
        printf("verbose: %d\n", g_verbose);
    }

    return COMMAND_OK;
}

int list_drives(void) {
    int i;

    for (i = 0; i < MAX_HOSTDRIVES; i++) {
        printf("host%d: %s\n", i, g_drives[i].rootdir);
    }

    return COMMAND_OK;
}

int mount_drive(void) {
    char *num;
    char *dir;
    int val;
    char *endp;

    num = strtok(NULL, " \t");
    dir = strtok(NULL, "");

    if ((!num) || (!dir)) {
        printf("Must specify a drive number and a directory\n");
        return COMMAND_ERR;
    }

    val = strtoul(num, &endp, 10);
    if (*endp) {
        printf("Invalid host driver number '%s'\n", num);
        return COMMAND_ERR;
    }

    if (!add_drive(val, dir)) {
        return COMMAND_ERR;
    }

    return COMMAND_OK;
}

void load_mapfile(const char *mapfile) {
    char path[PATH_MAX];
    FILE *fp;
    int line = 0;

    fp = fopen(mapfile, "r");
    if (fp == NULL) {
        printf("Couldn't open mapfile '%s'\n", g_mapfile);
        return;
    }

    while (fgets(path, PATH_MAX, fp)) {
        char *buf = path;
        int len;
        int num;

        line++;
        /* Remove whitespace */
        len = strlen(buf);
        while ((len > 0) && (isspace(buf[len - 1]))) {
            buf[len - 1] = 0;
            len--;
        }

        while (isspace(*buf)) {
            buf++;
            len--;
        }

        if (!isdigit(*buf)) {
            printf("Line %d: Entry does not start with the host number\n", line);
            continue;
        }

        if (len > 0) {
            char *endp;
            num = strtoul(buf, &endp, 10);
            if ((*endp != '=') || (*(endp + 1) == 0) || (isspace(*(endp + 1)))) {
                printf("Line %d: Entry is not of the form 'num=path'\n", line);
                continue;
            }

            endp++;

            add_drive(num, endp);
        }
    }

    fclose(fp);
}

int load_drives(void) {
    char *mapfile;

    mapfile = strtok(NULL, "");
    if (mapfile == NULL) {
        printf("Must specify a filename\n");
        return COMMAND_ERR;
    }

    load_mapfile(mapfile);

    return COMMAND_OK;
}

int save_drives(void) {
    char *mapfile;
    FILE *fp;
    int i;

    mapfile = strtok(NULL, "");
    if (mapfile == NULL) {
        printf("Must specify a filename\n");
        return COMMAND_ERR;
    }

    fp = fopen(mapfile, "w");
    if (fp == NULL) {
        printf("Couldn't open file '%s'\n", mapfile);
        return COMMAND_ERR;
    }

    for (i = 0; i < MAX_HOSTDRIVES; i++) {
        fprintf(fp, "%d=%s\n", i, g_drives[i].rootdir);
    }

    fclose(fp);

    return COMMAND_OK;
}

int print_wd(void) {
    printf("%s\n", g_rootdir);

    return COMMAND_OK;
}

int ch_dir(void) {
    char *dir;

    dir = strtok(NULL, "");
    if (dir == NULL) {
        printf("Must specify a directory\n");
        return COMMAND_ERR;
    }

    if (chdir(dir) == 0) {
        getcwd(g_rootdir, PATH_MAX);
    } else {
        printf("chdir: %s", strerror(errno));
    }

    return COMMAND_OK;
}

int help_cmd(void) {
    return COMMAND_HELP;
}

struct ShellCmd g_commands[] = {
        {"drives",   "Print the current drives",                          list_drives},
        {"mount",    "Mount a directory (mount num dir)",                 mount_drive},
        {"save",     "Save the list of mounts to a file (save filename)", save_drives},
        {"load",     "Load a list of mounts from a file (load filename)", load_drives},
        {"nocase",   "Set case sensitivity (nocase on|off)",              nocase_set},
        {"msslash",  "Convert backslash to forward slash in filename",    msslash_set},
        {"gdbdebug", "Set the GDB debug option (gdbdebug on|off)",        gdbdebug_set},
        {"verbose",  "Set the verbose level (verbose 0|1|2)",             verbose_set},
        {"pwd",      "Print the current directory",                       print_wd},
        {"cd",       "Change the current local directory",                ch_dir},
        {"help",     "Print this help",                                   help_cmd},
        {"exit",     "Exit the application",                              exit_app},
};

void parse_shell(char *buf) {
    int len;

    if (buf == NULL) {
        exit_app();
    }

    if ((buf) && (*buf)) {
#ifdef READLINE_SHELL
        add_history(buf);
#endif
        /* Remove whitespace */
        len = strlen(buf);
        while ((len > 0) && (isspace(buf[len - 1]))) {
            buf[len - 1] = 0;
            len--;
        }

        while (isspace(*buf)) {
            buf++;
            len--;
        }

        if (len > 0) {
            if (*buf == '!') {
                system(buf + 1);
            } else {
                const char *cmd;
                int i;
                int ret = COMMAND_HELP;

                cmd = strtok(buf, " \t");
                for (i = 0; i < (sizeof(g_commands) / sizeof(struct ShellCmd)); i++) {
                    if (strcmp(cmd, g_commands[i].name) == 0) {
                        if (g_commands[i].fn) {
                            ret = g_commands[i].fn();
                        }
                        break;
                    }
                }

                if (ret == COMMAND_HELP) {
                    int i;

                    printf("-= Help =-\n");
                    for (i = 0; i < (sizeof(g_commands) / sizeof(struct ShellCmd)); i++) {
                        printf("%-10s: %s\n", g_commands[i].name, g_commands[i].help);
                    }
                }
            }
        }
    }
}

#ifdef READLINE_SHELL

int init_readline(void) {
    rl_callback_handler_install("sh> ", parse_shell);

    return 1;
}

#endif

extern int readline_callback;

void *async_thread(void *arg) {

    char buf[512];
    char *data;
    struct AsyncCommand *cmd;
    fd_set read_set, read_save;
    struct sockaddr_in client;
    socklen_t size;
    int max_fd = 0;
    int flag = 1;
    int i;
    int quit = 0;

    FD_ZERO(&read_save);

    if (!g_daemon) {
#ifdef READLINE_SHELL
        //init_readline();
        psp2sell_cli_init();
#endif
        FD_SET(STDIN_FILENO, &read_save);
        max_fd = STDIN_FILENO;
    }

    for (i = 0; i < MAX_ASYNC_CHANNELS; i++) {
        if (g_servsocks[i] >= 0) {
            FD_SET(g_servsocks[i], &read_save);
            if (g_servsocks[i] > max_fd) {
                max_fd = g_servsocks[i];
            }
        }
    }

    cmd = (struct AsyncCommand *) buf;
    cmd->magic = LE32(ASYNC_MAGIC);
    data = buf + sizeof(struct AsyncCommand);

    while (!quit) {

        FD_ZERO(&read_save);
        FD_SET(fileno(stdin), &read_save);

        if (select(fileno(stdin) + 1, &read_save, NULL, NULL, NULL) < 0) {
            //continue (CTRL+C)
        } else if (FD_ISSET(fileno(stdin), &read_save) && readline_callback) {
            rl_callback_read_char();
        }

        /*
        read_set = read_save;
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) > 0) {
            if (!g_daemon) {
                if (FD_ISSET(STDIN_FILENO, &read_set)) {
#ifdef READLINE_SHELL
                    rl_callback_read_char();
#else
                    char buffer[4096];

                    if (fgets(buffer, sizeof(buffer), stdin)) {
                        parse_shell(buffer);
                    }
#endif
                }
            }
            */

        /*
        for (i = 0; i < MAX_ASYNC_CHANNELS; i++) {
            if (g_servsocks[i] >= 0) {
                if (FD_ISSET(g_servsocks[i], &read_set)) {
                    if (g_clientsocks[i] >= 0) {
                        FD_CLR(g_clientsocks[i], &read_save);
                        close(g_clientsocks[i]);
                    }
                    size = sizeof(client);
                    g_clientsocks[i] = accept(g_servsocks[i], (struct sockaddr *) &client, &size);
                    if (g_clientsocks[i] >= 0) {
                        printf("Accepting async connection (%d) from %s\n", i, inet_ntoa(client.sin_addr));
                        FD_SET(g_clientsocks[i], &read_save);
                        if ((g_daemon) && (i == ASYNC_SHELL)) {
                            // Duplicate to stdout for the local shell
                            dup2(g_clientsocks[i], 1);
                        }
                        setsockopt(g_clientsocks[i], SOL_TCP, TCP_NODELAY, &flag, sizeof(int));
                        if (g_clientsocks[i] > max_fd) {
                            max_fd = g_clientsocks[i];
                        }
                    }
                }
            }
        }

        for (i = 0; i < MAX_ASYNC_CHANNELS; i++) {
            if (g_clientsocks[i] >= 0) {
                if (FD_ISSET(g_clientsocks[i], &read_set)) {
                    int readbytes;

                    printf("async_thread: read...\n");
                    readbytes = read(g_clientsocks[i], data, sizeof(buf) - sizeof(struct AsyncCommand));
                    if (readbytes > 0) {
                        printf("async_thread: read: chan=%i, len=%i, data=%s\n", i, readbytes, data);
                        if ((i == ASYNC_GDB) && (g_gdbdebug)) {
                            print_gdbdebug(1, (uint8_t *) data, readbytes);
                        }

                        if (g_daemon) {
                            if ((i == ASYNC_SHELL) && (data[0] == '@')) {
                                // We assume locally it should be able to load everything in one go
                                if (readbytes < (sizeof(buf) - sizeof(struct AsyncCommand))) {
                                    data[readbytes] = 0;
                                } else {
                                    data[sizeof(buf) - sizeof(struct AsyncCommand) - 1] = 0;
                                }

                                parse_shell(&data[1]);
                                continue;
                            }
                        }

                        if (g_hDev) {
                            cmd->channel = LE32(i);
                            printf("async_thread: euid_usb_bulk_write: chan=%i, len=%i, buf=%s\n", cmd->channel,
                                   (int) (readbytes + sizeof(struct AsyncCommand)), buf);
                            euid_usb_bulk_write(g_hDev, 0x4, buf, readbytes + sizeof(struct AsyncCommand), 10000);
                        }
                    } else {
                        FD_CLR(g_clientsocks[i], &read_save);
                        if ((g_daemon) && (i == ASYNC_SHELL)) {
                            dup2(2, 1);
                        }
                        close(g_clientsocks[i]);
                        g_clientsocks[i] = -1;
                        printf("Closing async connection (%d)\n", i);
                    }
                }
            }
        }
        */
        //}

    }

    return NULL;
}

int main(int argc, char **argv) {
    int i;

    printf("USBHostFS (c) TyRaNiD 2k6\n");
    printf("PSP2SHELL (c) Cpasjuste\n");
    printf("Built %s %s - $Revision: 2368 $\n", __DATE__, __TIME__);

    if (parse_args(argc, argv)) {

        //g_verbose = 2;

        pthread_t thid;
        usb_init();

        //signal(SIGINT, signal_handler);
        //signal(SIGTERM, signal_handler);

        if (g_daemon) {
            pid_t pid = fork();

            if (pid > 0) {
                /* Parent, just exit */
                return 0;
            } else if (pid < 0) {
                /* Error, print and exit */
                perror("fork:");
                return 1;
            }

            /* Child, dup stdio to /dev/null and set process group */
            int fd = open("/dev/null", O_RDWR);
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            setsid();
        }

        if (g_mapfile) {
            load_mapfile(g_mapfile);
        }

        /*
        for (i = 0; i < MAX_ASYNC_CHANNELS; i++) {
            g_servsocks[i] = make_socket(g_baseport + i);
            g_clientsocks[i] = -1;
        }
        */

        //g_verbose = 2;
        pthread_create(&thid, NULL, async_thread, NULL);
        start_hostfs();

    } else {
        print_help();
    }

    return 0;
}
