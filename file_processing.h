#ifndef FILE_PROCESSING_H
#define FILE_PROCESSING_H

#include <stdio.h>

#define ALPH_SIZE 256

unsigned analyze_file(FILE *fInput);

void print_freq_table(void);

void print_file_len(void);

void encode_file(FILE *fInput, FILE *fOutput);

unsigned decode_file(FILE *fInput, FILE *fOutput);

unsigned get_sym_freq(unsigned char sym);

#endif // FILE_PROCESSING_H
