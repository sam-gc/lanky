#ifndef TRIE_H
#define TRIE_H

struct TrieNode;

typedef struct list_node {
    struct list_node *next;
    struct TrieNode *payload;
} list_node_t;

typedef struct TrieNode {
    list_node_t *children;
    char value;
    char final;
} TrieNode_t;

typedef struct {
    TrieNode_t *head;
} Trie_t;

Trie_t trie_new();
void trie_free(Trie_t t);
void trie_add(Trie_t t, char *str);
char trie_contains_word(Trie_t t, char *str);
char trie_contains_path(Trie_t t, char *str);

#endif