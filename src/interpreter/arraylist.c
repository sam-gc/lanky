#include "arraylist.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

arraylist arr_create(long count)
{
    arraylist list;
    list.items = malloc(sizeof(void *) * count);
    list.count = 0;
    list.allocated = count;

    long i;
    for(i = 0; i < count; i++)
    {
        list.items[i] = NULL;
    }

    return list;
}

void arr_manage_size(arraylist *list)
{
    long count = list->count;
    long alloc = list->allocated;
    if(count == alloc)
    {
        long nl = alloc * 2 < 100 ? alloc * 2 : alloc + 100;
        void **new_list = malloc(sizeof(void *) * nl);
        memcpy(new_list, list->items, sizeof(void *) * alloc);
        free(list->items);
        list->items = new_list;
        list->allocated = nl;
    }
}

void arr_append(arraylist *list, void *item)
{
    arr_manage_size(list);

    long count = list->count;

    list->items[count++] = item;
    
    if(count > 0 && !list->items[0])
    {
        printf("WTF\n");
    }
    
    list->count = count;
}

void arr_insert(arraylist *list, void *item, long idx)
{
    arr_manage_size(list);

    long i;
    for(i = list->count; i > idx; i--)
    {
        list->items[i] = list->items[i - 1];
    }

    list->items[idx] = item;
    list->count++;
}

void *arr_get(arraylist *list, long idx)
{
    return list->items[idx];
}

void arr_set(arraylist *list, void *item, long idx)
{
    list->items[idx] = item;
}

void remove_at_index(arraylist *list, long idx)
{
    long i;
    for(i = idx; i < list->count; i++)
    {
        list->items[i] = i < list->count - 1 ? list->items[i + 1] : NULL;
    }

    list->count--;
}

void arr_remove(arraylist *list, void *item, long idx)
{
    if(!item)
    {
        remove_at_index(list, idx);
        return;
    }

    idx = arr_index_of(list, item);
    remove_at_index(list, idx);
}

long arr_index_of(arraylist *list, void *obj)
{
    long i;
    for(i = 0; i < list->count; i++)
    {
        if(obj == list->items[i])
            return i;
    }

    return -1;
}

void arr_for_each(arraylist *list, arr_pointer_function callback)
{
    if(!callback)
        return;

    long i;
    for(i = 0; i < list->count; i++)
    {
        char res = callback(list->items[i]);
        if(!res)
            return;
    }
}

long arr_length(arraylist *list)
{
    return list->count;
}

void arr_free(arraylist *list)
{
    free(list->items);
}

void arr_sort_merge(void *data[], int count, arr_sort_function sf, void *misc, int offset)
{
    int i, j, k;
    i = j = k = 0;

    int p, q;

    if(offset)
    {
        p = offset;
        q = count - offset;
    }
    else
    {
        p = count / 2;
        q = count % 2 ? p + 1 : p;
    }

    void **B = data;
    void **C = data + p;
    void *A[count];

    for(; i < p && j < q; k++)
    {
        arr_sort_result r = sf(B[i], C[j], misc);
        if(r == SORT_RESULT_EQUAL || r == SORT_RESULT_SORTED)
            A[k] = B[i++];
        else
            A[k] = C[j++];
    }

    int rest;
    int rem;
    void **rar;
    if(i == p)
    {
        rar = C;
        rest = j;
        rem = q;
    }
    else
    {
        rar = B;
        rest = i;
        rem = p;
    }

    for(i = rest; i < rem; i++)
        A[k++] = rar[i]; 

    for(i = 0; i < count; i++)
        data[i] = A[i];
}

void arr_sort(arraylist *list, arr_sort_function sf, void *data)
{
    // This is a mergesort implementation

    void **items = list->items;

    // We need to pre sort the last two numbers to ensure everything
    // works okay if the list is uneven.
    //if(list->count % 2 && list->count > 2)
    //    arr_sort_merge(items + (list->count - 2), 2, sf, data, 0);

    int i, j;
    for(i = 2; i < list->count; i *= 2)
    {
        for(j = 0; j < list->count - 1; j += i)
        {
            void **section = items + j;
            arr_sort_merge(section, j + i < list->count - 1 ? i : list->count - j, sf, data, 0);
        }
    }

    arr_sort_merge(items, list->count, sf, data, i / 2);
}

