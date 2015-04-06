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

#include "lkyobj_builtin.h"
#include "lky_machine.h"
#include "lky_gc.h"
#include "stl_string.h"
#include "stl_object.h"
#include "stl_array.h"
#include "tools.h"
#include "aquarium.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

int lobjb_uses_pointer_tags_ = 1;

lky_object *lobjb_try_render_tagged_pointer(long value)
{
    if(!lobjb_uses_pointer_tags_)
        return NULL;

    uintptr_t p = 0;
    if(value < (long)0x00FFFFFF && value >= 0)
    {
        p = value << 8;
        p |= 1;
    }

    return (lky_object *)p;
}

lky_object *lobjb_alloc(lky_builtin_type t, lky_builtin_value v)
{
    if(t == LBI_INTEGER && v.i < 0x0FFFFFFF && v.i >= 0)
    {
        lky_object *attempt = lobjb_try_render_tagged_pointer(v.i);
        if(attempt) return attempt;
    }

    lky_object_builtin *obj = aqua_request_next_block(sizeof(lky_object_builtin));
    obj->type = t;
    obj->size = sizeof(lky_object_builtin);
    obj->mem_count = 0;
    obj->value = v;
    obj->on_gc = NULL;
    gc_add_object((lky_object *)obj);

    return (lky_object *)obj;
}

lky_object *lobjb_build_int(long value)
{
    lky_object *attempt = lobjb_try_render_tagged_pointer(value);
    if(attempt) return attempt;

    lky_builtin_value v;
    v.i = value;
    return lobjb_alloc(LBI_INTEGER, v);
}

lky_object *lobjb_build_float(double value)
{
    lky_builtin_value v;
    v.d = value;
    return lobjb_alloc(LBI_FLOAT, v);
}

lky_object *lobjb_build_blob(void *ptr, lobjb_void_ptr_function gc)
{
    lky_builtin_value v;
    v.b = ptr;
    lky_object_builtin *b = (lky_object_builtin *)lobjb_alloc(LBI_BLOB, v);
    
    b->on_gc = gc;
    return (lky_object *)b;
}

lky_object *lobjb_error_print(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_error *err = (lky_object_error *)func->owner;

    printf("%s: %s\n", err->name, err->text);

    return &lky_nil;
}

lky_object *lobjb_error_stringify(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);

    lky_object_error *err = (lky_object_error *)func->owner;
    char text[strlen(err->name) + strlen(err->text) + 5];
    sprintf(text, "%s: %s", err->name, err->text);

    return stlstr_cinit(text);
}

void lobjb_error_free(lky_object *o)
{
    lky_object_error *err = (lky_object_error *)o;

    free(err->name);
    free(err->text);
}

lky_object *lobjb_build_error(char *name, char *text)
{
    lky_object_error *err = aqua_request_next_block(sizeof(lky_object_error));
    err->type = LBI_ERROR;
    err->size = sizeof(lky_object_error);
    err->mem_count = 0;
    err->members = hst_create();
    err->members.duplicate_keys = 1;

    char *name_ = malloc(strlen(name) + 1);
    char *text_ = malloc(strlen(text) + 1);

    strcpy(name_, name);
    strcpy(text_, text);

    err->name = name_;
    err->text = text_;
    err->cls = NULL;

    lobj_set_member((lky_object *)err, "name", stlstr_cinit(name));
    lobj_set_member((lky_object *)err, "text", stlstr_cinit(text));
    lobj_set_member((lky_object *)err, "print", lobjb_build_func_ex((lky_object *)err, 0, (lky_function_ptr)lobjb_error_print));
    lobj_set_member((lky_object *)err, "stringify_", lobjb_build_func_ex((lky_object *)err, 0, (lky_function_ptr)lobjb_error_stringify));

    gc_add_object((lky_object *)err);

    return (lky_object *)err;
}

