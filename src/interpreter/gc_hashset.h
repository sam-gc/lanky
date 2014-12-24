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

#ifndef GC_HASHSET_H
#define GC_HASHSET_H

typedef void(*gchs_pointer_function)(void *data);

typedef struct _node {
    void *value;
    struct _node *next;
} gc_hashlist;

typedef struct {
    struct _node **buckets;
    int hash_size;
    int count;
} gc_hashset;

gc_hashset gchs_create(int hash_size);
void gchs_add(gc_hashset *set, void *obj);
int gchs_contains(gc_hashset *set, void *obj);
void gchs_remove(gc_hashset *set, void *obj);
void gchs_for_each(gc_hashset *set, gchs_pointer_function callback);
void gchs_free(gc_hashset *set);
void **gchs_to_list(gc_hashset *set);

#endif
