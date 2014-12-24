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

#include "Lanky.h"

lky_object *popen_big_loop(lky_object_seq *args, lky_object *func)
{
    int i;
    for(i = 0; i < 1000000; i++)
    {
        printf("%d\n", i);
    }

    LKY_RETURN_NIL;
}

lky_object *popen_call(lky_object_seq *args, lky_object *func)
{
    lky_object_custom *c = LKY_CUST(LKY_FIRST_ARG(args));
    char *name = lobjb_stringify(c);
    printf("%s\n", name);
    FILE *fp;

    char buf[3000];

    fp = popen(name, "r");
    if(!fp)
    {
        printf("Failed to execute command %s\n", name);
        LKY_RETURN_NIL;
    }

    while(fgets(buf, sizeof(buf) - 1, fp))
    {
        printf("%s", buf);
    }

    pclose(fp);

    LKY_RETURN_NIL;
}

lky_object *popen_init()
{
    lky_object *obj = lobj_alloc();
    LKY_ADD_METHOD(obj, "bigLoop", 0, &popen_big_loop);
    LKY_ADD_METHOD(obj, "call", 1, &popen_call);

    return obj;
}