lky_object *lobjb_make_exception(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);
    struct interp *in = BUW_INTERP(bundle);

    lky_object *first = (lky_object *)args->value;
    lky_object *second = (lky_object *)args->next->value;

    char *name = lobjb_stringify(first, in);
    char *text = lobjb_stringify(second, in);

    lky_object *ret = lobjb_build_error(name, text);
    free(name);
    free(text);

    return ret;
}

lky_object *lobjb_get_exception_class()
{
    return lobjb_build_func_ex(NULL, 2, (lky_function_ptr)lobjb_make_exception);
}

lky_object *lobjb_build_iterable(lky_object *owner, struct interp *interp)
{
    lky_object_iterable *it = aqua_request_next_block(sizeof(lky_object_iterable));
    it->type = LBI_ITERABLE;
    it->size = sizeof(lky_object_iterable);
    it->mem_count = 0;

    if(!lobj_is_of_class(owner, stlarr_get_class()))
    {
        if(owner->type == LBI_CUSTOM_EX || owner->type == LBI_CUSTOM)
        {
            lky_object *func = lobj_get_member(owner, "iterable_");
            if(!func)
                return NULL;

            owner = lobjb_call(func, NULL, interp);
        }
        else
        {
            return NULL;
        }
    }

    it->index = 0;
    it->store = stlarr_get_store(owner);
    it->owner = owner;

    gc_add_object((lky_object *)it);

    return (lky_object *)it;
}

lky_object_custom *lobjb_build_custom(size_t extra_size)
{
    lky_object_custom *obj = aqua_request_next_block(sizeof(lky_object_custom));
    obj->type = LBI_CUSTOM_EX;
    obj->size = sizeof(lky_object_custom) + extra_size;
    obj->mem_count = 0;
    obj->members = hst_create();
    obj->members.duplicate_keys = 1;
    obj->data = NULL;
    obj->freefunc = NULL;
    obj->savefunc = NULL;
    obj->cls = NULL;

    stlobj_seed((lky_object *)obj);

    gc_add_object((lky_object *)obj);

    return obj;
}

lky_object *lobjb_bind_func(lky_func_bundle *bundle)
{
    lky_object_seq *args = BUW_ARGS(bundle);

    lky_object_function *func = (lky_object_function *)BUW_FUNC(bundle)->owner;

    lky_object *obj = (lky_object *)args->value;
    func->bound = obj;

    return (lky_object *)func;
}

lky_object *lobjb_build_func(lky_object_code *code, int argc, arraylist inherited, mach_interp *interp)
{
    lky_object_function *func = aqua_request_next_block(sizeof(lky_object_function));
    func->type = LBI_FUNCTION;
    func->mem_count = 0;
    func->size = sizeof(lky_object_function);
    func->members = hst_create();
    func->members.duplicate_keys = 1;
    
    func->code = code;
    func->bucket = NULL;
    func->owner = NULL;
    func->bound = NULL;
    func->refname = code->refname;

    func->interp = interp;

    func->parent_stack = inherited;

    gc_add_object((lky_object *)func);

    lky_callable c;
    c.function = (lky_function_ptr)&lobjb_default_callable;
    c.argc = argc;
    func->callable = c;

    // Add some members to the function argument
    lobj_set_member((lky_object *)func, "argc", (lky_object *)lobjb_build_int(argc)); 
    lobj_set_member((lky_object *)func, "code_", (lky_object *)code);
    lobj_set_member((lky_object *)func, "bind", lobjb_build_func_ex((lky_object *)func, 1, (lky_function_ptr)lobjb_bind_func));
    
    return (lky_object *)func;
}

