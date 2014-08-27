#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"
#include "mach_binary_ops.h"
#include "lky_machine.h"
#include "lky_object.h"
#include "lky_gc.h"
#include "stl_array.h"
#include "hashmap.h"

#define PUSH(data) (push_node(frame, data))
#define POP() (pop_node(frame))
#define TOP() (top_node(frame))

#define PUSH_RC(data) do{ push_node(frame, data); rc_incr(data); }while(0)
#define POP_RC() rc_decr(POP())

#define POP_TWO() lky_object *a = POP(); lky_object *b = POP()
#define RC_TWO() rc_decr(a); rc_decr(b)

void mach_eval(stackframe *frame);

// static arraylist main_stack;
// static arraylist constants;
// char ops[6] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_BINARY_DIVIDE, LI_PRINT};
// char ops[9] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_LOAD_CONST, 2, LI_BINARY_ADD, LI_BINARY_MULTIPLY, LI_PRINT};
// static int pc = 0;
// static char *ops;
// static long tape_len;

int pushes = 0;
int good = 1;

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
    void *data = frame->data_stack[frame->stack_pointer];
    frame->data_stack[frame->stack_pointer] = NULL;
    frame->stack_pointer--;

    return data;
}

void mach_halt_with_err(lky_object *err)
{
    lky_object_error *error = (lky_object_error *)err;
    printf("Fatal error: %s\nMessage: %s\n\nHalting.\n", error->name, error->text);
    good = 0;
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
    
    frame->data_stack = stack;
    func->bucket = frame->bucket;
    
//    gc_add_root_object((lky_object *)func);
    
    //    rc_incr(frame.bucket);
    
    mach_eval(frame);
    
    //    rc_decr(frame.bucket);
    
//    gc_remove_root_object((lky_object *)func);
    
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
    // frame.data_stack = malloc(sizeof(void *) * code->stack_size);
    frame->stack_pointer = -1;
    frame->stack_size = code->stack_size;
    frame->names = code->names;
    frame->ret = NULL;
    
    frame->interp = interp;
    
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

    frame->data_stack = stack;

    func->bucket = frame->bucket;

//    gc_add_root_object((lky_object *)func);

//    rc_incr(frame.bucket);

    mach_eval(frame);

//    rc_decr(frame.bucket);

//    gc_remove_root_object((lky_object *)func);
    func->bucket = NULL;
    
    // Pop the stackframe.
    interp->stack = interp->stack->prev;
    if(interp->stack)
        interp->stack->next = NULL;

    lky_object *ret = frame->ret;
    free(frame);
    return ret;
    // print_ops();

    // arr_free(&main_stack);
    // arr_free(&constants);
    // free(ops);
}

// void mach_do_op(stackframe *frame, lky_instruction op);
// 
// {
//     while(frame->pc < frame->tape_len && !frame->ret)
//     {
//         mach_do_op(frame, frame->ops[++frame->pc]);
//     }
// 
//     // printf("%d\n", pushes);
// }
void print_op(lky_instruction i);

