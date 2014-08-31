#include "ast_compiler.h"
#include "arraylist.h"
#include "instruction_set.h"
#include "lkyobj_builtin.h"
#include "hashmap.h"
#include "bytecode_analyzer.h"
#include "stl_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    arraylist rops;
    arraylist rcon;
    arraylist rnames;
    Hashmap saved_locals;
    char save_val;
    int local_idx;
    int ifTag;
    int name_idx;
    int classargc;
} compiler_wrapper;

void compile(compiler_wrapper *cw, ast_node *root);
void compile_compound(compiler_wrapper *cw, ast_node *root);
void compile_single_if(compiler_wrapper *cw, ast_if_node *node, int tagOut, int tagNext);
lky_object_code *compile_ast_ext(ast_node *root, compiler_wrapper *incw);
void compile_set_member(compiler_wrapper *cw, ast_node *root);
void compile_set_index(compiler_wrapper *cw, ast_node *root);
int find_prev_name(compiler_wrapper *cw, char *name);
void int_to_byte_array(unsigned char *buffer, int val);

typedef struct tag_node {
    struct tag_node *next;
    long tag;
    long line;
} tag_node;

long get_line(tag_node *node, long tag)
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

void append_tag(tag_node *node, long tag, long line)
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

int get_next_local(compiler_wrapper *cw)
{
    return cw->local_idx++;
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
        {
            return stlstr_cinit(wrap.value.s);
        }
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

unsigned char *finalize_ops(compiler_wrapper *cw)
{
    unsigned char *ops = malloc(cw->rops.count);

    long i;
    for(i = 0; i < cw->rops.count; i++)
    {
        lky_object_builtin *obj = arr_get(&cw->rops, i);
        ops[i] = (unsigned char)obj->value.i;
    }

    return ops;
}

void append_op(compiler_wrapper *cw, long ins)
{
    lky_object *obj = lobjb_build_int(ins);
    pool_add(&ast_memory_pool, obj);
    arr_append(&cw->rops, obj);
}

void compile_binary(compiler_wrapper *cw, ast_node *root)
{
    ast_binary_node *node = (ast_binary_node *)root;

    if(node->opt == '=' && node->left->type == AMEMBER_ACCESS)
    {
        char *sid = ((ast_value_node *)(node->left))->value.s;
        if(!strcmp(sid, "build_"))
        {
            ast_node *r = node->right;
            if(r->type != AFUNC_DECL) { /* TODO: Compiler error */ }
            ast_func_decl_node *n = (ast_func_decl_node *)r;
            ast_node *arg = NULL;
            int args = 0;
            for(arg = n->params; arg; arg = arg->next)
                args++;

            cw->classargc = args;
        }

        compile_set_member(cw, root);
        return;
    }
    else if(node->opt == '=' && node->left->type == AINDEX)
    {
        compile_set_index(cw, root);
        return;
    }

    if(node->opt != '=')
        compile(cw, node->left);

    compile(cw, node->right);

    lky_instruction istr = LI_IGNORE;
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
    case '%':
        istr = LI_BINARY_MODULO;
        break;
    case 'l':
        istr = LI_BINARY_LT;
        break;
    case 'g':
        istr = LI_BINARY_GT;
        break;
    case 'L':
        istr = LI_BINARY_LTE;
        break;
    case 'G':
        istr = LI_BINARY_GTE;
        break;
    case 'E':
        istr = LI_BINARY_EQUAL;
        break;
    case 'n':
        istr = LI_BINARY_NE;
        break;
    case '&':
        istr = LI_BINARY_AND;
        break;
    case '|':
        istr = LI_BINARY_OR;
        break;
    case '=':
        {
            append_op(cw, LI_SAVE_CLOSE);
            char *sid = ((ast_value_node *)(node->left))->value.s;
            char *nsid = malloc(strlen(sid) + 1);
            strcpy(nsid, sid);

            int idx = find_prev_name(cw, nsid);
            if(idx < 0)
            {
                idx = (int)cw->rnames.count;
                arr_append(&cw->rnames, nsid);
            }

            append_op(cw, idx);
            return;
        }
        break;
//        {
//             append_op(cw, (char)LI_SAVE_LOCAL);
//             char *sid = ((ast_value_node *)(node->left))->value.s;
//             hm_error_t err;
//             int idx;
//             lky_object_builtin *o = hm_get(&cw->saved_locals, sid, &err);
//             if(err == HM_KEY_NOT_FOUND)
//             {
//                 idx = get_next_local(cw);
//                 lky_object *obj = lobjb_build_int(idx);
//                 pool_add(&ast_memory_pool, obj);
//                 hm_put(&cw->saved_locals, sid, obj);
//             }
//             else
//                 idx = o->value.i;
// 
//             // printf("==> %s %d\n", sid, idx);
// 
//             append_op(cw, idx);
//             // save_val = 1;
//             return;
//         }
//         break;
    }

    append_op(cw, (char)istr);
}

