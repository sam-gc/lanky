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
        free(data);
        break;
    case VDOUBLE:
        u.d = atof((char *)data);
        free(data);
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

void ast_free_binary_node(ast_node *node)
{
    ast_binary_node *bn = (ast_binary_node *)node;
    if(bn->left)
        ast_free(bn->left);
    if(bn->right)
        ast_free(bn->right);
}

void ast_free_value_node(ast_node *node)
{

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
    default:
        break;
    }

    ast_node *next = node->next;
    FREE(node);

    if(next)
        ast_free(next);
}