lky_object *lobjb_build_func_ex(lky_object *owner, int argc, lky_function_ptr ptr)
{
    lky_object_function *func = aqua_request_next_block(sizeof(lky_object_function));
    func->type = LBI_FUNCTION;
    func->mem_count = 0;
    func->size = sizeof(lky_object_function);
    func->members = hst_create();
    func->members.duplicate_keys = 1;
    func->owner = NULL;
    func->bound = NULL;
    func->refname = NULL;
    
    func->code = NULL;
    func->bucket = NULL;
    func->refname = NULL;

    func->parent_stack = arr_create(1);

    gc_add_object((lky_object *)func);

    lky_callable c;
    c.function = ptr;
    c.argc = argc;

    func->callable = c;
    func->owner = owner;

    lobj_set_member((lky_object *)func, "argc", (lky_object *)lobjb_build_int(argc)); 
    
    return (lky_object *)func;
}

char *lobjb_stringify(lky_object *a, struct interp *interp)
{
    char *ret = NULL;

    if((uintptr_t)(a) & 1)
    {
        long val = OBJ_NUM_UNWRAP(a);
        ret = malloc(100);
        sprintf(ret, "%ld", val);
        return ret;
    }

    lky_object_builtin *b = (lky_object_builtin *)a;

    switch(b->type)
    {
        case LBI_FLOAT:
            ret = malloc(100);
            if(b->value.d < 0.0001 && b->value.d != 0)
                sprintf(ret, "%e", b->value.d);
            else
                sprintf(ret, "%lf", b->value.d);
        break;
        case LBI_INTEGER:
            ret = malloc(100);
            sprintf(ret, "%ld", b->value.i);
        break;
        case LBI_BOOL:
            ret = malloc(4);
            sprintf(ret, "%s", a == &lky_yes ? "yes" : "no");
        break;
        case LBI_NIL:
            ret = malloc(6);
            strcpy(ret, "(nil)");
        break;
        case LBI_CUSTOM:
        case LBI_CUSTOM_EX:
        case LBI_ERROR:
        {
            lky_object_function *func = (lky_object_function *)lobj_get_member(a, "stringify_");
            
            if(!func)
            {
                ret = malloc(100);
                sprintf(ret, "%p", b);
                break;
            }
            lky_func_bundle b = MAKE_BUNDLE(func, NULL, interp);
            lky_object_custom *s = (lky_object_custom *)(func->callable.function)(&b);

            ret = malloc(strlen(s->data) + 1);
            strcpy(ret, s->data);
            break;
        }
        case LBI_CLASS:
            ret = malloc(100);
            sprintf(ret, "(lky_object_class | %p)", a);
            break;
        case LBI_FUNCTION:
            if(((lky_object_function *)a)->code == NULL) // We are dealing with a native function
            {
                ret = malloc(150);
                sprintf(ret, "(lky_object_function::native | %p)", a);
                break;
            }
            ret = malloc(100);
            sprintf(ret, "(lky_object_function | %p)", a);
            break;
        case LBI_CODE:
        {
            lky_object_code *code = (lky_object_code *)a;
            ret = NULL;

            int i;
            char ch[100];
            for(i = 0; i < code->op_len; i++)
            {
                sprintf(ch, "\\0x%X", code->ops[i]);
                auto_cat(&ret, ch);
            }

            break;           
        }
        default:
            ret = malloc(100);
            sprintf(ret, "%p", b);
            break;

    }
    
    return ret;
}

void str_print(lky_builtin_type t, lky_builtin_value v, char *buf)
{
    switch(t)
    {
        case LBI_FLOAT:
            sprintf(buf, "%.40lf", v.d);
        break;
        case LBI_INTEGER:
            sprintf(buf, "%ld", v.i);
        break;
        default:
        break;
    }
}

lky_object *lobjb_num_to_string(lky_object *a)
{
    lky_object_builtin *b = (lky_object_builtin *)a;
    char str[100];

    if((uintptr_t)(a) & 1)
        sprintf(str, "%ld", (long)OBJ_NUM_UNWRAP(a));
    else
        str_print(b->type, b->value, str);
    
    return stlstr_cinit(str);
}

