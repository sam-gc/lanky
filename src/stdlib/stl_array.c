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

//#define OLD_STYLE

#ifdef OLD_STYLE

#include <stdlib.h>
#include <string.h>
#include "stl_array.h"
#include "arraylist.h"
#include "lky_gc.h"
#include "lky_machine.h"
#include "stl_string.h"
#include "mach_binary_ops.h"

#define IS_TAGGED(a) ((uintptr_t)(a) & 1)
#define FAIL_CHECK(check, name, text) do { if(check) { mach_halt_with_err(lobjb_build_error(name, text)); return &lky_nil; } }while(0);

static lky_object *stlarr_class = NULL;
static lky_object *stlarr_proto = NULL;

typedef struct {
    arraylist container;
} stlarr_data;

lky_object *stlarr_append(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arr_append(&data->container, args->value);
    lobj_set_member((lky_object *)self, "count", lobjb_build_int(data->container.count));
    return (lky_object *)self;
}

lky_object *stlarr_insert(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;

    int idx = (int)OBJ_NUM_UNWRAP(args->next->value);
    stlarr_data *data = self->data;
    arr_insert(&data->container, args->value, idx);
    lobj_set_member((lky_object *)self, "count", lobjb_build_int(data->container.count));
    return (lky_object *)self;
}

arraylist stlarr_unwrap(lky_object *obj)
{
    lky_object_custom *self = (lky_object_custom *)obj;
    stlarr_data *data = self->data;
    return data->container;
}

arraylist *stlarr_get_store(lky_object *obj)
{
    lky_object_custom *self = (lky_object_custom *)obj;
    stlarr_data *data = self->data;
    return &data->container;
}

lky_object *stlarr_add(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;

    lky_object *obj = (lky_object *)args->value;
    if(!((uintptr_t)(obj) & 1) && obj->type != LBI_FLOAT && obj->type != LBI_INTEGER)
        return &lky_nil;

    long offset = OBJ_NUM_UNWRAP(obj);
    
    if(offset >= list.count)
    {
        // TODO: Array indexing error
    }

    arraylist nlist = arr_create(list.count - offset + 1);
    int i;
    for(i = (int)offset; i < list.count; i++)
    {
        arr_append(&nlist, arr_get(&list, i));
    }

    return stlarr_cinit(nlist);
}

lky_object *stlarr_set(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;

    lky_object *indexer = (lky_object *)args->value;
    lky_object *newobj = (lky_object *)args->next->value;

    if(!OBJ_IS_NUMBER(indexer))
    {
        mach_halt_with_err(lobjb_build_error("MismatchedType", "You can only index an array with an int or a float!"));
        return &lky_nil;
    }

    lky_object_builtin *b = (lky_object_builtin *)indexer;
    long idx = OBJ_NUM_UNWRAP(b);
    if(idx >= list.count || idx < 0)
    {
        mach_halt_with_err(lobjb_build_error("IndexOutOfBounds", "The given index is not valid for the array."));
        return &lky_nil;
    }

    list.items[idx] = newobj;

    return &lky_nil;
}

lky_object *stlarr_get(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    lky_object_builtin *b = (lky_object_builtin *)args->value;
    long idx = OBJ_NUM_UNWRAP(b);

    if(idx >= data->container.count)
    {
        mach_halt_with_err(lobjb_build_error("IndexOutOfBounds", "The given index is not valid for the array."));
        return &lky_nil;
    }

    return arr_get(&data->container, idx);
}

lky_object *stlarr_last(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;

    return arr_get(&data->container, data->container.count - 1);
}

lky_object *stlarr_contains(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;
    lky_object *a = (lky_object *)args->value;

    int toret = 0;

    long i;
    for(i = 0; i < list.count; i++)
    {
        lky_object *b = arr_get(&list, i);
        lky_object *result = lobjb_binary_equals(a, b, BUW_INTERP(bundle));
        if(IS_TAGGED(result) || result->type == LBI_INTEGER || result->type == LBI_FLOAT)
        {
            toret = !!OBJ_NUM_UNWRAP(result);
            if(toret)
                break;
        }
        else if(result == &lky_yes)
            return result;
    }

    return LKY_TESTC_FAST(toret);
}

