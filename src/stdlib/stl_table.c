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

#include "stl_table.h"
#include "lky_gc.h"
#include "hashtable.h"
#include "mach_binary_ops.h"
#include "tools.h"
#include "stl_string.h"
#include "stl_array.h"

typedef struct stltab_data_s {
    hashtable ht;
} stltab_data;

struct list_info {
    int use_keys;
    arraylist *list;
};

long stltab_autohash(void *key, void *data)
{
    lky_object *k = (lky_object *)key;

    // If we are dealing with numbers, we can just use
    // the numbers themselves as the hash. (True, this
    // can be problematic, but for simplicity we'll
    // just hope the user is not hashing a bunch of
    // very small floating point numbers
    if(OBJ_IS_NUMBER(k))
        return (long)OBJ_NUM_UNWRAP(k);
    
    // Even though string has a hash function, we want
    // to quickly be able to perform this calculation
    if((void *)k->cls == (void *)stlstr_class())
        return hst_djb2(((lky_object_custom *)k)->data, NULL);

    lky_object *hf = lobj_get_member((lky_object *)key, "hash_");
    
    if(!hf)
        return (long)key; // Use the pointer as the hash; mirrors equals

    lky_object *obj = lobjb_call(hf, NULL, NULL);

    return (long)OBJ_NUM_UNWRAP(obj);

    /*char *str = lobjb_stringify((lky_object *)key);
    long val = hst_djb2(str, NULL);
    free(str);
    return val;*/
}

int stltab_autoequ(void *a, void *b)
{
    return (int)LKY_CTEST_FAST(lobjb_binary_equals((lky_object *)a, (lky_object *)b, NULL));
}

void stltab_cat_each(void *key, void *val, void *data)
{
    char **buf = (char **)data;
    lky_object *k = (lky_object *)key;
    lky_object *v = (lky_object *)val;

    char *sk = lobjb_stringify(k, NULL);
    char *sv = lobjb_stringify(v, NULL);

    auto_cat(buf, "   ");
    auto_cat(buf, sk);
    auto_cat(buf, " : ");
    auto_cat(buf, sv);
    auto_cat(buf, "\n");

    free(sk);
    free(sv);
}

void stltab_append_array(void *key, void *val, void *data)
{
    struct list_info *lifo = (struct list_info *)data;
    lky_object *touse = lifo->use_keys ? key : val;

    arr_append(lifo->list, touse);
}

lky_object *stltab_stringify(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    char *buf = NULL;
    auto_cat(&buf, "[\n");   

    hst_for_each(&d->ht, stltab_cat_each, &buf);

    auto_cat(&buf, "]");

    lky_object *ret = stlstr_cinit(buf);
    free(buf);

    return ret;
}

lky_object *stltab_keys(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    arraylist list = arr_create(d->ht.count + 1);
    struct list_info lifo;
    lifo.use_keys = 1;
    lifo.list = &list;

    hst_for_each(&d->ht, stltab_append_array, &lifo);

    return stlarr_cinit(list);
}

lky_object *stltab_values(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    arraylist list = arr_create(d->ht.count + 1);
    struct list_info lifo;
    lifo.use_keys = 0;
    lifo.list = &list;

    hst_for_each(&d->ht, stltab_append_array, &lifo);

    return stlarr_cinit(list);
}

lky_object *stltab_cget(lky_object *table, lky_object *key)
{
    lky_object_custom *tab = (lky_object_custom *)table;
    stltab_data *d = tab->data;
    
    return hst_get(&d->ht, key, stltab_autohash, stltab_autoequ);
}

void stltab_cput(lky_object *table, lky_object *key, lky_object *val)
{
    lky_object_custom *tab = (lky_object_custom *)table;
    stltab_data *d = tab->data;
    
    hst_put(&d->ht, key, val, stltab_autohash, stltab_autoequ);
}

lky_object *stltab_put(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    lky_object *k = (lky_object *)args->value;
    lky_object *v = (lky_object *)args->next->value;

    hst_put(&d->ht, k, v, stltab_autohash, stltab_autoequ);

    lobj_set_member((lky_object *)tab, "count", lobjb_build_int(d->ht.count));
    lobj_set_member((lky_object *)tab, "size_", lobjb_build_int(d->ht.size));

    return &lky_nil;
}

lky_object *stltab_add_all(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    lky_object *other = (lky_object *)args->value;
    if((void *)other->cls != (void *)tab->cls)
        return &lky_nil;

    stltab_data *o = ((lky_object_custom *)other)->data;

    hst_add_all_from(&d->ht, &o->ht, stltab_autohash, stltab_autoequ);
    lobj_set_member((lky_object *)tab, "count", lobjb_build_int(d->ht.count));
    lobj_set_member((lky_object *)tab, "size_", lobjb_build_int(d->ht.size));

    return &lky_nil;
}

