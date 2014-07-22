#include "ast_compiler.h"
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"
#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static arraylist rops;
static arraylist rcon;

static char save_val;

void compile(ast_node *root);
int lookahead(ast_node *node, ast_node *tg);
void lookahead_single(ast_node *node, int *ct);
void compile_compound(ast_node *root);
void compile_single_if(ast_if_node *node, int tagOut, int tagNext);

typedef struct tag_node {
    struct tag_node *next;
    long tag;
    char line;
} tag_node;

char get_line(tag_node *node, tag)
{
    for(; node; node = node->next)
    {
        if(tag == node->tag)
            return node->line;
    }

    return -1;
}

tag_node *make_node()
{
    tag_node *node = malloc(sizeof(tag_node));
    node->next = NULL;
    node->tag = 0;
    node->line = -1;

    return node;
}

void *append_tag(tag_node *node, long tag, char line)
{
    for(; node->next; node = node->next);

    tag_node *n = make_node();
    n->tag = tag;
    n->line = line;

    node->next = n;
}

void free_tag_nodes(tag_node *node)
{
    while(node)
    {
        tag_node *next = node->next;
        free(node);
        node = next;
    }
}

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
    case VSTRING:
        t = LBI_STRING;
        v.s = malloc(strlen(wrap.value.s) + 1);
        strcpy(v.s, wrap.value.s);
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

int ifTag = 1000;
int next_if_tag()
{
    return ifTag++;
}

void compile_loop(ast_node *root)
{
    int tagOut = next_if_tag();

    ast_loop_node *node = (ast_loop_node *)root;

    if(node->init)
        compile(node->init);

    int start = rops.count;

    compile(node->condition);
    append_op(LI_JUMP_FALSE);
    arr_append(&rops, tagOut);

    compile_compound(node->payload->next);

    if(node->onloop)
        compile(node->onloop);

    arr_append(&rops, LI_JUMP);
    arr_append(&rops, (char)start);
    arr_append(&rops, tagOut);
}

void compile_if(ast_node *root)
{
    int tagNext = next_if_tag();
    int tagOut = next_if_tag();

    ast_if_node *node = (ast_if_node *)root;

    if(!node->next_if)
    {
        compile_single_if(node, tagNext, tagNext);
        arr_append(&rops, tagNext);
        return;
    }

    compile_single_if(node, tagOut, tagNext);

    node = node->next_if;
    while(node)
    {
        arr_append(&rops, tagNext);
        tagNext = next_if_tag();
        compile_single_if(node, tagOut, tagNext);
        node = node->next_if;
    }

    arr_append(&rops, tagOut);
}

void compile_single_if(ast_if_node *node, int tagOut, int tagNext)
{
    if(node->condition)
    {
        compile(node->condition);

        append_op(LI_JUMP_FALSE);
        arr_append(&rops, tagNext);
    }

    compile_compound(node->payload->next);

    if(arr_get(&rops, rops.count - 1) == LI_POP)
        arr_remove(&rops, NULL, rops.count - 1);

    if(tagOut != tagNext && node->condition)
    {
        append_op(LI_JUMP);
        arr_append(&rops, tagOut);
    }
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

long find_prev_const(lky_object *obj)
{
    long i;
    for(i = 0; i < rcon.count; i++)
    {
        lky_object *o = arr_get(&rcon, i);
        if(lobjb_quick_compare(obj, o))
            return i;
    }

    return -1;
}

void compile_value(ast_node *root)
{
    ast_value_node *node = (ast_value_node *)root;

    lky_object *obj = wrapper_to_obj(node_to_wrapper(node));
    rc_incr(obj);

    long idx = find_prev_const(obj);

    if(idx < 0)
    {
        idx = rcon.count;
        arr_append(&rcon, obj);
    }

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
    case AIF:
        compile_if(root);
    break;
    case ALOOP:
        compile_loop(root);
    break;
    }
}

void compile_compound(ast_node *root)
{
    while(root)
    {
        save_val = 0;
        compile(root);
        if(!save_val)
            append_op(LI_POP);

        root = root->next;
    }
}

void replace_tags()
{
    tag_node *tags = make_node();

    long i;
    for(i = rops.count - 1; i >= 0; i--)
    {
        long op = arr_get(&rops, i);
        if(op > 999)
        {
            char line = get_line(tags, op);
            if(line < 0)
            {
                append_tag(tags, op, i);
                rops.items[i] = LI_IGNORE;
                continue;
            }

            rops.items[i] = line;
        }
    }

    free_tag_nodes(tags);
}

lky_object_code *compile_ast(ast_node *root)
{
    rops = arr_create(50);
    rcon = arr_create(10);

    compile_compound(root);
    replace_tags();

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

int lookahead_save_val;

void lookahead_value(ast_node *n, int *ct)
{
    ast_value_node *node = (ast_value_node *)n;

    switch(node->value_type)
    {
        case VSTRING:
        case VDOUBLE:
        case VINT:
            *ct += 2;
        break;
    }
}

void lookahead_binary(ast_node *n, int *ct)
{
    ast_binary_node *node = (ast_binary_node *)n;

    lookahead_single(node->left, ct);
    lookahead_single(node->right, ct);

    *ct++;
}

void lookahead_unary(ast_node *n, int *ct)
{
    ast_unary_node *node = (ast_unary_node *)n;

    lookahead_single(node->target, ct);

    *ct++;
    lookahead_save_val = 1;
}

void lookahead_single(ast_node *node, int *ct)
{
    switch(node->type)
    {
        case AVALUE:
            lookahead_value(node, ct);
        break;
        case ABINARY_EXPRESSION:
            lookahead_binary(node, ct);
        break;
        case AUNARY_EXPRESSION:
            lookahead_unary(node, ct);
        break;
    }
}

int lookahead(ast_node *node, ast_node *tg)
{
    printf("%p ... \n", node);
    int ct = 0;

    while(node != tg && node)
    {
        lookahead_save_val = 0;

        lookahead_single(node, &ct);

        if(!lookahead_save_val)
            ct++;

        node = node->next;
    }

    return ct;
}
