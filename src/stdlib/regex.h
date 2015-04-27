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

#ifndef REGEX_H
#define REGEX_H

#define RGX_GLOBAL 1
#define RGX_IGNORE_CASE 2

struct regex;
typedef struct regex rgx_regex;

rgx_regex *rgx_compile(char *input);
void rgx_set_flags(rgx_regex *regex, unsigned flags);
unsigned rgx_get_flags(rgx_regex *regex);
int *rgx_collect_matches(rgx_regex *regex, char *input);
int rgx_matches(rgx_regex *regex, char *input);
int rgx_search(rgx_regex *regex, char *input);
void rgx_free(rgx_regex *regex);

#endif //REGEX_H
