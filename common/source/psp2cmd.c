//
// Created by cpasjuste on 17/05/17.
//

#include <psp2cmd.h>

#ifdef PSP2

#include <libk/stdio.h>
#include <libk/string.h>
#include <libk/stdlib.h>

#else

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#endif

int s_cmd_to_string(char *buffer, S_CMD *c) {

    if (!buffer || !c) {
        return -1;
    }

    memset(buffer, 0, SIZE_CMD);
    snprintf(buffer, SIZE_CMD,
             "%i \"%s\" \"%s\" %li",
             c->type, c->arg0, c->arg1, c->arg2);

    return 0;
}

int s_string_to_cmd(S_CMD *c, const char *buffer) {

    memset(c, 0, sizeof(S_CMD));

    if (strlen(buffer) < 0) {
        return -1;
    }

    const char *start, *end;

    // type
    char tmp[2];
    strncpy(tmp, buffer, 2);
    c->type = atoi(tmp);

    // arg0 (string)
    start = strstr(buffer, "\"") + 1;
    if (!start) {
        return -1;
    }
    end = strstr(start, "\"");
    if (!end) {
        return -1;
    }
    strncpy(c->arg0, start, end - start);

    // arg1 (string)
    start = strstr(end, "\"") + 3;
    if (!start) {
        return -1;
    }
    end = strstr(start, "\"");
    if (!end) {
        return -1;
    }
    strncpy(c->arg1, start, end - start);

    // arg2 (long)
    start = strstr(start, "\"") + 2;
    c->arg2 = atol(start);

    return 0;
}
