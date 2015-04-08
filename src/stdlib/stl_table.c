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
#include "class_builder.h"

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
    if(lobj_is_of_class(k, stlstr_get_class()))
        return hst_djb2(stlstr_unwrap(k), NULL);

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

void stltab_mark(void *key, void *val, void *data)
{
    lky_object *k = (lky_object *)key;
    lky_object *v = (lky_object *)val;

    gc_mark_object(k);
    gc_mark_object(v);
}

CLASS_MAKE_BLOB_FUNCTION(stltab_blob_func, stltab_data *, data, how,
    if(how == CGC_FREE)
    {
        hst_free(&data->ht);
        free(data);
        return;
    }

    hst_for_each(&data->ht, stltab_mark, NULL);
)

void stltab_common_init(lky_object *obj, arraylist *keys, arraylist *vals)
{
    stltab_data *data = malloc(sizeof(stltab_data));
    hashtable ht = hst_create();

    if(!keys)
    {
        lobj_set_member(obj, "count", lobjb_build_int(0));
        lobj_set_member(obj, "size_", lobjb_build_int(8));
        data->ht = ht;

        CLASS_SET_BLOB(obj, "hb_", data, stltab_blob_func);
        return;
    }

    int i;
    for(i = 0; i < keys->count; i++)
    {
        lky_object *k = keys->items[i];
        lky_object *v = vals->items[i];

        hst_put(&ht, k, v, stltab_autohash, stltab_autoequ);
    }

    CLASS_SET_BLOB(obj, "hb_", data, stltab_blob_func);
    data->ht = ht;

    lobj_set_member(obj, "count", lobjb_build_int(ht.count));
    lobj_set_member(obj, "size_", lobjb_build_int(ht.size));
}

CLASS_MAKE_INIT(stltab_init,
    arraylist *keys = $1 ? stlarr_get_store($1) : NULL;
    arraylist *vals = $2 ? stlarr_get_store($2) : NULL;

    stltab_common_init(self_, keys, vals);
)

void stltab_manual_init(lky_object *nobj, lky_object *cls, void *data)
{
    arraylist **lists = (arraylist **)data;
    stltab_common_init(nobj, lists[0], lists[1]);
}

lky_object *stltab_cinit(arraylist *keys, arraylist *vals)
{
    arraylist *lists[] = {keys, vals};
    return clb_instantiate(stltab_get_class(), stltab_manual_init, lists);
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

CLASS_MAKE_METHOD_EX(stltab_keys, self, stltab_data *, hb_,
    stltab_data *d = hb_;

    arraylist list = arr_create(d->ht.count + 1);
    struct list_info lifo;
    lifo.use_keys = 1;
    lifo.list = &list;

    hst_for_each(&d->ht, stltab_append_array, &lifo);

    return stlarr_cinit(list);
)

CLASS_MAKE_METHOD_EX(stltab_values, self, stltab_data *, hb_,
    stltab_data *d = hb_;

    arraylist list = arr_create(d->ht.count + 1);
    struct list_info lifo;
    lifo.use_keys = 0;
    lifo.list = &list;

    hst_for_each(&d->ht, stltab_append_array, &lifo);

    return stlarr_cinit(list);
)

lky_object *stltab_cget(lky_object *table, lky_object *key)
{
    stltab_data *d = CLASS_GET_BLOB(table, "hb_", stltab_data *);
    
    return hst_get(&d->ht, key, stltab_autohash, stltab_autoequ);
}

void stltab_cput(lky_object *table, lky_object *key, lky_object *val)
{
    stltab_data *d = CLASS_GET_BLOB(table, "hb_", stltab_data *);
    
    hst_put(&d->ht, key, val, stltab_autohash, stltab_autoequ);
}

CLASS_MAKE_METHOD_EX(stltab_put, self, stltab_data *, hb_,
    stltab_data *d = hb_;

    lky_object *k = $1;
    lky_object *v = $2;

    hst_put(&d->ht, k, v, stltab_autohash, stltab_autoequ);

    lobj_set_member(self, "count", lobjb_build_int(d->ht.count));
    lobj_set_member(self, "size_", lobjb_build_int(d->ht.size));
)

CLASS_MAKE_METHOD_EX(stltab_add_all, self, stltab_data *, hb_,
    stltab_data *d = hb_;

    lky_object *other = $1;
    if(!lobj_is_of_class(other, stltab_get_class()))
        return &lky_nil;

    stltab_data *o = CLASS_GET_BLOB(other, "hb_", stltab_data *);

    hst_add_all_from(&d->ht, &o->ht, stltab_autohash, stltab_autoequ);
    lobj_set_member(self, "count", lobjb_build_int(d->ht.count));
    lobj_set_member(self, "size_", lobjb_build_int(d->ht.size));

    return &lky_nil;
)

CLASS_MAKE_METHOD_EX(stltab_get, self, stltab_data *, hb_,
    stltab_data *d = hb_;

    lky_object *k = $1;

    lky_object *ret = hst_get(&d->ht, k, stltab_autohash, stltab_autoequ);

    return ret ? ret : &lky_nil;
)

CLASS_MAKE_METHOD_EX(stltab_remove, self, stltab_data *, hb_,
    stltab_data *d = hb_;

    lky_object *k = $1;

    lky_object *ret = hst_remove_key(&d->ht, k, stltab_autohash, stltab_autoequ);
    lobj_set_member(self, "count", lobjb_build_int(d->ht.count));

    return ret ? ret : &lky_nil;
)

CLASS_MAKE_METHOD_EX(stltab_remove_value, self, stltab_data *, hb_,
    stltab_data *d = hb_;

    lky_object *v = $1;

    hst_remove_val(&d->ht, v, stltab_autoequ);
    lobj_set_member(self, "count", lobjb_build_int(d->ht.count));
)

CLASS_MAKE_METHOD_EX(stltab_has_key, self, stltab_data *, hb_,
    return LKY_TESTC_FAST(hst_contains_key(&hb_->ht, $1, stltab_autohash, stltab_autoequ));
)

CLASS_MAKE_METHOD_EX(stltab_has_value, self, stltab_data *, hb_,
    return LKY_TESTC_FAST(hst_contains_value(&hb_->ht, $1, stltab_autoequ));
)

hashtable stltab_unwrap(lky_object *obj)
{
    return (CLASS_GET_BLOB(obj, "hb_", stltab_data *))->ht;
}

static lky_object *stltab_class_ = NULL;
lky_object *stltab_get_class()
{   
    if(stltab_class_)
        return stltab_class_;

    CLASS_MAKE(cls, NULL, stltab_init, 2,
        CLASS_PROTO("count", lobjb_build_int(-1));
        CLASS_PROTO("size_", lobjb_build_int(-1));
        CLASS_PROTO_METHOD("put", stltab_put, 2);
        CLASS_PROTO_METHOD("get", stltab_get, 1);
        CLASS_PROTO_METHOD("hasKey", stltab_has_key, 1);
        CLASS_PROTO_METHOD("hasValue", stltab_has_value, 1);
        CLASS_PROTO_METHOD("keys", stltab_keys, 0);
        CLASS_PROTO_METHOD("values", stltab_values, 0);
        CLASS_PROTO_METHOD("removeValue", stltab_remove_value, 1);
        CLASS_PROTO_METHOD("remove", stltab_remove, 1);
        CLASS_PROTO_METHOD("addAll", stltab_add_all, 1);
        CLASS_PROTO_METHOD("op_get_index_", stltab_get, 1);
        CLASS_PROTO_METHOD("op_set_index_", stltab_put, 2);
    );

    stltab_class_ = cls;
    return cls;
}

