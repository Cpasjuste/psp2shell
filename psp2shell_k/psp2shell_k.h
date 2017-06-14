#ifndef _PSP2SHELL_K_H_
#define _PSP2SHELL_K_H_

#include <libk/stdbool.h>

#define K_BUF_SIZE 0x1000

void kpsp2shell_set_ready(int rdy);

int kpsp2shell_wait_buffer(char *buffer);

int kpsp2shell_get_module_info(SceUID pid, SceUID modid, SceKernelModuleInfo *info);

int kpsp2shell_get_module_list(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num);

#endif //_PSP2SHELL_K_H_