#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hashmap.h"
#include "tools.h"

typedef struct _node {
    char *value;
    ast_value_wrapper dv;
    struct _node *next;
} node;

ast_value_wrapper make_nil_wrapper()
{
    ast_value_wrapper wrap;
    wrap.type = VNONE;
    return wrap;
}

unsigned long djb2(char *str)
{
    unsigned long hash = 5381;
    char c;

    while(c = *str++)
        hash = ((hash << 5) + hash) + c;

    return hash;
}

Hashmap hm_create(int hash_size, char copies_str)
{
    Hashmap set;
    set.hash_size = hash_size;
    set.copies_str = copies_str;
    set.buckets = malloc(sizeof(node *) * hash_size);

    int i;
    for(i = 0; i < hash_size; i++)
        set.buckets[i] = NULL;

    return set;
}

void hm_put(Hashmap *set, char *str, ast_value_wrapper val)
{
    unsigned long hash = djb2(str);
    hash %= set->hash_size;

    node *n;
    if(!set->buckets[hash])
    {
        n = malloc(sizeof(node));
        set->buckets[hash] = n;
    }
    else
    {
        node *last = NULL;
        for(n = set->buckets[hash]; n; n = n->next)
        {
            if(strcmp(n->value, str) == 0)
            {
                n->dv = val;
                return;
            }

            last = n;
        }

        n = last;
        n->next = malloc(sizeof(node));
        n = n->next;
    }

    char *value;
    if(set->copies_str)
    {
        value = malloc(strlen(str) + 1);
        strcpy(value, str);
    }
    else
        value = str;

    n->value = value;
    n->dv = val;
    n->next = NULL;
}

int hm_contains(Hashmap *set, char *str)
{
    unsigned long hash = djb2(str) % set->hash_size;

    if(!set->buckets[hash])
        return 0;

    node *n = set->buckets[hash];
    for(; n; n = n->next)
        if(strcmp(n->value, str) == 0)
            return 1;

    return 0;
}

int hm_count(Hashmap *set)
{
    int count = 0;

    int i;
    for(i = 0; i < set->hash_size; i++)
    {
        node *n = set->buckets[i];
        if(!n)
            continue;

        for(; n; n = n->next)
            count++;
    }

    return count;
}

ast_value_wrapper hm_get(Hashmap *set, char *key, hm_error_t *error)
{
    hm_error_t throwaway;

    if(!error)
        error = &throwaway;



    unsigned long hash = djb2(key) % set->hash_size;

    if(!set->buckets[hash])
    {
        *error = HM_KEY_NOT_FOUND;
        return make_nil_wrapper();
    }

    node *n = set->buckets[hash];
    for(; n; n = n->next)
    {
        if(strcmp(n->value, key) == 0)
        {
            *error = HM_NO_ERROR;
            return n->dv;
        }
    }

    *error = HM_KEY_NOT_FOUND;
    return make_nil_wrapper();
}

void hm_remove(Hashmap *set, char *str)
{
    unsigned long hash = djb2(str) % set->hash_size;

    if(!set->buckets[hash])
        return;

    node *n = set->buckets[hash];
    node *prev = NULL;
    for(; n; n = n->next)
    {
        if(strcmp(n->value, str))
        {
            prev = n;
            continue;
        }

        node *next = n->next;

        if(prev)
            prev->next = next;
        else
            set->buckets[hash] = next;

        if(set->copies_str)
            free(n->value);
        free(n);

        break;
    }
}

void hm_free(Hashmap *set)
{
    int i;
    for(i = 0; i < set->hash_size; i++)
    {
        node *n = set->buckets[i];
        if(!n)
            continue;

        while(n)
        {
            node *next = n->next;
            if(set->copies_str)
                free(n->value);
            if(n->dv.type == VSTRING && n->dv.value.s)
            {
                FREE(n->dv.value.s);
            }
            free(n);
            n = next; 
        }
    }

    free(set->buckets);
}