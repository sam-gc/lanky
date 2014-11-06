#include "ast.h"
#include "tools.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// In order to not have to worry about memory during
// compilation, use this pool struct to collect bulk-
// free the memory created.
lky_mempool ast_memory_pool = {NULL, NULL};

ast_node *create_root_node()
{
    ast_node *node = MALLOC(sizeof(ast_block_node));
    pool_add(&ast_memory_pool, node);
    node->type = ABLOCK;
    node->next = NULL;

    return node;
}

// Used to chain statements together (i.e. two statements)
void ast_add_node(ast_node *curr, ast_node *next)
{
    ast_node *node = curr;

    while(node->next)
        node = node->next;

    node->next = next;
}

// Helper method to create a value node with a given type
ast_node *create_value_node(ast_value_type type, void *data)
{
    ast_value_node *node = MALLOC(sizeof(ast_value_node));
    pool_add(&ast_memory_pool, node);
    node->type = AVALUE;
    node->next = NULL;

    ast_value_union u;
    switch(type) 
    {
    case VINT:
        u.i = atol((char *)data);
        // FREE(data);
        break;
    case VDOUBLE:
        u.d = atof((char *)data);
        // FREE(data);
        break;
    case VSTRING:
    {
        char *raw = (char *)data;
        char *str = MALLOC(strlen(raw) - 1);
        pool_add(&ast_memory_pool, str);
        memset(str, 0, strlen(raw) - 1);
        memcpy(str, raw + 1, strlen(raw) - 2);
        u.s = (char *)str;
        // FREE(raw);
        break;
    }
    case VVAR:
        u.s = data;
        break;
    default:
//        DEBUG("Shouldn't have reached here...");
        break;
    }

    node->value_type = type;
    node->value = u;

    return (ast_node *)node;
}

ast_node *create_unit_value_node(char *valstr, char *fmt)
{
    ast_unit_value_node *node = MALLOC(sizeof(ast_unit_value_node));
    pool_add(&ast_memory_pool, node);
    node->type = AUNIT;
    node->next = NULL;

    char *raw = (char *)fmt;
    char *str = MALLOC(strlen(raw) - 1);
    pool_add(&ast_memory_pool, str);
    memset(str, 0, strlen(raw) - 1);
    memcpy(str, raw + 1, strlen(raw) - 2);

    node->val = atof(valstr);
    node->fmt = str;

    return (ast_node *)node;
}

ast_node *create_binary_node(ast_node *left, ast_node *right, char opt)
{
    ast_binary_node *node = MALLOC(sizeof(ast_binary_node));
    pool_add(&ast_memory_pool, node);
    node->type = ABINARY_EXPRESSION;
    node->next = NULL;

    node->left = left;
    node->right = right;
    node->opt = opt;

    return (ast_node *)node;
}

ast_node *create_unary_node(ast_node *target, char opt)
{
    ast_unary_node *node = MALLOC(sizeof(ast_unary_node));
    pool_add(&ast_memory_pool, node);
    node->type = AUNARY_EXPRESSION;
    node->next = NULL;

    node->target = target;
    node->opt = opt;

    return (ast_node *)node;
}

ast_node *create_assignment_node(char *left, ast_node *right)
{
    ast_binary_node *node = MALLOC(sizeof(ast_binary_node));
    pool_add(&ast_memory_pool, node);
    node->type = ABINARY_EXPRESSION;
    node->next = NULL;

    ast_node *var = create_value_node(VVAR, (void *)left);
    node->left = var;
    node->right = right;
    node->opt = '=';

    return (ast_node *)node;
}

ast_node *create_block_node(ast_node *payload)
{
    ast_block_node *node = MALLOC(sizeof(ast_block_node));
    pool_add(&ast_memory_pool, node);
    node->type = ABLOCK;
    node->next = NULL;

    node->payload = payload;
    return (ast_node *)node;
}

ast_node *create_array_node(ast_node *payload)
{
    ast_array_node *node = MALLOC(sizeof(ast_array_node));
    pool_add(&ast_memory_pool, node);
    node->type = AARRAY;
    node->next = NULL;

    node->list = payload;
    return (ast_node *)node;
}

ast_node *create_table_node(ast_node *payload)
{
    ast_table_node *node = MALLOC(sizeof(ast_table_node));
    pool_add(&ast_memory_pool, node);
    node->type = ATABLE;
    node->next = NULL;

    node->list = payload;
    return (ast_node *)node;
}

ast_node *create_index_node(ast_node *target, ast_node *indexer)
{
    ast_index_node *node = MALLOC(sizeof(ast_index_node));
    pool_add(&ast_memory_pool, node);
    node->type = AINDEX;
    node->next = NULL;

    node->target = target;
    node->indexer = indexer;
    return (ast_node *)node;
}

