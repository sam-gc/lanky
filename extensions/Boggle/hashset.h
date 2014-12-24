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

#ifndef HASHSET_H
#define HASHSET_H

typedef int(*sort_function)(const char *, const char *);

typedef struct _node {
    char *value;
    struct _node *next;
} Hashlist;

typedef struct {
    struct _node **buckets;
    int hash_size;
    char copies_str;
    char no_strcmp;
} Hashset;

Hashset HS_create(int hash_size, char copies_str);
void HS_add(Hashset *set, char *str);
int HS_contains(Hashset *set, char *str);
int HS_count(Hashset *set);
void HS_remove(Hashset *set, char *str);
void HS_free(Hashset *set);
Hashlist *HS_to_list(Hashset *set);
void HS_list_sort(Hashlist *head, sort_function function);

#endif