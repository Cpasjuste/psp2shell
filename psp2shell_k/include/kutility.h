
#ifndef KUTILITY_H
#define KUTILITY_H

#define BUF_SIZE 512

extern char log_buf[BUF_SIZE];

void log_write(const char *msg);

#define LOG(...) \
    do { \
        snprintf(log_buf, sizeof(log_buf), ##__VA_ARGS__); \
        log_write(log_buf); \
    } while (0)

#endif // KUTILITY_H