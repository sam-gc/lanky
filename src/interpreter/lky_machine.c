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

// lky_machine.c
// ===================================
//
// The guts of the virtual machine are kept in this file. In the end, the lanky
// interpreter boils down to a giant switch statement below. Each of the
// opcodes is covered in this switch (the list of instructions can be seen in
// the "instruction_set.h" header file). The virtual machine is stack-based (as
// opposed to the register-based machine on which you are probably reading this
// document) and as such has one data stack that the machine uses to keep track
// of state. The machine reads the tap character by character and decides what
// to do. A C stack frame is pushed every time a function is called, allowing
// "native code" (i.e. the C api) to interact with lanky code as a standard
// object. The interpretation unit has reached a rough stage of completion and
// should be relatively stable.

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"
#include "mach_binary_ops.h"
#include "mach_unary_ops.h"
#include "lky_machine.h"
#include "lky_object.h"
#include "lky_gc.h"
#include "stl_array.h"
#include "stl_table.h"
#include "hashmap.h"
#include "module.h"

// Macros to abstract the notion of "pushing"
// and "popping" from the state machine
#define PUSH(data) (push_node(frame, data))
#define POP() (pop_node(frame))
#define TOP() (top_node(frame))
#define SECOND_TOP() (frame->data_stack[frame->stack_pointer - 1])

#define POP_TWO() lky_object *a = POP(); lky_object *b = POP()

void mach_eval(stackframe *frame);

int pushes = 0;
lky_object_error *thrown_exception = NULL;

void push_node(stackframe *frame, void *data)
{
    if(frame->stack_pointer >= frame->stack_size)
    {
        // WE HAVE HAD A STACK OVERFLOW
        // TODO: Make this an error.
        
        printf("Stack Overflow! Exiting.\n");
        exit(0);
    }
    frame->data_stack[++frame->stack_pointer] = data;
    pushes++;
}

void *top_node(stackframe *frame)
{
    return frame->data_stack[frame->stack_pointer];
}

void *pop_node(stackframe *frame)
{
    if(frame->stack_pointer < 0)
    {
        int *test = NULL;
    }
    void *data = frame->data_stack[frame->stack_pointer];
    frame->data_stack[frame->stack_pointer] = NULL;
    frame->stack_pointer--;

    return data;
}

void mach_halt_with_err(lky_object *err)
{
    lky_object_error *error = (lky_object_error *)err;
    thrown_exception = err;
}

lky_object *mach_interrupt_exec(lky_object_function *func)
{
    mach_interp *interp = func->interp;
    
    lky_object_code *code = func->code;
    stackframe *frame = malloc(sizeof(stackframe));
    
    stackframe *curr = interp->stack;
    
    frame->parent_stack = curr->parent_stack;
    frame->bucket = curr->bucket;
    frame->constants = code->constants;
    frame->locals = code->locals;
    frame->pc = -1;
    frame->ops = code->ops;
    frame->tape_len = code->op_len;
    frame->stack_pointer = -1;
    frame->stack_size = code->stack_size;
    frame->names = code->names;
    frame->ret = NULL;
    frame->interp = interp;
    frame->locals_count = code->num_locals;
    
    frame->prev = interp->stack ? interp->stack : NULL;
    frame->next = NULL;
    
    func->parent_stack = frame->parent_stack;
    
    if(interp->stack)
        interp->stack->next = frame;
    else
        gc_add_func_stack(frame);
    
    interp->stack = frame;
    
    void *stack[code->stack_size];
    memset(stack, 0, sizeof(void *) * code->stack_size);
    
    int catch_stack[code->catch_size];
    memset(catch_stack, 0, sizeof(int) * code->catch_size);

    frame->data_stack = stack;
    frame->catch_stack = catch_stack;
    func->bucket = frame->bucket;
    
    gc_add_root_object((lky_object *)func);
    
    mach_eval(frame);
    
    gc_remove_root_object((lky_object *)func);
    
    func->bucket = NULL;
    func->parent_stack = arr_create(1);
    
    // Pop the stackframe.
    interp->stack = interp->stack->prev;
    if(interp->stack)
        interp->stack->next = NULL;
    
    lky_object *ret = &lky_nil;
    if(frame->stack_pointer > -1)
        ret = frame->data_stack[frame->stack_pointer];
    
    free(frame);
    
    return ret;
}

