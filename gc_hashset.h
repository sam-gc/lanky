#ifndef GC_HASHSET_H
#define GC_HASHSET_H

typedef void(*gchs_pointer_function)(void *data);

typedef struct _node {
    void *value;
    struct _node *next;
} gc_hashlist;

typedef struct {
    struct _node **buckets;
    int hash_size;
    int count;
} gc_hashset;

gc_hashset gchs_create(int hash_size);
void gchs_add(gc_hashset *set, void *obj);
int gchs_contains(gc_hashset *set, void *obj);
void gchs_remove(gc_hashset *set, void *obj);
void gchs_for_each(gc_hashset *set, gchs_pointer_function callback);
void gchs_free(gc_hashset *set);
void **gchs_to_list(gc_hashset *set);

#endif
