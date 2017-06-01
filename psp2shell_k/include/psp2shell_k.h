#ifndef _PSP2SHELL_K_H_
#define _PSP2SHELL_K_H_

int kpsp2shell_get_sock();

void kpsp2shell_set_sock(int s);

void kpsp2shell_print(unsigned int size, const char *msg);

void kpsp2shell_print_sock(int sock, unsigned int size, const char *msg);

#endif //_PSP2SHELL_K_H_