lky_object *lobjb_call(lky_object *func, lky_object_seq *args, struct interp *interp)
{
    if(func->type != LBI_FUNCTION && func->type != LBI_CLASS && func->type != LBI_CUSTOM_EX && func->type != LBI_CUSTOM)
        return NULL;

    lky_callable c;
    switch(func->type)
    {
        case LBI_FUNCTION:
        case LBI_CLASS:
            c = ((lky_object_function *)func)->callable;
            break;
        case LBI_CUSTOM:
        case LBI_CUSTOM_EX:
            c = func->callable;
            break;
        default:
            return NULL;
    }
    
    lky_func_bundle b = MAKE_BUNDLE(func, args, interp);
    return (lky_object *)c.function(&b);
}

lky_object *lobjb_default_callable(lky_func_bundle *bundle)
{
    lky_object_function *func = BUW_FUNC(bundle);
    lky_object_seq *args = BUW_ARGS(bundle);
    lky_object_code *code = func->code;

    func->bucket = lobj_alloc();

    long i;
    for(i = 0; args && i < func->callable.argc; i++, args = args->next)
    {
        char *name = code->names[i];
        lobj_set_member(func->bucket, name, (lky_object *)args->value);
    }

    if(func->bound && func->refname)
    {
        lobj_set_member(func->bucket, func->refname, func->bound);
        func->bound = NULL;
    }

    for(; i < func->callable.argc; i++)
    {
        char *name = code->names[i];
        lobj_set_member(func->bucket, name, &lky_nil);
    }

    char needs_va_args = 0;
    arraylist list = arr_create(10);
    for(; args; args = args->next)
    {
        needs_va_args = 1;
        arr_append(&list, args->value);
    }

    if(needs_va_args)
        lobj_set_member(func->bucket, "va_args_", stlarr_cinit(list));
    else
    {
        lobj_set_member(func->bucket, "va_args_", &lky_nil);
        arr_free(&list);
    }

    lky_object *ret = mach_execute(func);

    return ret;
}

lky_object *lobjb_unary_load_index(lky_object *obj, lky_object *indexer, struct interp *interp)
{
    lky_object_function *func = (lky_object_function *)lobj_get_member(obj, "op_get_index_");

    if(!func)
    {
        mach_halt_with_err(lobjb_build_error("MismatchedType", "The given type cannot be indexed."));
        return &lky_nil;
    }

    lky_func_bundle b = MAKE_BUNDLE(func, lobjb_make_seq_node(indexer), interp);
    lky_object *ret = (lky_object *)func->callable.function(&b);

    return ret;
}

lky_object *lobjb_unary_save_index(lky_object *obj, lky_object *indexer, lky_object *newobj, struct interp *interp)
{
    lky_object_function *func = (lky_object_function *)lobj_get_member(obj, "op_set_index_");

    if(!func)
    {
        mach_halt_with_err(lobjb_build_error("MismatchedType", "The given type cannot be indexed."));
        return &lky_nil;
    }

    lky_object_seq *args = lobjb_make_seq_node(indexer);
    args->next = lobjb_make_seq_node(newobj);

    lky_func_bundle b = MAKE_BUNDLE(func, args, interp);
    lky_object *ret = (lky_object *)func->callable.function(&b);

    return ret;
}

lky_object *lobjb_unary_negative(lky_object *obj)
{
    if((uintptr_t)(obj) & 1)
        return lobjb_build_int(-(OBJ_NUM_UNWRAP(obj)));
    if(obj->type != LBI_FLOAT && obj->type != LBI_INTEGER)
        return &lky_nil;

    switch(obj->type)
    {
        case LBI_FLOAT:
            return lobjb_build_float(-OBJ_NUM_UNWRAP(obj));
        case LBI_INTEGER:
            return lobjb_build_int(-OBJ_NUM_UNWRAP(obj));
        default: break;
    }

    return NULL;
}

