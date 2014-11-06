#include "stl_table.h"
#include "lky_gc.h"
#include "hashtable.h"
#include "mach_binary_ops.h"

typedef struct stltab_data_s {
    hashtable ht;
} stltab_data;

long stltab_autohash(void *key, void *data)
{
    char *str = lobjb_stringify((lky_object *)key);
    long val = hst_djb2(str, NULL);
    free(str);
    return val;
}

int stltab_autoequ(void *a, void *b)
{
    return (int)OBJ_NUM_UNWRAP(lobjb_binary_equals((lky_object *)a, (lky_object *)b));
}

lky_object *stltab_put(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    lky_object *k = (lky_object *)args->value;
    lky_object *v = (lky_object *)args->next->value;

    hst_put(&d->ht, k, v, stltab_autohash, stltab_autoequ);

    lobj_set_member((lky_object *)tab, "count", lobjb_build_int(d->ht.count));
    lobj_set_member((lky_object *)tab, "size_", lobjb_build_int(d->ht.size));

    return &lky_nil;
}

lky_object *stltab_get(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *tab = (lky_object_custom *)func->owner;
    stltab_data *d = tab->data;

    lky_object *k = (lky_object *)args->value;

    lky_object *ret = hst_get(&d->ht, k, stltab_autohash, stltab_autoequ);

    return ret ? ret : &lky_nil;
}

void stltab_mark(void *key, void *val)
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

    hst_for_each(&data->ht, stltab_mark);
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
    lobj_set_member(obj, "op_get_index_", getter);
    lobj_set_member(obj, "op_set_index_", setter);

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

    return obj;
}

lky_object *stltab_build(lky_object_seq *args, lky_object *caller)
{
    return stltab_cinit(NULL, NULL);
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

