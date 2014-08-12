#include "lky_gc.h"
#include "arraylist.h"

void gc_mark();
void gc_collect();

typedef struct {
    arraylist pool;
    arraylist roots;
    arraylist root_stacks;
    size_t max_size;
    size_t cur_size;
} gc_bundle;

typedef struct {
    void **stack;
    int size;
} gc_stack;

gc_bundle bundle;
char gc_started = 0;

void gc_init()
{
    bundle.pool = arr_create(1000000);
    bundle.roots = arr_create(10);
    bundle.root_stacks = arr_create(10);
    //bundle.max_size = 16000000;
    bundle.max_size = 5000;
    bundle.cur_size = 0;
    gc_started = 1;
}

void gc_add_root_object(lky_object *obj)
{
    if(!gc_started)
        return;

    arr_append(&bundle.roots, obj);
    arr_append(&bundle.pool, obj);
}

void gc_remove_root_object(lky_object *obj)
{
    arr_remove(&bundle.roots, obj, 0);
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

    arr_append(&bundle.pool, obj);
    bundle.cur_size += obj->size;
}

void gc_gc()
{
    if(bundle.cur_size < bundle.max_size)
        return;

    gc_mark();
    gc_collect();
}

void gc_collect()
{
    arraylist pool = bundle.pool;
    int i;
    for(i = pool.count - 1; i >= 0; i--)
    {
        lky_object *o = arr_get(&pool, i);
        if(!o->mem_count)
        {
            printf("-- %p (%d)\n", o, o->type);
            bundle.cur_size -= o->size;
            // TODO: This will need to change.
            lobj_dealloc(o);
            arr_remove(&pool, NULL, i);
        }
        else
            o->mem_count = 0;
    }

    bundle.pool = pool;
}

void gc_mark_object(lky_object *o)
{
    printf("%p (%d)\n", o, o->type);
    if(o->mem_count)
        return;

    o->mem_count = 1;
    printf("LSKDJFLSKDJFLKSDJFLKDSFJ\n");

    trie_for_each(o->members, (trie_pointer_function)&gc_mark_object);

    switch(o->type)
    {
        case LBI_FUNCTION:
        {
            lky_object_function *func = (lky_object_function *)o;

            if(func->bucket)
                gc_mark_object(func->bucket);
            gc_mark_object(func->code);

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
                gc_mark_object(seq->next);
            if(seq->value)
                gc_mark_object(seq->value);
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
    }
}

char gc_mark_object_with_return(lky_object *o)
{
    gc_mark_object(o);
    return 1;
}

char gc_mark_stack_with_return(gc_stack *st)
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

char gc_reset_mark(lky_object *o)
{
    if(!o)
        return 1;
    o->mem_count = 0;
    return 1;
}

void gc_mark()
{
    arr_for_each(&bundle.pool, (arr_pointer_function)&gc_reset_mark);
    arr_for_each(&bundle.roots, (arr_pointer_function)&gc_mark_object_with_return);
    arr_for_each(&bundle.root_stacks, (arr_pointer_function)&gc_mark_stack_with_return);
}
