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

#include <libk/stdio.h>

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/net/net.h>
#include <psp2/appmgr.h>
#include <taihen.h>

#include "../../psp2shell_m/include/psp2shell.h"

void _start() __attribute__ ((weak, alias ("module_start")));

void dummy() {
    sceKernelDelayThread(1000);
}

int module_start(SceSize argc, const void *args) {

    sceKernelDelayThread(1000*1000);
    psp2shell_print_color_advanced(256, COL_GREEN, "HelloModule1\n");

    sceKernelDelayThread(1000*1000);
    psp2shell_print_color_advanced(256, COL_GREEN, "HelloModule2\n");

    sceKernelDelayThread(1000*1000);
    psp2shell_print_color_advanced(256, COL_GREEN, "HelloModule3\n");

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    return SCE_KERNEL_STOP_SUCCESS;
}

void module_exit(void) {

}
