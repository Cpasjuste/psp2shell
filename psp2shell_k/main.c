/*
	PSP2SHELL
	Copyright (C) 2016, Cpasjuste

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/io/fcntl.h>
#include <taihen.h>

#include "psp2shell_k.h"

#define CHUNK_SIZE 2048
static uint8_t chunk[CHUNK_SIZE];

#define MAX_HOOKS 32
static SceUID g_hooks[MAX_HOOKS];
static tai_hook_ref_t ref_hooks[MAX_HOOKS];
static int __stdout_fd = 1073807367;

void p2s_cmd_parse(s_client *client, P2S_CMD *cmd);

#if 0
static bool ready = false;

static kp2s_msg kmsg_list[MSG_MAX];

static void kp2s_add_msg(int len, const char *msg) {
    for (int i = 0; i < MSG_MAX; i++) {
        if (kmsg_list[i].len <= 0) {
            memset(kmsg_list[i].msg, 0, P2S_MSG_LEN);
            int new_len = len > P2S_MSG_LEN ? P2S_MSG_LEN : len;
            strncpy(kmsg_list[i].msg, msg, (size_t) new_len);
            kmsg_list[i].len = new_len;
            break;
        }
    }
}
#endif

#if 0
static int ksceDebugPrintf_hook(const char *fmt, ...) {
    /*
    char temp_buf[P2S_MSG_LEN];
    memset(temp_buf, 0, P2S_MSG_LEN);
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(temp_buf, P2S_MSG_LEN, fmt, args);
    va_end(args);
    if (ready) {
        kp2s_add_msg(len, temp_buf);
    }
    */
    va_list args;
    va_start(args, fmt);
    va_end(args);
    psp2shell_print_color(COL_NONE, fmt, args);

    return TAI_CONTINUE(int, ref_hooks[2], fmt, args);
}

static int ksceDebugPrintf2_hook(int num0, int num1, const char *fmt, ...) {
    /*
    char temp_buf[P2S_MSG_LEN];
    memset(temp_buf, 0, P2S_MSG_LEN);
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(temp_buf, P2S_MSG_LEN, fmt, args);
    va_end(args);
    if (ready) {
        kp2s_add_msg(len, temp_buf);
    }
    */

    va_list args;
    va_start(args, fmt);
    va_end(args);
    psp2shell_print_color(COL_NONE, fmt, args);

    return TAI_CONTINUE(int, ref_hooks[3], num0, num1, fmt, args);
}

static int sceIoWrite_hook(SceUID fd, const void *data, SceSize size) {
    if (ref_hooks[0] <= 0) {
        return 0;
    }

    if (fd == __stdout_fd && size < P2S_MSG_LEN) {
        char temp_buf[size];
        memset(temp_buf, 0, size);
        ksceKernelStrncpyUserToKernel(temp_buf, data, size);
        psp2shell_print_color(COL_NONE, temp_buf);
        //kp2s_add_msg((int) size, temp_buf);
    }

    return TAI_CONTINUE(int, ref_hooks[0], fd, data, size);
}

static int sceKernelGetStdout_hook() {
    if (ref_hooks[1] <= 0) {
        return 0;
    }

    int fd = TAI_CONTINUE(int, ref_hooks[1]);
    __stdout_fd = fd;

    return fd;
}
#endif

