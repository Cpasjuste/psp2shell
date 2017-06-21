//
// Created by cpasjuste on 13/09/16.
//

#ifndef PROJECT_CMD_H
#define PROJECT_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <readline/readline.h>
#include <readline/history.h>
#include <stddef.h>

#define MAX_ARGS 20
#define MAX_COMS 20
#define ARGS_DELIMITER " "

// struct COMMAND definition
typedef struct {
    const char *name; /* command line name */
    const char *args; /* arguments (for help) */
    const char *desc; /* short description */
    int (*func)(int argc, char **argv); /* internal function triggered by the command */
} COMMAND;

extern COMMAND cmd[];

COMMAND *cmd_find(char *name);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //PROJECT_CMD_H
