#include "aquarium.h"

typedef struct aqua_node_ {
    void *ptr;
    struct aqua_node_ *next;
} aqua_node;

typedef struct {
    aqua_node *free;
    aqua_node *used;
} aqua_current;

typedef struct {
    void *head;
    int tot;
    int used;
} aqua_pool;

typedef struct {
    aqua_node *pools;
    aqua_current current;
} aqua_bundle;

static aqua_bundle small = {NULL, 

void aqua_init()
{
}

void *aqua_request_next_block(size_t size)
{
}

void aqua_release(void *block)
{
}
