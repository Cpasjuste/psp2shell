//
// Created by cpasjuste on 18/05/17.
//

#include <psp2cmd.h>
#include "pool.h"

#ifdef MODULE
#include <libmodule.h>
#else

#include <stdlib.h>

#endif

static unsigned int data_index = 0;
static void *data_pool = NULL;

void pool_alloc() {

    data_pool = malloc(SIZE_DATA);
}

void pool_free() {

    if (data_pool != NULL) {
        free(data_pool);
    }
}

void *pool_data_malloc(unsigned int size) {

    if ((data_index + size) <= SIZE_DATA) {
        void *ptr = (void *) ((unsigned int) data_pool + data_index);
        data_index += size;
        return ptr;
    } else {
        data_index = 0;
        return pool_data_malloc(size);
    }
}
