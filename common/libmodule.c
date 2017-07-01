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

void *p2s_malloc(size_t size) {

    void *p = NULL;

#ifdef __KERNEL__
    SceUID uid = sceKernelAllocMemBlock(
            "m", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, (size + 0xFFF) & (~0xFFF), 0);
#else
    SceUID uid = sceKernelAllocMemBlock(
            "m", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, (size + 0xFFF) & (~0xFFF), 0);
#endif
    if (uid >= 0) {
        sceKernelGetMemBlockBase(uid, &p);
    }

    return p;
}

void p2s_free(void *p) {

    SceUID uid = sceKernelFindMemBlockByAddr(p, 1);
    if (uid >= 0) {
        sceKernelFreeMemBlock(uid);
    }
}
