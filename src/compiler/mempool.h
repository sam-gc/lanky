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

#ifndef MEMPOOL_H
#define MEMPOOL_H

struct poolnode {
    struct poolnode *next;
    void *data;
};

typedef struct {
    struct poolnode *head;
    void (*free_func)(void *);
    struct poolnode *tail;
} lky_mempool;

lky_mempool pool_create();
void pool_add(lky_mempool *pool, void *obj);
void pool_drain(lky_mempool *pool);

#endif
