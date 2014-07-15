#include "tools.h"
#include <stdlib.h>
#include <string.h>

char *alloc_str(char *str)
{
    char *tmp = malloc(strlen(str) + 1);
    strcpy(tmp, str);
    return tmp;
}

static int malloc_count = 0;
void malloc_add()
{
    malloc_count++;
}

int get_malloc_count()
{
    return malloc_count;
}

static int free_count = 0;
void free_add()
{
    free_count++;
}

int get_free_count()
{
    return free_count;
}