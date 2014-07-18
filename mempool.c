#include "mempool.h"
#include "tools.h"
#include <stdlib.h>

struct poolnode *gen_node(void *obj)
{
    struct poolnode *node = MALLOC(sizeof(struct poolnode));
    node->next = NULL;
    node->data = obj;

    return node;
}

void append_to_list(struct poolnode **head, struct poolnode *next)
{
    if(!*head)
    {
        *head = next;
        return;
    }

    struct poolnode *node = *head;
    while(node->next)
        node = node->next;

    node->next = next;
}

lky_mempool pool_create()
{
    lky_mempool pool;
    pool.head = NULL;
}

void pool_add(lky_mempool *pool, void *obj)
{
    struct poolnode *next = gen_node(obj);
    append_to_list(&(pool->head), next);
}

void pool_drain(lky_mempool *pool)
{
    struct poolnode *node = pool->head;
    while(node)
    {
        void *data = node->data;
        FREE(data);
        struct poolnode *cur = node;
        node = node->next;
        FREE(cur);
    }

    pool->head = NULL;
}