int kpsp2shell_dump(SceUID pid, const char *filename, void *addr, unsigned int size) {
    SceUID fd;
    fd = ksceIoOpen(filename, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
    if (fd <= 0) {
        return fd;
    }

    size_t i = 0;
    size_t bufsize;

    while (i < size) {
        if (size - i > CHUNK_SIZE) {
            bufsize = CHUNK_SIZE;
        } else {
            bufsize = size - i;
        }
        ksceKernelMemcpyUserToKernelForPid(pid, chunk, addr + i, bufsize);
        i += bufsize;

        ksceIoWrite(fd, chunk, bufsize);
    }

    ksceIoClose(fd);

    return 0;
}

int kpsp2shell_dump_module(SceUID pid, SceUID uid, const char *dst) {
    int ret;
    SceKernelModuleInfo kinfo;
    memset(&kinfo, 0, sizeof(SceKernelModuleInfo));
    kinfo.size = sizeof(SceKernelModuleInfo);

    uint32_t state;
    ENTER_SYSCALL(state);

    SceUID kid = ksceKernelKernelUidForUserUid(pid, uid);
    if (kid < 0) {
        kid = uid;
    }

    ret = ksceKernelGetModuleInfo(pid, kid, &kinfo);
    if (ret >= 0) {
        char path[128];
        for (int i = 0; i < 4; i++) {
            SceKernelSegmentInfo *seginfo = &kinfo.segments[i];

            if (seginfo->memsz <= 0 || seginfo->vaddr == NULL || seginfo->size != sizeof(*seginfo)) {
                continue;
            }

            memset(path, 0, 128);
            ksceKernelStrncpyUserToKernel(path, dst, 128);
            snprintf(path + strlen(path), 128, "/%s_0x%08X_seg%d.bin",
                     kinfo.module_name, (uintptr_t) seginfo->vaddr, i);

            // TODO
            //ksceDebugPrintf_hook("dumping: %s\n", path);
            ret = kpsp2shell_dump(pid, path, seginfo->vaddr, seginfo->memsz);
        }
    }

    EXIT_SYSCALL(state);

    return ret;
}

/*
int kpsp2shell_get_module_info(SceUID pid, SceUID uid, SceKernelModuleInfo *info) {
    int ret;
    SceKernelModuleInfo kinfo;
    memset(&kinfo, 0, sizeof(SceKernelModuleInfo));
    kinfo.size = sizeof(SceKernelModuleInfo);

    uint32_t state;
    ENTER_SYSCALL(state);

    SceUID kid = ksceKernelKernelUidForUserUid(pid, uid);
    if (kid < 0) {
        kid = uid;
    }

    ret = ksceKernelGetModuleInfo(pid, kid, &kinfo);
    if (ret >= 0) {
        ksceKernelMemcpyKernelToUser(info, &kinfo, sizeof(SceKernelModuleInfo));
    }

    EXIT_SYSCALL(state);

    return ret;
}

int kpsp2shell_get_module_list(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num) {
    size_t count = 256;
    SceUID kmodids[256];

    uint32_t state;
    ENTER_SYSCALL(state);

    memset(kmodids, 0, sizeof(SceUID) * 256);
    int res = ksceKernelGetModuleList(pid, flags1, flags2, kmodids, &count);
    if (res >= 0) {
        ksceKernelMemcpyKernelToUser(modids, &kmodids, sizeof(SceUID) * 256);
        ksceKernelMemcpyKernelToUser(num, &count, sizeof(size_t));
    } else {
        ksceKernelMemcpyKernelToUser(num, &count, sizeof(size_t));
    }

    EXIT_SYSCALL(state);

    return res;
}
*/

void set_hooks() {
#if 0
    uint32_t state;
    ENTER_SYSCALL(state);

    g_hooks[0] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[0],
            "SceIofilemgr",
            0xF2FF276E, // SceIofilemgr
            0x34EFD876, // sceIoWrite
            sceIoWrite_hook);
    //LOG("hook: sceIoWrite: 0x%08X\n", g_hooks[0]);

    g_hooks[1] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[1],
            "SceProcessmgr",
            0x2DD91812, // SceProcessmgr
            0xE5AA625C, // sceKernelGetStdout
            sceKernelGetStdout_hook);
    //LOG("hook: sceKernelGetStdout: 0x%08X\n", g_hooks[1]);

    g_hooks[2] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[2],
            "SceSysmem",
            0x88758561, // SceDebugForDriver
            0x391B74B7, // ksceDebugPrintf
            ksceDebugPrintf_hook);
    //LOG("hook: _printf: 0x%08X\n", g_hooks[2]);

    g_hooks[3] = taiHookFunctionExportForKernel(
            KERNEL_PID,
            &ref_hooks[3],
            "SceSysmem",
            0x88758561, // SceDebugForDriver
            0x02B04343, // ksceDebugPrintf2
            ksceDebugPrintf2_hook);
    //LOG("hook: _printf2: 0x%08X\n", g_hooks[3]);

    EXIT_SYSCALL(state);