int next_if_tag(compiler_wrapper *cw)
{
    return cw->ifTag++;
}

void compile_loop(compiler_wrapper *cw, ast_node *root)
{
    int tagOut = next_if_tag(cw);

    ast_loop_node *node = (ast_loop_node *)root;

    if(node->init)
    {
        char save = cw->save_val;
        cw->save_val = 0;
        compile(cw, node->init);
        if(!cw->save_val)
            append_op(cw, LI_POP);
        cw->save_val = save;
    }

    int start = (int)cw->rops.count;

    compile(cw, node->condition);
    append_op(cw, LI_JUMP_FALSE);
    append_op(cw, tagOut);
    append_op(cw, -1);
    append_op(cw, -1);
    append_op(cw, -1);

    compile_compound(cw, node->payload->next);

    if(node->onloop)
    {
        char save = cw->save_val;
        cw->save_val = 0;
        compile(cw, node->onloop);
        if(!cw->save_val)
            append_op(cw, LI_POP);
        cw->save_val = save;
    }

    append_op(cw, LI_JUMP);
    unsigned char buf[4];
    int_to_byte_array(buf, start);

    append_op(cw, buf[0]);
    append_op(cw, buf[1]);
    append_op(cw, buf[2]);
    append_op(cw, buf[3]);
    append_op(cw, tagOut);

    cw->save_val = 1;
}

void compile_if(compiler_wrapper *cw, ast_node *root)
{
    int tagNext = next_if_tag(cw);
    int tagOut = next_if_tag(cw);

    ast_if_node *node = (ast_if_node *)root;

    if(!node->next_if)
    {
        compile_single_if(cw, node, tagNext, tagNext);
        append_op(cw, tagNext);
        cw->save_val = 1;
        return;
    }

    compile_single_if(cw, node, tagOut, tagNext);

    node = (ast_if_node *)node->next_if;
    while(node)
    {
        append_op(cw, tagNext);
        tagNext = node->next_if ? next_if_tag(cw) : tagOut;
        compile_single_if(cw, node, tagOut, tagNext);
        node = (ast_if_node *)node->next_if;
    }

    append_op(cw, tagOut);
    // printf("-%d\n", tagOut);

    cw->save_val = 1;
}

void compile_single_if(compiler_wrapper *cw, ast_if_node *node, int tagOut, int tagNext)
{
    if(node->condition)
    {
        compile(cw, node->condition);

        append_op(cw, LI_JUMP_FALSE);
        append_op(cw, tagNext);
        append_op(cw, -1);
        append_op(cw, -1);
        append_op(cw, -1);
        // printf("%d\n", tagNext);
    }

    compile_compound(cw, node->payload->next);

    // This is used if we want to return the last
    // line of if statements. It is broken and
    // should not be used.
    // if(arr_get(&rops, rops.count - 1) == LI_POP)
    //     arr_remove(&rops, NULL, rops.count - 1);

    if(tagOut != tagNext && node->condition)
    {
        append_op(cw, LI_JUMP);
        append_op(cw, tagOut);
        append_op(cw, -1);
        append_op(cw, -1);
        append_op(cw, -1);
    }
}

void compile_array(compiler_wrapper *cw, ast_node *n)
{
    ast_array_node *node = (ast_array_node *)n;

    int ct = 0;
    ast_node *list = node->list;
    for(; list; list = list->next)
    {
        compile(cw, list);
        ct++;
    }

    append_op(cw, LI_MAKE_ARRAY);

    unsigned char buf[4];
    int_to_byte_array(buf, ct);

    append_op(cw, buf[0]);
    append_op(cw, buf[1]);
    append_op(cw, buf[2]);
    append_op(cw, buf[3]);
}

void compile_indx(compiler_wrapper *cw, ast_node *n)
{
    ast_index_node *node = (ast_index_node *)n;

    compile(cw, node->target);
    compile(cw, node->indexer);

    append_op(cw, LI_LOAD_INDEX);
}

