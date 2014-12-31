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

#include "mempool.h"
#include "tools.h"
#include <stdlib.h>


struct poolnode *gen_node(void *obj)
{
    struct poolnode *node = MALLOC(sizeof(struct poolnode));
    node->next = NULL;
    node->data = obj;

    return node;
}

void append_to_list(struct poolnode **head, struct poolnode *next)
{
    if(!*head)
    {
        *head = next;
        return;
    }

    struct poolnode *node = *head;
    while(node->next)
        node = node->next;

    node->next = next;
}

lky_mempool pool_create()
{
    lky_mempool pool;
    pool.head = NULL;
    pool.free_func = NULL;

    return pool;
}

void pool_add(lky_mempool *pool, void *obj)
{
    struct poolnode *next = gen_node(obj);
    append_to_list(&(pool->head), next);
}

void pool_drain(lky_mempool *pool)
{
    struct poolnode *node = pool->head;
    while(node)
    {
        void *data = node->data;
        if(pool->free_func)
            pool->free_func(data);
        else if(!((uintptr_t)(data) & 1))
            FREE(data);
        struct poolnode *cur = node;
        node = node->next;
        FREE(cur);
    }

    pool->head = NULL;
}
