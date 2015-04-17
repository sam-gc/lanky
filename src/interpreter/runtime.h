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

#ifndef LANKY_RUNTIME_H
#define LANKY_RUNTIME_H

#include <pthread.h>
#include "arraylist.h"
#include "lky_object.h"

typedef struct {
    arraylist events;
    arraylist event_queue;
} runtime;

typedef struct {
    unsigned fire:1;
    lky_object *callback;
    pthread_t thread;
    void *data;
} rt_event;

runtime rt_make();
lky_object *rt_next(runtime *rt);
lky_object *rt_timeout(lky_func_bundle *bundle);

void rt_clean(runtime *rt);

#endif //LANKY_RUNTIME_H