lky_object *stltab_get(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    lky_object *k = (lky_object *)args->value;

    lky_object *ret = hst_get(&d->ht, k, stltab_autohash, stltab_autoequ);

    return ret ? ret : &lky_nil;
}

lky_object *stltab_remove(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    lky_object *k = (lky_object *)args->value;

    lky_object *ret = hst_remove_key(&d->ht, k, stltab_autohash, stltab_autoequ);
    lobj_set_member((lky_object *)tab, "count", lobjb_build_int(d->ht.count));

    return ret ? ret : &lky_nil;
}

lky_object *stltab_remove_value(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    lky_object *v = (lky_object *)args->value;

    hst_remove_val(&d->ht, v, stltab_autoequ);
    lobj_set_member((lky_object *)tab, "count", lobjb_build_int(d->ht.count));

    return &lky_nil;
}

lky_object *stltab_has_key(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    return lobjb_build_int(hst_contains_key(&d->ht, args->value, stltab_autohash, stltab_autoequ));
}

lky_object *stltab_has_value(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    return lobjb_build_int(hst_contains_value(&d->ht, args->value, stltab_autoequ));
}

void stltab_mark(void *key, void *val, void *data)
{
    lky_object *k = (lky_object *)key;
    lky_object *v = (lky_object *)val;

    gc_mark_object(k);
    gc_mark_object(v);
}

void stltab_dealloc(lky_object *obj)
{   
    lky_object_custom *c = (lky_object_custom *)obj;
    stltab_data *data = c->data;

    hst_free(&data->ht);
    free(data);
}

void stltab_save(lky_object *obj)
{
    lky_object_custom *c = (lky_object_custom *)obj;
    stltab_data *data = c->data;

    hst_for_each(&data->ht, stltab_mark, NULL);
}

/*lky_object *stlarr_get_class()
{
    if(stlarr_class)
        return stlarr_class;

    lky_object *clsobj = lobj_alloc();
    lky_callable c;
    c.argc = 0;
    c.function = (lky_function_ptr)stlarr_build;
    clsobj->callable = c;

    stlarr_class = clsobj;
    return clsobj;
}*/

lky_object *stltab_cinit(arraylist *keys, arraylist *vals)
{
    lky_object_custom *tab = lobjb_build_custom(sizeof(stltab_data));
    lky_object *obj = (lky_object *)tab;

    lky_object *getter = lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltab_get);
    lky_object *setter = lobjb_build_func_ex(obj, 2, (lky_function_ptr)stltab_put);

    lobj_set_member(obj, "put", setter);
    lobj_set_member(obj, "get", getter);
    lobj_set_member(obj, "hasKey", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltab_has_key));
    lobj_set_member(obj, "hasValue", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltab_has_value));
    lobj_set_member(obj, "keys", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stltab_keys));
    lobj_set_member(obj, "values", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stltab_values));
    lobj_set_member(obj, "remove", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltab_remove));
    lobj_set_member(obj, "removeValue", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltab_remove_value));
    lobj_set_member(obj, "addAll", lobjb_build_func_ex(obj, 1, (lky_function_ptr)stltab_add_all));
    lobj_set_member(obj, "op_get_index_", getter);
    lobj_set_member(obj, "op_set_index_", setter);
    //lobj_set_member(obj, "stringify_", lobjb_build_func_ex(obj, 0, (lky_function_ptr)stltab_stringify));

    tab->freefunc = stltab_dealloc;
    tab->savefunc = stltab_save;

    stltab_data *data = malloc(sizeof(stltab_data));
    hashtable ht = hst_create();

    if(!keys)
    {
        lobj_set_member(obj, "count", lobjb_build_int(0));
        lobj_set_member(obj, "size_", lobjb_build_int(8));
        tab->data = data;
        data->ht = ht;
        return obj;
    }

    int i;
    for(i = 0; i < keys->count; i++)
    {
        lky_object *k = keys->items[i];
        lky_object *v = vals->items[i];

        hst_put(&ht, k, v, stltab_autohash, stltab_autoequ);
    }

    tab->data = data;
    data->ht = ht;

    lobj_set_member(obj, "count", lobjb_build_int(ht.count));
    lobj_set_member(obj, "size_", lobjb_build_int(ht.size));

    lobj_set_class(obj, stltab_get_class());

    return obj;
}

lky_object *stltab_build(lky_object_seq *args, lky_object *caller)
{
    return stltab_cinit(NULL, NULL);
}

hashtable stltab_unwrap(lky_object *obj)
{
    return ((stltab_data *)((lky_object_custom *)obj)->data)->ht;
}

static lky_object *stltab_class = NULL;
lky_object *stltab_get_class()
{   
    if(stltab_class)
        return stltab_class;

    lky_object *clsobj = lobj_alloc();
    lky_callable c;
    c.argc = 0;
    c.function = (lky_function_ptr)stltab_build;
    clsobj->callable = c;

    stltab_class = clsobj;
    return clsobj;
}

