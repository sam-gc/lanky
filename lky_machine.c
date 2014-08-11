#include <stdio.h>
#include <stdlib.h>
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"
#include "lky_object.h"
#include "lky_gc.h"
#include "hashmap.h"

#define PUSH(data) (push_node(frame, data))
#define POP() (pop_node(frame))
#define TOP() (top_node(frame))

#define PUSH_RC(data) do{ push_node(frame, data); rc_incr(data); }while(0)
#define POP_RC() rc_decr(POP())

#define POP_TWO() lky_object *a = POP(); lky_object *b = POP()
#define RC_TWO() rc_decr(a); rc_decr(b)

typedef struct stackframe {
    arraylist parent_stack;
    lky_object *bucket;
    void **constants;
    void **locals;
    void **data_stack;
    char **names;
    long pc;
    char *ops;
    long tape_len;

    long stack_pointer;
    long stack_size;
    lky_object *ret;
} stackframe;

void mach_eval(stackframe *frame);

// static arraylist main_stack;
// static arraylist constants;
// char ops[6] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_BINARY_DIVIDE, LI_PRINT};
// char ops[9] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_LOAD_CONST, 2, LI_BINARY_ADD, LI_BINARY_MULTIPLY, LI_PRINT};
// static int pc = 0;
// static char *ops;
// static long tape_len;

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
    void *data = frame->data_stack[frame->stack_pointer];
    frame->stack_pointer--;

    return data;
}

lky_object *mach_execute(lky_object_function *func)
{
    lky_object_code *code = func->code;
    stackframe frame;
    frame.parent_stack = func->parent_stack;
    if(func->bucket)
        frame.bucket = func->bucket;
    else
        frame.bucket = lobj_alloc();
    frame.constants = code->constants;
    frame.locals = code->locals;
    frame.pc = -1;
    frame.ops = code->ops;
    frame.tape_len = code->op_len;
    // frame.data_stack = malloc(sizeof(void *) * code->stack_size);
    frame.stack_pointer = -1;
    frame.stack_size = code->stack_size;
    frame.names = code->names;
    frame.ret = NULL;

    void *stack[code->stack_size];
    memset(stack, 0, sizeof(void *) * code->stack_size);

    frame.data_stack = stack;

    func->bucket = frame.bucket;

    gc_add_root_stack(stack, frame.stack_size);

    rc_incr(frame.bucket);

    mach_eval(&frame);

    rc_decr(frame.bucket);

    gc_remove_root_stack(NULL);
    func->bucket = NULL;

    return frame.ret;
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
            lky_object *obj = lobjb_binary_equals(b, a);
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
            char idx = frame->ops[++frame->pc];
            frame->pc = idx < frame->pc ? idx - 1 : idx;
            goto _opcode_whiplash_;
        }
        break;
        case LI_JUMP_FALSE:
        {
            lky_object *obj = POP();
            char idx = frame->ops[++frame->pc];

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
            if(obj->type != LBI_FUNCTION)
            {
                // TODO: Error
            }

            lky_object_function *func = (lky_object_function *)obj;
            lky_callable c = func->callable;
            lky_object_seq *seq = NULL;
            int i;
            for(i = 0; i < c.argc; i++)
            {
                lky_object *arg = POP();
                lky_object_seq *ns = lobjb_make_seq_node(arg);

                if(!seq)
                    seq = ns;
                else
                {
                    ns->next = seq;
                    seq = ns;
                }
            }

            lky_function_ptr ptr = c.function;
            lky_object *ret = (lky_object *)(*ptr)(seq, (struct lky_object *)func);

            rc_decr(obj);
            lobjb_free_seq(seq);

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
            val = val ? val : &lky_nil;

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
            lky_object *func = lobjb_build_func(code, argc, nplist);

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
            for(i = ps.count - 1; i >= 0 && !bk; i--)
            {
                lky_object *n = arr_get(&ps, i);
                if(lobj_get_member(n, name))
                {
                    bk = n;
                }
            }

            if(!bk)
                bk = frame->bucket;

            lky_object *old = lobj_get_member(bk, name);

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
            for(i = ps.count - 1; i >= 0 && !bk; i--)
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
                PUSH(&lky_nil);
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
    case LI_LOAD_CLOSE:
        name = "LOAD_CLOSE";
        break;
    case LI_SAVE_CLOSE:
        name = "SAVE_CLOSE";
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

