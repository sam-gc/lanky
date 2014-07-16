#include <stdio.h>
#include <stdlib.h>
#include "ast_interpreter.h"
#include "ast_binary_ops.h"
#include "tools.h"
#include "context.h"

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

ast_value_wrapper resolve_variable(ast_value_wrapper wrap)
{
    if(wrap.type != VVAR)
        return wrap;

    return ctx_get_var(wrap.value.s);
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
    }

    if(!root->next)
        return ret;
    else
    {
        print_value(ret);
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