void mach_eval(stackframe *frame)
{
    lky_instruction op;
    // print_stack();

    // printf("==> %d\n", op);
    // print_op(op);
_opcode_whiplash_:
    if(frame->pc >= frame->tape_len || frame->ret)
        return;
    if(!good)
    {
        frame->ret = &lky_nil;
        return;
    }

    gc_gc();

    op = frame->ops[++frame->pc];

    switch(op)
    {
        case LI_LOAD_CONST:
        {
            char idx = frame->ops[++frame->pc];
            lky_object *obj = frame->constants[idx];
            PUSH_RC(obj);
            rc_incr(obj);
            rc_incr(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_ADD:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_add(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_SUBTRACT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_subtract(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_MULTIPLY:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_multiply(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_DIVIDE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_divide(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_MODULO:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_modulo(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_LT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_lessthan(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_GT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_greaterthan(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_LTE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_lessequal(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_GTE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_greatequal(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_EQUAL:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_equals(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_NE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_notequal(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_AND:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_and(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_BINARY_OR:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_or(b, a);
            RC_TWO();

            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_PRINT:
        {
            lky_object *a = POP();
            lobjb_print(a);
            rc_decr(a);
            goto _opcode_whiplash_;
        }
        break;
        case LI_POP:
        {
            POP_RC();
            goto _opcode_whiplash_;
        }
        break;
        case LI_JUMP:
        {
            unsigned int idx = *(unsigned int *)(frame->ops + (++frame->pc));
            frame->pc += 3;
            //long idx = mach_calc_jump_idx(frame, len);

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
            //long idx = mach_calc_jump_idx(frame, len);

            // printf("=> %d\n", obj == &lky_nil || obj->type != LBI_STRING && !OBJ_NUM_UNWRAP(obj));
            char needs_jump = 0;

            if(obj == &lky_nil)
                needs_jump = 1;
            else if(obj->type == LBI_FLOAT || obj->type == LBI_INTEGER)
                needs_jump = !OBJ_NUM_UNWRAP(obj);

            if(needs_jump)
            {
                frame->pc = idx;
            }
            else
            {
                // PUSH(&lky_nil);
            }
            rc_decr(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_SAVE_LOCAL:
        {
            lky_object *obj = TOP();
            char idx = frame->ops[++frame->pc];
            lky_object *old = frame->locals[idx];
            if(old)
                rc_decr(old);

            frame->locals[idx] = obj;
            rc_incr(obj);
            // printf("=> %d\n", idx);
            // lobjb_print(obj);

            goto _opcode_whiplash_;
            // rc_decr(obj);
        }
        break;
        case LI_LOAD_LOCAL:
        {
            char idx = frame->ops[++frame->pc];
            lky_object *obj = frame->locals[idx];
            PUSH_RC(obj);
            goto _opcode_whiplash_;
        }
        break;
        case LI_PUSH_NIL:
        {
            PUSH(&lky_nil);
            goto _opcode_whiplash_;
        }
        break;
        case LI_CALL_FUNC:
        {
            lky_object *obj = POP();

            lky_object_function *func = (lky_object_function *)obj;
            lky_callable c = func->callable;
            lky_object_seq *seq = NULL;
            lky_object_seq *first = NULL;
            int i;
            for(i = 0; i < c.argc; i++)
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

            lky_function_ptr ptr = c.function;
            lky_object *ret = (lky_object *)(*ptr)(seq, (struct lky_object *)func);

//            lobjb_free_seq(seq);
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
            rc_incr(obj);
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

            PUSH_RC(val);
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

            rc_decr(obj);
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
                rc_incr(obk);

                arr_append(&nplist, obk);
            }

            arr_append(&nplist, frame->bucket);
            rc_incr(frame->bucket);

            char argc = frame->ops[++frame->pc];
            lky_object *func = lobjb_build_func(code, argc, nplist, frame->interp);

            //rc_decr(code);
            PUSH_RC(func);
            goto _opcode_whiplash_;
        }
        break;
        case LI_MAKE_CLASS:
        {
            lky_object_function *func = POP();
            
            int idx = frame->ops[++frame->pc];
            char *name = frame->names[idx];

            lky_object *cls = lobjb_build_class(func, name);

            PUSH_RC(cls);
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

            if(!strcmp(name, "curr"))
            {
                printf("");
            }
            lobj_set_member(bk, name, obj);
            
            goto _opcode_whiplash_;
        }
        break;
        case LI_LOAD_CLOSE:
        {
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

            lky_object *obj = lobj_get_member(bk, name);

            if(obj)
                PUSH_RC(obj);
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
        default:
            goto _opcode_whiplash_;
        break;
    }

    // printf("=> %d\t", frame.data_stack.count);
    // if(frame.pc > 0)
    //     print_op(frame.ops[frame.pc - 1]);
    // printf("\t");
    // print_op(frame.ops[frame.pc]);

    // printf("----> %d\n", frame.pc);
}

void print_op(lky_instruction op)
{
    char *name = NULL;
    switch(op)
    {
    case LI_BINARY_ADD:
        name = "BINARY_ADD";
        break;
    case LI_BINARY_SUBTRACT:
        name = "BINARY_SUBTRACT";
        break;
    case LI_BINARY_MULTIPLY:
        name = "BINARY_MULTIPLY";
        break;
    case LI_BINARY_DIVIDE:
        name = "BINARY_DIVIDE";
        break;
    case LI_BINARY_MODULO:
        name = "BINARY_MODULO";
        break;
    case LI_BINARY_LT:
        name = "BINARY_LT";
        break;
    case LI_BINARY_GT:
        name = "BINARY_GT";
        break;
    case LI_BINARY_EQUAL:
        name = "BINARY_EQUAL";
        break;
    case LI_BINARY_LTE:
        name = "BINARY_LTE";
        break;
    case LI_BINARY_GTE:
        name = "BINARY_GTE";
        break;
    case LI_BINARY_NE:
        name = "BINARY_NE";
        break;
    case LI_BINARY_AND:
        name = "BINARY_AND";
        break;
    case LI_BINARY_OR:
        name = "BINARY_OR";
        break;
    case LI_LOAD_CONST:
        name = "LOAD_CONST";
        break;
    case LI_PRINT:
        name = "PRINT";
        break;
    case LI_POP:
        name = "POP";
        break;
    case LI_JUMP_FALSE:
        name = "JUMP_FALSE";
        break;
    case LI_JUMP_TRUE:
        name = "JUMP_TRUE";
        break;
    case LI_JUMP:
        name = "JUMP";
        break;
    case LI_IGNORE:
        name = "IGNORE";
        break;
    case LI_SAVE_LOCAL:
        name = "SAVE_LOCAL";
        break;
    case LI_LOAD_LOCAL:
        name = "LOAD_LOCAL";
        break;
    case LI_PUSH_NIL:
        name = "PUSH_NIL";
        break;
    case LI_CALL_FUNC:
        name = "CALL_FUNC";
        break;
    case LI_RETURN:
        name = "RETURN";
        break;
    case LI_LOAD_MEMBER:
        name = "LOAD_MEMBER";
        break;
    case LI_SAVE_MEMBER:
        name = "SAVE_MEMBER";
        break;
    case LI_MAKE_FUNCTION:
        name = "MAKE_FUNCTION";
        break;
    case LI_MAKE_CLASS:
        name = "MAKE_CLASS";
        break;
    case LI_SAVE_CLOSE:
        name = "SAVE_CLOSE";
        break;
    case LI_LOAD_CLOSE:
        name = "LOAD_CLOSE";
        break;
    case LI_MAKE_ARRAY:
        name = "MAKE_ARRAY";
        break;
    case LI_LOAD_INDEX:
        name = "LOAD_INDEX";
        break;
    case LI_SAVE_INDEX:
        name = "SAVE_INDEX";
        break;
    default:
        printf("   --> %d\n", op);
        return;
    }

    printf("%s\n", name);
}

void print_ops(char *ops, int tape_len)
{
    int i;
    for(i = 0; i < tape_len; i++)
    {
        printf("%d :: ", i);
        print_op(ops[i]);
    }
}

