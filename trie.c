#include <stdlib.h>
#include <string.h>
#include "trie.h"

TrieNode_t *new_node(char v)
{
    TrieNode_t *node = malloc(sizeof(TrieNode_t));
    node->value = v;
    node->final = 0;
    node->children = NULL;

    return node;
}

Trie_t trie_new()
{
    Trie_t t;
    t.head = new_node('\0');
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

void node_free(TrieNode_t *node)
{
    list_node_t *ln = node->children;

    while(ln)
    {
        node_free(ln->payload);
        list_node_t *nx = ln->next;
        free(ln);
        ln = nx;
    }

    free(node);
}

void trie_free(Trie_t t)
{
    node_free(t.head);
}

void node_add(TrieNode_t *node, char *str)
{
    char first = str[0];

    TrieNode_t *next = child_with(node, first);

    if(!next)
    {
        next = new_node(first);
        trie_list_add(&node->children, next);
    }

    if(strlen(str) > 1)
        node_add(next, str + 1);
    else
        next->final = 1;
}

char node_contains_word(TrieNode_t *node, char *str)
{
    char first = str[0];

    TrieNode_t *next = child_with(node, first);

    if(strlen(str) == 1 && next)
        return next->final;

    if(!next)
        return 0;

    return node_contains_word(next, str + 1);
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

void trie_add(Trie_t t, char *str)
{
    node_add(t.head, str);
}

char trie_contains_word(Trie_t t, char *str)
{
    return node_contains_word(t.head, str);
}

char trie_contains_path(Trie_t t, char *str)
{
    return node_contains_path(t.head, str);
}
