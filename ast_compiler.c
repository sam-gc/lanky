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

static Hashmap saved_locals;

static char save_val;

void compile(ast_node *root);
void compile_compound(ast_node *root);
void compile_single_if(ast_if_node *node, int tagOut, int tagNext);

typedef struct tag_node {
    struct tag_node *next;
    long tag;
    char line;
} tag_node;

char get_line(tag_node *node, long tag)
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

void append_tag(tag_node *node, long tag, char line)
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

int local_idx = 0;
int get_next_local()
{
    return local_idx++;
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
        break;
    default:
        printf("--> %d\n", wrap.type);
        return NULL;
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
        lky_object_builtin *obj = arr_get(&rops, i);
        ops[i] = (char)obj->value.i;
    }

    return ops;
}

void append_op(long ins)
{
    lky_object *obj = lobjb_build_int(ins);
    pool_add(&ast_memory_pool, obj);
    arr_append(&rops, obj);
}

void compile_binary(ast_node *root)
{
    ast_binary_node *node = (ast_binary_node *)root;

    if(node->opt != '=')
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
    case 'l':
        istr = LI_BINARY_LT;
        break;
    case '=':
        {
            append_op((char)LI_SAVE_LOCAL);
            char *sid = ((ast_value_node *)(node->left))->value.s;
            hm_error_t err;
            int idx;
            lky_object_builtin *o = hm_get(&saved_locals, sid, &err);
            if(err == HM_KEY_NOT_FOUND)
            {
                idx = get_next_local();
                lky_object *obj = lobjb_build_int(idx);
                pool_add(&ast_memory_pool, obj);
                hm_put(&saved_locals, sid, obj);
            }
            else
                idx = o->value.i;

            append_op(idx);
            save_val = 1;
            return;
        }
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
    append_op(tagOut);

    compile_compound(node->payload->next);

    if(node->onloop)
        compile(node->onloop);

    append_op(LI_JUMP);
    append_op(start);
    append_op(tagOut);

    save_val = 1;
}

void compile_if(ast_node *root)
{
    int tagNext = next_if_tag();
    int tagOut = next_if_tag();

    ast_if_node *node = (ast_if_node *)root;

    if(!node->next_if)
    {
        compile_single_if(node, tagNext, tagNext);
        append_op(tagNext);
        save_val = 1;
        return;
    }

    compile_single_if(node, tagOut, tagNext);

    node = (ast_if_node *)node->next_if;
    while(node)
    {
        append_op(tagNext);
        tagNext = next_if_tag();
        compile_single_if(node, tagOut, tagNext);
        node = (ast_if_node *)node->next_if;
    }

    append_op(tagOut);

    save_val = 1;
}

void compile_single_if(ast_if_node *node, int tagOut, int tagNext)
{
    if(node->condition)
    {
        compile(node->condition);

        append_op(LI_JUMP_FALSE);
        append_op(tagNext);
    }

    compile_compound(node->payload->next);

    // This is used if we want to return the last
    // line of if statements. It is broken and
    // should not be used.
    // if(arr_get(&rops, rops.count - 1) == LI_POP)
    //     arr_remove(&rops, NULL, rops.count - 1);

    if(tagOut != tagNext && node->condition)
    {
        append_op(LI_JUMP);
        append_op(tagOut);
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

void compile_var(ast_value_node *node)
{
    lky_object_builtin *obj = hm_get(&saved_locals, node->value.s, NULL);

    int idx = obj->value.i;

    append_op(LI_LOAD_LOCAL);
    append_op((char)idx);
}

void compile_value(ast_node *root)
{
    ast_value_node *node = (ast_value_node *)root;

    if(node->value_type == VVAR)
    {
        compile_var(node);
        return;
    }

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
        default:
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
        long op = ((lky_object_builtin *)arr_get(&rops, i))->value.i;
        if(op > 999)
        {
            char line = get_line(tags, op);
            if(line < 0)
            {
                append_tag(tags, op, i);

                lky_object *obj = lobjb_build_int(LI_IGNORE);
                pool_add(&ast_memory_pool, obj);
                rops.items[i] = obj;
                continue;
            }
            lky_object *obj = lobjb_build_int(line);
            pool_add(&ast_memory_pool, obj);
            rops.items[i] = obj;
        }
    }

    free_tag_nodes(tags);
}

void **make_cons_array()
{
    void **data = malloc(sizeof(void *) * rcon.count);

    long i;
    for(i = 0; i < rcon.count; i++)
    {
        data[i] = arr_get(&rcon, i);
    }

    return data;
}

lky_object_code *compile_ast(ast_node *root)
{
    rops = arr_create(50);
    rcon = arr_create(10);
    saved_locals = hm_create(100, 1);

    compile_compound(root);
    replace_tags();

    lky_object_code *code = malloc(sizeof(lky_object_code));
    code->constants = make_cons_array();
    code->num_constants = rcon.count;
    code->num_locals = local_idx;
    code->ops = finalize_ops();
    code->op_len = rops.count;
    code->locals = malloc(sizeof(void *) * local_idx);

    int i;
    for(i = 0; i < local_idx; i++)
        code->locals[i] = NULL;

    return code;
}

void write_to_file(char *name, lky_object_code *code)
{
    FILE *f = fopen(name, "w");

    void **cons = code->constants;
    fwrite(&(code->num_constants), sizeof(long), 1, f);
    fwrite(&(code->num_locals), sizeof(long), 1, f);
    int i;
    for(i = 0; i < code->num_constants; i++)
    {
        lky_object *obj = cons[i];
        lobjb_serialize(obj, f);
    }

    fwrite(&(code->op_len), sizeof(long), 1, f);

    fwrite(code->ops, sizeof(char), code->op_len, f);

    fclose(f);
}
