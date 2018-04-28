#ifndef FILE_PROCESSING_H
#define FILE_PROCESSING_H

#include <stdio.h>
#include <stdint.h>

//auxiliary functions

void file_set_pos(FILE *file, unsigned pos);

void file_shift_pos(FILE *file, unsigned shift);

unsigned file_copy_block(FILE *from, FILE *to, unsigned size);

unsigned concat_files(FILE *to, FILE *from);

unsigned get_file_size(FILE *file);

void *file_close(FILE *file);

//checksum

uint32_t crc32_for_byte(uint32_t r);

void crc32(const void *data, size_t n_bytes, uint32_t* crc);

uint32_t get_checksum(FILE *file);

#endif // FILE_PROCESSING_H
