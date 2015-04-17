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

#include <unistd.h>
#include <pthread.h>
#include "runtime.h"
#include "lkyobj_builtin.h"
#include "lky_gc.h"
#include "arraylist.h"

rt_event *rt_make_event(lky_object *callback)
{
    rt_event *event = malloc(sizeof(*event));
    event->callback = callback;
    event->fire = 0;

    return event;
}

runtime rt_make()
{
    runtime run;
    run.event_queue = arr_create(10);
    run.events = arr_create(10);

    return run;
}

rt_event *rt_register(runtime *rt, lky_object *callback, void *(*start_routine)(void *), void *data)
{
    rt_event *event = rt_make_event(callback);
    arr_append(&rt->events, event);
    event->data = data;

    gc_add_root_object(callback);
    pthread_create(&event->thread, NULL, start_routine, event);

    return event;
}

lky_object *rt_poll(runtime *rt)
{
    if(rt->event_queue.count)
    {
        lky_object *ret = rt->event_queue.items[rt->event_queue.count - 1];
        arr_remove(&rt->event_queue, NULL, rt->event_queue.count - 1);
        return ret;
    }

    return NULL;
}

void rt_scan(runtime *rt)
{
    int i;
    for(i = 0; i < rt->events.count; i++)
    {
        rt_event *event = rt->events.items[i];
        if(event->fire)
        {
            arr_remove(&rt->events, NULL, i);
            arr_insert(&rt->event_queue, event->callback, 0);
            free(event);
        }
    }
}

lky_object *rt_next(runtime *rt)
{
    while(rt->events.count || rt->event_queue.count)
    {
        rt_scan(rt);
        lky_object *callback = rt_poll(rt);

        if(callback)
        {
            gc_remove_root_object(callback);
            return callback;
        }
        
        usleep(10000);
    }

    return NULL;
}

void *rt_example_threaded(void *data)
{
    rt_event *e = (rt_event *)data;
    sleep((unsigned int)OBJ_NUM_UNWRAP((lky_object *)e->data));

    e->fire = 1;

    return NULL;
}

lky_object *rt_timeout(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object *time = (lky_object *)args->value;
    lky_object *callback = (lky_object *)(args->next->value);

    rt_register((runtime *)(BUW_INTERP(bundle)->rtime), callback, rt_example_threaded, time);

    return &lky_nil;
}

void rt_clean(runtime *rt)
{
    arr_free(&rt->events);
    arr_free(&rt->event_queue);
}

//void rt_gc(runtime *rt)
//{
//    int i;
//    for(i = 0; i < rt->events.count; i++)
//        gc_mark_object(((rt_event *)rt->events.items[0])->callback);
//
//    for(i = 0; i < rt->event_queue.count; i++)
//        gc_mark_object(rt->event_queue.items[i]);
//}