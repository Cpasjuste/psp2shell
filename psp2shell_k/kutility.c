//
// Created by cpasjuste on 02/06/17.
//

#include <vitasdkkern.h>
#include <libk/string.h>
#include "kutility.h"

char log_buf[K_BUF_SIZE];

void log_write(const char *msg) {

    //ksceIoMkdir("ux0:/tai/", 6);
    SceUID fd = ksceIoOpen("ux0:/tai/psp2shell_k.log",
                           SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
    if (fd < 0)
        return;

    ksceIoWrite(fd, msg, strlen(msg));
    ksceIoClose(fd);
}