lky_object *mach_execute(lky_object_function *func)
{
    mach_interp *interp = func->interp;
    
    lky_object_code *code = func->code;
    stackframe *frame = malloc(sizeof(stackframe));
    frame->parent_stack = func->parent_stack;
    if(func->bucket)
        frame->bucket = func->bucket;
    else
        frame->bucket = lobj_alloc();
    frame->constants = code->constants;
    frame->locals = code->locals;
    frame->pc = -1;
    frame->ops = code->ops;
    frame->tape_len = code->op_len;
    frame->stack_pointer = -1;
    frame->stack_size = code->stack_size;
    frame->names = code->names;
    frame->ret = NULL;
    
    frame->interp = interp;
    frame->locals_count = code->num_locals;
    
    // Setup stackframe with the previous stack
    frame->prev = interp->stack ? interp->stack : NULL;
    frame->next = NULL;
    
    if(interp->stack)
        interp->stack->next = frame;
    else
        gc_add_func_stack(frame);
    
    interp->stack = frame;

    void *stack[code->stack_size];
    memset(stack, 0, sizeof(void *) * code->stack_size);
    
    int catch_stack[code->catch_size];
    memset(catch_stack, 0, sizeof(int) * code->catch_size);

    frame->data_stack = stack;
    frame->catch_stack = catch_stack;

    frame->catch_pointer = 0;

    func->bucket = frame->bucket;

    gc_add_root_object((lky_object *)func);

    mach_eval(frame);


    gc_remove_root_object((lky_object *)func);
    func->bucket = NULL;
    
    // Pop the stackframe.
    interp->stack = interp->stack->prev;
    if(interp->stack)
        interp->stack->next = NULL;

    lky_object *ret = frame->ret;
    free(frame);
    return ret;
}

void print_stack(stackframe *frame)
{
    printf("printing frame\n");
    int i = 0;
    for(i = 0; i <= frame->stack_pointer; i++)
    {
        lobjb_print(frame->data_stack[i]);
    }
}

