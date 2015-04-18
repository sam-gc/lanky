/* Lanky -- Scripting Language and Virtual Machine
 * Copyright (C) 2014  Sam Olsen *
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
#include "runtime.h"
#include "class_builder.h"

//#define COMPUTED_GOTO

// Macros to abstract the notion of "pushing"
// and "popping" from the state machine
#define PUSH(data) (push_node(frame, data))
#define POP() (pop_node(frame))
#define TOP() (top_node(frame))
#define SECOND_TOP() (frame->data_stack[frame->stack_pointer - 1])

#ifdef COMPUTED_GOTO
    #define dispatch_() goto *dispatch_table_[frame->ops[++frame->pc] - 50]
    #define vmop(op_, code_) LI_ ## op_ : do{\
    code_\
    if(frame->pc >= frame->tape_len || frame->ret)\
        return;\
    if(interp->error)\
    {\
        lky_object *exc = interp->error;\
        interp->error = NULL;\
\
        if(!frame->catch_pointer && !frame->prev)\
        {\
            char *errtxt = lobjb_stringify(exc, frame->interp);\
            printf("Fatal error--\n%s\n\nHalting.\n", errtxt);\
            free(errtxt);\
            frame->ret = &lky_nil;\
            return;\
        }\
        else if(!frame->catch_pointer)\
        {\
            frame->prev->thrown = exc;\
            return;\
        }\
\
        PUSH(exc);\
        frame->pc = frame->catch_stack[--frame->catch_pointer];\
    }\
\
    gc_gc();\
    dispatch_();}while(0);
    #define vmvm(code) dispatch_(); code
#else
    #define vmop(op, code) case LI_ ## op : { code goto _opcode_whiplash_; } break;
    #define vmvm(code) switch((op = frame->ops[++frame->pc])) { code default: printf("HIT DEFAULT. BUG!\n"); goto _opcode_whiplash_; break; }
    #define dispatch_() goto _opcode_whiplash_
#endif

#define POP_TWO() lky_object *a = POP(); lky_object *b = POP()

void mach_eval(stackframe *frame);

int pushes = 0;

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
        // TODO: Nope...
    }
    void *data = frame->data_stack[frame->stack_pointer];
    frame->data_stack[frame->stack_pointer] = NULL;
    frame->stack_pointer--;

    return data;
}


arraylist mach_build_trace(mach_interp *interp)
{
    stackframe *frame = interp->stack;
    arraylist out = arr_create(10);
    for(; frame && frame->indices; frame = frame->prev) arr_append(&out, lobjb_build_int(frame->indices[frame->pc]));

    return out;
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
    frame->indices = code->indices;
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
    
    lky_object_error *err = (lky_object_error *)frame->prev->thrown;
    if(err)
    { 
        char *txt = lobjb_stringify((lky_object *)err, frame->interp);
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

    frame->indices = code->indices;
    
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

    // Poll the runtime callbacks if we have nothing else to do.
    if(!interp->stack->prev)
    {
        rt_event *event;
        while((event = rt_next((runtime *)interp->rtime)))
        {
            lobjb_call(event->callback, event->args, interp);
            if(interp->stack->thrown)
            {
                char *errtxt = lobjb_stringify(interp->stack->thrown, interp);
                printf("Fatal error--\n%s\n\nHalting.\n", errtxt);
                free(errtxt);
                return NULL;
            }

            free(event);
        }
    }

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

void mach_eval(stackframe *frame)
{
    struct interp *interp = frame->interp;
#ifdef COMPUTED_GOTO
static void *dispatch_table_[] = {
    &&LI_BINARY_ADD, &&LI_BINARY_SUBTRACT, &&LI_BINARY_MULTIPLY, &&LI_BINARY_DIVIDE, 
    &&LI_BINARY_MODULO, &&LI_BINARY_POWER, &&LI_BINARY_LT, &&LI_BINARY_GT, &&LI_BINARY_EQUAL, 
    &&LI_BINARY_LTE, &&LI_BINARY_GTE, &&LI_BINARY_NE, &&LI_BINARY_AND, &&LI_BINARY_OR, 
    &&LI_BINARY_NC, &&LI_BINARY_BAND, &&LI_BINARY_BOR, &&LI_BINARY_BXOR, &&LI_BINARY_BLSHIFT, 
    &&LI_BINARY_BRSHIFT, &&LI_UNARY_NOT, &&LI_UNARY_NEGATIVE, &&LI_LOAD_CONST, &&LI_PRINT, 
    &&LI_POP, &&LI_JUMP_FALSE, &&LI_JUMP_TRUE, &&LI_JUMP, &&LI_JUMP_FALSE_ELSE_POP, 
    &&LI_JUMP_TRUE_ELSE_POP, &&LI_IGNORE, &&LI_SAVE_LOCAL, &&LI_LOAD_LOCAL, &&LI_PUSH_NIL, &&LI_PUSH_BOOL,
    &&LI_PUSH_NEW_OBJECT, &&LI_CALL_FUNC, &&LI_RETURN, &&LI_LOAD_MEMBER, &&LI_SAVE_MEMBER, 
    &&LI_MAKE_FUNCTION, &&LI_MAKE_CLASS, &&LI_SAVE_CLOSE, &&LI_LOAD_CLOSE, &&LI_MAKE_ARRAY, 
    &&LI_MAKE_TABLE, &&LI_MAKE_OBJECT, &&LI_LOAD_INDEX, &&LI_SAVE_INDEX, &&LI_SDUPLICATE, 
    &&LI_DDUPLICATE, &&LI_FLIP_TWO, &&LI_SINK_FIRST, &&LI_MAKE_ITER, &&LI_NEXT_ITER_OR_JUMP, 
    &&LI_ITER_INDEX, &&LI_LOAD_MODULE, &&LI_PUSH_CATCH, &&LI_POP_CATCH, &&LI_RAISE
};
#else
lky_instruction op;
_opcode_whiplash_:
    if(frame->pc >= frame->tape_len || frame->ret)
        return;
    if(interp->error)
    {
        lky_object_error *exc = (lky_object_error *)interp->error;
        interp->error = NULL;

        if(!frame->catch_pointer && !frame->prev)
        {
            char *errtxt = lobjb_stringify((lky_object *)exc, frame->interp);
            printf("Fatal error on line %ld--\n%s\n\nHalting.\n", (long)OBJ_NUM_UNWRAP(arr_get(&exc->trace, 0)), errtxt);
            free(errtxt);
            frame->ret = &lky_nil;
            return;
        }
        else if(!frame->catch_pointer)
        {
            frame->prev->thrown = (lky_object *)exc;
            return;
        }

        PUSH(exc);
        frame->pc = frame->catch_stack[--frame->catch_pointer];
    }

    gc_gc();
#endif
    vmvm(
        vmop(LOAD_CONST,
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;
            lky_object *obj = frame->constants[idx];
            PUSH(obj);
        )
        vmop(BINARY_ADD,
            POP_TWO();
            lky_object *obj = lobjb_binary_add(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_SUBTRACT,
            POP_TWO();
            lky_object *obj = lobjb_binary_subtract(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_MULTIPLY,
            POP_TWO();
            lky_object *obj = lobjb_binary_multiply(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_DIVIDE,
            POP_TWO();
            lky_object *obj = lobjb_binary_divide(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_MODULO,
            POP_TWO();
            lky_object *obj = lobjb_binary_modulo(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_POWER,
            POP_TWO();
            lky_object *obj = lobjb_binary_power(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_LT,
            POP_TWO();
            lky_object *obj = lobjb_binary_lessthan(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_GT,
            POP_TWO();
            lky_object *obj = lobjb_binary_greaterthan(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_LTE,
            POP_TWO();
            lky_object *obj = lobjb_binary_lessequal(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_GTE,
            POP_TWO();
            lky_object *obj = lobjb_binary_greatequal(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_EQUAL,
            POP_TWO();
            lky_object *obj = lobjb_binary_equals(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_NE,
            POP_TWO();
            lky_object *obj = lobjb_binary_notequal(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_AND,
            POP_TWO();
            lky_object *obj = lobjb_binary_and(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_OR,
            POP_TWO();
            lky_object *obj = lobjb_binary_or(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_NC,
            POP_TWO();
            lky_object *obj = lobjb_binary_nc(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_BAND,
            POP_TWO();
            lky_object *obj = lobjb_binary_band(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_BOR,
            POP_TWO();
            lky_object *obj = lobjb_binary_bor(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_BXOR,
            POP_TWO();
            lky_object *obj = lobjb_binary_bxor(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_BLSHIFT,
            POP_TWO();
            lky_object *obj = lobjb_binary_blshift(b, a, interp);

            PUSH(obj);
        )
        vmop(BINARY_BRSHIFT,
            POP_TWO();
            lky_object *obj = lobjb_binary_brshift(b, a, interp);

            PUSH(obj);
        )
        vmop(UNARY_NOT,
            lky_object *a = POP();
            lky_object *obj = lobjb_unary_not(a, frame->interp);

            PUSH(obj);
        )
        vmop(UNARY_NEGATIVE,
            lky_object *a = POP();
            lky_object *obj = lobjb_unary_negative(a);

            PUSH(obj);
        )
        vmop(PRINT,
            lky_object *a = POP();
            lobjb_print(a, frame->interp);
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

            /*
            char needs_jump = 0;

            if(obj == &lky_nil)
                needs_jump = 1;
            else if(((uintptr_t)(obj) & 1) || obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
                needs_jump = !OBJ_NUM_UNWRAP(obj);
            */

            if(!LKY_CTEST_FAST(obj))
                frame->pc = idx;
        )
        vmop(JUMP_FALSE_ELSE_POP,
            lky_object *obj = TOP();

            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            /*
            char needs_jump = 0;
            if(obj == &lky_nil)
                needs_jump = 0;
            else if(((uintptr_t)(obj) & 1) || obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
                needs_jump = !!(OBJ_NUM_UNWRAP(obj));
            */

            if(!LKY_CTEST_FAST(obj))
                frame->pc = idx;
            else
                POP();
        )
        vmop(JUMP_TRUE_ELSE_POP,
            lky_object *obj = TOP();

            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            /*
            char needs_jump = 0;
            if(obj == &lky_nil)
                needs_jump = 0;
            else if(((uintptr_t)(obj) & 1) || obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
                needs_jump = !!(OBJ_NUM_UNWRAP(obj));
            */

            if(LKY_CTEST_FAST(obj))
                frame->pc = idx;
            else
                POP();

        )
        vmop(SAVE_LOCAL,
            lky_object *obj = TOP();
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

            frame->locals[idx] = obj;
        )
        vmop(LOAD_LOCAL,
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;
            lky_object *obj = frame->locals[idx];
            PUSH(obj);
        )
        vmop(PUSH_NIL,
            PUSH(&lky_nil);
        )
        vmop(PUSH_BOOL,
            PUSH(LKY_TESTC_FAST(frame->ops[++frame->pc]));
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

            lky_object *ret = lobjb_call(obj, seq, frame->interp);
            if(frame->thrown)
            {
                interp->error = frame->thrown;
                frame->thrown = NULL;
                dispatch_();
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

            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;
            char *name = frame->names[idx];

            if(obj == &lky_nil || obj == &lky_yes || obj == &lky_no || OBJ_IS_NUMBER(obj))
            {
                char str[200 + strlen(name)];
                sprintf(str, "Requesting member '%s' from memberless-object.", name);
                interp->error = lobjb_build_error(obj == &lky_nil ? "NullPointer" : "InvalidType", str, interp);
                dispatch_();
            }

            lky_object *val = lobj_get_member(obj, name);
            
            if(!val)
            {
                char str[200 + strlen(name)];
                sprintf(str, "Object has no member named '%s'.", name);
                interp->error = lobjb_build_error("UndeclaredIdentifier", str, interp);
                dispatch_();
            }


            PUSH(val);
        )
        vmop(SAVE_MEMBER,
            lky_object *obj = POP();
            lky_object *val = TOP();

            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;
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
            int count = frame->ops[++frame->pc];
            int has_init = frame->ops[++frame->pc];

            lky_object *init = has_init & 1 ? POP() : NULL;
            lky_object *super = has_init & 2 ? POP() : NULL;
            
            lky_object *cls = clb_init_class(init, super);

            int i;
            for(i = 0; i < count; i++)
            {
                lky_class_prefix prfx = frame->ops[++frame->pc];
                unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
                frame->pc += 3;
                
                char *name = frame->names[idx];
                clb_add_member(cls, name, POP(), prfx);
            }

            PUSH(cls);
        )
        vmop(SAVE_CLOSE,
            lky_object *obj = TOP();
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;

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
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;
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
            {
                char str[200 + strlen(name)];
                sprintf(str, "Could not load closure variable '%s'.", name);
                interp->error = lobjb_build_error("UndeclaredIdentifier", str, interp);
            }
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
                unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
                frame->pc += 3;
                char *name = frame->names[idx];
                lobj_set_member(obj, name, member);
            }
             
            PUSH(obj);   

        )
        vmop(LOAD_INDEX,
            lky_object *idx = POP();
            lky_object *targ = POP();

            PUSH(lobjb_unary_load_index(targ, idx, frame->interp));
        )
        vmop(SAVE_INDEX,
            lky_object *idx = POP();
            lky_object *targ = POP();
            lky_object *nobj = TOP();

            lobjb_unary_save_index(targ, idx, nobj, frame->interp);

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
            lky_object *it  = lobjb_build_iterable(obj, frame->interp);

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
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;
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

            lky_object *loaded = md_load(name, lobjb_stringify(obj, frame->interp), frame->interp);
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
            interp->error = lobjb_build_error("", "", interp);
            lobj_set_member(interp->error, "custom_", POP());
        )
        // Unused...
        vmop(IGNORE,
        )
        vmop(JUMP_TRUE,
        )
    )
}

