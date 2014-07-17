#include <stdio.h>
#include <stdlib.h>
#include "ast_interpreter.h"
#include "ast_binary_ops.h"
#include "tools.h"
#include "context.h"
#include "ast_unary_ops.h"

ast_value_wrapper eval_binary_expression(ast_node *n)
{
    ast_binary_node *node = (ast_binary_node *)n;

    // Special case for assignment:
    if(node->opt == '=')
    {
        ast_value_wrapper l;
        l.type = VVAR;
        l.value = ((ast_value_node *)node->left)->value;
        return binary_set_equal(l, eval(node->right));
    }

    ast_value_wrapper left = eval(node->left);
    ast_value_wrapper right = eval(node->right);

    switch(node->opt)
    {
    case '+':
        return binary_add(left, right);
    case '-':
        return binary_subtract(left, right);
    case '*':
        return binary_multiply(left, right);
    case '/':
        return binary_divide(left, right);
    case '%':
        return binary_modulo(left, right);
    case '^':
        return binary_power(left, right);
    case 'E':
        return binary_equal(left, right);
    case 'g':
        return binary_greater(left, right);
    case 'l':
        return binary_lesser(left, right);
    case 'G':
        return binary_greaterequal(left, right);
    case 'L':
        return binary_lesserequal(left, right);
    case 'n':
        return binary_notequal(left, right);
    case '&':
        return binary_and(left, right);
    case '|':
        return binary_or(left, right);
    case '=':
        return binary_set_equal(left, right);
    }

    DEBUG("Should not have reached here.");
    ast_value_wrapper bad;
    bad.type = VNONE;
    return bad;
}

ast_value_wrapper eval_unary_expression(ast_node *n)
{
    ast_unary_node *node = (ast_unary_node *)n;

    ast_value_wrapper targ = eval(node->target);
    ast_value_wrapper er;
    er.type = VNONE;

    switch(node->opt)
    {
    case 'p':
        unary_print(targ);
        return er;
    }

    DEBUG("Should not have reached here.");
    return er;
}

ast_value_wrapper resolve_variable(ast_value_wrapper wrap)
{
    if(wrap.type != VVAR)
        return wrap;

    ast_value_wrapper w = ctx_get_var(wrap.value.s);

    if(w.type != VSTRING)
        return w;

    ast_value_wrapper ret;
    ret.type = VSTRING;
    ret.value.s = alloc_str(w.value.s);
    return ret;
}

ast_value_wrapper handle_if_block(ast_node *n)
{
    ast_if_node *node = (ast_if_node *)n;

    // If there is no condition, we assume
    // it is an 'else' node.
    if(!node->condition)
        return eval(node->payload);
    
    ast_value_wrapper cond = eval(node->condition);
    if(NUMERIC_UNWRAP(cond) != 0)
    {
        ast_value_wrapper wrap = eval(node->payload);
        return wrap;
    }
    else if(node->next_if)
    {
        return handle_if_block(node->next_if);
    }

    ast_value_wrapper ret;
    ret.type = VNONE;
    return ret;
}

ast_value_wrapper eval(ast_node *root)
{
    ast_value_wrapper ret;
    switch(root->type)
    {
    case AVALUE:
    {
        ast_value_wrapper wrap;
        ast_value_node *n = (ast_value_node *)root;
        wrap.type = n->value_type;
        wrap.value = n->value;
        ret = resolve_variable(wrap);
        break;
    }
    case ABINARY_EXPRESSION:
    {
        ast_value_wrapper wrap = eval_binary_expression(root);

        ret = wrap;
        break;
    }
    case AUNARY_EXPRESSION:
    {
        ast_value_wrapper wrap = eval_unary_expression(root);

        ret = wrap;
        break;
    }
    case AIF:
    {
        ret = handle_if_block(root);
    }
    }

    if(!root->next)
        return ret;
    else
    {
        // print_value(ret);
        value_wrapper_free(ret);
        return eval(root->next);
    }
}

void value_wrapper_free(ast_value_wrapper wrap)
{
    switch(wrap.type)
    {
    case VSTRING:
        FREE(wrap.value.s);
        break;
    }
}

void ast_print(ast_node *root)
{
    printf("%p", root);
    root = root->next;
    while(root)
    {
        printf(" -- %p", root);
        root = root->next;
    }

    printf("\n");
}

void print_value(ast_value_wrapper wrap)
{
    switch(wrap.type)
    {
    case VDOUBLE:
        printf("%lf\n", wrap.value.d);
        break;
    case VINT:
        printf("%lld\n", wrap.value.i);
        break;
    case VSTRING:
        printf("%s\n", wrap.value.s);
        break;
    case VNONE:
        printf("None\n");
        break;
    }
}