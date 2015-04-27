/* Lanky -- Scripting Language and Virtual Machine
 * Copyright (C) 2014  Sam Olsen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "ast.h"
#include "tools.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// In order to not have to worry about memory during
// compilation, use this pool struct to collect bulk-
// free the memory created.
lky_mempool ast_memory_pool = {NULL, NULL};

extern int yylineno;

void set_line_no(void *n) // I know using a void pointer is hacky but I hate to have to cast
{
    ast_node *node = (ast_node *)n;
    node->lineno = yylineno;
}

ast_node *create_root_node()
{
    ast_node *node = MALLOC(sizeof(ast_block_node));
    pool_add(&ast_memory_pool, node);
    node->type = ABLOCK;
    node->next = NULL;

    set_line_no(node);
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
        break;
    case VDOUBLE:
        u.d = atof((char *)data);
        break;
    case VSTRING:
    {
        char *raw = (char *)data;
        char *str = MALLOC(strlen(raw) - 1);
        pool_add(&ast_memory_pool, str);
        memset(str, 0, strlen(raw) - 1);
        memcpy(str, raw + 1, strlen(raw) - 2);
        u.s = (char *)str;
        break;
    }
    case VVAR:
        u.s = data;
        break;
    default:
        u.i = -11;
        break;
    }

    node->value_type = type;
    node->value = u;

    set_line_no(node);
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

    set_line_no(node);
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

    set_line_no(node);
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

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_load_node(void *data)
{
    ast_load_node *node = MALLOC(sizeof(ast_load_node));
    node->type = ALOAD;
    pool_add(&ast_memory_pool, node);

    char *raw = (char *)data;
    char *str = MALLOC(strlen(raw) - 1);
    pool_add(&ast_memory_pool, str);
    memset(str, 0, strlen(raw) - 1);
    memcpy(str, raw + 1, strlen(raw) - 2);

    node->name = str;
    node->next = NULL;

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_regex_node(void *data)
{
    ast_regex_node *node = MALLOC(sizeof(ast_regex_node));
    node->type = AREGEX;
    pool_add(&ast_memory_pool, node);

    char *raw = (char *)data;
    char *str = MALLOC(strlen(raw) - 2);
    char *flg = MALLOC(3);

    pool_add(&ast_memory_pool, str);
    pool_add(&ast_memory_pool, flg);

    memset(str, 0, strlen(raw) - 2);
    memset(flg, 0, 3);

    int i;
    for(raw = raw + 1, i = 0; *raw != '/'; raw++, i++)
        str[i] = *raw;
    for(raw = raw + 1, i = 0; *raw != '/'; raw++, i++)
        flg[i] = *raw;

    node->pattern = str;
    node->flags = flg;
    node->next = NULL;

    set_line_no(node);
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

    if(right->type == AFUNC_DECL)
        ((ast_func_decl_node *)right)->impl_name = left;

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_block_node(ast_node *payload)
{
    ast_block_node *node = MALLOC(sizeof(ast_block_node));
    pool_add(&ast_memory_pool, node);
    node->type = ABLOCK;
    node->next = NULL;

    node->payload = payload;

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_array_node(ast_node *payload)
{
    ast_array_node *node = MALLOC(sizeof(ast_array_node));
    pool_add(&ast_memory_pool, node);
    node->type = AARRAY;
    node->next = NULL;

    node->list = payload;

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_table_node(ast_node *payload)
{
    ast_table_node *node = MALLOC(sizeof(ast_table_node));
    pool_add(&ast_memory_pool, node);
    node->type = ATABLE;
    node->next = NULL;

    node->list = payload;

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_object_decl_node_ex(ast_node *payload, char *refname, ast_node *obj)
{
    ast_object_decl_node *node = MALLOC(sizeof(ast_object_decl_node));
    pool_add(&ast_memory_pool, node);
    node->type = AOBJDECL;
    node->next = NULL;

    node->obj = obj;

    node->payload = payload;

    node->refname = MALLOC(strlen(refname) + 1);
    strcpy(node->refname, refname);
    pool_add(&ast_memory_pool, node->refname);

    ast_node *target  = obj ? obj : create_unary_node(NULL, '1');

    ast_node *selfset = create_assignment_node(refname, target);
    ast_node *retnode = create_unary_node(create_value_node(VVAR, refname), 'r');

    ast_node *root = create_root_node();
    ast_add_node(root, selfset);
    ast_add_node(root, (struct ast_node *)node);
    ast_add_node(root, retnode);

    return create_func_call_node(create_func_decl_node(NULL, root, NULL), NULL);
}

ast_node *create_object_decl_node(ast_node *payload, char *refname, ast_node *obj)
{
    if(refname)
       return create_object_decl_node_ex(payload, refname, obj);
    
    ast_object_decl_node *node = MALLOC(sizeof(ast_object_decl_node));
    pool_add(&ast_memory_pool, node);
    node->type = AOBJDECL;
    node->next = NULL;
    node->obj  = obj;

    node->payload = payload;
    
    node->refname = NULL;

    set_line_no(node);
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

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_triple_set_node(ast_node *index_node, ast_node *value, char type)
{
    ast_triple_set_node *node = MALLOC(sizeof(ast_triple_set_node));
    pool_add(&ast_memory_pool, node);
    node->type = ATRIPLESET;
    node->next = NULL;
    
    node->index_node = index_node;
    node->new_val = value;
    node->op = type;

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_try_catch_node(ast_node *tryblock, ast_node *catchblock, ast_node *exception_name)
{
    ast_try_catch_node *node = MALLOC(sizeof(ast_try_catch_node));
    pool_add(&ast_memory_pool, node);
    node->type = ATRYCATCH;
    node->next = NULL;

    node->tryblock = tryblock;
    node->catchblock = catchblock;
    node->exception_name = exception_name;

    set_line_no(node);
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

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_cond_node(ast_node *left, ast_node *right, char type)
{
    ast_cond_node *node = MALLOC(sizeof(ast_cond_node));
    pool_add(&ast_memory_pool, node);
    node->type = ACOND_CHAIN;
    node->next = NULL;

    node->left = left;
    node->right = right;
    node->opt = type;

    set_line_no(node);
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

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_iter_loop_node(ast_node *store, ast_node *index, ast_node *condition, ast_node *payload)
{
    ast_loop_node *node = MALLOC(sizeof(ast_loop_node));
    pool_add(&ast_memory_pool, node);
    node->type = AITERLOOP;
    node->next = NULL;

    node->init = store;
    node->condition = index;
    node->onloop = condition;
    node->payload = payload;

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_func_decl_node(ast_node *params, ast_node *payload, char *refname)
{
    ast_func_decl_node *node = MALLOC(sizeof(ast_func_decl_node));
    pool_add(&ast_memory_pool, node);
    
    node->type = AFUNC_DECL;
    node->next = NULL;
    
    node->params = params;
    node->payload = payload;

    node->refname = refname;
    node->impl_name = NULL;

    set_line_no(node);
    return (ast_node *)node;
}


ast_node *create_class_decl_node(ast_node *members, ast_node *super, char *instrefname, char *classrefname)
{
    ast_node *col = MALLOC(sizeof(ast_node));
    ast_class_decl_node *node = MALLOC(sizeof(ast_class_decl_node));
    pool_add(&ast_memory_pool, col);
    pool_add(&ast_memory_pool, node);

    node->type = ACLASS_DECL;
    node->next = NULL;
    node->init = NULL;
    node->super = super;

    ast_node *cur = col;

    for(; members; members = members->next)
    {
        ast_class_member_node *m = (ast_class_member_node *)members;
        if(m->prefix == LCP_INIT)
            node->init = members;
        else
        {
            if(instrefname && m->prefix == LCP_PROTO && m->payload->type == AFUNC_DECL)
            {
                ast_func_decl_node *f = (ast_func_decl_node *)m->payload;
                if(!f->refname)
                    f->refname = instrefname;
            }
            else if(classrefname && m->prefix == LCP_STATIC && m->payload->type == AFUNC_DECL)
            {
                ast_func_decl_node *f = (ast_func_decl_node *)m->payload;
                if(!f->refname)
                    f->refname = classrefname;
            }
            cur->next = members;
            cur = cur->next;
        }
    }

    node->members = col;

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_class_member_node(lky_class_prefix p, char *refname, ast_node *payload)
{
    ast_class_member_node *node = MALLOC(sizeof(ast_class_member_node));
    pool_add(&ast_memory_pool, node);

    node->type = ACLASSMEMBER;
    node->next = NULL;

    node->name = refname;
    node->prefix = p;

    if(p == LCP_INIT)
    {
        ast_func_decl_node *f = (ast_func_decl_node *)payload;
        ast_node *args = f->params;
        f->params = create_value_node(VVAR, (void *)refname);
        f->params->next = args;
    }

    node->payload = payload;

    set_line_no(node);
    return (ast_node *)node;
}
/*
ast_node *create_class_decl_node(char *refname, ast_node *payload)
{
    ast_class_decl_node *node = MALLOC(sizeof(ast_class_decl_node));
    pool_add(&ast_memory_pool, node);

    node->type = ACLASS_DECL;
    node->next = NULL;

    node->refname = refname;
    node->payload = payload;

    return (ast_node *)node;
}*/
 
ast_node *create_func_call_node(ast_node *ident, ast_node *arguments)
{
    ast_func_call_node *node = MALLOC(sizeof(ast_func_call_node));
    pool_add(&ast_memory_pool, node);

    node->type = AFUNC_CALL;
    node->next = NULL;

    node->ident = ident;
    node->arguments = arguments;

    set_line_no(node);
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

    set_line_no(node);
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

    set_line_no(node);
    return (ast_node *)node;
}

ast_node *create_one_off_node(char opt)
{
    ast_one_off_node *node = MALLOC(sizeof(ast_one_off_node));
    pool_add(&ast_memory_pool, node);

    node->type = AONEOFF;
    node->next = NULL;

    node->opt = opt;

    set_line_no(node);
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
}