void compile_ternary(compiler_wrapper *cw, ast_node *n)
{
    ast_ternary_node *node = (ast_ternary_node *)n;

    int tagNext = next_if_tag(cw);
    int tagOut = next_if_tag(cw);

    // Example: a ? b : c

    // Compile 'a'
    compile(cw, node->condition);
    append_op(cw, LI_JUMP_FALSE);
    append_op(cw, tagNext);
    append_op(cw, -1);
    append_op(cw, -1);
    append_op(cw, -1);

    // Compile 'b'
    compile(cw, node->first);
    append_op(cw, LI_JUMP);
    append_op(cw, tagOut);
    append_op(cw, -1);
    append_op(cw, -1);
    append_op(cw, -1);

    // Compile 'c'
    append_op(cw, tagNext);
    compile(cw, node->second);

    // Jump out
    append_op(cw, tagOut);
}

int find_prev_name(compiler_wrapper *cw, char *name)
{
    long i;
    for(i = 0; i < cw->rnames.count; i++)
    {
        char *n = arr_get(&cw->rnames, i);
        if(strcmp(name, n) == 0)
            return (int)i;
    }

    return -1;
}

void compile_member_access(compiler_wrapper *cw, ast_node *n)
{
    ast_member_access_node *node = (ast_member_access_node *)n;

    char *name = node->ident;

    int idx = find_prev_name(cw, name);

    if(idx < 0)
    {
        idx = (int)cw->rnames.count;
        arr_append(&cw->rnames, name);
    }

    compile(cw, node->object);
    append_op(cw, LI_LOAD_MEMBER);
    append_op(cw, idx);
}

void compile_set_member(compiler_wrapper *cw, ast_node *root)
{
    ast_binary_node *bin = (ast_binary_node *)root;
    ast_member_access_node *left = (ast_member_access_node *)bin->left;
    ast_node *right = bin->right;

    compile(cw, right);

    compile(cw, left->object);

    int idx = find_prev_name(cw, left->ident);

    if(idx < 0)
    {
        idx = (int)cw->rnames.count;
        arr_append(&cw->rnames, left->ident);
    }

    append_op(cw, LI_SAVE_MEMBER);
    append_op(cw, idx);
}

void compile_set_index(compiler_wrapper *cw, ast_node *root)
{
    ast_binary_node *bin = (ast_binary_node *)root;
    ast_index_node *left = (ast_index_node *)bin->left;
    ast_node *right = bin->right;

    compile(cw, right);
    compile(cw, left->target);
    compile(cw, left->indexer);

    append_op(cw, LI_SAVE_INDEX);
}

void compile_unary(compiler_wrapper *cw, ast_node *root)
{
    ast_unary_node *node = (ast_unary_node *)root;

    // Only if there actually is a target. We use a unary
    // node for lky_nil for convenience, but that does
    // not have a target itself.
    if(node->target)
        compile(cw, node->target);

    lky_instruction istr = LI_IGNORE;
    switch(node->opt)
    {
    case 'p':
        istr = LI_PRINT;
        cw->save_val = 1;
        break;
    case 'r':
        istr = LI_RETURN;
        cw->save_val = 1;
        break;
    case '!':
        istr = LI_UNARY_NOT;
        break;
    case '0':
        istr = LI_PUSH_NIL;
        break;
    }

    append_op(cw, (char)istr);
}

long find_prev_const(compiler_wrapper *cw, lky_object *obj)
{
    long i;
    for(i = 0; i < cw->rcon.count; i++)
    {
        lky_object *o = arr_get(&cw->rcon, i);
        if(lobjb_quick_compare(obj, o))
            return i;
    }

    return -1;
}

void compile_var(compiler_wrapper *cw, ast_value_node *node)
{
    int idx = find_prev_name(cw, node->value.s);

    if(idx < 0)
    {
        idx = (int)cw->rnames.count;
        char *ns = malloc(strlen(node->value.s) + 1);
        strcpy(ns, node->value.s);
        arr_append(&cw->rnames, ns);
    }

    append_op(cw, LI_LOAD_CLOSE);
    append_op(cw, idx);
//    lky_object_builtin *obj = hm_get(&cw->saved_locals, node->value.s, NULL);
//
//    int idx = obj->value.i;
//
//    append_op(cw, LI_LOAD_LOCAL);
//    append_op(cw, (char)idx);
}

void compile_value(compiler_wrapper *cw, ast_node *root)
{
    ast_value_node *node = (ast_value_node *)root;

    if(node->value_type == VVAR)
    {
        compile_var(cw, node);
        return;
    }

    lky_object *obj = wrapper_to_obj(node_to_wrapper(node));
    rc_incr(obj);

    long idx = find_prev_const(cw, obj);

    if(idx < 0)
    {
        idx = cw->rcon.count;
        arr_append(&cw->rcon, obj);
    }

    append_op(cw, LI_LOAD_CONST);
    append_op(cw, (char)idx);
}

