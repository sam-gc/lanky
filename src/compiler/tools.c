#include "tools.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *alloc_str(char *str)
{
    char *tmp = MALLOC(strlen(str) + 1);
    pool_add(&ast_memory_pool, tmp);
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

int next_pow_two(int x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    return x + 1;
}

void auto_cat(char **buf, char *cat)
{
    size_t clen = strlen(cat);
    size_t blen = *buf ? strlen(*buf) : 0;
    int np = next_pow_two(blen);

    if(blen + clen + 1 > np || np == 0)
    {
        char *next = malloc(2 * (np > 0 ? np : 1));
        if(*buf)
        {
            strcpy(next, *buf);
            free(*buf);
        }
        else
            sprintf(next, "");
        *buf = next;
    }

    strcat(*buf, cat);
}

