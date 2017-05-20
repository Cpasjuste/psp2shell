//
// Created by cpasjuste on 18/05/17.
//

#include "pool.h"

#ifdef MODULE

#include <libmodule.h>

#else
#include <stdlib.h>
#endif

#define POOL_SIZE   (0x5000) // 32K

static unsigned int data_index = 0;
static void *data_pool = NULL;

void pool_create() {
    data_pool = malloc(POOL_SIZE);
}

void pool_destroy() {
    if (data_pool != NULL) {
        free(data_pool);
    }
}

void *pool_data_malloc(unsigned int size) {
    if ((data_index + size) < POOL_SIZE) {
        void *ptr = (void *) ((unsigned int) data_pool + data_index);
        data_index += size;
        return ptr;
    } else {
        data_index = 0;
        return pool_data_malloc(size);
    }
}
