#ifndef _PSP2SHELL_K_H_
#define _PSP2SHELL_K_H_

void kpsp2shell_set_ready(int rdy);

void kpsp2shell_wait_buffer(char *buffer, unsigned int size);

int kpsp2shell_get_module_info(SceUID pid, SceUID modid, SceKernelModuleInfo *info);

int kpsp2shell_get_module_list(SceUID pid, int flags1, int flags2, SceUID *modids, size_t *num);

#endif //_PSP2SHELL_K_H_