/* Lanky -- Scripting Language and Virtual Machine
 * Copyright (C) 2014  Sam Olsen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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
            strcpy(next, "");
        *buf = next;
    }

    strcat(*buf, cat);
}

int file_is_binary(char *filename)
{
    FILE *f = fopen(filename, "rb");
    if(!f)
        return 0;
    
    int c = fgetc(f);
    if(c == EOF)
    {
        fclose(f);
        return 0;
    }

    // The first byte of a valid serialized code
    // file will always be 7, as that is the value
    // of LBI_CODE; A code object must always be
    // the first item serialized.
    int ret = c == 7;   
    fclose(f);
    return ret;
}

