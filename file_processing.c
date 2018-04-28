#include "file_processing.h"

//checksum

uint32_t crc32_for_byte(uint32_t r) {
    for (int j = 0; j < 8; ++j) {
        r = ((r & 1) ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;
    }
    return r ^ (uint32_t)0xFF000000L;
}

void crc32(const void *data, size_t n_bytes, uint32_t* crc) {
    static uint32_t table[0x100];
    if (!*table) {
        for (size_t i = 0; i < 0x100; ++i) {
            table[i] = crc32_for_byte(i);
        }
    }
    for (size_t i = 0; i < n_bytes; ++i) {
        *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
    }
}

uint32_t get_checksum(FILE *file) {
    static char buf[1L << 15];
    uint32_t crc = 0;
    while (!feof(file) && !ferror(file)) {
        crc32(buf, fread(buf, 1, sizeof(buf), file), &crc);
    }
    return crc;
}

//auxiliary stuff

#define BUF_SIZE 1024

static unsigned char copybuf[BUF_SIZE] = {0};

void file_set_pos(FILE *file, unsigned pos) {
    fseek(file, pos, SEEK_SET);
}

unsigned file_copy_block(FILE *from, FILE *to, unsigned block_size) {
    unsigned bytes_num = 0, bytes_total = 0;
    for (; block_size > BUF_SIZE; block_size -= BUF_SIZE) {
        bytes_num = fread(copybuf, sizeof(char), BUF_SIZE, from);
        fwrite(copybuf, sizeof(char), bytes_num, to);
        bytes_total += bytes_num;
        if (bytes_num < BUF_SIZE) {
            //in-file have reached the EOF
            return bytes_total;
        }
    }
    //read & write the remainder
    bytes_num = fread(copybuf, sizeof(char), block_size, from);
    fwrite(copybuf, sizeof(char), bytes_num, to);
    bytes_total += bytes_num;
    return bytes_total;
}

void file_shift_pos(FILE *file, unsigned shift) {
    fseek(file, shift, SEEK_CUR);
}

unsigned concat_files(FILE *to, FILE *from) {
    unsigned bytes_num = 0, bytes_total = 0;
    while ((bytes_num = fread(copybuf, sizeof(char), BUF_SIZE, from)) > 0) {
        fwrite(copybuf, sizeof(char), bytes_num, to);
        bytes_total += bytes_num;
    }
    return bytes_total;
}

unsigned get_file_size(FILE *file) {
    fseek(file, 0, SEEK_END);
    unsigned file_size = ftell(file);
    rewind(file);
    return file_size;
}

void *file_close(FILE *file) {
    if (file != NULL) {
        fclose(file);
    }
    return NULL;
}
