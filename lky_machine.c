#include <stdio.h>
#include <stdlib.h>
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"
#include "lky_object.h"
#include "hashmap.h"

#define PUSH(data) (push_node(frame, data))
#define POP() (pop_node(frame))
#define TOP() (top_node(frame))

#define PUSH_RC(data) push_node(frame, data); rc_incr(data)
#define POP_RC() rc_decr(POP())

#define POP_TWO() lky_object *a = POP(); lky_object *b = POP()
#define RC_TWO() rc_decr(a); rc_decr(b)

typedef struct stackframe {
    arraylist data_stack;
    void **constants;
    void **locals;
    long pc;
    char *ops;
    long tape_len;

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
    arr_append(&frame->data_stack, data);
    pushes++;
}

void *top_node(stackframe *frame)
{
    return arr_get(&frame->data_stack, frame->data_stack.count - 1);
}

static void *pop_node(stackframe *frame)
{
    void *data = arr_get(&frame->data_stack, frame->data_stack.count - 1);
    arr_remove(&frame->data_stack, NULL, frame->data_stack.count - 1);

    pushes--;

    return data;
}

lky_object *mach_execute(lky_object_code *code)
{
    stackframe frame;
    frame.data_stack = arr_create(100);
    frame.constants = code->constants;
    frame.locals = code->locals;
    frame.pc = -1;
    frame.ops = code->ops;
    frame.tape_len = code->op_len;
    frame.ret = NULL;

    arr_append(&frame.data_stack, NULL);

    // printf("%d\n", tape_len);
    
    mach_eval(&frame);

    arr_free(&frame.data_stack);

    return frame.ret;
    // print_ops();

    // arr_free(&main_stack);
    // arr_free(&constants);
    // free(ops);
}

void mach_do_op(stackframe *frame, lky_instruction op);

void mach_eval(stackframe *frame)
{
    while(frame->pc < frame->tape_len && !frame->ret)
    {
        mach_do_op(frame, frame->ops[++frame->pc]);
    }

    // printf("%d\n", pushes);
}
// void print_op(lky_instruction i);
void mach_do_op(stackframe *frame, lky_instruction op)
{
    // print_stack();

    // printf("==> %d\n", op);
    // print_op(op);

    switch(op)
    {
        case LI_LOAD_CONST:
        {
            char idx = frame->ops[++frame->pc];
            lky_object *obj = frame->constants[idx];

            PUSH_RC(obj);
        }
        break;
        case LI_BINARY_ADD:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_add(b, a);
            RC_TWO();

            PUSH_RC(obj);
        }
        break;
        case LI_BINARY_SUBTRACT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_subtract(b, a);
            RC_TWO();

            PUSH_RC(obj);
        }
        break;
        case LI_BINARY_MULTIPLY:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_multiply(b, a);
            RC_TWO();

            PUSH_RC(obj);
        }
        break;
        case LI_BINARY_DIVIDE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_divide(b, a);
            RC_TWO();

            PUSH_RC(obj);
        }
        break;
        case LI_BINARY_LT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_lessthan(b, a);
            RC_TWO();

            PUSH_RC(obj);
        }
        break;
        case LI_BINARY_GT:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_greaterthan(b, a);
            RC_TWO();

            PUSH_RC(obj);
        }
        break;
        case LI_BINARY_LTE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_lessequal(b, a);
            RC_TWO();

            PUSH_RC(obj);
        }
        break;
        case LI_BINARY_GTE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_greatequal(b, a);
            RC_TWO();

            PUSH_RC(obj);
        }
        break;
        case LI_BINARY_EQUAL:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_equals(b, a);
            RC_TWO();

            PUSH_RC(obj);
        }
        break;
        case LI_BINARY_NE:
        {
            POP_TWO();
            lky_object *obj = lobjb_binary_equals(b, a);
            RC_TWO();

            PUSH_RC(obj);
        }
        break;
        case LI_PRINT:
        {
            lky_object *a = POP();
            lobjb_print(a);
            rc_decr(a);
        }
        break;
        case LI_POP:
        {
            POP_RC();
        }
        break;
        case LI_JUMP:
        {
            char idx = frame->ops[++frame->pc];
            frame->pc = idx < frame->pc ? idx - 1 : idx;
        }
        break;
        case LI_JUMP_FALSE:
        {
            lky_object *obj = POP();
            // printf("=> %d\n", obj == &lky_nil || obj->type != LBI_STRING && !OBJ_NUM_UNWRAP(obj));
            if(obj == &lky_nil || (obj->type != LBI_STRING && !OBJ_NUM_UNWRAP(obj)))
            {
                char idx = frame->ops[++frame->pc];
                frame->pc = idx;
            }
            else
            {
                // PUSH(&lky_nil);
            }
            rc_decr(obj);
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

            // rc_decr(obj);
        }
        break;
        case LI_LOAD_LOCAL:
        {
            char idx = frame->ops[++frame->pc];
            lky_object *obj = frame->locals[idx];
            PUSH_RC(obj);
        }
        break;
        case LI_PUSH_NIL:
        {
            PUSH(&lky_nil);
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
            lky_object *ret = (*ptr)(seq, (lky_object *)func);

            rc_decr(obj);
            lobjb_free_seq(seq);

            PUSH(ret);
        }
        break;
        case LI_RETURN:
        {
            lky_object *obj = POP();
            frame->ret = obj;
        }
        break;
        default:
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