ast_node *create_if_node(ast_node *condition, ast_node *payload)
{
    ast_if_node *node = MALLOC(sizeof(ast_if_node));
    pool_add(&ast_memory_pool, node);
    node->type = AIF;
    node->next = NULL;

    node->next_if = NULL;
    node->payload = payload;
    node->condition = condition;

    return (ast_node *)node;
}

ast_node *create_loop_node(ast_node *init, ast_node *condition, ast_node *onloop, ast_node *payload)
{
    ast_loop_node *node = MALLOC(sizeof(ast_loop_node));
    pool_add(&ast_memory_pool, node);
    node->type = ALOOP;
    node->next = NULL;

    node->init = init;
    node->condition = condition;
    node->onloop = onloop;
    node->payload = payload;

    node->loop_type = init ? LFOR : LWHILE;

    return (ast_node *)node;
}

ast_node *create_func_decl_node(ast_node *params, ast_node *payload)
{
    ast_func_decl_node *node = MALLOC(sizeof(ast_func_decl_node));
    pool_add(&ast_memory_pool, node);
    
    node->type = AFUNC_DECL;
    node->next = NULL;
    
    node->params = params;
    node->payload = payload;
    
    return (ast_node *)node;
}

ast_node *create_class_decl_node(char *refname, ast_node *payload)
{
    ast_class_decl_node *node = MALLOC(sizeof(ast_class_decl_node));
    pool_add(&ast_memory_pool, node);

    node->type = ACLASS_DECL;
    node->next = NULL;

    node->refname = refname;
    node->payload = payload;

    return (ast_node *)node;
}
 
ast_node *create_func_call_node(ast_node *ident, ast_node *arguments)
{
    ast_func_call_node *node = MALLOC(sizeof(ast_func_call_node));
    pool_add(&ast_memory_pool, node);

    node->type = AFUNC_CALL;
    node->next = NULL;

    node->ident = ident;
    node->arguments = arguments;

    return (ast_node *)node;
}

ast_node *create_ternary_node(ast_node *condition, ast_node *first, ast_node *second)
{
    ast_ternary_node *node = MALLOC(sizeof(ast_ternary_node));
    pool_add(&ast_memory_pool, node);

    node->type = ATERNARY;
    node->next = NULL;

    node->condition = condition;
    node->first = first;
    node->second = second;

    return (ast_node *)node;
}

ast_node *create_member_access_node(ast_node *object, char *ident)
{
    ast_member_access_node *node = MALLOC(sizeof(ast_member_access_node));
    pool_add(&ast_memory_pool, node);

    node->type = AMEMBER_ACCESS;
    node->next = NULL;

    node->object = object;
    node->ident = ident;

    return (ast_node *)node;
}

ast_node *create_one_off_node(char opt)
{
    ast_one_off_node *node = MALLOC(sizeof(ast_one_off_node));
    pool_add(&ast_memory_pool, node);

    node->type = AONEOFF;
    node->next = NULL;

    node->opt = opt;
    
    return (ast_node *)node;
}

void ast_add_if_node(ast_node *curr, ast_node *next)
{
    ast_if_node *node = (ast_if_node *)curr;

    while(node->next_if)
        node = (ast_if_node *)node->next_if;

    node->next_if = next;
}

void ast_free_binary_node(ast_node *node)
{
    ast_binary_node *bn = (ast_binary_node *)node;
    if(bn->left)
        ast_free(bn->left);
    if(bn->right)
        ast_free(bn->right);
}

void ast_free_unary_node(ast_node *node)
{
    ast_unary_node *un = (ast_unary_node *)node;
    if(un->target)
        ast_free(un->target);
}

void ast_free_value_node(ast_node *node)
{
    ast_value_node *vn = (ast_value_node *)node;
    switch(vn->value_type)
    {
    case VSTRING:
    case VVAR:
        FREE(vn->value.s);
        break;
    default:
        break;
    }
}

void ast_free_if_node(ast_node *node)
{
    ast_if_node *in = (ast_if_node *)node;

    if(in->condition)
        ast_free(in->condition);
    ast_free(in->payload);

    if(in->next_if)
        ast_free(in->next_if);
}

void ast_free(ast_node *node)
{
    pool_drain(&ast_memory_pool);
    // switch(node->type)
    // {
    // case ABINARY_EXPRESSION:
    //     ast_free_binary_node(node);
    //     break;
    // case AVALUE:
    //     ast_free_value_node(node);
    //     break;
    // case AIF:
    //     ast_free_if_node(node);
    //     break;
    // case AUNARY_EXPRESSION:
    //     ast_free_unary_node(node);
    //     break;
    // default:
    //     break;
    // }

    // ast_node *next = node->next;
    // FREE(node);

    // if(next)
    //     ast_free(next);
}
