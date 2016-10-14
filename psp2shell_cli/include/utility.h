//
// Created by cpasjuste on 13/09/16.
//

#ifndef PROJECT_UTILITY_H
#define PROJECT_UTILITY_H

#include <stdio.h>

#define RES "\x1B[0m"
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"

char **strsplit(const char *str, const char *delim, size_t *numtokens);

#endif //PROJECT_UTILITY_H
