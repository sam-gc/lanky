#include <stdio.h>
#include <stdlib.h>
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"

#define PUSH(data) (push_node(data))
#define POP() (pop_node())

#define PUSH_RC(data) push_node(data); rc_incr(data)
#define POP_RC() rc_decr(POP())

#define POP_TWO() lky_object *a = POP(); lky_object *b = POP()
#define RC_TWO() rc_decr(a); rc_decr(b)

void mach_eval();

static arraylist main_stack;
static arraylist constants;
// char ops[6] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_BINARY_DIVIDE, LI_PRINT};
// char ops[9] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_LOAD_CONST, 2, LI_BINARY_ADD, LI_BINARY_MULTIPLY, LI_PRINT};
static int pc = 0;
static char *ops;
static long tape_len;

static void push_node(void *data)
{
    // stack_node *node = malloc(sizeof(stack_node));
    // node->next = main_stack;
    // node->data = data;
    // main_stack = node;

    arr_append(&main_stack, data);
}

static void *pop_node()
{
    void *data = arr_get(&main_stack, main_stack.count - 1);
    arr_remove(&main_stack, NULL, main_stack.count - 1);

    return data;
}

static void print_stack()
{
    long i;
    for(i = main_stack.count - 1; i > -1; i--)
    {
        lobjb_print(arr_get(&main_stack, i));
    }
}

void mach_execute(lky_object_code *code)
{
    main_stack = arr_create(100);

    arr_append(&main_stack, NULL);

    constants = code->constants;
    tape_len = code->op_len;
    ops = code->ops;

    // printf("%d\n", tape_len);

    mach_eval();

    // arr_free(&main_stack);
    arr_free(&constants);
    free(ops);
}

void mach_do_op(lky_instruction op);

void mach_eval()
{
    pc = -1;
    while(pc < tape_len)
    {
        mach_do_op(ops[++pc]);
    }
}

void mach_do_op(lky_instruction op)
{
    // print_stack();

    // printf("==> %d\n", op);

    switch(op)
    {
        case LI_LOAD_CONST:
        {
            char idx = ops[++pc];
            lky_object *obj = arr_get(&constants, idx);

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
            char idx = ops[++pc];
            pc = idx < pc ? idx - 1 : idx;
        }
        break;
        case LI_JUMP_FALSE:
        {
            lky_object *obj = POP();
            // printf("=> %d\n", obj == &lky_nil || obj->type != LBI_STRING && !OBJ_NUM_UNWRAP(obj));
            if(obj == &lky_nil || obj->type != LBI_STRING && !OBJ_NUM_UNWRAP(obj))
            {
                char idx = ops[++pc];
                pc = idx;
            }
            else
            {
                PUSH(&lky_nil);
            }
            rc_decr(obj);
        }
        break;
    }
}

