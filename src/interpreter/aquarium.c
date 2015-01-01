/* Lanky -- Scripting Language and Virtual Machine
 * Copyright (C) 2014  Sam Olsen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "aquarium.h"
#include "lkyobj_builtin.h"

#define LARGE_SIZE 112
#define LARGE_COUNT 100000

// #define SCRUB_POOL
// #define FULL_OUTPUT
// #define DEBUG

typedef struct aqua_tide_pool_ {
    struct aqua_tide_pool_ *next;
    void *inlet;
    void *outlet;
    size_t fish_size;
    size_t count;
    size_t free;
    size_t used;
} aqua_tide_pool;

static aqua_tide_pool *large_pool = NULL;

int aqua_use_system_malloc_free_ = 0;

aqua_tide_pool *aqua_find_pool_of(void *ptr, aqua_tide_pool **prev)
{
    aqua_tide_pool *pool = large_pool;
    aqua_tide_pool *tprev = NULL;
    int count = 0;
    for(; pool; pool = pool->next)
    {
        if(ptr >= pool->inlet && ptr < pool->inlet + (pool->count * pool->fish_size))
        {
            if(prev)
                *prev = tprev;
            return pool;
        }
        count++;
        tprev = pool;
    }

    return NULL;
}

aqua_tide_pool *aqua_init_pool(size_t size, size_t count)
{
    int i;
    void *here;
    void *next;

    aqua_tide_pool *pool = malloc(sizeof(aqua_tide_pool));

    pool->next = NULL;
    pool->fish_size = size;
    pool->count = count;
    pool->free = count;
    pool->used = 0;
    pool->inlet = malloc(pool->fish_size * pool->count);
    pool->outlet = pool->inlet;

    here = pool->inlet;

    for(i = 0; i < pool->count - 1; i++)
    {
        next = here + pool->fish_size;
        memcpy(here, &next, sizeof(void *));
#ifdef FULL_OUTPUT
        printf("Chain: %p; Actual: %p\n", next, *(void **)(here));
#endif
        here += pool->fish_size;
    }

    return pool;
}

void aqua_init()
{
    if(aqua_use_system_malloc_free_)
        return;

    large_pool = aqua_init_pool(LARGE_SIZE, LARGE_COUNT);
#ifdef SCRUB_POOL
    printf("Init pool: %p\nCount: %d; Block size: %d\n", large_pool->inlet, large_pool->count, large_pool->fish_size);
#endif
}

void *aqua_request_next_block(size_t size)
{   
    if(aqua_use_system_malloc_free_)
        return malloc(size);

    aqua_tide_pool *pool = large_pool;
    aqua_tide_pool *prev = NULL;
    for(; pool && !pool->free; prev = pool, pool = pool->next);

    if(!pool)
    {
        pool = aqua_init_pool(LARGE_SIZE, LARGE_COUNT);
        prev->next = pool;
#ifdef DEBUG
        printf("Allocating new pool\n");
#endif
    }


    void *ptr = pool->outlet;
    memcpy(&pool->outlet, ptr, sizeof(void *));
    pool->used++;
    pool->free--;

#ifdef SCRUB_POOL
    printf("Allocating object: %p\nFree: %d; Used: %d; Next: %p\n", ptr, pool->free, pool->used, pool->outlet);
#endif

    return ptr;
}

void aqua_release(void *block)
{
    if(aqua_use_system_malloc_free_)
    {
        free(block);
        return;
    }

    aqua_tide_pool *prev = NULL;
    aqua_tide_pool *pool = aqua_find_pool_of(block, &prev);

    if(!pool)
    {
        printf("%p\n", block);
        exit(1); // This is a critical error;
        return;
    }

    memcpy(block, &pool->outlet, sizeof(void *));
    pool->outlet = block;
    pool->used--;
    pool->free++;

    if(!pool->used)
    {
#ifdef DEBUG
        printf("Freeing pool\n");
#endif
        aqua_tide_pool *next = pool->next;
        free(pool->inlet);
        free(pool);
        if(prev)
            prev->next = next;
    }
}

int aqua_is_managed_pointer(void *ptr)
{
    return !aqua_use_system_malloc_free_ && !!aqua_find_pool_of(ptr, NULL);
}

void aqua_teardown()
{
    aqua_tide_pool *next = NULL;
    aqua_tide_pool *pool = large_pool;
    while(pool)
    {
        next = pool->next;
        free(pool->inlet);
        free(pool);
        pool = next;
    }
}

