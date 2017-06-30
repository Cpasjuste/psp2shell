#ifndef _PSP2SHELL_K_H_
#define _PSP2SHELL_K_H_

#ifdef __USB__

#include "p2s_cmd.h"

#define P2S_KMSG_SIZE   400
#else
#define P2S_KMSG_SIZE   0x1000
#endif

#ifdef __PSP2__

#include <psp2/kernel/modulemgr.h>
#include <libk/stdbool.h>

#ifdef __USB__
#ifdef __KERNEL__

int kp2s_print_stdout(const char *data, size_t size);
int kp2s_print_color(int color, const char *fmt, ...);

#else
int kp2s_print_stdout_user(const char *data, size_t size);
int kp2s_print_color_user(int color, const char *data, size_t size);
#endif

int kp2s_wait_cmd(P2S_CMD *cmd);

#else
SceSize kp2s_wait_buffer(char *buffers);
#endif

void kp2s_set_ready(bool ready);

int kp2s_get_module_info(SceUID pid, SceUID uid, SceKernelModuleInfo *info);

int kp2s_get_module_list(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num);

int kp2s_dump_module(SceUID pid, SceUID uid, const char *dst);

#endif // __PSP2__
#endif //_PSP2SHELL_K_H_