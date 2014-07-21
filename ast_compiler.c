#include "ast_compiler.h"
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"
#include <stdio.h>
#include <stdlib.h>

static arraylist rops;
static arraylist rcon;

static char save_val;

void compile(ast_node *root);

lky_object *wrapper_to_obj(ast_value_wrapper wrap)
{
    lky_builtin_type t;
    lky_builtin_value v;
    switch(wrap.type)
    {
    case VDOUBLE:
        t = LBI_FLOAT;
        v.d = wrap.value.d;
        break;
    case VINT:
        t = LBI_INTEGER;
        v.i = wrap.value.i;
        break;
    }

    return (lky_object *)lobjb_alloc(t, v);
}

ast_value_wrapper node_to_wrapper(ast_value_node *node)
{
    ast_value_wrapper wrap;

    wrap.type = node->value_type;
    wrap.value = node->value;

    return wrap;
}

char *finalize_ops()
{
    char *ops = malloc(rops.count);

    long i;
    for(i = 0; i < rops.count; i++)
    {
        ops[i] = (char)arr_get(&rops, i);
    }

    return ops;
}

void append_op(char ins)
{
    arr_append(&rops, (void *)ins);
}

void compile_binary(ast_node *root)
{
    ast_binary_node *node = (ast_binary_node *)root;

    compile(node->left);
    compile(node->right);

    lky_instruction istr;
    switch(node->opt)
    {
    case '+':
        istr = LI_BINARY_ADD;
        break;
    case '-':
        istr = LI_BINARY_SUBTRACT;
        break;
    case '*':
        istr = LI_BINARY_MULTIPLY;
        break;
    case '/':
        istr = LI_BINARY_DIVIDE;
        break;
    }

    append_op((char)istr);
}

void compile_unary(ast_node *root)
{
    ast_unary_node *node = (ast_unary_node *)root;

    compile(node->target);

    lky_instruction istr;
    switch(node->opt)
    {
    case 'p':
        istr = LI_PRINT;
        save_val = 1;
        break;
    }

    append_op((char)istr);
}

void compile_value(ast_node *root)
{
    ast_value_node *node = (ast_value_node *)root;

    long idx = rcon.count;

    lky_object *obj = wrapper_to_obj(node_to_wrapper(node));
    arr_append(&rcon, obj);

    append_op(LI_LOAD_CONST);
    append_op((char)idx);
}

void compile(ast_node *root)
{
    switch(root->type)
    {
    case ABINARY_EXPRESSION:
        compile_binary(root);
    break;
    case AUNARY_EXPRESSION:
        compile_unary(root);
    break;
    case AVALUE:
        compile_value(root);
    break;
    }
}

lky_object_code *compile_ast(ast_node *root)
{
    rops = arr_create(50);
    rcon = arr_create(10);

    while(root)
    {
        save_val = 0;
        compile(root);
        if(!save_val)
            append_op(LI_POP);

        root = root->next;
    }

    lky_object_code *code = malloc(sizeof(lky_object_code));
    code->constants = rcon;
    code->ops = finalize_ops();
    code->op_len = rops.count;

    return code;
}

void write_to_file(char *name, lky_object_code *code)
{
    FILE *f = fopen(name, "w");

    arraylist cons = code->constants;
    fwrite(&(cons.count), sizeof(long), 1, f);
    int i;
    for(i = 0; i < cons.count; i++)
    {
        lky_object *obj = arr_get(&cons, i);
        lobjb_serialize(obj, f);
    }

    fwrite(&(code->op_len), sizeof(long), 1, f);

    fwrite(code->ops, sizeof(char), code->op_len, f);

    fclose(f);
}
