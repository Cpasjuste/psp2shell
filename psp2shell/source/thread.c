/*
	PSP2SHELL
	Copyright (C) 2016, Cpasjuste

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2/kernel/threadmgr.h>
#include "psp2shell.h"
#include <string.h>

#include "thread.h"

#define THREADS_START 0x40010000
#define THREADS_RANGE 0x100000

#define printf psp2shell_print

static void printThreadInfo(SceKernelThreadInfo *threadInfo, int thid) {

    psp2shell_print_color(COL_GREEN, "thread_name: %s\n", threadInfo->name);
    printf("\tthid: 0x%08X\n", thid);
    printf("\tprocessId: %08X\n", threadInfo->processId);
    printf("\tattr: %04X\n", threadInfo->attr);
    printf("\tstatus: %i\n", threadInfo->status);
    printf("\tentry: 0x%08X\n", threadInfo->entry);
    printf("\tstack: 0x%08X\n", threadInfo->stack);
    printf("\tstackSize: 0x%08X\n", threadInfo->stackSize);
    printf("\tinitPriority: %i\n", threadInfo->initPriority);
    printf("\tcurrentPriority: %i\n", threadInfo->currentPriority);
    printf("\tinitCpuAffinityMask: %i\n", threadInfo->initCpuAffinityMask);
    printf("\tcurrentCpuAffinityMask: %i\n", threadInfo->currentCpuAffinityMask);
    printf("\tcurrentCpuId: %i\n", threadInfo->currentCpuId);
    printf("\tlastExecutedCpuId: %i\n", threadInfo->lastExecutedCpuId);
    printf("\twaitId: %08X\n", threadInfo->waitId);
    printf("\texitStatus: %i\n", threadInfo->exitStatus);
    printf("\tintrPreemptCount: %i\n", threadInfo->intrPreemptCount);
    printf("\tthreadPreemptCount: %i\n", threadInfo->threadPreemptCount);
    printf("\tthreadReleaseCount: %i\n", threadInfo->threadReleaseCount);
    printf("\tfNotifyCallback: %08X\n", threadInfo->fNotifyCallback);
    printf("\n\n");
}

int ps_threadList() {

    int i = 1;
    while (i <= THREADS_RANGE) {
        SceKernelThreadInfo status;
        status.size = sizeof(SceKernelThreadInfo);
        int ret = sceKernelGetThreadInfo(THREADS_START + i, &status);
        if (ret >= 0) {
            printThreadInfo(&status, THREADS_START + i);
        }
        i++;
    }

    return 0;
}

// TODO: pause main thread while reloading
/*
volatile uint8_t term_dummies = 0;

int dummy_thread(SceSize args, void *argp) {
    for (;;) {
        if (term_dummies) break;
    }
    sceKernelExitDeleteThread(0);
    return 0;
}

int ps_threadResume() {

    int i = 1;
    term_dummies = 1;

    while (i <= THREADS_RANGE) {

        SceKernelThreadInfo info;
        info.size = sizeof(SceKernelThreadInfo);
        int ret = sceKernelGetThreadInfo(THREADS_START + i, &info);
        if (ret >= 0) {
            sceKernelChangeThreadPriority(THREADS_START + i, info.initPriority);
            if (strcmp(info.name, "cmd_thread") == 0
                || strcmp(info.name, "thread_wait_client") == 0
                || strcmp(info.name, "thread_power_lock") == 0
                || strcmp(info.name, "dummy") == 0) {
                continue;
            } else {
                printf("resuming thread: %s\n", info.name);
            }
        }
        i++;
    }

    return 0;
}

int ps_threadPause() {

    int i;

    sceKernelChangeThreadPriority(0, 0x0);

    term_dummies = 0;
    for (i = 0; i < 4; i++) {
        SceUID thid = sceKernelCreateThread("dummy", dummy_thread, 0x0, 0x10000, 0, 0, NULL);
        if (thid >= 0)
            sceKernelStartThread(thid, 0, NULL);
    }

    i = 1;
    while (i <= 0x5) {

        SceKernelThreadInfo info;
        info.size = sizeof(SceKernelThreadInfo);
        int ret = sceKernelGetThreadInfo(THREADS_START + i, &info);
        if (ret >= 0) {
            if (strcmp(info.name, "cmd_thread") == 0
                || strcmp(info.name, "thread_wait_client") == 0
                || strcmp(info.name, "thread_power_lock") == 0
                || strcmp(info.name, "dummy") == 0) {
                //if (strcmp(info.name, "cmd_thread") == 0)
                // sceKernelChangeThreadPriority(THREADS_START + i, 0x0);
            } else {
                printf("pausing thread: %s\n", info.name);
                sceKernelChangeThreadPriority(THREADS_START + i, 0x7F);
                SceKernelThreadInfo th;
                for (;;) {
                    th.size = sizeof(SceKernelThreadInfo);
                    sceKernelGetThreadInfo(THREADS_START + i, &th);
                    if (th.status == SCE_THREAD_RUNNING) {
                        sceKernelDelayThread(100);
                    } else break;
                }
                break;
            }
        }
        i++;
    }

    //sceKernelChangeThreadPriority(0, 0x0);

    return 0;
}
*/