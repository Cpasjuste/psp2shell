#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <semaphore.h>
#include <unistd.h>

#include "p2s_msg.h"
#include "cmd.h"
#include "utility.h"
#include "main.h"

int done = 0;
char **_argv;
int msg_sock = -1;
int cmd_sock = -1;

int psp2sell_cli_exit();

extern void *g_hDev;

extern int exit_app(void);

// readline
char history_path[512];

void process_line(char *line);

/*
tcflag_t old_lflag;
cc_t old_vtime;
struct termios term;
*/
int readline_callback = 0;
// readline

void setup_terminal() {
    /*
    if (tcgetattr(STDIN_FILENO, &term) < 0) {
        perror("tcgetattr");
        exit(1);
    }
    old_lflag = term.c_lflag;
    old_vtime = term.c_cc[VTIME];
    term.c_lflag &= ~ICANON;
    term.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
        perror("tcsetattr");
        exit(1);
    }
    */

    rl_callback_handler_install("psp2shell> ", process_line);
    readline_callback = 1;
}

void close_terminal() {
    /*
    term.c_lflag = old_lflag;
    term.c_cc[VTIME] = old_vtime;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
        perror("tcsetattr");
        exit(1);
    }
    */

    readline_callback = 0;
    rl_callback_handler_remove();
    //fflush(stdin);
}

void reset_terminal(void) {
    rl_replace_line("", 0);
    rl_refresh_line(0, 0);
}

void sig_handler(int sig) {
    if (sig == SIGINT) {
        reset_terminal();
    }
}

void print_hex(char *line) {

    size_t num_tokens;
    char **tokens = strsplit(line, " ", &num_tokens);

    if (num_tokens == 5) {

        unsigned char chars[16];
        memset(chars, 0, sizeof(unsigned char) * 16);

        for (int i = 0; i < 4; i++) {

            unsigned int hex = (unsigned int) strtoul(tokens[i + 1], NULL, 16);

            int index = i + (i * 3);
            chars[index] = (unsigned char) ((hex >> 24) & 0xFF);
            if (!isprint(chars[index])) {
                chars[index] = '.';
            }
            chars[index + 1] = (unsigned char) ((hex >> 16) & 0xFF);
            if (!isprint(chars[index + 1])) {
                chars[index + 1] = '.';
            }
            chars[index + 2] = (unsigned char) ((hex >> 8) & 0xFF);
            if (!isprint(chars[index + 2])) {
                chars[index + 2] = '.';
            }
            chars[index + 3] = (unsigned char) hex;
            if (!isprint(chars[index + 3])) {
                chars[index + 3] = '.';
            }
        }

        line[strlen(line) - 1] = '\0';

        printf(GRN
                       "%s | %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n"
                       RES,
               line,
               chars[0], chars[1], chars[2], chars[3], chars[4],
               chars[5], chars[6], chars[7], chars[8], chars[9],
               chars[10], chars[11], chars[12], chars[13], chars[14], chars[15]);
        //rl_refresh_line(0, 0);
    }
}

int msg_parse(P2S_MSG *msg) {

    switch (msg->color) {

        case COL_RED:
            printf(RED
                           "%s"
                           RES, msg->buffer);
            break;
        case COL_YELLOW:
            printf(YEL
                           "%s"
                           RES, msg->buffer);
            break;
        case COL_GREEN:
            printf(GRN
                           "%s"
                           RES, msg->buffer);
            break;
        case COL_HEX:
            print_hex(msg->buffer);
            break;
        default:
            printf("%s", msg->buffer);
            break;
    }

    fflush(stdout);

    ssize_t len = strlen(msg->buffer);
    if (len > 1
        && msg->buffer[len - 2] == '\r'
        && msg->buffer[len - 1] == '\n') {
        setup_terminal();
        //rl_refresh_line(0, 0);
    }
}

void process_line(char *line) {

    if (line == NULL) {
        psp2sell_cli_exit();
        exit(0);
    }

    if (strlen(line) > 0) {

        // handle history
        add_history(line);
        append_history(1, history_path);

        // parse cmd
        size_t num_tokens;
        char **tokens = strsplit(line, " ", &num_tokens);
        if (num_tokens > 0) {
            COMMAND *cmd = cmd_find(tokens[0]);
            if (cmd == NULL) {
                printf("Command not found. Use ? for help.\n");
            } else {
                if (g_hDev == NULL) {
                    printf("Not connected\n");
                    setup_terminal();
                } else {
                    close_terminal();
                    if (cmd->func((int) num_tokens, tokens) != 0) {
                        setup_terminal();
                    }
                }
            }
        }

        if (tokens != NULL) {
            for (int i = 0; i < num_tokens; i++) {
                if (tokens[i] != NULL)
                    free(tokens[i]);
            }
            free(tokens);
        }
    }

    free(line);
}

int psp2sell_cli_init() {

    // load history from file
    memset(history_path, 0, 512);
    snprintf(history_path, 512, "%s/.psp2shell_history", getenv("HOME"));
    if (read_history(history_path) != 0) { // append_history needs the file created
        write_history(history_path);
    }

    setup_terminal();

    // catch CTRL+C
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    cmd_sock = 0; // ASYNC_SHELL (CMD OUT)
    msg_sock = 0; // ASYNC_SHELL (MSG IN)

    return 0;
}

int psp2sell_cli_exit() {

    exit_app();
    signal(SIGINT, SIG_DFL);
    close_terminal();

    return 0;
}
