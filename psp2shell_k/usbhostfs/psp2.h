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
