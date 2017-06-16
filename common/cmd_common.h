#ifndef CMD_COMMON_H
#define CMD_COMMON_H

#define P2S_ERR_SOCKET          0x80000001
#define P2S_ERR_INVALID_CMD     0x80000002

#define SIZE_CHAR   (256)
#define SIZE_DATA   (8 * 1024)
#define SIZE_PRINT  (512)

#define MAX_ARGS    (8)

typedef struct S_CMD {
    int type;
    char args[MAX_ARGS][SIZE_PRINT];
} S_CMD;

#define SIZE_CMD    (sizeof(S_CMD) + 1024)

enum cmd_t {
    CMD_START = 10,
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
    CMD_LOAD,
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
    CMD_MODLS_PID,
    CMD_MODINFO,
    CMD_MODINFO_PID,
    CMD_MODLOADSTART,
    CMD_MODLOADSTART_PID,
    CMD_MODSTOPUNLOAD,
    CMD_MODSTOPUNLOAD_PID,
    CMD_KMODLOADSTART,
    CMD_KMODSTOPUNLOAD,
    CMD_THLS,
    CMD_THPAUSE,
    CMD_THRESUME,
    CMD_MEMR,
    CMD_MEMW,
    CMD_RESET,
    CMD_REBOOT,
    CMD_EXIT,
    CMD_HELP,

    CMD_OK = 64,
    CMD_NOK = 65
};

int p2s_cmd_receive(int sock, S_CMD *cmd);

size_t p2s_cmd_receive_buffer(int sock, void *buffer, size_t size);

int p2s_cmd_receive_resp(int sock);

void p2s_cmd_send(int sock, int cmdType);

void p2s_cmd_send_cmd(int sock, S_CMD *cmd);

void p2s_cmd_send_fmt(int sock, const char *fmt, ...);

void p2s_cmd_send_string(int sock, int cmdType, const char *value);

void p2s_cmd_send_strings(int sock, int cmdType, int argc, char *argv[]);

void p2s_cmd_send_int(int sock, int cmdType, int value);

void p2s_cmd_send_long(int sock, int cmdType, long value);

int p2s_cmd_to_string(char *buffer, S_CMD *c);

int p2s_string_to_cmd(S_CMD *c, const char *buffer);

#endif