lky_object *lobjb_iterable_get_next(lky_object *obj)
{
    lky_object_iterable *it = (lky_object_iterable *)obj;
    if(it->type != LBI_ITERABLE)
    {
        // TODO: Error
        return NULL;
    }

    arraylist *store = it->store;
    int idx = ++it->index;

    if(idx < store->count)
        return store->items[idx];

    // NULL is used to indicate end of iteration.
    return NULL;
}

char lobjb_quick_compare(lky_object *a, lky_object *b)
{
    if(((uintptr_t)(a) & 1) || ((uintptr_t)(b) & 1))
        return a == b;

    BI_CAST(a, ab);
    BI_CAST(b, bb);

    if((void *)a->cls == (void *)stlstr_class() || (void *)b->cls == (void *)stlstr_class())
    {
        if((void *)a->cls != (void *)stlstr_class() || (void *)b->cls != (void *)stlstr_class())
            return 0;

        lky_object_custom *ac = (lky_object_custom *)a;
        lky_object_custom *bc = (lky_object_custom *)b;
        return !strcmp((char *)bc->data, (char *)ac->data);
    }

    if(a == &lky_nil || b == &lky_nil)
    {
        return a == b;
    }

    if(ab->type == LBI_CODE || bb->type == LBI_CODE)
        return ab == bb;

    return OBJ_NUM_UNWRAP(ab) == OBJ_NUM_UNWRAP(bb);
}

void lobjb_print_object(lky_object *a, struct interp *interp)
{
    
    char *txt = lobjb_stringify(a, interp);
    printf("%s", txt);
    free(txt);
}

void lobjb_print(lky_object *a, struct interp *interp)
{
    lobjb_print_object(a, interp);
    printf("\n");
}

lky_object_seq *lobjb_make_seq_node(lky_object *value)
{
    lky_object_seq *seq = aqua_request_next_block(sizeof(lky_object_seq));
    seq->type = LBI_SEQUENCE;
    seq->mem_count = 0;
    seq->size = sizeof(lky_object_seq);
    gc_add_object((lky_object *)seq);

    seq->value = (struct lky_object *)value;
    seq->next = NULL;
    return seq;
}

void lobjb_free_seq(lky_object_seq *seq)
{
    while(seq)
    {
        lky_object_seq *next = seq->next;
        seq = next;
    }
}

lky_object_seq *lobjb_build_args(lky_object *arg, ...)
{
    lky_object *cur = arg;

    if(!arg)
        return NULL;

    lky_object_seq *head = lobjb_make_seq_node(arg);
    lky_object_seq *last = head;

    va_list ap;
    va_start(ap, arg);
    while((cur = va_arg(ap, lky_object *)))
    {
        last->next = lobjb_make_seq_node(cur);
        last = last->next;
    }
    va_end(ap);

    return head;
}

lky_object *lobjb_test(lky_object *cond)
{
    if(!cond)
        return &lky_no;

    if(OBJ_IS_NUMBER(cond))
        return !!OBJ_NUM_UNWRAP(cond) ? &lky_yes : &lky_no;

    if(cond == &lky_nil)
        return &lky_no;

    if(cond->type == LBI_BOOL)
        return cond;

    return !!cond ? &lky_yes : &lky_no;
}

int lobjb_ctest(lky_object *cond)
{
    if(!cond)
        return 0;

    if(OBJ_IS_NUMBER(cond))
        return !!OBJ_NUM_UNWRAP(cond);

    if(cond == &lky_nil)
        return 0;

    if(cond->type == LBI_BOOL)
        return cond == &lky_yes;

    return 1;
}

void lobjb_clean(lky_object *a)
{
    lky_object_builtin *obj = (lky_object_builtin *)a;

    switch(obj->type)
    {
        case LBI_FUNCTION:
            arr_free(&((lky_object_function *)a)->parent_stack);
        break;
        case LBI_ERROR:
            lobjb_error_free(a);
        break;
        case LBI_BLOB:
            if(obj->on_gc)
                obj->on_gc(obj->value.b, CGC_FREE);
            break;
        default:
        break;
    }
}