lky_object *stlarr_for_each(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;

    stlarr_data *data = self->data;
    arraylist list = data->container;

    lky_object_function *callback = (lky_object_function *)args->value;
    FAIL_CHECK(IS_TAGGED(callback) || callback->type != LBI_FUNCTION, "MismatchedType", "Expected function type");
    FAIL_CHECK(!callback->callable.argc || callback->callable.argc > 2, "MismatchedType", "Expected function with 1 or 2 arguments");

    char useidx = 0;
    if(callback->callable.argc == 2)
        useidx = 1;

    long i;
    for(i = 0; i < list.count; i++)
        lobjb_call((lky_object *)callback, LKY_ARGS(arr_get(&list, i), useidx ? lobjb_build_int(i) : NULL), BUW_INTERP(bundle));

    return &lky_nil;
}

lky_object *stlarr_index_of(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;
    lky_object *a = (lky_object *)args->value;

    long i;
    for(i = 0; i < list.count; i++)
    {
        lky_object *b = arr_get(&list, i);
        lky_object *result = lobjb_binary_equals(a, b, BUW_INTERP(bundle));
        if(IS_TAGGED(result) || result->type == LBI_INTEGER || result->type == LBI_FLOAT)
        {
            if(OBJ_NUM_UNWRAP(result))
                return lobjb_build_int(i);
            continue;
        }

        if(result == &lky_nil)
            continue;

        return lobjb_build_int(i);
    }

    return lobjb_build_int(-1);
}

void stlarr_dealloc(lky_object *o)
{
    lky_object_custom *self = (lky_object_custom *)o;
    stlarr_data *data = self->data;
    arr_free(&data->container);
    free(data);
}

void stlarr_save(lky_object *o)
{
    lky_object_custom *self = (lky_object_custom *)o;
    stlarr_data *data = self->data;

    long i;
    for(i = 0; i < data->container.count; i++)
    {
        lky_object *obj = arr_get(&data->container, i);
        if(!obj)
            continue;
        
        gc_mark_object(arr_get(&data->container, i));
    }

}

lky_object *stlarr_slice_with_count(arraylist list, long start, long count)
{
    arraylist nw;
    nw.items = malloc((count + 8) * sizeof(void *));
    nw.count = count;
    nw.allocated = count + 8;
    
    memcpy(nw.items, list.items + start, count * sizeof(void *));

    return stlarr_cinit(nw);
}

lky_object *stlarr_remove_at(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    
    arraylist *list = &data->container;
    
    long idx = OBJ_NUM_UNWRAP(args->value);
    
    lky_object *obj = arr_get(list, idx);
    arr_remove(list, NULL, idx);
    
    lobj_set_member((lky_object *)self, "count", lobjb_build_int(data->container.count));
    
    return obj;
}

