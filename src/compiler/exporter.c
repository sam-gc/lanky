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

#include "exporter.h"

void exp_send_to_binary_file(char *data, size_t len, char *filename)
{
    FILE *f = fopen(filename, "wb");
    if(!f)
        return;

    fwrite(data, 1, len, f);

    fclose(f);
}

void exp_send_to_c_source(char *data, size_t len, char *filename)
{
    FILE *f = fopen(filename, "w");
    if(!f)
        return;

    fprintf(f, "long lky_bottled_bytecode_len_ = %d;\n", len);
    fprintf(f, "unsigned char lky_bottled_bytecode_data_[] = {\n    ");
    int i;
    for(i = 0; i < len; i++)
    {
        unsigned char c = data[i];
        fprintf(f, "0x%X%s %s", c, i < len - 1 ? "," : "", i % 20 == 19 ? "\n    " : "");
    }

    fprintf(f, "\n};");
    fclose(f);
}

