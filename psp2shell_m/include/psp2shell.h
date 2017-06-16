#ifndef _psp2_shell_h_
#define _psp2_shell_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "p2s_cmd.h"

#ifdef DEBUG
int sceClibPrintf(const char *, ...);
#define printf sceClibPrintf
#endif

enum colors_t {
    COL_NONE = 0,
    COL_RED = 1,
    COL_YELLOW = 2,
    COL_GREEN = 3,
    COL_HEX = 9
};

// init psp2shell on specified port with delay in seconds.
// setting a delay will pause (delay) the application
// allowing for early debug message to be shown
// and eventually give time to "reload" your app over network.
int psp2shell_init(int port, int delay);

void psp2shell_exit();

void psp2shell_print(const char *fmt, ...);

void psp2shell_print_color(int color, const char *fmt, ...);

void psp2shell_print_color_advanced(SceSize size, int color, const char *fmt, ...);

#define psp2shell_print(...) psp2shell_print_color_advanced(P2S_SIZE_PRINT, 0, __VA_ARGS__)
#define psp2shell_print_color(x, ...) psp2shell_print_color_advanced(P2S_SIZE_PRINT, x, __VA_ARGS__)

#define PRINT_ERR(...) psp2shell_print_color(COL_RED, __VA_ARGS__)
#define PRINT_OK(...) psp2shell_print_color(COL_GREEN, __VA_ARGS__)

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _psp2_shell_h_
