#include <stdio.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>
#include <taihen.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf

#define PSP2S_K 0
#define PSP2S_U 1

#define MOD_TYPE_K 0
#define MOD_TYPE_U 1

int sceKernelStartModule(SceUID modid, SceSize args, void *argp, int flags, void *option, int *status);

typedef struct Module {
    char *name;
    char *path;
    int type;
    SceUID uid;
} Module;

Module modules[2] = {
        {"psp2shell_k", "ux0:tai/psp2shell_k.skprx", MOD_TYPE_K, -1},
        {"psp2shell_m", "ux0:tai/psp2shell_m.suprx", MOD_TYPE_U, -1}
};

int exitTimeout(SceUInt delay) {

    sceKernelDelayThread(delay * 1000 * 1000);
    sceKernelExitProcess(0);

    return 0;
}

void start_module(Module *module) {

    // load
    printf("loading %s\n", module->name);

    if (module->uid >= 0) {
        printf("%s module already loaded\n", module->name);
        return;
    }

    module->uid = module->type == MOD_TYPE_U ?
                  sceKernelLoadModule(module->path, 0, NULL) :
                  taiLoadKernelModule(module->path, 0, NULL);

    if (module->uid >= 0) {
        printf("%s module loaded\n", module->name);
    } else {
        printf("could not load %s: 0x%08X", module->name, module->uid);
        return;
    }

    // start
    printf("starting %s (0x%08X)\n", module->name, module->uid);

    int res = 0;
    int ret = module->type == MOD_TYPE_U ?
              sceKernelStartModule(module->uid, 0, NULL, 0, NULL, &res) :
              taiStartKernelModule(module->uid, 0, NULL, 0, NULL, &res);

    if (ret >= 0) {
        printf("%s module started\n", modules->name);
    } else {
        printf("could not start %s: 0x%08X", modules->name, res);
        if (module->type == MOD_TYPE_U) {
            sceKernelUnloadModule(modules->uid, 0, NULL);
        } else {
            taiUnloadKernelModule(modules->uid, 0, NULL);
        }
        module->uid = -1;
    }

    sceKernelDelayThread(1000 * 1000);
}

void stop_module(Module *module) {

    printf("stopping %s\n", module->name);

    if (module->uid < 0) {
        printf("%s module not loaded\n", module->name);
        return;
    }

    int res = 0;
    int ret = module->type == MOD_TYPE_U ?
              sceKernelStopUnloadModule(module->uid, 0, NULL, 0, NULL, &res) :
              taiStopUnloadKernelModule(module->uid, 0, NULL, 0, NULL, &res);
    if (ret >= 0) {
        module->uid = -1;
        printf("%s module unloaded\n", module->name);
    } else {
        printf("could not unload %s: 0x%08X\n", module->name, ret);
    }

    sceKernelDelayThread(1000 * 500);
}

int main(int argc, char *argv[]) {

    SceCtrlData ctrl;

    psvDebugScreenInit();

    printf("PSP2SHELL LOADER @ Cpasjuste\n\n");

    printf("Square to load psp2shell_k module\n");
    printf("Triangle to unload psp2shell_k module\n\n");

    printf("Cross to load psp2shell_m module\n");
    printf("Circle to unload psp2shell_m module\n\n");

    printf("R to printf\n");

    printf("Start to exit\n");

    while (1) {

        sceCtrlPeekBufferPositive(0, &ctrl, 1);
        if (ctrl.buttons == SCE_CTRL_RTRIGGER) {
            sceClibPrintf("Hello Module1\n");
            printf("Hello Module2\n");
        } else if (ctrl.buttons == SCE_CTRL_SQUARE) {
            start_module(&modules[PSP2S_K]);
        } else if (ctrl.buttons == SCE_CTRL_TRIANGLE) {
            stop_module(&modules[PSP2S_K]);
        } else if (ctrl.buttons == SCE_CTRL_CROSS) {
            start_module(&modules[PSP2S_U]);
        } else if (ctrl.buttons == SCE_CTRL_CIRCLE) {
            stop_module(&modules[PSP2S_U]);
        } else if (ctrl.buttons == SCE_CTRL_START) {
            break;
        }
    }

    return exitTimeout(0);
}
