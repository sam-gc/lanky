#include <stdlib.h>
#include <string.h>
#include "trie.h"

TrieNode_t *new_node(char v)
{
    TrieNode_t *node = malloc(sizeof(TrieNode_t));
    node->value = v;
    node->object = NULL;
    node->children = NULL;

    return node;
}

Trie_t trie_new()
{
    Trie_t t;
    t.head = new_node('\0');
    t.free_func = NULL;
    t.count = 0;
    return t;
}

TrieNode_t *child_with(TrieNode_t *n, char v)
{
    list_node_t *ln = n->children;

    while(ln)
    {
        if(ln->payload->value == v)
            return ln->payload;

        ln = ln->next;
    }

    return NULL;
}

void trie_list_add(list_node_t **list, TrieNode_t *t)
{
    list_node_t *new = malloc(sizeof(list_node_t));
    new->next = NULL;
    new->payload = t;

    if(!*list)
    {
        *list = new;
        return;
    }

    list_node_t *last = *list;
    while(last->next)
        last = last->next;

    last->next = new;
}

void node_free(TrieNode_t *node, trie_pointer_function free_func)
{
    list_node_t *ln = node->children;

    while(ln)
    {
        list_node_t *nx = ln->next;
        if(free_func && ln->payload->object)
            free_func(ln->payload->object);
        
        node_free(ln->payload, free_func);
        free(ln);
        ln = nx;
    }

    free(node);
}

void trie_free(Trie_t t)
{
    node_free(t.head, t.free_func);
}

char node_add(TrieNode_t *node, char *str, void *val)
{
    char ret = 0;
    char first = str[0];

    TrieNode_t *next = child_with(node, first);

    if(!next)
    {
        ret = 1;
        next = new_node(first);
        trie_list_add(&node->children, next);
    }

    if(strlen(str) > 1)
        ret = node_add(next, str + 1, val);
    else
    {
        ret = !next->object;
        next->object = val;
    }

    return ret;
}

void *node_get(TrieNode_t *node, char *str)
{
    char first = str[0];

    TrieNode_t *next = child_with(node, first);

    if(!next)
        return NULL;

    if(strlen(str) > 1)
        return node_get(next, str + 1);
    else
        return next->object;
}

void node_count(TrieNode_t *node, int *count)
{
    list_node_t *child = node->children;

    for(; child; child = child->next)
    {
        node_count(child->payload, count);
        (*count)++;
    }
}

void node_for_each(TrieNode_t *node, trie_pointer_function callback)
{
    if(!node)
        return;
    list_node_t *ln = node->children;

    while(ln)
    {
        node_for_each(ln->payload, callback);
        list_node_t *nx = ln->next;
        if(callback && ln->payload->object)
            callback(ln->payload->object);
        ln = nx;
    }
}

char node_contains_path(TrieNode_t *node, char *str)
{
    char first = str[0];
    
    TrieNode_t *next = child_with(node, first);
    
    if(strlen(str) == 1 && next)
        return 1;
    
    if(!next)
        return 0;
    
    return node_contains_path(next, str + 1);
}

void trie_add(Trie_t *t, char *str, void *value)
{
    if(node_add(t->head, str, value))
        t->count++;
}

void trie_for_each(Trie_t *t, trie_pointer_function callback)
{
    node_for_each(t->head, callback);
}


void *trie_get(Trie_t *t, char *str)
{
    return node_get(t->head, str);
}

char trie_contains_path(Trie_t *t, char *str)
{
    return node_contains_path(t->head, str);
}
