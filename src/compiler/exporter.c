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

