#include <stdio.h>
#include <stdlib.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/sysmodule.h>
#include <psp2/net/netctl.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>

#include <taihen.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf
#define PRX "ux0:tai/psp2shell.suprx"

SceUID modid = -1;

int exitTimeout(SceUInt delay) {

    sceKernelDelayThread(delay * 1000 * 1000);
    sceKernelExitProcess(0);

    return 0;
}

typedef struct {
} args_t;

void load() {

    if (modid >= 0) {
        printf("psp2shell module already loaded\n");
        return;
    }

    printf("loading %s\n", PRX);

    args_t arg;
    int res, ret;

    //uid = taiLoadKernelModule(PRX, 0, NULL);
    modid = sceKernelLoadStartModule(PRX, 0, NULL, 0, NULL, 0);
    if (modid >= 0) {
        //ret = taiStartKernelModule(uid, sizeof(arg), &arg, 0, NULL, &res);
        //if (ret >= 0) {
        printf("psp2shell module loaded\n");
       // SceNetCtlInfo netInfo;
       // sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &netInfo);
       // printf("connect with psp2shell_cli to %s 3333\n", netInfo.ip_address);
        //} else {
        //    printf("could not load psp2shell: %i\n", ret);
        //}
    } else {
        printf("could not load psp2shell: 0x%08X", modid);
    }

    sceKernelDelayThread(1000 * 1000);
}

void unload() {

    printf("unloading psp2shell\n");

    int res;
    if (modid >= 0) {
        //int ret = taiStopUnloadKernelModule(id, 0, NULL, 0, NULL, &res);
        int ret = sceKernelStopUnloadModule(modid, 0, NULL, 0, NULL, 0);
        if (ret >= 0) {
            modid = -1;
            printf("psp2shell module unloaded\n");
        } else {
            printf("could not unload psp2shell: %i\n", ret);
        }
    } else {
        printf("psp2shell module not loaded\n");
    }

    sceKernelDelayThread(1000 * 500);
}

int main(int argc, char *argv[]) {

    SceCtrlData ctrl;

    psvDebugScreenInit();

    printf("PSP2SHELL LOADER @ Cpasjuste\n\n");
    printf("Triangle to load psp2shell module\n");
    printf("Square to unload psp2shell module\n");
    printf("Circle to test sceClibPrintf hook\n");
    printf("Cross/Circle to exit\n");

    while (1) {

        sceCtrlPeekBufferPositive(0, &ctrl, 1);
        if (ctrl.buttons == (SCE_CTRL_CIRCLE | SCE_CTRL_CROSS))
            break;
        else if (ctrl.buttons == SCE_CTRL_TRIANGLE)
            load();
        else if (ctrl.buttons == SCE_CTRL_SQUARE)
            unload();
        else if (ctrl.buttons == SCE_CTRL_CIRCLE) {
            sceClibPrintf("Hello Module1\n");
            printf("Hello Module2\n");
            fprintf(stdout, "Hello Module3\n");
        }
    }

    return exitTimeout(0);
}
