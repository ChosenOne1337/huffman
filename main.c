#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "file_processing.h"
#include "huffman_tree.h"

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Incorrect parameters!\n");
        return 0;
    }

    FILE *fInput = fopen(argv[2], "rb");
    if (fInput == NULL) {
        fprintf(stderr, "Failed to open %s!\n", argv[2]);
        exit(1);
    }
    FILE *fOutput = fopen(argv[3], "wb");
    if (fOutput == NULL) {
        fprintf(stderr, "Failed to open %s!\n", argv[3]);
        fclose(fInput);
        exit(1);
    }

    if (!strcmp(argv[1], "-c")) {
        encode_file(fInput, fOutput);
        printf("Done!\n");
    }
    else if (!strcmp(argv[1], "-d")) {
        decode_file(fInput, fOutput);
        printf("Done!\n");
    }
    else {
        printf("Incorrect options!\n");
    }

    fclose(fInput);
    fclose(fOutput);

    return 0;
}
