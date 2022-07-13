//
// Created by cpasjuste on 12/07/22.
//

#ifndef PSP2SHELL_NET_H
#define PSP2SHELL_NET_H

int p2s_netInit();

int p2s_bind_port(int sock, int port);

int p2s_get_sock(int sock);

int p2s_close_sock(int sock);

int p2s_recv_all(int sock, void *buffer, int size, int flags);

size_t p2s_recv_file(int sock, SceUID fd, long size);

#endif //PSP2SHELL_NET_H
