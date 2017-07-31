#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
    if (argc != 3) {
        abort();
    }

    const char *output = argv[1];
    const char *input = argv[2];

    FILE *fin = fopen(input, "rb");
    FILE *fout = fopen(output, "wb");

#define READ_BUF_SIZE 4096
    char read_buf[READ_BUF_SIZE];
    while (!feof(fin)) {
        size_t nread = fread(read_buf, 1, READ_BUF_SIZE, fin);

        for (size_t i = 0; i < nread; ++i) {
            char c = read_buf[i];
            fprintf(fout, "0x%X, ", c);
        }
    }

    fprintf(fout, "0x00");

    fclose(fin);
    fclose(fout);

    return 0;
}