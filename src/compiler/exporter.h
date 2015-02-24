#ifndef EXPORTER_H
#define EXPORTER_H

#include <stdlib.h>
#include <stdio.h>

void exp_send_to_binary_file(char *data, size_t len, char *filename);
void exp_send_to_c_source(char *data, size_t len, char *filename);

#endif
