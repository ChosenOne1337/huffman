#include <string.h>
#include <limits.h>
#include <time.h>
#include "file_processing.h"
#include "huffman_tree.h"
#include "binary_buffer.h"

//character frequency

unsigned freq_table[ALPH_SIZE] = {0};

void reset_freq_table(void) {
    memset(freq_table, 0, sizeof(unsigned) * ALPH_SIZE);
}

unsigned get_sym_freq(unsigned char sym) {
    return freq_table[sym];
}

void analyze_file(FILE *fInput) {
    //the file is supposed to be successfully opened
    reset_freq_table();
    int char_num = 0;
    while ((char_num = read_from_file(fInput)) > 0) {
        for (int i = 0; i < char_num; ++i) {
            ++freq_table[inbuf_get_byte()];
            inbuf_next_byte();
        }
    }
    rewind(fInput);
}

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

unsigned char read_char_from_inbuf(FILE *fInput) {
    unsigned char char_val = 0;
    for (unsigned char bit_mask = 1u; bit_mask > 0; bit_mask <<= 1u) {
        if (inbuf_get_bit()) {
            char_val |= bit_mask;
        }
        if (inbuf_next_bit()) {
            read_from_file(fInput);
        }
    }
    return char_val;
}

void write_char_to_outbuf(FILE *fOutput, unsigned char char_val) {
    for (unsigned char bit_mask = 1u; bit_mask > 0; bit_mask <<= 1u) {
        if (char_val & bit_mask) {
            outbuf_set_bit();
        }
        if (outbuf_next_bit()) {
            write_to_file(fOutput);
        }
    }
}

void write_code_to_outbuf(FILE *fOutput, const char *code) {
    for (; *code; ++code) {
        //write a bit to the buffer
        if (*code == '1') {
            outbuf_set_bit();
        }
        //move to the next buffer's bit
        if (outbuf_next_bit()) {
            //dump buffer to the file in case of overflow
            write_to_file(fOutput);
        }
    }
}

//write/read tree

void write_tree(FILE *fOutput, Tree *node) {
    //a tree is written to the file by preorder traversal
    //tree encoding: 0 - go down, 1 + *symbol's 8 bits* - a leaf with a symbol
    if (node == NULL) {
        return;
    }
    if (node->label.sym <= UCHAR_MAX) {
        //a leaf has been reached
        outbuf_set_bit();
        if (outbuf_next_bit()) {
            write_to_file(fOutput);
        }
        write_char_to_outbuf(fOutput, node->label.sym);
    }
    else if (outbuf_next_bit()) {
        write_to_file(fOutput);
    }
    write_tree(fOutput, node->left);
    write_tree(fOutput, node->right);
}

Tree *read_tree(FILE *fInput) {
    Tree *node = tree_create(make_tag(UINT_MAX, 0));
    unsigned is_leaf = inbuf_get_bit();
    if (inbuf_next_bit()) {
        read_from_file(fInput);
    }
    if (is_leaf) {
        //read the leaf's symbol
        node->label.sym = read_char_from_inbuf(fInput);
        return node;
    }
    //read node's childs
    node->left = read_tree(fInput);
    node->right = read_tree(fInput);
    return node;
}

//encoding

void encode(FILE *fInput, FILE *fOutput) {
    inbuf_reset();
    char *code = NULL;
    unsigned char_num = 0;
    while ((char_num = read_from_file(fInput)) > 0) {
        for (unsigned i = 0; i < char_num; ++i) {
            code = get_code(inbuf_get_byte());
            inbuf_next_byte();
            //write symbol's code to the buffer
            write_code_to_outbuf(fOutput, code);
        }
    }
    write_to_file(fOutput);
}

void encode_file(FILE *fInput, FILE *fOutput) {
    //get characters' frequences & size of the file
    analyze_file(fInput);
    //build code tree & table
    Tree *root = build_code_tree();
    build_code_table(root);
    //write file header
    write_tree(fOutput, root);
    //write encoded symbols to the buffer
    encode(fInput, fOutput);
    //free resources
    tree_destroy(root);
}

//decoding

void decode(FILE *fInput, FILE *fOutput, unsigned file_size, Tree *root) {
    Tree *node = root;
    outbuf_reset(); //re-consider
    for (unsigned char_ix = 0; char_ix < file_size; ++char_ix) {
        while (node->label.sym > UCHAR_MAX) {
            if (inbuf_get_bit()) {
                node = node->right;
            }
            else {
                node = node->left;
            }
            if (inbuf_next_bit()) {
                read_from_file(fInput);
            }
        }
        outbuf_set_byte(node->label.sym);
        if (outbuf_next_byte()) {
            write_to_file(fOutput);
        }
        node = root;
    }
    write_to_file(fOutput);
}

void decode_file(FILE *fInput, FILE *fOutput, unsigned file_size) {
    Tree *root = NULL;
    //read file header
    read_from_file(fInput); //re-consider
    if (file_size > 0) {
        //read the code tree
        root = read_tree(fInput);
    }
    //decode the input file
    decode(fInput, fOutput, file_size, root);
    //free resources
    tree_destroy(root);
}
