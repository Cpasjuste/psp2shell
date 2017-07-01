//
// Created by cpasjuste on 30/06/17.
//

#include <psp2kern/io/stat.h>
#include <libk/string.h>
#include <vitasdkkern.h>
#include "kp2s_io.h"
#include "../psp2shell_m/include/psp2shell.h"
#include "psp2shell_k.h"
#include "../common/p2s_msg.h"

static char *mount_points[] = {
        "app0:",
        "gro0:",
        "grw0:",
        "os0:",
        "pd0:",
        "sa0:",
        "savedata0:",
        "tm0:",
        "ud0:",
        "ur0:",
        "ux0:",
        "vd0:",
        "vs0:",
        "host0:"
};

#define N_MOUNT_POINTS (sizeof(mount_points) / sizeof(char **))

int kp2s_io_list_drives() {

    PRINT("\n-----\n");
    PRINT("root:\n");
    PRINT("-----\n");

    for (int i = 0; i < N_MOUNT_POINTS; i++) {
        if (mount_points[i]) {
            SceIoStat stat;
            memset(&stat, 0, sizeof(SceIoStat));
            if (ksceIoGetstat(mount_points[i], &stat) >= 0) {
                PRINT("\t%s\n", mount_points[i]);
            }
        }
    }

    PRINT("\r\n");

    return 0;
};

int kp2s_io_list_dir(const char *path) {

    SceUID dfd = ksceIoDopen(path);
    if (dfd < 0) {
        PRINT_ERR("directory does not exist: %s\n", path);
        return dfd;
    }

    int len = strlen(path);
    PRINT("\n");
    for (int i = 0; i < len; i++) {
        PRINT("-");
    }
    PRINT("\n%s:\n", path);
    for (int i = 0; i < len; i++) {
        PRINT("-");
    }
    PRINT("\n");

    int res = 0;
    do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = ksceIoDread(dfd, &dir);
        if (res > 0) {
            if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
                continue;
            if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                PRINT("\t%s/\n", dir.d_name);
            } else {
                PRINT("\t%s\n", dir.d_name);
            }
        }
    } while (res > 0);

    PRINT("\r\n");

    ksceIoDclose(dfd);

    return 0;
}
