#ifndef CMD_H
#define CMD_H

#define SIZE_CHAR 256
#define SIZE_BUFFER (1024 * 1024)
#define SIZE_PRINT (2 * 1024)
#define SIZE_CMD (2 * SIZE_PRINT)

enum cmd_t {
    CMD_NONE=0,
    CMD_LS,
    CMD_CD,
    CMD_RM,
    CMD_RMDIR,
    CMD_MKDIR,
    CMD_MV,
    CMD_PWD,
    CMD_GET,
    CMD_PUT,
    CMD_LAUNCH,
    CMD_RELOAD,
    CMD_TEST,
    CMD_MOUNT,
    CMD_UMOUNT,
    CMD_OVADD,
    CMD_OVDEL,
    CMD_OVLS,
    CMD_FIOSCP,
    CMD_MODLS,
    CMD_MODLD,
    CMD_THLS,
    CMD_THPAUSE,
    CMD_THRESUME,
    CMD_MEMR,
    CMD_MEMW,
    CMD_RESET,
    CMD_EXIT,
    CMD_HELP
};

#endif