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
#include "class_builder.h"

// Macros to abstract the notion of "pushing"
// and "popping" from the state machine
#define PUSH(data) (push_node(frame, data))
#define POP() (pop_node(frame))
#define TOP() (top_node(frame))
#define SECOND_TOP() (frame->data_stack[frame->stack_pointer - 1])
#define vmop(op, code) case LI_ ## op : { code goto _opcode_whiplash_; } break;
#define vmvm(code) switch((op = frame->ops[++frame->pc])) { code default: goto _opcode_whiplash_; break; }

#define POP_TWO() lky_object *a = POP(); lky_object *b = POP()

void mach_eval(stackframe *frame);

int pushes = 0;
lky_object *thrown_exception = NULL;

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
    frame->catch_pointer = 0;
    
    frame->prev = interp->stack ? interp->stack : NULL;
    frame->thrown = NULL;
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
    
    lky_object *err = frame->prev->thrown;
    if(err)
    { 
        char *txt = lobjb_stringify(err);
        printf("Interrupt caught exception.\n%s\n", txt);
        free(txt);
        frame->prev->thrown = NULL;
        
        ret = &lky_nil;
    }

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
    frame->thrown = NULL;
    frame->catch_pointer = 0;
    
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
        lky_object *exc = thrown_exception;
        thrown_exception = NULL;

        if(!frame->catch_pointer && !frame->prev)
        {
            char *errtxt = lobj_stringify(exc);
            printf("Fatal error--\n%s\n\nHalting.\n", errtxt);
            free(errtxt);
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

    vmvm(
        vmop(LOAD_CONST,
            char idx = frame->ops[++frame->pc];
            lky_object *obj = frame->constants[idx];
            PUSH(obj);
        )
        vmop(BINARY_ADD,
            POP_TWO();
            lky_object *obj = lobjb_binary_add(b, a);

            PUSH(obj);
        )
        vmop(BINARY_SUBTRACT,
            POP_TWO();
            lky_object *obj = lobjb_binary_subtract(b, a);

            PUSH(obj);
        )
        vmop(BINARY_MULTIPLY,
            POP_TWO();
            lky_object *obj = lobjb_binary_multiply(b, a);

            PUSH(obj);
        )
        vmop(BINARY_DIVIDE,
            POP_TWO();
            lky_object *obj = lobjb_binary_divide(b, a);

            PUSH(obj);
        )
        vmop(BINARY_MODULO,
            POP_TWO();
            lky_object *obj = lobjb_binary_modulo(b, a);

            PUSH(obj);
        )
        vmop(BINARY_POWER,
            POP_TWO();
            lky_object *obj = lobjb_binary_power(b, a);

            PUSH(obj);
        )
        vmop(BINARY_LT,
            POP_TWO();
            lky_object *obj = lobjb_binary_lessthan(b, a);

            PUSH(obj);
        )
        vmop(BINARY_GT,
            POP_TWO();
            lky_object *obj = lobjb_binary_greaterthan(b, a);

            PUSH(obj);
        )
        vmop(BINARY_LTE,
            POP_TWO();
            lky_object *obj = lobjb_binary_lessequal(b, a);

            PUSH(obj);
        )
        vmop(BINARY_GTE,
            POP_TWO();
            lky_object *obj = lobjb_binary_greatequal(b, a);

            PUSH(obj);
        )
        vmop(BINARY_EQUAL,
            POP_TWO();
            lky_object *obj = lobjb_binary_equals(b, a);

            PUSH(obj);
        )
        vmop(BINARY_NE,
            POP_TWO();
            lky_object *obj = lobjb_binary_notequal(b, a);

            PUSH(obj);
        )
        vmop(BINARY_AND,
            POP_TWO();
            lky_object *obj = lobjb_binary_and(b, a);

            PUSH(obj);
        )
        vmop(BINARY_OR,
            POP_TWO();
            lky_object *obj = lobjb_binary_or(b, a);

            PUSH(obj);
        )
        vmop(BINARY_NC,
            POP_TWO();
            lky_object *obj = lobjb_binary_nc(b, a);

            PUSH(obj);
        )
        vmop(BINARY_BAND,
            POP_TWO();
            lky_object *obj = lobjb_binary_band(b, a);

            PUSH(obj);
        )
        vmop(BINARY_BOR,
            POP_TWO();
            lky_object *obj = lobjb_binary_bor(b, a);

            PUSH(obj);
        )
        vmop(BINARY_BXOR,
            POP_TWO();
            lky_object *obj = lobjb_binary_bxor(b, a);

            PUSH(obj);
        )
        vmop(BINARY_BLSHIFT,
            POP_TWO();
            lky_object *obj = lobjb_binary_blshift(b, a);

            PUSH(obj);
        )
        vmop(BINARY_BRSHIFT,
            POP_TWO();
            lky_object *obj = lobjb_binary_brshift(b, a);

            PUSH(obj);
        )
        vmop(UNARY_NOT,
            lky_object *a = POP();
            lky_object *obj = lobjb_unary_not(a);

            PUSH(obj);
        )
        vmop(UNARY_NEGATIVE,
            lky_object *a = POP();
            lky_object *obj = lobjb_unary_negative(a);

            PUSH(obj);
        )
        vmop(PRINT,
            lky_object *a = POP();
            lobjb_print(a);
        )
        vmop(POP,
            POP();
        )
        vmop(JUMP,
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            if(!idx)
                frame->pc = -1;
            else
                frame->pc = idx < frame->pc ? idx - 1 : idx;
        )
        vmop(JUMP_FALSE,
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
        )
        case LI_JUMP_FALSE_ELSE_POP:
        vmop(JUMP_TRUE_ELSE_POP,
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

        )
        vmop(SAVE_LOCAL,
            lky_object *obj = TOP();
            char idx = frame->ops[++frame->pc];
            lky_object *old = frame->locals[idx];

            frame->locals[idx] = obj;
        )
        vmop(LOAD_LOCAL,
            char idx = frame->ops[++frame->pc];
            lky_object *obj = frame->locals[idx];
            PUSH(obj);
        )
        vmop(PUSH_NIL,
            PUSH(&lky_nil);
        )
        vmop(PUSH_NEW_OBJECT,
            PUSH(lobj_alloc());
        )
        vmop(CALL_FUNC,
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
        )
        vmop(RETURN,
            lky_object *obj = POP();
            frame->ret = obj;
        )
        vmop(LOAD_MEMBER,
            lky_object *obj = POP();

            int idx = frame->ops[++frame->pc];
            char *name = frame->names[idx];
            lky_object *val = lobj_get_member(obj, name);
            
            if(!val)
                mach_halt_with_err(lobjb_build_error("UndeclaredIdentifier", "The provided object does not have a member with the provided name."));

            PUSH(val);
        )
        vmop(SAVE_MEMBER,
            lky_object *obj = POP();
            lky_object *val = TOP();

            int idx = frame->ops[++frame->pc];
            char *name = frame->names[idx];

            lobj_set_member(obj, name, val);

        )
        vmop(MAKE_FUNCTION,
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
        )
        vmop(MAKE_CLASS,
            /*lky_object_function *func = POP();
            
            int idx = frame->ops[++frame->pc];
            char *name = frame->names[idx];

            lky_object *cls = lobjb_build_class(func, name, NULL);

            PUSH(cls);*/
            int count = frame->ops[++frame->pc];
            int has_init = frame->ops[++frame->pc];

            lky_object *init = has_init ? POP() : NULL;
            lky_object *cls = clb_init_class(init);

            int i;
            for(i = 0; i < count; i++)
            {
                lky_class_prefix prfx = frame->ops[++frame->pc];
                int idx = frame->ops[++frame->pc];
                
                char *name = frame->names[idx];
                clb_add_member(cls, name, POP(), prfx);
            }

            PUSH(cls);
        )
        vmop(SAVE_CLOSE,
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
            
        )
        vmop(LOAD_CLOSE,
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

        )
        vmop(MAKE_ARRAY,
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

        )
        vmop(MAKE_TABLE,
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

        )
        vmop(MAKE_OBJECT,
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

        )
        vmop(LOAD_INDEX,
            lky_object *idx = POP();
            lky_object *targ = POP();

            PUSH(lobjb_unary_load_index(targ, idx));
        )
        vmop(SAVE_INDEX,
            lky_object *idx = POP();
            lky_object *targ = POP();
            lky_object *nobj = TOP();

            lobjb_unary_save_index(targ, idx, nobj);

        )
        vmop(SDUPLICATE,
            PUSH(TOP());
            
        )
        vmop(DDUPLICATE,
            lky_object *topa = TOP();
            lky_object *topb = SECOND_TOP();
            
            PUSH(topb);
            PUSH(topa);
            
        )
        vmop(FLIP_TWO,
            lky_object *topa = POP();
            lky_object *topb = POP();
            
            PUSH(topa);
            PUSH(topb);
            
        )
        vmop(SINK_FIRST,
            lky_object *topa = POP();
            lky_object *topb = POP();
            lky_object *topc = POP();
            
            PUSH(topa);
            PUSH(topc);
            PUSH(topb);
            
        )
        vmop(MAKE_ITER,
            lky_object *obj = POP();
            lky_object *it  = lobjb_build_iterable(obj);

            PUSH(it);

        )
        vmop(NEXT_ITER_OR_JUMP,
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

        )
        vmop(ITER_INDEX,
            lky_object *it = TOP();
            lky_object_iterable *i = (lky_object_iterable *)it;
            PUSH(lobjb_build_int(i->index - 1));

        )
        vmop(LOAD_MODULE,
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

        )
        vmop(PUSH_CATCH,
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            frame->catch_stack[frame->catch_pointer++] = idx;

        )
        vmop(POP_CATCH,
            frame->catch_stack[frame->catch_pointer--] = 0;
        )
        vmop(RAISE,
            mach_halt_with_err(POP());
        )
    )
}

