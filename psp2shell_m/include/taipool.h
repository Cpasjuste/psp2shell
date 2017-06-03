#ifndef _TAIPOOL_H_
#define _TAIPOOL_H_

/* 
 *  Initialize a mempool
 *  NOTE: mempool is NOT allocated on Heap
 */
int taipool_init(size_t size);

/*
 *  Terminate already initialized mempool
 */
void taipool_term(void);

/*
 *  Returns currently available free space on mempool
 * NOTE: Memory can be segmented so even if enough free space should be available, a taipool_alloc could fail
 */
size_t taipool_get_free_space(void);

/*
 *  Resets mempool
 */
void taipool_reset(void);

/*
 *  Allocate a memory block on mempool
 */
void* taipool_alloc(size_t size);

/*
 *  Reallocate an already allocated memory block on mempool
 */
void* taipool_realloc(void* ptr, size_t size);

/*
 *  Allocate a memory block on mempool and zero initialize it
 */
void* taipool_calloc(size_t num, size_t size);

/*
 *  Free a memory block on mempool
 */
void taipool_free(void* ptr);

#endif