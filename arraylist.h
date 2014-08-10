#ifndef ARRAYLIST_H
#define ARRAYLIST_H

typedef struct {
    void **items;
    long count;
    long allocated;
} arraylist;

typedef char(*arr_pointer_function)(void *);

arraylist arr_create(long count);
void arr_append(arraylist *list, void *item);
void arr_insert(arraylist *list, void *item, long idx);
void *arr_get(arraylist *list, long idx);
void arr_set(arraylist *list, void *item, long idx);
void arr_remove(arraylist *list, void *item, long idx);
long arr_length(arraylist *list);
long arr_index_of(arraylist *list, void *obj);
void arr_for_each(arraylist *list, arr_pointer_function callback);
void arr_free(arraylist *list);

#endif