lky_object *stlarr_joined(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;

    char *joiner = lobj_stringify((lky_object *)args->value, BUW_INTERP(bundle));

    if(!joiner)
        return &lky_nil;
    
    if(list.count == 0)
        return stlstr_cinit("");

    char *innards[list.count];
    size_t tlen = 0;

    int i;
    for(i = 0; i < list.count; i++)
    {
        lky_object_custom *strobj = NULL;
        lky_object *obj = arr_get(&list, i);
        lky_object_function *f = (lky_object_function *)lobj_get_member(obj, "stringify_");
        
        if(!f)
        {
            char str[100];
            
            if(IS_TAGGED(obj) || obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
                strobj = (lky_object_custom *)lobjb_num_to_string(obj);
            else
            {
                sprintf(str, "%p", obj);
                strobj = (lky_object_custom *)stlstr_cinit(str);
            }
        }
        else
        {
            lky_func_bundle b = MAKE_BUNDLE(f, NULL, BUW_INTERP(bundle));
            strobj = (lky_object_custom *)(f->callable.function)(&b);
        }
        
        char *str = strobj->data;
        
        innards[i] = str;
        tlen += strlen(str);
    }
    
    size_t jlen = strlen(joiner);

    // Adding to the total length:
    //      brackets      commas & spaces                           null termination
    tlen += jlen * (list.count ? list.count - 1 : 0) + 1;
    
    char *str = malloc(tlen);
    strcpy(str, "");
    
    for(i = 0; i < list.count - 1; i++)
    {
        strcat(str, innards[i]);
        strcat(str, joiner);
    }
    
    strcat(str, innards[i]);

    lky_object *ret = stlstr_cinit(str);
    free(str);

    return ret;
}

lky_object *stlarr_reverse(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;

    arraylist nlist = arr_create(list.count + 1);

    long i;
    for(i = list.count - 1; i >= 0; i--)
    {
        arr_append(&nlist, arr_get(&list, i));
    }

    return stlarr_cinit(nlist);
}

arr_sort_result stlarr_wrap_sort(void *left, void *right, void *data)
{
    lky_object *lto = lobjb_binary_lessthan(left, right, NULL);
    lky_object *eqo = lobjb_binary_equals(left, right, NULL);

    int lt = (int)OBJ_NUM_UNWRAP(lto);
    int eq = (int)OBJ_NUM_UNWRAP(eqo);

    if(eq)
        return SORT_RESULT_EQUAL;
    return lt ? SORT_RESULT_SORTED : SORT_RESULT_REVERSE;
}

lky_object *stlarr_sort(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;

    arr_sort(&list, stlarr_wrap_sort, NULL);

    return (lky_object *)self;
}

lky_object *stlarr_copy(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;

    arraylist nw;
    nw.items = malloc(list.allocated * sizeof(void *));
    nw.count = list.count;
    nw.allocated = list.allocated;

    memcpy(nw.items, list.items, list.count * sizeof(void *));

    return stlarr_cinit(nw);
}

lky_object *stlarr_sorted(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;
    
    arraylist nw;
    nw.items = malloc(list.allocated * sizeof(void *));
    nw.count = list.count;
    nw.allocated = list.allocated;

    memcpy(nw.items, list.items, list.count * sizeof(void *));

    arr_sort(&nw, stlarr_wrap_sort, NULL);

    return stlarr_cinit(nw);
}

lky_object *stlarr_slice(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;

    long start = OBJ_NUM_UNWRAP(args->value);
    long count = OBJ_NUM_UNWRAP(args->next->value);

    return stlarr_slice_with_count(list, start, count);
}

lky_object *stlarr_slice_to(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;

    long start = OBJ_NUM_UNWRAP(args->value);
    long end =   OBJ_NUM_UNWRAP(args->next->value);
    long count = end - start + 1;

    return stlarr_slice_with_count(list, start, count);
}

lky_object *stlarr_stringify(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *self = (lky_object_custom *)func->owner;
    stlarr_data *data = self->data;
    arraylist list = data->container;

    // TODO: There is a bug in this function...
    // Without the following check, the program should normally
    // return "[ ]" as expected, but there is a memory corruption
    // bug due to trying to copy 0 bytes. This should ideally be
    // fixed in a more robust way than using the simple check
    // below. For now, this check __NEEDS__ to stay in place.
    if(!list.count)
        return stlstr_cinit("[ ]");
    
    char *innards[list.count];
    size_t tlen = 0;
    
    int i;
    for(i = 0; i < list.count; i++)
    {
        lky_object *obj = arr_get(&list, i);
        char *str = lobjb_stringify(obj, BUW_INTERP(bundle));
        
        innards[i] = str;
        tlen += strlen(str);
    }
    
    // Adding to the total length:
    //      brackets      commas & spaces                           null termination
    tlen +=    4     +  2 * (list.count > 0 ? list.count - 1 : 0)  +       1;
    
    char *str = malloc(tlen);
    strcpy(str, "[ ");
    
    for(i = 0; i < list.count - 1; i++)
    {
        strcat(str, innards[i]);
        strcat(str, ", ");
        free(innards[i]);
    }
    
    if(list.count)
        strcat(str, innards[i]);
    strcat(str, " ]");

    free(innards[i]);
    
    lky_object *ret = stlstr_cinit(str);
    free(str);
    
    return ret;
}

lky_object *stlarr_get_proto()
{
    if(stlarr_proto)
        return stlarr_proto;

    stlarr_proto = lobj_alloc();
    return stlarr_proto;
}

lky_object *stlarr_cinit(arraylist inlist)
{
    lky_object_custom *cobj = lobjb_build_custom(sizeof(stlarr_data));
    stlarr_data *data = malloc(sizeof(stlarr_data));
    data->container = inlist;
    cobj->data = data;
    
    lky_object *obj = (lky_object *)cobj;
    lobj_set_class(obj, stlarr_get_class());

    lky_object *getter = lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlarr_get);
    lky_object *setter = lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlarr_set);

    lobj_set_member(obj, "append",          lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlarr_append));
    lobj_set_member(obj, "get",             getter); // We want to let people directly call get
    lobj_set_member(obj, "op_get_index_",   getter); // For the builtin getting syntax
    lobj_set_member(obj, "set",             setter); // Direct call
    lobj_set_member(obj, "op_set_index_",   setter); // For the builtin setting syntax
    lobj_set_member(obj, "forEach",         lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlarr_for_each));
    lobj_set_member(obj, "count",           lobjb_build_int(inlist.count));
    lobj_set_member(obj, "contains",        lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlarr_contains));
    lobj_set_member(obj, "indexOf",         lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlarr_index_of));
    lobj_set_member(obj, "stringify_",      lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlarr_stringify));
    lobj_set_member(obj, "removeAt",        lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlarr_remove_at));
    lobj_set_member(obj, "joined",          lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlarr_joined));
    lobj_set_member(obj, "reverse",         lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlarr_reverse));
    lobj_set_member(obj, "insert",          lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlarr_insert));
    lobj_set_member(obj, "op_add_",         lobjb_build_func_ex(obj, 1, (lky_function_ptr)stlarr_add));
    lobj_set_member(obj, "last",            lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlarr_last));
    lobj_set_member(obj, "sort",            lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlarr_sort));
    lobj_set_member(obj, "sorted",          lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlarr_sorted));
    lobj_set_member(obj, "copy",            lobjb_build_func_ex(obj, 0, (lky_function_ptr)stlarr_copy));
    lobj_set_member(obj, "slice",           lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlarr_slice));
    lobj_set_member(obj, "sliceTo",         lobjb_build_func_ex(obj, 2, (lky_function_ptr)stlarr_slice_to));
    lobj_set_member(obj, "proto_",          stlarr_get_proto());

    cobj->freefunc = stlarr_dealloc;
    cobj->savefunc = stlarr_save;
    return (lky_object *)obj;
}

