#ifndef MEMPOOL_H
#define MEMPOOL_H

struct poolnode {
    struct poolnode *next;
    void *data;
};

typedef struct {
    struct poolnode *head;
} lky_mempool;

lky_mempool pool_create();
void pool_add(lky_mempool *pool, void *obj);
void pool_drain(lky_mempool *pool);

#endif