#endif
}

void delete_hooks() {
#if 0
    for (int i = 0; i < MAX_HOOKS; i++) {
        if (g_hooks[i] >= 0)
            taiHookReleaseForKernel(g_hooks[i], ref_hooks[i]);
    }
#endif
}

#include <psp2kern/kernel/threadmgr/thread.h>
#include <psp2kern/net/net.h>
#include <string.h>
#include "patch.h"
#include "net.h"
#include "utility.h"

int listen_port = 4444;
SceUID tid_wait = -1;
SceUID tid_client = -1;
s_client *client;
int server_sock_msg;
int server_sock_cmd;
int quit = 0;

void close_server() {
    server_sock_msg = p2s_close_sock(server_sock_msg);
    server_sock_cmd = p2s_close_sock(server_sock_cmd);
}

int open_server() {
    close_server();

    server_sock_msg = p2s_bind_port(server_sock_msg, listen_port);
    if (server_sock_msg <= 0) {
        return -1;
    }
    server_sock_cmd = p2s_bind_port(server_sock_cmd, listen_port + 1);
    if (server_sock_cmd <= 0) {
        return -1;
    }

    return 0;
}

void p2s_msg_wait_reply() {
    if (client->msg_sock < 0) {
        return;
    }

    char buf[1];

    // wait for client to receive message
    long timeout = 1000000000;
    ksceNetSetsockopt(client->msg_sock,
                      SCE_NET_SOL_SOCKET, SCE_NET_SO_RCVTIMEO, &timeout, 4);
    ksceNetRecv(client->msg_sock, buf, 1, 0);
    timeout = 0;
    ksceNetSetsockopt(client->msg_sock,
                      SCE_NET_SOL_SOCKET, SCE_NET_SO_RCVTIMEO, &timeout, 4);
}

void psp2shell_print_color(int color, const char *fmt, ...) {
    if (client->msg_sock < 0) {
        return;
    }

    client->msg.color = color;
    memset(client->msg.buffer, 0, P2S_KMSG_SIZE);
    va_list args;
    va_start(args, fmt);
    vsnprintf(client->msg.buffer, P2S_KMSG_SIZE, fmt, args);
    va_end(args);

    if (client->msg_sock >= 0) {
        if (p2s_msg_send_msg(client->msg_sock, &client->msg) == 0) {
            // wait for client to receive message
            p2s_msg_wait_reply();
        }
    }
}

void welcome() {
    char msg[1024];
    memset(msg, 0, 1024);
    strncpy(msg, "\n", 1023);
    strncat(msg, "                     ________         .__           .__  .__   \n", 1023);
    strncat(msg, "______  ____________ \\_____  \\   _____|  |__   ____ |  | |  |  \n", 1023);
    strncat(msg, "\\____ \\/  ___/\\____ \\ /  ____/  /  ___/  |  \\_/ __ \\|  | |  |  \n", 1023);
    strncat(msg, "|  |_> >___ \\ |  |_> >       \\  \\___ \\|   Y  \\  ___/|  |_|  |__\n", 1023);
    strncat(msg, "|   __/____  >|   __/\\_______ \\/____  >___|  /\\___  >____/____/\n", 1023);
    snprintf(msg + strlen(msg), 1023, "|__|       \\/ |__|           \\/     \\/     \\/     \\/ %s\n\n", VERSION);
    PRINT_OK(msg);
}

int cmd_thread(SceSize args, void *argp) {
    printf("cmd_thread\n");
    int sock = *((int *) argp);

    memset(client, 0, sizeof(s_client));
    client->msg_sock = sock;

    // init client file listing memory
    memset(&client->fileList, 0, sizeof(s_FileList));
    strncpy(client->fileList.path, HOME_PATH, MAX_PATH_LENGTH);
    s_fileListGetEntries(&client->fileList, HOME_PATH);

    // Welcome!
    printf("welcome client\n");
    welcome();

    // get data sock
    client->cmd_sock = p2s_get_sock(server_sock_cmd);
    printf("got data sock: %i\n", client->cmd_sock);

    while (!quit) {
        int res = p2s_cmd_receive(client->cmd_sock, &client->cmd);
        if (res != 0) {
            if (res == P2S_ERR_SOCKET) {
                PRINT_ERR("p2s_cmd_receive sock failed: 0x%08X\n", res);
                break;
            } else {
                PRINT_ERR("p2s_cmd_receive failed: 0x%08X\n", res);
            }
        } else {
            p2s_cmd_parse(client, &client->cmd);
        }
    }

    printf("closing connection\n");
    client->msg_sock = p2s_close_sock(client->msg_sock);
    client->cmd_sock = p2s_close_sock(client->cmd_sock);
    s_fileListEmpty(&client->fileList);

    ksceKernelExitDeleteThread(0);
    return 0;
}

