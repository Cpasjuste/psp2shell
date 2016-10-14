#include <stdio.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/sysmodule.h>
#include <psp2/net/netctl.h>
#include <psp2/io/stat.h>
#include <psp2/ctrl.h>
#include <stdlib.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf
#define SUPRX "ux0:data/psp2shell/psp2shell.suprx"

int exitTimeout(SceUInt delay) {

    sceKernelDelayThread(delay * 1000 * 1000);
    sceKernelExitProcess(0);

    return 0;
}

int exitWaitKey() {

    SceCtrlData ctrl;

    printf("Press any key to exit\n");
    while (1) {
        sceCtrlPeekBufferPositive(0, &ctrl, 1);
        if (ctrl.buttons == SCE_CTRL_ANY)
            break;
        sceKernelDelayThread(1000);
    }

    sceKernelExitProcess(0);
    return 0;
}

int fileExist(char *path) {

    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));
    int res = sceIoGetstat(path, &stat);

    return res >= 0;
}

void netInit() {

    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    SceNetInitParam netInitParam;
    size_t size = 1 * 1024 * 1024;
    netInitParam.memory = malloc(size);
    netInitParam.size = size;
    netInitParam.flags = 0;
    sceNetInit(&netInitParam);
    sceNetCtlInit();
}

void load() {

    netInit();

    int ret = sceKernelLoadStartModule(SUPRX, 0, NULL, 0, NULL, NULL);
    if (ret >= 0) {
        printf("psp2shell module loaded\n");
        SceNetCtlInfo netInfo;
        sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &netInfo);
        printf("connect with psp2shell_cli to %:3333\n", netInfo.ip_address);
        exitWaitKey();

    } else {
        printf("could not start psp2shell: %i\n", ret);
    }
}

void unload() {

}

int main(int argc, char *argv[]) {

    SceCtrlData ctrl;

    psvDebugScreenInit();

    printf("PSP2SHELL LOADER @ Cpasjuste\n\n");
    printf("Triangle to load psp2shell module\n");
    printf("Square to unload psp2shell module\n");
    printf("Cross/Circle to exit\n");

    while (1) {

        sceCtrlPeekBufferPositive(0, &ctrl, 1);
        if (ctrl.buttons == (SCE_CTRL_CIRCLE | SCE_CTRL_CROSS))
            break;
        else if (ctrl.buttons == SCE_CTRL_TRIANGLE)
            load();
        else if (ctrl.buttons == SCE_CTRL_SQUARE)
            unload();

        sceKernelDelayThread(1000 * 1000);
    }

    return exitTimeout(0);
}
