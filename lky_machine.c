#include <stdio.h>
#include <stdlib.h>
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"

#define PUSH(data) (push_node(data))
#define POP() (pop_node())

#define PUSH_RC(data) push_node(data); rc_incr(data)

#define POP_TWO() lky_object *a = POP(); lky_object *b = POP()
#define RC_TWO() rc_decr(a); rc_decr(b)

void eval();

typedef struct stack_node {
    struct stack_node *next;
    void *data;
} stack_node;

static stack_node *main_stack;
static arraylist constants;
// char ops[6] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_BINARY_DIVIDE, LI_PRINT};
// char ops[9] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_LOAD_CONST, 2, LI_BINARY_ADD, LI_BINARY_MULTIPLY, LI_PRINT};
static int pc = 0;
static char *ops;
static long tape_len;

static void push_node(void *data)
{
    stack_node *node = malloc(sizeof(stack_node));
    node->next = main_stack;
    node->data = data;
    main_stack = node;
}

static void *pop_node()
{
    stack_node *node = main_stack;
    main_stack = node->next;
    void *data = node->data;
    free(node);

    return data;
}

static void print_stack()
{
    stack_node *node = main_stack;

    while(node->next)
    {
        lobjb_print(node->data);
        node = node->next;
    }
}

int main()
{
    // constants = arr_create(10);

    // lky_object *a = lobjb_build_int(98);
    // lky_object *b = lobjb_build_int(12);
    // lky_object *c = lobjb_build_int(45);
    // arr_append(&constants, a);
    // arr_append(&constants, b);
    // arr_append(&constants, c);

    // rc_incr(a);
    // rc_incr(b);

    main_stack = malloc(sizeof(stack_node));
    main_stack->next = NULL;
    main_stack->data = NULL;

    // ops = 

    // FILE *f = fopen("test", "r");

    // long len;
    // char type;
    // fread(&len, sizeof(long), 1, f);
    // printf("Size of constants: %ld\n", len);

    // fread(&len, sizeof(long), 1, f);
    // printf("%ld\n", len);
    // fread(&type, sizeof(char), 1, f);
    // long tmp1, tmp2;
    // fread(&tmp1, len, 1, f);

    // fread(&len, sizeof(long), 1, f);
    // fread(&type, sizeof(char), 1, f);
    // fread(&tmp2, len, 1, f);

    // printf("Constants: %ld %ld\n", tmp1, tmp2);

    // char instructions[3] = {}

    lky_object_code *code = lobjb_load_file("test");
    constants = code->constants;
    tape_len = code->op_len;
    ops = code->ops;

    eval();

    return 0;
}

void do_op(lky_instruction op);

void eval()
{
    pc = -1;
    while(pc < tape_len)
    {
        do_op(ops[++pc]);
    }
}

void do_op(lky_instruction op)
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
            lky_object *a = POP();
            rc_decr(a);
        }
        break;
    }
}

