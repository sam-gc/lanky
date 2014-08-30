#include "../Lanky.h"
#include "solver.h"
#include <string.h>

typedef struct {
    Trie_t trie;
} boggle_dict_data;

typedef struct {
    lky_object *dict_object;
    Trie_t dict_trie;
} boggle_boggle_data;

int length_sort(const char *a, const char *b)
{
    int alen = strlen(a), blen = strlen(b);
    if(alen < blen)
        return 1;
    if(blen < alen)
        return -1;

    return strcmp(a, b);
}

Trie_t boggle_load_dictionary()
{
    Trie_t trie = trie_new();
    FILE *f = fopen("ospd.txt", "r");

    char *buf = NULL;
    size_t sz = 0;
    while(getline(&buf, &sz, f) != -1)
    {
        // Replace the newline character.
        buf[strlen(buf) - 1] = '\0';
        trie_add(&trie, buf, (void *)1);

        free(buf);
        buf = NULL;
        sz = 0;
    }
    
    if(buf)
        free(buf);

    fclose(f);

    return trie;
}

lky_object *boggle_dict_contains_word(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = LKY_CUST(func->owner);
    boggle_dict_data *data = self->data;

    char *str = LKY_CUST(LKY_FIRST_ARG(args))->data;

    return lobjb_build_int((long)trie_get(&data->trie, str));
}

lky_object *boggle_dict_contains_prefix(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = LKY_CUST(func->owner);
    boggle_dict_data *data = self->data;

    char *str = LKY_CUST(LKY_FIRST_ARG(args))->data;

    return lobjb_build_int((long)trie_contains_path(&data->trie, str));
}

void boggle_dict_free(lky_object_custom *obj)
{
    boggle_dict_data *data = obj->data;
    trie_free(data->trie);
    free(data);

    printf("Dictionary destructor called.\n");
}

lky_object *boggle_build_dictionary(lky_object_seq *args, lky_object *func)
{
    lky_object_custom *custom = LKY_CUST(lobjb_build_custom(sizeof(boggle_dict_data)));
    boggle_dict_data *data = malloc(sizeof(boggle_dict_data));
    data->trie = boggle_load_dictionary();

    custom->data = data;
    lky_object *obj = LKY_OBJ(custom);

    LKY_ADD_METHOD(obj, "isWord", 1, &boggle_dict_contains_word);
    LKY_ADD_METHOD(obj, "isPrefix", 1, &boggle_dict_contains_prefix);

    custom->freefunc = LKY_FREE_FUNC(&boggle_dict_free);

    return obj;
}

void boggle_save(lky_object_custom *self)
{
    boggle_boggle_data *data = self->data;
    gc_mark_object(data->dict_object);
}

void boggle_boggle_free(lky_object_custom *self)
{
    // We are going to let the dictionary
    // free the information.
    free(self->data);
}

lky_object *boggle_begin(lky_object_seq *args, lky_object *func)
{
    solver_start_add();
    LKY_RETURN_NIL;
}

lky_object *boggle_add_char(lky_object_seq *args, lky_object *func)
{
    char *str = LKY_CUST(LKY_FIRST_ARG(args))->data;

    solver_add(str[0]);

    LKY_RETURN_NIL;
}

lky_object *boggle_add_string(lky_object_seq *args, lky_object *func)
{
    char *str = LKY_CUST(LKY_FIRST_ARG(args))->data;

    unsigned long len = strlen(str);
    int i;
    for(i = 0; i < len; i++)
    {
        solver_add(str[i]);
    }

    LKY_RETURN_NIL;
}

lky_object *boggle_solve(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *self = LKY_CUST(func->owner);
    boggle_boggle_data *data = self->data;

    // Boggle = C.import('extensions/Boggle/boggle');
    Hashset words = solver_solve(data->dict_trie);
    Hashlist *listed = HS_to_list(&words);
    HS_list_sort(listed, length_sort);

    arraylist list = arr_create(50);

    while(listed)
    {
        arr_append(&list, stlstr_cinit(listed->value));
        Hashlist *next = listed->next;
        free(listed);
        listed = next;
    }

    lky_object *ret = stlarr_cinit(list);

    HS_free(&words);

    return ret;
}

lky_object *boggle_build_boggle(lky_object_seq *args, lky_object_function *func)
{
    lky_object_custom *dict = LKY_CUST(boggle_build_dictionary(NULL, NULL));
    lky_object_custom *self = LKY_CUST(lobjb_build_custom(sizeof(boggle_boggle_data)));

    self->freefunc = LKY_FREE_FUNC(&boggle_boggle_free);
    self->savefunc = LKY_GC_FUNC(&boggle_save);

    boggle_boggle_data *data = malloc(sizeof(boggle_boggle_data));
    data->dict_object = dict;
    data->dict_trie = ((boggle_dict_data *)dict->data)->trie;

    lobj_set_member(LKY_OBJ(self), "dictionary", LKY_OBJ(dict));
    LKY_ADD_METHOD(self, "begin", 0, LKY_CFUNC(&boggle_begin));
    LKY_ADD_METHOD(self, "addChar", 1, LKY_CFUNC(&boggle_add_char));
    LKY_ADD_METHOD(self, "addString", 1, LKY_CFUNC(&boggle_add_string));
    LKY_ADD_METHOD(self, "solve", 0, LKY_CFUNC(&boggle_solve));

    self->data = data;

    return LKY_OBJ(self);
}

lky_object *boggle_init()
{
    lky_object *obj = lobj_alloc();
    LKY_ADD_METHOD(obj, "Solver", 0, LKY_CFUNC(&boggle_build_boggle));
    return obj;
}