#ifndef DICTIONARY_H
#define DICTIONARY_H

typedef struct {
    void **container;
    int count;
    int hash_size;
} dictionary;

dictionary dict_create();
void dict_put(dictionary *dict, long hash, void *data);
void *dict_get(dictionary *dict, long hash);

#endif
