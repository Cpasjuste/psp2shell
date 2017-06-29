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

#ifndef _psp2_shell_h_
#define _psp2_shell_h_

#ifdef __cplusplus
extern "C" {
#endif

// init psp2shell on specified port
int psp2shell_init(int port);

void psp2shell_exit();

#ifdef __USB__
#ifdef __KERNEL__
#define p2s_print_color kp2s_print_color
#else
#include "../psp2shell_k/psp2shell_k.h"
#define p2s_print_color kp2s_print_color_user
#endif
#else
void p2s_print_color(int color, const char *fmt, ...);
#endif

#define psp2shell_print(...) p2s_print_color(COL_NONE, __VA_ARGS__)
#define PRINT_ERR(fmt, ...) p2s_print_color(COL_RED, "\n\n" fmt "\n\r\n", ## __VA_ARGS__)
#define PRINT_OK(fmt, ...) p2s_print_color(COL_GREEN, "\n\n" fmt "\n\r\n", ## __VA_ARGS__)
#define PRINT(...) p2s_print_color(COL_YELLOW, __VA_ARGS__)

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _psp2_shell_h_
