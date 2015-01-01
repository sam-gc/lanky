#ifndef AQUARIUM_H
#define AQUARIUM_H

#include <stdlib.h>

void aqua_init();
void aqua_teardown();
void *aqua_request_next_block(size_t size);
void aqua_release(void *block);
int aqua_is_managed_pointer(void *ptr);

extern int aqua_use_system_malloc_free_;

#endif
