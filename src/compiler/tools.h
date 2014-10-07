#ifndef TOOLS_H
#define TOOLS_H

#define MALLOC(size) malloc(size); malloc_add();
#define FREE(obj) free(obj); free_add();

char *alloc_str(char *str);
void malloc_add();
int get_malloc_count();
void free_add();
int get_free_count();

#endif
