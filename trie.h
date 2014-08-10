#ifndef TRIE_H
#define TRIE_H

struct TrieNode;

typedef void(*trie_pointer_function)(void *);

typedef struct list_node {
    struct list_node *next;
    struct TrieNode *payload;
} list_node_t;

typedef struct TrieNode {
    list_node_t *children;
    char value;
    void *object;
} TrieNode_t;

typedef struct {
    TrieNode_t *head;
    trie_pointer_function free_func;
} Trie_t;

Trie_t trie_new();
void trie_free(Trie_t t);
void trie_add(Trie_t t, char *str, void *value);
void *trie_get(Trie_t t, char *str);

#endif
