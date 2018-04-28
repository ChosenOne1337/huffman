#ifndef HUFFMAN_CODING_H
#define HUFFMAN_CODING_H

#include <stdio.h>

#define ALPH_SIZE 256

void analyze_file(FILE *fInput);

void encode_file(FILE *fInput, FILE *fOutput);

void decode_file(FILE *fInput, FILE *fOutput, unsigned file_size);

unsigned get_sym_freq(unsigned char sym);

#endif // HUFFMAN_CODING_H
