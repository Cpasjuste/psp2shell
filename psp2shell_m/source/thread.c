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

#include "libmodule.h"
#include "psp2shell.h"
#include "thread.h"

#define THREADS_START 0x40010000
#define THREADS_RANGE 0x100000

/*
static void printThreadInfoFull(SceKernelThreadInfo *threadInfo, int thid) {

    p2s_print_color(COL_GREEN, "thread_name: %s\n", threadInfo->name);
    psp2shell_print("\tthid: 0x%08X\n", thid);
    psp2shell_print("\tprocessId: %08X\n", threadInfo->processId);
    psp2shell_print("\tattr: %04X\n", threadInfo->attr);
    psp2shell_print("\tstatus: %i\n", threadInfo->status);
    psp2shell_print("\tentry: 0x%08X\n", threadInfo->entry);
    psp2shell_print("\tstack: 0x%08X\n", threadInfo->stack);
    psp2shell_print("\tstackSize: 0x%08X\n", threadInfo->stackSize);
    psp2shell_print("\tinitPriority: %i\n", threadInfo->initPriority);
    psp2shell_print("\tcurrentPriority: %i\n", threadInfo->currentPriority);
    psp2shell_print("\tinitCpuAffinityMask: %i\n", threadInfo->initCpuAffinityMask);
    psp2shell_print("\tcurrentCpuAffinityMask: %i\n", threadInfo->currentCpuAffinityMask);
    psp2shell_print("\tcurrentCpuId: %i\n", threadInfo->currentCpuId);
    psp2shell_print("\tlastExecutedCpuId: %i\n", threadInfo->lastExecutedCpuId);
    psp2shell_print("\twaitId: %08X\n", threadInfo->waitId);
    psp2shell_print("\texitStatus: %i\n", threadInfo->exitStatus);
    psp2shell_print("\tintrPreemptCount: %i\n", threadInfo->intrPreemptCount);
    psp2shell_print("\tthreadPreemptCount: %i\n", threadInfo->threadPreemptCount);
    psp2shell_print("\tthreadReleaseCount: %i\n", threadInfo->threadReleaseCount);
    psp2shell_print("\tfNotifyCallback: %08X\n", threadInfo->fNotifyCallback);
    psp2shell_print("\n\n");
}
*/

#ifndef __KERNEL__
static void printThreadInfo(SceKernelThreadInfo *threadInfo, int thid) {

    PRINT_OK("%s id: 0x%08X, stack: 0x%08X (0x%08X)\n",
             threadInfo->name, thid, threadInfo->stack, threadInfo->stackSize);
}
#endif

int ps_threadList() {

#ifdef __KERNEL__
    PRINT_ERR("TODO: p2s_moduleInfo\n");
    return 0;
#else
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
#endif
}