void compile_function(compiler_wrapper *cw, ast_node *root)
{
    ast_func_decl_node *node = (ast_func_decl_node *)root;
    
    compiler_wrapper nw;
    nw.local_idx = 0;
    nw.saved_locals = hm_create(100, 1);
    nw.rnames = arr_create(10);
    
    int argc = 0;
    ast_value_node *v = (ast_value_node *)node->params;
    for(; v; v = (ast_value_node *)v->next)
    {
        char *idf = v->value.s;

        char *nid = malloc(strlen(idf) + 1);
        strcpy(nid, idf);

        arr_append(&nw.rnames, nid);

       //  long idx = get_next_local(&nw);
       //  lky_object *obj = lobjb_build_int(idx);
       //  pool_add(&ast_memory_pool, obj);
       //  hm_put(&nw.saved_locals, v->value.s, obj);
       argc++;
    }
    
    nw.save_val = 0;
    lky_object_code *code = compile_ast_ext(node->payload->next, &nw);

    rc_incr((lky_object *)code);
    
    long idx = cw->rcon.count;
    arr_append(&cw->rcon, code);
    
    append_op(cw, LI_LOAD_CONST);
    append_op(cw, idx);
    append_op(cw, LI_MAKE_FUNCTION);
    append_op(cw, argc);
}

void compile_class_decl(compiler_wrapper *cw, ast_node *root)
{
    ast_class_decl_node *node = (ast_class_decl_node *)root;

    compiler_wrapper nw;
    nw.local_idx = 0;
    nw.saved_locals = hm_create(100, 1);
    nw.rnames = arr_create(10);

    long idx = find_prev_name(cw, node->refname);

    if(idx < 0)
    {
        idx = cw->rnames.count;
        char *nid = malloc(strlen(node->refname) + 1);
        strcpy(nid, node->refname);
        arr_append(&cw->rnames, nid);
    }

    nw.save_val = 0;
    lky_object_code *code = compile_ast_ext(node->payload->next, &nw);

    rc_incr((lky_object *)code);

    long cidx = cw->rcon.count;
    arr_append(&cw->rcon, code);

    append_op(cw, LI_LOAD_CONST);
    append_op(cw, cidx);
    append_op(cw, LI_MAKE_FUNCTION);
    append_op(cw, nw.classargc);
    printf("%d --- \n", nw.classargc);
    append_op(cw, LI_MAKE_CLASS);
    append_op(cw, idx);
}

void compile_function_call(compiler_wrapper *cw, ast_node *root)
{
    ast_func_call_node *node = (ast_func_call_node *)root;

    ast_node *arg = node->arguments;

    for(; arg; arg = arg->next)
        compile(cw, arg);

    compile(cw, node->ident);
    append_op(cw, LI_CALL_FUNC);

    // cw->save_val = 1;
}

void compile(compiler_wrapper *cw, ast_node *root)
{
    switch(root->type)
    {
        case ABINARY_EXPRESSION:
            compile_binary(cw, root);
        break;
        case AUNARY_EXPRESSION:
            compile_unary(cw, root);
        break;
        case AVALUE:
            compile_value(cw, root);
        break;
        case AIF:
            compile_if(cw, root);
        break;
        case ALOOP:
            compile_loop(cw, root);
        break;
        case AFUNC_DECL:
            compile_function(cw, root);
        break;
        case AFUNC_CALL:
            compile_function_call(cw, root);
        break;
        case ACLASS_DECL:
            compile_class_decl(cw, root);
        break;
        case ATERNARY:
            compile_ternary(cw, root);
        break;
        case AMEMBER_ACCESS:
            compile_member_access(cw, root);
        break;
        case AARRAY:
            compile_array(cw, root);
        break;
        case AINDEX:
            compile_indx(cw, root);
        break;
        default:
        break;
    }
}

void compile_compound(compiler_wrapper *cw, ast_node *root)
{
    while(root)
    {
        cw->save_val = 0;
        compile(cw, root);
        if(!cw->save_val)
            append_op(cw, LI_POP);

        root = root->next;
    }
}

// TODO: We should probably worry about endienness
void int_to_byte_array(unsigned char *buffer, int val)
{
    buffer[3] = (val >> 24) & 0xFF;
    buffer[2] = (val >> 16) & 0xFF;
    buffer[1] = (val >> 8)  & 0xFF;
    buffer[0] = val         & 0xFF;
}

