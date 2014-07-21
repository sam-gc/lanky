#include <stdio.h>
#include <stdlib.h>
#include "instruction_set.h"
#include "lkyobj_builtin.h"

struct helper {
    long len;
    long sz1;
    char type1;
    long num1;
    long sz2;
    char type2;
    long num2;
};

void work_on_list(arraylist list, FILE *f)
{
    fwrite(&(list.count), sizeof(long), 1, f);
    int i;
    for(i = 0; i < list.count; i++)
    {
        lky_object *obj = arr_get(&list, i);
        lobjb_serialize(obj, f);
    }
}

void do_work(char *filename)
{
    // long len = sizeof(long) * 2 + sizeof(long) * 2 + 2;

    // struct helper h;
    // h.len = len;
    // h.sz1 = h.sz2 = sizeof(long);
    // h.num1 = 3;
    // h.num2 = 4;
    // h.type1 = h.type2 = LT_LONG;

    FILE *f = fopen(filename, "w");
    // // fwrite(&h, sizeof(struct helper), 1, f);
    // fwrite(&h.len, sizeof(long), 1, f);
    // fwrite(&h.sz1, sizeof(long), 1, f);
    // fwrite(&h.type1, sizeof(char), 1, f);
    // fwrite(&h.num1, sizeof(long), 1, f);
    // fwrite(&h.sz2, sizeof(long), 1, f);
    // fwrite(&h.type2, sizeof(char), 1, f);
    // fwrite(&h.num2, sizeof(long), 1, f);

    lky_object *a = lobjb_build_int(98);
    lky_object *b = lobjb_build_int(12);
    lky_object *c = lobjb_build_int(45);

    arraylist list = arr_create(5);
    arr_append(&list, a);
    arr_append(&list, b);
    arr_append(&list, c);

    work_on_list(list, f);

    long len = 9;
    fwrite(&len, sizeof(long), 1, f);

    char ops[9] = {LI_LOAD_CONST, 0, LI_LOAD_CONST, 1, LI_LOAD_CONST, 2, LI_BINARY_ADD, LI_BINARY_MULTIPLY, LI_PRINT};
    fwrite(ops, sizeof(char), 9, f);

    fclose(f);
}

int main()
{
    do_work("test");
    return 0;
}