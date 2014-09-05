#ifndef HASHMAP_H
#define HASHMAP_H

struct _node;

typedef struct {
    struct _node **buckets;
    int hash_size;
    char copies_str;
    int count;
} Hashmap;

typedef enum {
    HM_NO_ERROR = 0,
    HM_KEY_NOT_FOUND
} hm_error_t;

Hashmap hm_create(int hash_size, char copies_str);
void hm_put(Hashmap *set, char *str, void *val);
void *hm_get(Hashmap *set, char *key, hm_error_t *error);
int hm_contains(Hashmap *set, char *str);
int hm_count(Hashmap *set);
void hm_remove(Hashmap *set, char *str);
void hm_free(Hashmap *set);
char **hm_list_keys(Hashmap *set);

#endif