void replace_tags(compiler_wrapper *cw)
{
    tag_node *tags = make_node();

    long i;
    for(i = cw->rops.count - 1; i >= 0; i--)
    {
        long op = ((lky_object_builtin *)arr_get(&cw->rops, i))->value.i;
        if(op < 0)
            continue;

        if(op > 999)
        {
            long line = get_line(tags, op);
            if(line < 0)
            {
                append_tag(tags, op, i);

                lky_object *obj = lobjb_build_int(LI_IGNORE);
                pool_add(&ast_memory_pool, obj);
                cw->rops.items[i] = obj;
                continue;
            }

            unsigned char buf[4];
            int_to_byte_array(buf, (int)line);

            int j;
            for(j = 0; j < 4; j++)
            {
                lky_object *obj = lobjb_build_int(buf[j]);
                pool_add(&ast_memory_pool, obj);
                cw->rops.items[i + j] = obj;
            }
        }
    }

    free_tag_nodes(tags);
}

void **make_cons_array(compiler_wrapper *cw)
{
    void **data = malloc(sizeof(void *) * cw->rcon.count);

    long i;
    for(i = 0; i < cw->rcon.count; i++)
    {
        data[i] = arr_get(&cw->rcon, i);
    }

    return data;
}

char **make_names_array(compiler_wrapper *cw)
{
    char **names = malloc(sizeof(char *) * cw->rnames.count);

    long i;
    for(i = 0; i < cw->rnames.count; i++)
    {
        char *txt = arr_get(&cw->rnames, i);
        char *nw = malloc(strlen(txt) + 1);
        strcpy(nw, txt);
        names[i] = nw;
    }

    return names;
}

lky_object_code *compile_ast_ext(ast_node *root, compiler_wrapper *incw)
{
    compiler_wrapper cw;
    cw.rops = arr_create(50);
    cw.rcon = arr_create(10);
    cw.ifTag = 1000;
    cw.save_val = 0;
    cw.name_idx = 0;
    cw.classargc = 0;
    
    if(incw)
    {
        cw.saved_locals = incw->saved_locals;
        cw.local_idx = incw->local_idx;
        cw.rnames = incw->rnames;
    }
    else
    {
        cw.saved_locals = hm_create(100, 1);
        cw.local_idx = 0;
        cw.rnames = arr_create(50);
    }


    compile_compound(&cw, root);
    replace_tags(&cw);

    // We want to propogate the classargc
    // (number of args passed to build_)
    // back up to the caller.
    if(incw)
        incw->classargc = cw.classargc;
    
    append_op(&cw, LI_PUSH_NIL);
    append_op(&cw, LI_RETURN);

    lky_object_code *code = malloc(sizeof(lky_object_code));
    code->constants = make_cons_array(&cw);
    code->num_constants = cw.rcon.count;
    code->num_locals = cw.local_idx;
    code->size = sizeof(lky_object_code);
    code->mem_count = 0;
    code->type = LBI_CODE;
    code->num_names = cw.rnames.count;
    code->members = trie_new();
    code->ops = finalize_ops(&cw);
    code->op_len = cw.rops.count;
    code->locals = malloc(sizeof(void *) * cw.local_idx);
    code->names = make_names_array(&cw);
    code->cls = NULL;
    code->stack_size = calculate_max_stack_depth(code->ops, (int)code->op_len);

    int i;
    for(i = 0; i < cw.local_idx; i++)
        code->locals[i] = NULL;

    return code;
}

lky_object_code *compile_ast(ast_node *root)
{
    return compile_ast_ext(root, NULL);
}

void write_to_file(char *name, lky_object_code *code)
{
    FILE *f = fopen(name, "w");

    void **cons = code->constants;
    fwrite(&(code->num_constants), sizeof(long), 1, f);
    fwrite(&(code->num_locals), sizeof(long), 1, f);
    fwrite(&(code->num_names), sizeof(long), 1, f);
    fwrite(&(code->stack_size), sizeof(long), 1, f);

    int i;
    for(i = 0; i < code->num_names; i++)
    {
        char *str = code->names[i];
        long len = strlen(str) + 1;
        fwrite(&len, sizeof(long), 1, f);
        fwrite(str, sizeof(unsigned char), len, f);
    }

    for(i = 0; i < code->num_constants; i++)
    {
        lky_object *obj = cons[i];
        lobjb_serialize(obj, f);
    }

    fwrite(&(code->op_len), sizeof(long), 1, f);

    fwrite(code->ops, sizeof(char), code->op_len, f);

    fclose(f);
}
