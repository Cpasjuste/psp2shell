#ifndef _PSP2_H_
#define _PSP2_H_

#include <libk/stdio.h>
#include <libk/string.h>
#include <psp2kern/udcd.h>
#include <psp2kern/types.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/dirent.h>
#include <psp2kern/io/stat.h>
#include <psp2kern/kernel/modulemgr.h>

int io_init();

int io_exit();

int io_open(char *file, int mode, SceMode mask);

int io_close(SceUID fd);

int usb_read_data(int fd, void *data, int len);

int io_read(SceUID fd, char *data, int len);

int usb_write_data(int fd, const void *data, int len);

int io_write(SceUID fd, const char *data, int len);

SceOff io_lseek(SceUID fd, SceOff ofs, int whence);

int io_ioctl(SceUID fd, unsigned int cmdno, void *indata, int inlen, void *outdata, int outlen);

int io_remove(const char *name);

int io_mkdir(const char *name, SceMode mode);

int io_rmdir(const char *name);

int io_dopen(const char *dir);

int io_dclose(SceUID fd);

int io_dread(SceUID fd, SceIoDirent *dir);

int io_getstat(const char *file, SceIoStat *stat);

int io_chstat(const char *file, SceIoStat *stat, int bits);

int io_rename(const char *oldname, const char *newname);

int io_chdir(const char *dir);

int io_mount();

int io_umount();

int io_devctl(const char *name, unsigned int cmdno, void *indata, int inlen, void *outdata, int outlen);

typedef unsigned int u32;

#define PSP_EVENT_WAITOR SCE_EVENT_WAITOR
#define PSP_EVENT_WAITCLEAR SCE_EVENT_WAITCLEAR

struct UsbData {
    unsigned char devdesc[20];

    struct Config {
        void *pconfdesc;
        void *pinterfaces;
        void *pinterdesc;
        void *pendp;
    } config;

    struct ConfDesc {
        unsigned char desc[12];
        void *pinterfaces;
    } confdesc;

    unsigned char pad1[8];
    struct Interfaces {
        void *pinterdesc[2];
        unsigned int intcount;
    } interfaces;

    struct InterDesc {
        unsigned char desc[12];
        void *pendp;
        unsigned char pad[32];
    } interdesc;

    struct Endp {
        unsigned char desc[16];
    } endp[4];
} __attribute__((packed));

#define UsbDriver SceUdcdDriver
#define UsbdDeviceReq SceUdcdDeviceRequest
#define DeviceDescriptor SceUdcdDeviceDescriptor
#define ConfigDescriptor SceUdcdConfigDescriptor
#define InterfaceDescriptor SceUdcdInterfaceDescriptor
#define EndpointDescriptor SceUdcdEndpointDescriptor
#define UsbEndpoint SceUdcdEndpoint
#define UsbInterface SceUdcdInterface
#define sceUsbbdRegister ksceUdcdRegister
#define sceUsbbdUnregister ksceUdcdUnregister
#define sceUsbbdReqRecv ksceUdcdReqRecv
#define sceUsbbdReqSend ksceUdcdReqSend
#define DeviceRequest SceUdcdEP0DeviceRequest
#define StringDescriptor SceUdcdStringDescriptor

#define sceKernelCreateSema ksceKernelCreateSema
#define sceKernelSignalSema ksceKernelSignalSema
#define sceKernelWaitSema ksceKernelWaitSema
#define sceKernelCreateEventFlag(a, b, c, d) ksceKernelCreateEventFlag(a, b, c, d)
#define sceKernelSetEventFlag ksceKernelSetEventFlag
#define sceKernelClearEventFlag ksceKernelClearEventFlag
#define sceKernelWaitEventFlag ksceKernelWaitEventFlag
#define sceKernelDeleteSema ksceKernelDeleteSema
#define sceKernelDeleteEventFlag ksceKernelDeleteEventFlag
#define sceKernelCreateThread(a, b, c, d, e, f) ksceKernelCreateThread(a, b, c, d, e, 0, f)
#define sceKernelStartThread ksceKernelStartThread
#define sceKernelDelayThread ksceKernelDelayThread
#define sceKernelExitDeleteThread ksceKernelExitDeleteThread
#define sceKernelTerminateDeleteThread ksceKernelExitDeleteThread
#define sceKernelDcacheInvalidateRange ksceKernelCpuDcacheAndL2InvalidateRange
#define sceKernelDcacheWritebackRange ksceKernelCpuDcacheAndL2WritebackRange
#define pspSdkEnableInterrupts ksceKernelCpuEnableInterrupts
#define pspSdkDisableInterrupts ksceKernelCpuDisableInterrupts

#endif // _PSP2_H_