int thread_wait(SceSize args, void *argp) {
    // Enso fix ?!
    ksceKernelDelayThread(1000 * 1000 * 5);

    /*
    // load/wait network modules
    while (p2s_netInit() != 0) {
        if (quit) {
            psp2shell_exit();
            ksceKernelExitDeleteThread(0);
            return -1;
        }
        ksceKernelDelayThread(1000 * 1000);
    }
    */

    // setup sockets
    if (open_server() != 0) {
        printf("open_server failed\n");
        psp2shell_exit();
        ksceKernelExitDeleteThread(0);
        return -1;
    }

    // accept incoming connections
    printf("waiting for connections...\n");
    while (!quit) {
        //printf("thread_wait: p2s_get_sock\n");
        int client_sock = p2s_get_sock(server_sock_msg);
        //printf("thread_wait: p2s_get_sock = %i\n", client_sock);
        if (client_sock < 0) {
            if (quit) {
                break;
            }
            printf("network disconnected: reset (%i)\n", client_sock);
            ksceKernelDelayThread(1000 * 1000);
            open_server();
            continue;
        } else if (client_sock == 0) {
            printf("client_sock == 0\n");
        }

        if (quit) {
            break;
        }

        printf("new connexion on port %i (sock=%i)\n", listen_port, client_sock);
        // max client/socket count reached
        if (client->cmd_sock > 0) {
            printf("connection refused, max client reached (1)\n");
            client_sock = p2s_close_sock(client_sock);
            continue;
        }

        printf("connection accepted\n");
        tid_client = ksceKernelCreateThread("psp2shell_cmd", cmd_thread, 0x40, 0x5000, 0, 0, 0);
        if (tid_client >= 0)
            ksceKernelStartThread(tid_client, sizeof(int), (void *) &client_sock);
    }

    psp2shell_exit();
    ksceKernelExitDeleteThread(0);

    return 0;
}

void psp2shell_exit() {
    if (quit) {
        return;
    }

    printf("psp2shell_exit\n");
    quit = 1;

    printf("close_client\n");
    if (client != NULL) {
        client->msg_sock = p2s_close_sock(client->msg_sock);
        client->cmd_sock = p2s_close_sock(client->cmd_sock);
        if (tid_client >= 0) {
            ksceKernelWaitThreadEnd(tid_client, NULL, NULL);
        }
        p2s_free(client);
        client = NULL;
    }

    printf("close_server\n");
    close_server();
    if (tid_wait >= 0) {
        printf("ksceKernelWaitThreadEnd...\n");
        ksceKernelWaitThreadEnd(tid_wait, NULL, NULL);
        printf("ksceKernelWaitThreadEnd... done\n");
    }
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {
    patch_sceNetRecvfrom();
    set_hooks();

    // setup clients data
    client = p2s_malloc(sizeof(s_client));
    if (client == NULL) { // crap
        printf("client alloc failed\n");
        psp2shell_exit();
        ksceKernelExitDeleteThread(0);
        return -1;
    }

    memset(client, 0, sizeof(s_client));
    client->msg_sock = -1;
    client->cmd_sock = -1;

    tid_wait = ksceKernelCreateThread("p2sk", thread_wait, 0x40, 0x2000, 0, 0, 0);
    if (tid_wait >= 0) {
        ksceKernelStartThread(tid_wait, 0, NULL);
    }

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    psp2shell_exit();
    delete_hooks();
    return SCE_KERNEL_STOP_SUCCESS;
}
