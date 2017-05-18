#ifndef CMD_H
#define CMD_H

#define SIZE_CHAR   (256)
#define SIZE_DATA   (16 * 1024)
#define SIZE_PRINT  (1024)

typedef struct S_CMD {
    int type;
    char arg0[SIZE_PRINT];
    char arg1[SIZE_PRINT];
    long arg2;
} S_CMD;

#define SIZE_CMD    ((SIZE_PRINT * 2) + 64)

enum cmd_t {
    CMD_NONE = 0,
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
    CMD_TITLE,
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
    CMD_REBOOT,
    CMD_EXIT,
    CMD_HELP
};

int s_cmd_to_string(char *buffer, S_CMD *c);

int s_string_to_cmd(S_CMD *c, const char *buffer);

#endif