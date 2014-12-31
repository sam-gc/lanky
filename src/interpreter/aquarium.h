#ifndef AQUARIUM_H
#define AQUARIUM_H

#include <stdlib.h>

void *aqua_request_next_block(size_t size);
void aqua_release(void *block);

#endif
