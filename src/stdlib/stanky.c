#include "stanky.h"
#include "stl_array.h"

Trie_t get_stdlib_objects()
{
    Trie_t t = trie_new();
    trie_add(t, "Array", stlarr_get_class());
    return t;
}