lky_object *stlarr_build(lky_func_bundle *bundle)
{
    return stlarr_cinit(arr_create(10));
}

lky_object *stlarr_get_class()
{
    if(stlarr_class)
        return stlarr_class;

    lky_object *clsobj = lobj_alloc();

    /*lky_callable c;
    c.argc = 0;
    c.function = (lky_function_ptr)stlarr_build;
    clsobj->callable = c;
    */

    lobj_set_member(clsobj, "model_", stlarr_get_proto());
    lobj_set_member(clsobj, "new", lobjb_build_func_ex(clsobj, 0, (lky_function_ptr)stlarr_build));

    stlarr_class = clsobj;
    return clsobj;
}

#else

#include <stdlib.h>
#include <string.h>
#include "stl_array.h"
#include "lky_gc.h"
#include "lky_machine.h"
#include "stl_string.h"
#include "mach_binary_ops.h"
#include "class_builder.h"

#define IS_TAGGED(a) ((uintptr_t)(a) & 1)

static lky_object *stlarr_class_ = NULL;

typedef struct {
    arraylist container;
} stlarr_bl;

CLASS_MAKE_BLOB_FUNCTION(stlarr_bl_manage, stlarr_bl *, blob, how, 
    if(how == CGC_FREE)
    {
        arr_free(&blob->container);
        free(blob);    
        return;
    }

    long i;
    for(i = 0; i < blob->container.count; i++)
        gc_mark_object((lky_object *)blob->container.items[i]);
)

