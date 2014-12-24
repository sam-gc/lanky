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

#ifndef HASHMAP_H
#define HASHMAP_H

struct _node;

typedef struct {
    struct _node **buckets;
    int hash_size;
    char copies_str;
    int count;
} Hashmap;

typedef enum {
    HM_NO_ERROR = 0,
    HM_KEY_NOT_FOUND
} hm_error_t;

Hashmap hm_create(int hash_size, char copies_str);
void hm_put(Hashmap *set, char *str, void *val);
void *hm_get(Hashmap *set, char *key, hm_error_t *error);
int hm_contains(Hashmap *set, char *str);
int hm_count(Hashmap *set);
void hm_remove(Hashmap *set, char *str);
void hm_free(Hashmap *set);
char **hm_list_keys(Hashmap *set);

#endif
