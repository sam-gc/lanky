#ifndef HASHSET_H
#define HASHSET_H

typedef int(*sort_function)(const char *, const char *);

typedef struct _node {
    char *value;
    struct _node *next;
} Hashlist;

typedef struct {
    struct _node **buckets;
    int hash_size;
    char copies_str;
    char no_strcmp;
} Hashset;

Hashset HS_create(int hash_size, char copies_str);
void HS_add(Hashset *set, char *str);
int HS_contains(Hashset *set, char *str);
int HS_count(Hashset *set);
void HS_remove(Hashset *set, char *str);
void HS_free(Hashset *set);
Hashlist *HS_to_list(Hashset *set);
void HS_list_sort(Hashlist *head, sort_function function);

#endif