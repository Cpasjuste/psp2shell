#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/net/netctl.h>

#include "debugScreen.h"

#include "../../psp2shell/include/psp2shell.h"

#define printf psvDebugScreenPrintf

int main(int argc, char *argv[]) {

    // init debug screen
    psvDebugScreenInit();

    printf("psp2shell\n\n");
    if (psp2shell_init(3333, 0) != 0) {
        printf("psp2shell_init failed\n");
    } else {
        SceNetCtlInfo info;
        sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
        printf("init ok: %s 3333\n", info.ip_address);
    }

    SceCtrlData ctrl;

    while (1) {

        //printf("thread running... ");
        sceCtrlPeekBufferPositive(0, &ctrl, 1);
        if (ctrl.buttons == SCE_CTRL_SELECT)
            break;

        sceKernelDelayThread(1000 * 1000);
    }

    psp2shell_exit();
    sceKernelExitProcess(0);

    return 0;
}