CLASS_MAKE_INIT(stlarr_init, 
    stlarr_bl *b = malloc(sizeof(*b));
    b->container = arr_create(10);

    CLASS_SET_BLOB(self_, "ab_", b, stlarr_bl_manage);
)

void stlarr_manual_init(lky_object *nobj, lky_object *cls, void *data)
{
    stlarr_bl *b = malloc(sizeof(*b));
    memcpy(&b->container, data, sizeof(*b));

    CLASS_SET_BLOB(nobj, "ab_", b, stlarr_bl_manage);
    lobj_set_member(nobj, "count", lobjb_build_int(b->container.count));
}

lky_object *stlarr_cinit(arraylist list)
{
    return clb_instantiate(stlarr_get_class(), stlarr_manual_init, &list);
}

CLASS_MAKE_METHOD_EX(stlarr_append, self, stlarr_bl *, ab_, 
    arr_append(&ab_->container, $1);
    lobj_set_member(self, "count", lobjb_build_int(ab_->container.count));
    return self;
)

CLASS_MAKE_METHOD_EX(stlarr_insert, self, stlarr_bl *, ab_, 
    int idx = OBJ_NUM_UNWRAP($2);
    CLASS_ERROR_ASSERT(idx < ab_->container.count, "OutOfBounds", "The specified index is out of bounds.");
    arr_insert(&ab_->container, $1, idx);
    lobj_set_member(self, "count", lobjb_build_int(ab_->container.count));
    return self;
)

CLASS_MAKE_METHOD_EX(stlarr_set, self, stlarr_bl *, ab_,
    arraylist *list = &ab_->container;
    int idx = (int)OBJ_NUM_UNWRAP($1);
    CLASS_ERROR_ASSERT(idx < list->count, "OutOfBounds", "The specified index is out of bounds.");
    list->items[idx] = $2;
)

CLASS_MAKE_METHOD_EX(stlarr_get, self, stlarr_bl *, ab_,
    int idx = OBJ_NUM_UNWRAP($1);
    CLASS_ERROR_ASSERT(idx < ab_->container.count, "OutOfBounds", "The specified index is out of bounds.");
    return arr_get(&ab_->container, idx);
)

CLASS_MAKE_METHOD_EX(stlarr_contains, self, stlarr_bl *, ab_,
    arraylist list = ab_->container;
    
    int toret = 0;
    long i;
    for(i = 0; i < list.count; i++)
    {
        lky_object *b = arr_get(&list, i);
        lky_object *result = lobjb_binary_equals($1, b, interp_);
        if(IS_TAGGED(result) || result->type == LBI_INTEGER || result->type == LBI_FLOAT)
        {
            toret = !!OBJ_NUM_UNWRAP(result);
            if(toret)
                break;
        }
        else if(result == &lky_yes)
            return result;
    }

    return LKY_TESTC_FAST(toret); 
)

CLASS_MAKE_METHOD_EX(stlarr_for_each, self, stlarr_bl *, ab_,
    arraylist list = ab_->container;
    lky_object_function *callback = (lky_object_function *)$1;

    char useidx = 0;
    if(callback->callable.argc == 2)
        useidx = 1;

    long i;
    for(i = 0; i < list.count; i++)
        lobjb_call($1, LKY_ARGS(arr_get(&list, i), useidx ? lobjb_build_int(i) : NULL), interp_);
)

CLASS_MAKE_METHOD_EX(stlarr_index_of, self, stlarr_bl *, ab_,
    arraylist list = ab_->container;
    long i;
    for(i = 0; i < list.count; i++)
    {
        lky_object *t = arr_get(&list, i);
        lky_object *result = lobjb_binary_equals($1, t, interp_);
        if(IS_TAGGED(result) || OBJ_IS_NUMBER(result))
        {
            if(OBJ_NUM_UNWRAP(result))
                return lobjb_build_int(i);
            continue;
        }

        if(result == &lky_nil || result == &lky_no)
            continue;

        return lobjb_build_int(i);
    }

    return lobjb_build_int(-1);
)

