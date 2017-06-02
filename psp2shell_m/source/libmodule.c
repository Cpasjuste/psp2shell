//
// Created by cpasjuste on 07/11/16.
//

#ifdef MODULE

#include "libmodule.h"

#ifdef __VITA_KERNEL__
#define MEMTYPE SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW
#else
#define MEMTYPE SCE_KERNEL_MEMBLOCK_TYPE_USER_RW
#endif

void *malloc(size_t size) {

    void *p = NULL;

    if (MEMTYPE == SCE_KERNEL_MEMBLOCK_TYPE_USER_RW) {
        size = (size + 0xFFF) & (~0xFFF); // align 4K
    }

    SceUID uid = sceKernelAllocMemBlock("m", MEMTYPE, size, 0);
    if (uid >= 0) {
        sceKernelGetMemBlockBase(uid, &p);
    }
    return p;
}

void free(void *p) {

    SceUID uid = sceKernelFindMemBlockByAddr(p, 1);
    if (uid >= 0) {
        sceKernelFreeMemBlock(uid);
    }
}

#endif // MODULE