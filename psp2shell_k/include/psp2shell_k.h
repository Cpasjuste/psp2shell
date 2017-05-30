#ifndef _PSP2SHELL_K_H_
#define _PSP2SHELL_K_H_

typedef void (*StdoutCallback)(const void *data, SceSize size);

void setStdoutCb(StdoutCallback cb);

#endif //_PSP2SHELL_K_H_