#include <stdio.h>
#include <stdlib.h>
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"
#include "hashmap.h"

#define PUSH(data) (push_node(data))
#define POP() (pop_node())

#define PUSH_RC(data) push_node(data); rc_incr(data)
#define POP_RC() rc_decr(POP())

#define POP_TWO() lky_object *a = POP(); lky_object *b = POP()
#define RC_TWO() rc_decr(a); rc_decr(b)

void print_ops();

typedef struct stackframe {
    arraylist data_stack;
    arraylist constants;
    arraylist locals;
    long pc;
    char *ops;
    long tape_len;
    struct stackframe *next;
} stackframe;

static stackframe frame;



void mach_eval();

// static arraylist main_stack;
// static arraylist constants;
// char ops[6] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_BINARY_DIVIDE, LI_PRINT};
// char ops[9] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_LOAD_CONST, 2, LI_BINARY_ADD, LI_BINARY_MULTIPLY, LI_PRINT};
// static int pc = 0;
// static char *ops;
// static long tape_len;

int pushes = 0;

void push_node(void *data)
{
    arr_append(&frame.data_stack, data);
    pushes++;
}

static void *pop_node()
{
    void *data = arr_get(&frame.data_stack, frame.data_stack.count - 1);
    arr_remove(&frame.data_stack, NULL, frame.data_stack.count - 1);

    pushes--;

    return data;
}

static void print_stack()
{
    printf("===================\n");
    long i;
    for(i = frame.data_stack.count - 1; i > -1; i--)
    {
        lobjb_print(arr_get(&frame.data_stack.count, i));
    }

    printf("===================\n");
}

void mach_execute(lky_object_code *code)
{

    frame.data_stack = arr_create(100);
    frame.constants = code->constants;
    frame.locals = code->locals;
    frame.pc = -1;
    frame.ops = code->ops;
    frame.tape_len = code->op_len;

    arr_append(&frame.data_stack, NULL);

    // printf("%d\n", tape_len);
    
    mach_eval();
    // print_ops();

    // arr_free(&main_stack);
    // arr_free(&constants);
    // free(ops);
}

void mach_do_op(lky_instruction op);

void mach_eval()
{
    while(frame.pc < frame.tape_len)
    {
        mach_do_op(frame.ops[++frame.pc]);
    }

    // printf("%d\n", pushes);
}
void print_op(lky_instruction i);
void mach_do_op(lky_instruction op)
{
    // print_stack();

    // printf("==> %d\n", op);

    switch(op)
    {
        case LI_LOAD_CONST:
        {
            char idx = frame.ops[++frame.pc];
            lky_object *obj = arr_get(&frame.constants, idx);

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
            char idx = frame.ops[++frame.pc];
            frame.pc = idx < frame.pc ? idx - 1 : idx;
        }
        break;
        case LI_JUMP_FALSE:
        {
            lky_object *obj = POP();
            // printf("=> %d\n", obj == &lky_nil || obj->type != LBI_STRING && !OBJ_NUM_UNWRAP(obj));
            if(obj == &lky_nil || obj->type != LBI_STRING && !OBJ_NUM_UNWRAP(obj))
            {
                char idx = frame.ops[++frame.pc];
                frame.pc = idx;
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
            lky_object *obj = POP();
            char idx = frame.ops[++frame.pc];
            lky_object *old = arr_get(&frame.locals, idx);
            if(old)
                rc_decr(old);

            arr_set(&frame.locals, obj, idx);

            // rc_decr(obj);
        }
        break;
        case LI_LOAD_LOCAL:
        {
            char idx = frame.ops[++frame.pc];
            lky_object *obj = arr_get(&frame.locals, idx);
            PUSH_RC(obj);
        }
        break;
        case LI_PUSH_NIL:
        {
            PUSH(&lky_nil);
        }
        break;
    }

    // printf("=> %d\t", frame.data_stack.count);
    // if(frame.pc > 0)
    //     print_op(frame.ops[frame.pc - 1]);
    // printf("\t");
    // print_op(frame.ops[frame.pc]);

    // printf("\n");
}

void print_op(lky_instruction i)
{
    char *name;
    switch(i)
        {
        case LI_BINARY_ADD:
            name = "LI_BINARY_ADD";
            break;
        case LI_BINARY_SUBTRACT:
            name = "LI_BINARY_SUBTRACT";
            break;
        case LI_BINARY_MULTIPLY:
            name = "LI_BINARY_MULTIPLY";
            break;
        case LI_BINARY_DIVIDE:
            name = "LI_BINARY_DIVIDE";
            break;
        case LI_LOAD_CONST:
            name = "LI_LOAD_CONST";
            break;
        case LI_PRINT:
            name = "LI_PRINT";
            break;
        case LI_POP:
            name = "LI_POP";
            break;
        case LI_JUMP_FALSE:
            name = "LI_JUMP_FALSE";
            break;
        case LI_JUMP_TRUE:
            name = "LI_JUMP_TRUE";
            break;
        case LI_JUMP:
            name = "LI_JUMP";
            break;
        case LI_IGNORE:
            name = "LI_IGNORE";
            break;
        case LI_SAVE_LOCAL:
            name = "LI_SAVE_LOCAL";
            break;
        case LI_LOAD_LOCAL:
            name = "LI_LOAD_LOCAL";
            break;
        default:
            printf("%d", i);
            return;
        }
    printf("%s\n",name );
}

void print_ops()
{
    int i;
    for(i = 0; i < frame.tape_len; i++)
    {
        char *name;
        switch(frame.ops[i])
        {
        case LI_BINARY_ADD:
            name = "LI_BINARY_ADD";
            break;
        case LI_BINARY_SUBTRACT:
            name = "LI_BINARY_SUBTRACT";
            break;
        case LI_BINARY_MULTIPLY:
            name = "LI_BINARY_MULTIPLY";
            break;
        case LI_BINARY_DIVIDE:
            name = "LI_BINARY_DIVIDE";
            break;
        case LI_LOAD_CONST:
            name = "LI_LOAD_CONST";
            break;
        case LI_PRINT:
            name = "LI_PRINT";
            break;
        case LI_POP:
            name = "LI_POP";
            break;
        case LI_JUMP_FALSE:
            name = "LI_JUMP_FALSE";
            break;
        case LI_JUMP_TRUE:
            name = "LI_JUMP_TRUE";
            break;
        case LI_JUMP:
            name = "LI_JUMP";
            break;
        case LI_IGNORE:
            name = "LI_IGNORE";
            break;
        case LI_SAVE_LOCAL:
            name = "LI_SAVE_LOCAL";
            break;
        case LI_LOAD_LOCAL:
            name = "LI_LOAD_LOCAL";
            break;
        case LI_PUSH_NIL:
            name = "LI_PUSH_NIL";
            break;
        default:
            printf("%d\n", frame.ops[i]);
            continue;
        }

        printf("%d :: %s\n", i, name);
    }
}

