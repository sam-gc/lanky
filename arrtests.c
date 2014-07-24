#include <stdio.h>
#include <stdlib.h>
#include "arraylist.h"

int main(int argc, char *argv[])
{
    arraylist list = arr_create(100);
    // char *a = "hello";
    // char *b = "world";
    // char *c = "...";

    // arr_append(&list, a);
    // arr_append(&list, b);
    // arr_append(&list, c);

    // printf("%s\n%s\n%s\n", arr_get(&list, 0), arr_get(&list, 1), arr_get(&list, 2));

    // arr_remove(&list, NULL, 2);

    // printf("\n%s\n%s\n", arr_get(&list, 0), arr_get(&list, 1));

    // arr_remove(&list, NULL, 0);
    // arr_remove(&list, NULL, 0);

    // int i;
    // for(i = 0; i < 1000; i++)
    // {
    //     int *j = malloc(sizeof(int));
    //     *j = i;
    //     arr_append(&list, j);
    // }

    // for(i = 0; i < 1000; i++)
    // {
    //     int *j = arr_get(&list, 0);
    //     printf("%d : %d\n", i, *j);
    //     arr_remove(&list, NULL, 0);
    // }

    int i;
    for(i = 0; i < 1000; i++)
    {
        arr_append(&list, i);
    }

    for(i = list.count - 1; i > 500; i--)
    {
        arr_remove(&list, NULL, i);
    }

    for(i = 0; i < list.count; i++)
    {
        printf("%d\n", arr_get(&list, i));
    }

    printf("\n");

    arr_free(&list);

    return 0;
}