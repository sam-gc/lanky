#include "ast.h"
#include "tools.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


ast_node *create_root_node()
{
    ast_node *node = MALLOC(sizeof(ast_block_node));
    node->type = ABLOCK;
    node->next = NULL;

    return node;
}

void ast_add_node(ast_node *curr, ast_node *next)
{
    ast_node *node = curr;

    while(node->next)
        node = node->next;

    node->next = next;
}

ast_node *create_value_node(ast_value_type type, void *data)
{
    ast_value_node *node = MALLOC(sizeof(ast_value_node));
    node->type = AVALUE;
    node->next = NULL;

    ast_value_union u;
    switch(type) 
    {
    case VINT:
        u.i = atol((char *)data);
        FREE(data);
        break;
    case VDOUBLE:
        u.d = atof((char *)data);
        FREE(data);
        break;
    case VSTRING:
    {
        char *raw = (char *)data;
        char *str = MALLOC(strlen(raw) - 1);
        strcpy(str, "");
        memcpy(str, raw + 1, strlen(raw) - 2);
        u.s = (char *)str;
        FREE(raw);
        break;
    }
    case VVAR:
        u.s = data;
        break;
    default:
        DEBUG("Shouldn't have reached here...");
        break;
    }

    node->value_type = type;
    node->value = u;

    return (ast_node *)node;
}

ast_node *create_binary_node(ast_node *left, ast_node *right, char opt)
{
    ast_binary_node *node = MALLOC(sizeof(ast_binary_node));
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
    node->type = AUNARY_EXPRESSION;
    node->next = NULL;

    node->target = target;
    node->opt = opt;

    return (ast_node *)node;
}

ast_node *create_assignment_node(char *left, ast_node *right)
{
    ast_binary_node *node = MALLOC(sizeof(ast_binary_node));
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
    node->type = ABLOCK;
    node->next = NULL;

    node->payload = payload;
    return (ast_node *)node;
}

ast_node *create_if_node(ast_node *condition, ast_node *payload)
{
    ast_if_node *node = MALLOC(sizeof(ast_if_node));
    node->type = AIF;
    node->next = NULL;

    node->next_if = NULL;
    node->payload = payload;
    node->condition = condition;

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
    switch(node->type)
    {
    case ABINARY_EXPRESSION:
        ast_free_binary_node(node);
        break;
    case AVALUE:
        ast_free_value_node(node);
        break;
    case AIF:
        ast_free_if_node(node);
        break;
    case AUNARY_EXPRESSION:
        ast_free_unary_node(node);
        break;
    default:
        break;
    }

    ast_node *next = node->next;
    FREE(node);

    if(next)
        ast_free(next);
}