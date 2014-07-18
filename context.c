#include "context.h"
#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>

static Hashmap lanky_main_store;

typedef struct stack_node {
    struct stack_node *next;
    Hashmap local_store;
} stack_node;

static stack_node *lanky_stacks;

stack_node *create_stack_node()
{
    stack_node *node = malloc(sizeof(struct stack_node));
    node->next = NULL;
    node->local_store = hm_create(100, 1);
    return node;
}

void free_stack_node(stack_node *node)
{
    hm_free(&node->local_store);
    free(node);
}

static void push_stack_node()
{
    stack_node *n = create_stack_node();

    stack_node *c = lanky_stacks;
    while(c->next)
        c = c->next;

    c->next = n;
}

static void pop_stack_node()
{
    stack_node *c = lanky_stacks;
    stack_node *prev = NULL;
    while(c->next)
    {
        prev = c;
        c = c->next;
    }

    free_stack_node(c);
    if(prev)
        prev->next = NULL;
}

static stack_node *stack_top()
{
    stack_node *c = lanky_stacks;
    while(c->next)
        c = c->next;

    return c;
}

void ctx_init()
{
    lanky_stacks = create_stack_node();
}

void ctx_push_stack()
{
    push_stack_node();
}

void ctx_pop_stack()
{
    pop_stack_node();
}

void ctx_set_var(char *frmt, ast_value_wrapper wrap)
{
    stack_node *node = lanky_stacks;

    Hashmap map = node->local_store;

    while(!hm_contains(&map, frmt) && node)
    {
        map = node->local_store;
        node = node->next;
    }

    hm_put(&map, frmt, wrap);
}

ast_value_wrapper ctx_get_var(char *frmt)
{
    stack_node *node = lanky_stacks;
    Hashmap map = node->local_store;

    while(!hm_contains(&map, frmt) && node)
    {
        map = node->local_store;
        node = node->next;
    }

    hm_error_t error;
    ast_value_wrapper val = hm_get(&map, frmt, &error);
    return val;
}

void ctx_clean_up()
{
    hm_free(&(lanky_stacks->local_store));
}