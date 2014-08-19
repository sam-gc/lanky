#include "lky_gc.h"
#include "arraylist.h"
#include "gc_hashset.h"

void gc_mark();
void gc_collect();

typedef struct {
    gc_hashset pool;
    gc_hashset roots;
    arraylist root_stacks;
    size_t max_size;
    size_t cur_size;
    
    size_t growth_size;
    size_t marked_size;
} gc_bundle;

typedef struct {
    void **stack;
    int size;
} gc_stack;

gc_bundle bundle;
char gc_started = 0;

void gc_init()
{
    bundle.pool = gchs_create(8);
    bundle.roots = gchs_create(10);
    bundle.root_stacks = arr_create(10);
//    bundle.max_size = 16000000;
    bundle.growth_size = 5000;
    bundle.marked_size = 0;
    bundle.max_size = 5000;
    bundle.cur_size = 0;
    gc_started = 1;
}

void gc_add_root_object(lky_object *obj)
{
    if(!gc_started)
        return;

    gchs_add(&bundle.roots, obj);
    gchs_add(&bundle.pool, obj);
}

void gc_remove_root_object(lky_object *obj)
{
    gchs_remove(&bundle.roots, obj);
}

void gc_add_root_stack(void **stack, int size)
{
    if(!gc_started)
        return;

    gc_stack *st = malloc(sizeof(gc_stack));
    st->stack = stack;
    st->size = size;

    arr_append(&bundle.root_stacks, st);
}

void gc_remove_root_stack(void **stack)
{
    gc_stack *st = arr_get(&bundle.root_stacks, bundle.root_stacks.count - 1);
    free(st);
    arr_remove(&bundle.root_stacks, NULL, bundle.root_stacks.count - 1);
}

void gc_add_object(lky_object *obj)
{
    if(!gc_started || !obj)
        return;

    gchs_add(&bundle.pool, obj);
    bundle.cur_size += obj->size;
}

void gc_gc()
{
    if(bundle.cur_size < bundle.max_size)
        return;

    gc_mark();
    gc_collect();
    
    bundle.max_size = bundle.marked_size + bundle.growth_size;
}

void gc_collect()
{
    bundle.marked_size = 0;
    
    gc_hashset pool = bundle.pool;
    void **objs = gchs_to_list(&bundle.pool);

    int i;
    for(i = pool.count - 1; i >= 0; i--)
    {
        lky_object *o = objs[i];
        if(!o->mem_count)
        {
            bundle.cur_size -= o->size;
            // TODO: This will need to change.
            lobj_dealloc(o);
            gchs_remove(&bundle.pool, o);
        }
        else
        {
            bundle.marked_size += o->size;
            o->mem_count = 0;
        }
    }
    
    free(objs);
}

void gc_mark_object(lky_object *o)
{
    if(o->mem_count)
        return;

    o->mem_count = 1;

    trie_for_each(o->members, (trie_pointer_function)&gc_mark_object);

    switch(o->type)
    {
        case LBI_FUNCTION:
        {
            lky_object_function *func = (lky_object_function *)o;

            if(func->bucket)
                gc_mark_object(func->bucket);
            if(func->code)
                gc_mark_object((lky_object *)func->code);
            if(func->owner)
                gc_mark_object((lky_object *)func->owner);

            int i;
            for(i = 0; i < func->parent_stack.count; i++)
            {
                gc_mark_object(arr_get(&func->parent_stack, i));
            }
        }
        break;
        case LBI_SEQUENCE:
        {
            lky_object_seq *seq = (lky_object_seq *)o;
            if(seq->next)
                gc_mark_object((lky_object *)seq->next);
            if(seq->value)
                gc_mark_object((lky_object *)seq->value);
        }
        break;
        case LBI_CODE:
        {
            lky_object_code *code = (lky_object_code *)o;
            int i;
            for(i = 0; i < code->num_constants; i++)
                gc_mark_object(code->constants[i]);
        }
        break;
        case LBI_CLASS:
        {
            lky_object_class *cls = (lky_object_class *)o;
            gc_mark_object((lky_object *)cls->builder);
        }
        break;
        case LBI_CUSTOM_EX:
        {
            lky_object_custom *cu = (lky_object_custom *)o;
            if(!cu->savefunc)
                break;
            cu->savefunc(o);
        }
        break;
        default:
        break;
    }
}

char gc_mark_stack(gc_stack *st)
{
    void **stack = st->stack;
    int size = st->size;
    int i;
    for(i = 0; i < size; i++)
    {
        if(!stack[i])
            break;
        gc_mark_object(stack[i]);
    }
    
    return 1;
}

void gc_mark()
{
    gchs_for_each(&bundle.roots, (gchs_pointer_function)&gc_mark_object);
    arr_for_each(&bundle.root_stacks, (arr_pointer_function)&gc_mark_stack);

    // arr_for_each(&bundle.pool, (arr_pointer_function)&gc_reset_mark);
    // arr_for_each(&bundle.roots, (arr_pointer_function)&gc_mark_object_with_return);
    // arr_for_each(&bundle.root_stacks, (arr_pointer_function)&gc_mark_stack_with_return);
}