CLASS_MAKE_METHOD_EX(stlarr_remove_at, self, stlarr_bl *, ab_,
    arraylist *list = &ab_->container;

    long idx = OBJ_NUM_UNWRAP($1);
    lky_object *obj = arr_get(list, idx);
    arr_remove(list, NULL, idx);

    lobj_set_member(self, "count", lobjb_build_int(list->count));

    return obj;
)

CLASS_MAKE_METHOD_EX(stlarr_joined, self, stlarr_bl *, ab_,
    arraylist *list = &ab_->container;
    char *joiner = lobjb_stringify($1, interp_);

    int put_strings = LKY_CTEST_FAST($2);

    if(!joiner)
        return &lky_nil;

    if(list->count == 0)
        return stlstr_cinit("");

    char *innards[list->count];
    size_t lens[list->count];
    size_t tlen = 0;

    int i;
    for(i = 0; i < list->count; i++)
    {
        lky_object *obj = arr_get(list, i);
        char *text = lobjb_stringify(obj, interp_);

        if(put_strings && lobj_is_of_class(obj, stlstr_get_class())) 
        {
            char *tmp = malloc(strlen(text) + 3);
            sprintf(tmp, "'%s'", text);
            free(text);
            text = tmp;
        }

        innards[i] = text;

        size_t il = strlen(text);
        lens[i] = il;
        tlen += il;
    }

    size_t jlen = strlen(joiner);
    // Adding to the total length:
    tlen += jlen * (list->count ? list->count - 1 : 0) + 1;

    char *str = malloc(tlen);
    char *cur = str;
    for(i = 0; i < list->count - 1; i++)
    {
        memcpy(cur, innards[i], lens[i]);
        cur += lens[i];
        free(innards[i]);
        memcpy(cur, joiner, jlen);
        cur += jlen;
    }

    memcpy(cur, innards[i], lens[i]);
    str[tlen - 1] = '\0';

    lky_object *ret = stlstr_cinit(str);
    free(str);
    free(joiner);
    free(innards[i]);

    return ret;
)


CLASS_MAKE_METHOD_EX(stlarr_copy, self, stlarr_bl *, ab_,
    arraylist *list = &ab_->container;
    arraylist copy = arr_create(list->count + 10);
    memcpy(copy.items, list->items, list->count * sizeof(void *));
    copy.count = list->count;

    return stlarr_cinit(copy);
)

CLASS_MAKE_METHOD_EX(stlarr_map, self, stlarr_bl *, ab_,
    arraylist *list = &ab_->container;
    arraylist mapped = arr_create(list->count + 10);
    int i;
    for(i = 0; i < list->count; i++)
        mapped.items[i] = lobjb_call($1, lobjb_make_seq_node(list->items[i]), interp_);

    mapped.count = list->count;
    return stlarr_cinit(mapped);
)

CLASS_MAKE_METHOD_EX(stlarr_reduce, self, stlarr_bl *, ab_,
    arraylist *list = &ab_->container;
    lky_object *prev = &lky_nil;
    int i;
    for(i = 0; i < list->count; i++)
        prev = lobjb_call($1, LKY_ARGS(prev, list->items[i]), interp_);

    return prev;
)

CLASS_MAKE_METHOD(stlarr_stringify, self,
    $1 = stlstr_cinit(", ");
    lky_object_function *func = (lky_object_function *)lobjb_build_func_ex(NULL, 0, NULL);
    func->bound = self;
    lky_func_bundle b = MAKE_BUNDLE(func, LKY_ARGS($1, &lky_yes), interp_);
    lky_object *res = stlarr_joined(&b);
    char *tmp = lobjb_stringify(res, interp_);
    char *out = malloc(strlen(tmp) + 5);
    sprintf(out, "[ %s ]", tmp);

    lky_object *ret = stlstr_cinit(out);
    free(out);
    free(tmp);

    return ret;
)