void mach_eval(stackframe *frame)
{
    lky_instruction op;
    
_opcode_whiplash_:
    if(frame->pc >= frame->tape_len || frame->ret)
        return;
    if(thrown_exception)
    {
        lky_object_error *exc = thrown_exception;
        thrown_exception = NULL;

        if(!frame->catch_pointer && !frame->prev)
        {
            printf("Fatal error: %s\nMessage: %s\n\nHalting.\n", exc->name, exc->text);
            frame->ret = &lky_nil;
            return;
        }
        else if(!frame->catch_pointer)
        {
            frame->prev->thrown = exc;
            return;
        }

        PUSH(exc);
        frame->pc = frame->catch_stack[--frame->catch_pointer];
    }

    gc_gc();

    switch((op = frame->ops[++frame->pc]))
    {
        case LI_LOAD_CONST:
        {
            char idx = frame->ops[++frame->pc];
            lky_object *obj = frame->constants[idx];
            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_ADD:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_add(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_SUBTRACT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_subtract(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_MULTIPLY:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_multiply(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_DIVIDE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_divide(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_MODULO:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_modulo(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_POWER:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_power(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_LT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_lessthan(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_GT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_greaterthan(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_LTE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_lessequal(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_GTE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_greatequal(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_EQUAL:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_equals(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_NE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_notequal(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_AND:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_and(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_OR:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_or(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_NC:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_nc(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_BAND:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_band(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_BOR:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_bor(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_BXOR:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_bxor(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_BLSHIFT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_blshift(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_BRSHIFT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_brshift(b, a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_UNARY_NOT:
        {
            lky_object *a = POP();
            lky_object *obj = lobjb_unary_not(a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_UNARY_NEGATIVE:
        {
            lky_object *a = POP();
            lky_object *obj = lobjb_unary_negative(a);

            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_PRINT:
        {
            lky_object *a = POP();
            lobjb_print(a);
            goto _opcode_whiplash_;
        }
        break;
        case LI_POP:
        {
            POP();
            goto _opcode_whiplash_;
        }
        break;
        case LI_JUMP:
        {
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            if(!idx)
                frame->pc = -1;
            else
                frame->pc = idx < frame->pc ? idx - 1 : idx;
            goto _opcode_whiplash_;
        }
        break;
        case LI_JUMP_FALSE:
        {
            lky_object *obj = POP();
            
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            char needs_jump = 0;

            if(obj == &lky_nil)
                needs_jump = 1;
            else if(((uintptr_t)(obj) & 1) || obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
                needs_jump = !OBJ_NUM_UNWRAP(obj);

            if(needs_jump)
            {
                frame->pc = idx;
            }

            goto _opcode_whiplash_;
        }
        break;
        case LI_JUMP_FALSE_ELSE_POP:
        case LI_JUMP_TRUE_ELSE_POP:
        {
            lky_object *obj = TOP();

            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            char needs_jump = 0;
            if(obj == &lky_nil)
                needs_jump = 0;
            else if(((uintptr_t)(obj) & 1) || obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
                needs_jump = !!(OBJ_NUM_UNWRAP(obj));

            if(needs_jump && op == LI_JUMP_TRUE_ELSE_POP ||
               !needs_jump && op == LI_JUMP_FALSE_ELSE_POP)
                frame->pc = idx;
            else
                POP();

            goto _opcode_whiplash_;
        }
        break;
        case LI_SAVE_LOCAL:
        {
            lky_object *obj = TOP();
            char idx = frame->ops[++frame->pc];
            lky_object *old = frame->locals[idx];

            frame->locals[idx] = obj;

            goto _opcode_whiplash_;
        }
        break;
        case LI_LOAD_LOCAL:
        {
            char idx = frame->ops[++frame->pc];
            lky_object *obj = frame->locals[idx];
            PUSH(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_PUSH_NIL:
        {
            PUSH(&lky_nil);
            goto _opcode_whiplash_;
        }
        break;
        case LI_PUSH_NEW_OBJECT:
        {
            PUSH(lobj_alloc());
            goto _opcode_whiplash_;
        }
        break;
        case LI_CALL_FUNC:
        {
            char ct = frame->ops[++frame->pc];
            lky_object *obj = POP();

            lky_object_seq *seq = NULL;
            lky_object_seq *first = NULL;
            int i;
            for(i = 0; i < ct; i++)
            {
                lky_object *arg = POP();
                lky_object_seq *ns = lobjb_make_seq_node(arg);

                if(!seq)
                {
                    seq = ns;
                    gc_add_root_object((lky_object *)seq);
                    first = seq;
                }
                else
                {
                    ns->next = seq;
                    seq = ns;
                }
            }

            lky_object *ret = lobjb_call(obj, seq);
            if(frame->thrown)
            {
                mach_halt_with_err(frame->thrown);
                frame->thrown = NULL;
                goto _opcode_whiplash_;
            }

            if(seq)
                gc_remove_root_object((lky_object *)first);

            PUSH(ret);
            goto _opcode_whiplash_;
        }
        break;
        case LI_RETURN:
        {
            lky_object *obj = POP();
            frame->ret = obj;
            goto _opcode_whiplash_;
        }
        break;
        case LI_LOAD_MEMBER:
        {
            lky_object *obj = POP();

            int idx = frame->ops[++frame->pc];
            char *name = frame->names[idx];
            lky_object *val = lobj_get_member(obj, name);
            
            if(!val)
                mach_halt_with_err(lobjb_build_error("UndeclaredIdentifier", "The provided object does not have a member with the provided name."));

            PUSH(val);
            goto _opcode_whiplash_;
        }
        break;
        case LI_SAVE_MEMBER:
        {
            lky_object *obj = POP();
            lky_object *val = TOP();

            int idx = frame->ops[++frame->pc];
            char *name = frame->names[idx];

            lobj_set_member(obj, name, val);

            goto _opcode_whiplash_;
        }
        break;
        case LI_MAKE_FUNCTION:
        {
            lky_object_code *code = POP();

            arraylist pstack = frame->parent_stack;
            arraylist nplist = arr_create(pstack.count + 1);
            int i;
            for(i = 0; i < pstack.count; i++)
            {
                lky_object *obk = arr_get(&pstack, i);

                arr_append(&nplist, obk);
            }

            arr_append(&nplist, frame->bucket);

            char argc = frame->ops[++frame->pc];
            lky_object *func = lobjb_build_func(code, argc, nplist, frame->interp);

            PUSH(func);
            goto _opcode_whiplash_;
        }
        break;
        case LI_MAKE_CLASS:
        {
            lky_object_function *func = POP();
            
            int idx = frame->ops[++frame->pc];
            char *name = frame->names[idx];

            lky_object *cls = lobjb_build_class(func, name, NULL);

            PUSH(cls);
            goto _opcode_whiplash_;
        }
        break;
        case LI_SAVE_CLOSE:
        {
            lky_object *obj = TOP();
            int idx = frame->ops[++frame->pc];

            char *name = frame->names[idx];

            lky_object *bk = NULL;
            arraylist ps = frame->parent_stack;

            int i;
            for(i = (int)ps.count - 1; i >= 0 && !bk; i--)
            {
                lky_object *n = arr_get(&ps, i);
                if(lobj_get_member(n, name))
                {
                    bk = n;
                }
            }

            if(!bk)
                bk = frame->bucket;

            lobj_set_member(bk, name, obj);
            
            goto _opcode_whiplash_;
        }
        break;
        case LI_LOAD_CLOSE:
        {
            int idx = frame->ops[++frame->pc];
            char *name = frame->names[idx];

            lky_object *bk = NULL;
            lky_object *obj = NULL;
            arraylist ps = frame->parent_stack;

            if(!(obj = lobj_get_member(frame->bucket, name)))
            {
                int i;
                for(i = (int)ps.count - 1; i >= 0 && !bk; i--)
                {
                    lky_object *n = arr_get(&ps, i);
                    if((obj = lobj_get_member(n, name)))
                    {
                        bk = n;
                    }
                }
            }

            if(obj)
                PUSH(obj);
            else
                mach_halt_with_err(lobjb_build_error("UndeclaredIdentifier", "A bad name was used..."));

            goto _opcode_whiplash_;
        }
        break;
        case LI_MAKE_ARRAY:
        {
            unsigned int ct = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            arraylist arr = arr_create(ct + 10);

            int i;
            for(i = ct - 1; i >= 0; i--)
            {
                lky_object *obj = frame->data_stack[frame->stack_pointer - i];
                frame->data_stack[frame->stack_pointer - i] = NULL;
                arr_append(&arr, obj);
            }

            frame->stack_pointer -= ct;

            lky_object *outobj = stlarr_cinit(arr);
            PUSH(outobj);

            goto _opcode_whiplash_;
        }
        break;
        case LI_MAKE_TABLE:
        {
            unsigned int ct = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            arraylist keys = arr_create(ct + 1);
            arraylist vals = arr_create(ct + 1);

            int i;
            for(i = 2 * (ct - 1); i >= 0; i -= 2)
            {
                lky_object *v = frame->data_stack[frame->stack_pointer - i];
                lky_object *k = frame->data_stack[frame->stack_pointer - i - 1];
                frame->data_stack[frame->stack_pointer - i] = NULL;
                frame->data_stack[frame->stack_pointer - i - 1] = NULL;

                arr_append(&keys, k);
                arr_append(&vals, v);
            }

            frame->stack_pointer -= 2 * ct;

            lky_object *outobj = stltab_cinit(&keys, &vals);
            arr_free(&keys);
            arr_free(&vals);

            PUSH(outobj);

            goto _opcode_whiplash_;
        }
        break;
        case LI_MAKE_OBJECT:
        {
            int ct = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            lky_object *obj = POP();

            while(0 <=-- ct)
            {
                lky_object *member = POP();
                char *name = frame->names[*(char *)(frame->ops + (++frame->pc))];
                lobj_set_member(obj, name, member);
            }
             
            PUSH(obj);   

            goto _opcode_whiplash_;
        }
        break;
        case LI_LOAD_INDEX:
        {
            lky_object *idx = POP();
            lky_object *targ = POP();

            PUSH(lobjb_unary_load_index(targ, idx));
            goto _opcode_whiplash_;
        }
        break;
        case LI_SAVE_INDEX:
        {
            lky_object *idx = POP();
            lky_object *targ = POP();
            lky_object *nobj = TOP();

            lobjb_unary_save_index(targ, idx, nobj);

            goto _opcode_whiplash_;
        }
        break;
        case LI_SDUPLICATE:
        {
            PUSH(TOP());
            
            goto _opcode_whiplash_;
        }
        break;
        case LI_DDUPLICATE:
        {
            lky_object *topa = TOP();
            lky_object *topb = SECOND_TOP();
            
            PUSH(topb);
            PUSH(topa);
            
            goto _opcode_whiplash_;
        }
        break;
        case LI_FLIP_TWO:
        {
            lky_object *topa = POP();
            lky_object *topb = POP();
            
            PUSH(topa);
            PUSH(topb);
            
            goto _opcode_whiplash_;
        }
        break;
        case LI_SINK_FIRST:
        {
            lky_object *topa = POP();
            lky_object *topb = POP();
            lky_object *topc = POP();
            
            PUSH(topa);
            PUSH(topc);
            PUSH(topb);
            
            goto _opcode_whiplash_;
        }
        break;
        case LI_MAKE_ITER:
        {
            lky_object *obj = POP();
            lky_object *it  = lobjb_build_iterable(obj);

            PUSH(it);

            goto _opcode_whiplash_;
        }
        break;
        case LI_NEXT_ITER_OR_JUMP:
        {
            lky_object *it = TOP();
            lky_object *nxt = LKY_NEXT_ITERABLE(it);

            if(nxt)
            {
                PUSH(nxt);
                frame->pc += 4;
            }
            else
            {
                unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
                frame->pc += 3;

                if(!idx)
                    frame->pc = -1;
                else
                    frame->pc = idx < frame->pc ? idx - 1 : idx;
            }

            goto _opcode_whiplash_;
        }
        break;
        case LI_ITER_INDEX:
        {
            lky_object *it = TOP();
            lky_object_iterable *i = (lky_object_iterable *)it;
            PUSH(lobjb_build_int(i->index - 1));

            goto _opcode_whiplash_;
        }
        break;
        case LI_LOAD_MODULE:
        {
            int idx = (frame->ops[++frame->pc]);
            char *name = frame->names[idx];

            lky_object *bk = NULL;
            lky_object *obj = NULL;
            arraylist ps = frame->parent_stack;

            if(!(obj = lobj_get_member(frame->bucket, "dirname_")))
            {
                int i;
                for(i = (int)ps.count - 1; i >= 0 && !bk; i--)
                {
                    lky_object *n = arr_get(&ps, i);
                    if((obj = lobj_get_member(n, "dirname_")))
                    {
                        bk = n;
                    }
                }
            }

            lky_object *loaded = md_load(name, lobjb_stringify(obj), frame->interp);
            if(!loaded)
                loaded = &lky_nil;

            PUSH(loaded);

            goto _opcode_whiplash_;
        }
        break;
        case LI_PUSH_CATCH:
        {
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            frame->catch_stack[frame->catch_pointer++] = idx;

            goto _opcode_whiplash_;
        }
        break;
        case LI_POP_CATCH:
        {
            frame->catch_stack[frame->catch_pointer--] = 0;
            goto _opcode_whiplash_;
        }
        break;
        case LI_RAISE:
        {
            mach_halt_with_err(POP());
            goto _opcode_whiplash_;
        }
        break;
        default:
            goto _opcode_whiplash_;
        break;
    }
}

