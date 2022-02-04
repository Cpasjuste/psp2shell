#ifndef _PSP2SHELL_K_H_
#define _PSP2SHELL_K_H_

#define P2S_KMSG_SIZE    0x1000

#ifdef __PSP2__

#include <psp2/kernel/modulemgr.h>
#include <libk/stdbool.h>

#include "p2s_cmd.h"
#include "p2s_msg.h"
#include "file.h"

#define P2S_MSG_LEN 256
#define MSG_MAX 64

typedef struct kp2s_msg {
    char msg[P2S_MSG_LEN];
    int len;
} kp2s_msg;

#define BOOL int
#define TRUE 1
#define FALSE 0

typedef struct {
    int msg_sock;
    int cmd_sock;
    P2S_CMD cmd;
    P2S_MSG msg;
    s_FileList fileList;
} s_client;

void kpsp2shell_set_ready(bool ready);

SceSize kpsp2shell_wait_buffer(kp2s_msg *msg_list);

int kpsp2shell_get_module_info(SceUID pid, SceUID uid, SceKernelModuleInfo *info);

int kpsp2shell_get_module_list(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num);

int kpsp2shell_dump_module(SceUID pid, SceUID uid, const char *dst);

void psp2shell_print_color(int color, const char *fmt, ...);

int psp2shell_init(int port);

void psp2shell_exit();

void psp2shell_print_color(int color, const char *fmt, ...);

#define psp2shell_print(...) psp2shell_print_color(COL_NONE, __VA_ARGS__)
#define PRINT(...) psp2shell_print_color(COL_NONE, __VA_ARGS__)
#define PRINT_ERR(...) psp2shell_print_color(COL_RED, __VA_ARGS__)
#define PRINT_OK(...) psp2shell_print_color(COL_GREEN, __VA_ARGS__)

#endif // __PSP2__
#endif //_PSP2SHELL_K_H_