CLASS_MAKE_METHOD(stlarr_alloc, self,
    long size = OBJ_NUM_UNWRAP($1);
    return stlarr_cinit(arr_create(size));
)

CLASS_MAKE_METHOD(stlarr_memcpy, self,
    stlarr_bl *a = CLASS_GET_BLOB($1, "ab_", stlarr_bl *);
    stlarr_bl *b = CLASS_GET_BLOB($2, "ab_", stlarr_bl *);

    arraylist *al = &a->container;
    arraylist *bl = &b->container;

    long size = OBJ_NUM_UNWRAP($3);

    CLASS_ERROR_TEST(size > al->allocated, "OutOfBounds", "The destination does not have enough memory allocated");
    CLASS_ERROR_TEST(size > bl->count, "OutOfBounds", "The source does not have enough elements to copy");

    memcpy(al->items, bl->items, size * sizeof(void *));

    al->count = al->count < size ? size : al->count;
    lobj_set_member($1, "count", lobjb_build_int(al->count));
)

CLASS_MAKE_METHOD_EX(stlarr_size, self, stlarr_bl *, ab_,
    return lobjb_build_int(ab_->container.allocated);
)

lky_object *stlarr_get_class()
{
    if(stlarr_class_)
        return stlarr_class_;

    stlarr_bl *proto_bl = malloc(sizeof(*proto_bl));
    proto_bl->container = arr_create(2);
    
    lky_object *proto_blob = lobjb_build_blob(proto_bl, stlarr_bl_manage);

    CLASS_MAKE(cls, NULL, stlarr_init, 0,
        CLASS_PROTO("count", lobjb_build_int(0));
        CLASS_PROTO("ab_", proto_blob);
        CLASS_PROTO_METHOD("stringify_", stlarr_stringify, 0);
        CLASS_PROTO_METHOD("append", stlarr_append, 1);
        CLASS_PROTO_METHOD("get", stlarr_get, 1);
        CLASS_PROTO_METHOD("set", stlarr_set, 2);
        CLASS_PROTO_METHOD("op_get_index_", stlarr_get, 1);
        CLASS_PROTO_METHOD("op_set_index_", stlarr_set, 2);
        CLASS_PROTO_METHOD("forEach", stlarr_for_each, 1);
        CLASS_PROTO_METHOD("contains", stlarr_contains, 1);
        CLASS_PROTO_METHOD("indexOf", stlarr_index_of, 1);
        CLASS_PROTO_METHOD("removeAt", stlarr_remove_at, 1);
        CLASS_PROTO_METHOD("joined", stlarr_joined, 2);
        CLASS_PROTO_METHOD("insert", stlarr_insert, 2);
        CLASS_PROTO_METHOD("copy", stlarr_copy, 0);
        CLASS_PROTO_METHOD("map", stlarr_map, 1);
        CLASS_PROTO_METHOD("reduce", stlarr_reduce, 1);
        CLASS_PROTO_METHOD("size_", stlarr_size, 0);
        CLASS_STATIC_METHOD("memcpy", stlarr_memcpy, 3);
        CLASS_STATIC_METHOD("alloc", stlarr_alloc, 1);
    );

    stlarr_class_ = cls;
    return cls;
}

arraylist stlarr_unwrap(lky_object *a)
{
    lky_object_builtin *blob = (lky_object_builtin *)lobj_get_member(a, "ab_");
    stlarr_bl *bl = blob->value.b;
   
    return bl->container; 
}

arraylist *stlarr_get_store(lky_object *a)
{
    lky_object_builtin *blob = (lky_object_builtin *)lobj_get_member(a, "ab_");
    stlarr_bl *bl = blob->value.b;
   
    return &bl->container; 
}

lky_object *stlarr_get_proto()
{
    return lobj_get_member(stlarr_get_class(), "model_");
}

#endif
