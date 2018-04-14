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

unsigned analyze_file(FILE *fInput) {
    //the file is supposed to be successfully opened
    reset_freq_table();
    int char_num = 0;
    while ((char_num = read_from_file(fInput)) > 0) {
        for (int i = 0; i < char_num; ++i) {
            ++freq_table[inbuf_get_byte()];
            inbuf_next_byte();
        }
    }
    unsigned file_size = ftell(fInput);
    rewind(fInput);
    return file_size;
}

//auxiliary stuff

void print_freq_table(void) {
    for (int i = 0; i < ALPH_SIZE; ++i) {
        if (freq_table[i] > 0) {
            printf("%c)  -  %u\n", i, freq_table[i]);
        }
    }
    printf("\n");
}

void print_file_len(void) {
    unsigned long long len = 0;
    for (unsigned i = 0; i < ALPH_SIZE; ++i) {
        len += get_sym_freq(i) * strlen(get_code(i));
    }
    printf("File's predicted length: %I64u bytes + %I64u bits\n",
           len / CHAR_BIT, len % CHAR_BIT);
}

unsigned read_int_from_inbuf(FILE *fInput) {
    unsigned int_val = 0;
    for (unsigned bit_mask = 1u; bit_mask > 0; bit_mask <<= 1u) {
        if (inbuf_get_bit()) {
            int_val |= bit_mask;
        }
        if (inbuf_next_bit()) {
            read_from_file(fInput);
        }
    }
    return int_val;
}

void write_int_to_outbuf(FILE *fOutput, unsigned int_val) {
    for (unsigned bit_mask = 1u; bit_mask > 0; bit_mask <<= 1u) {
        if (int_val & bit_mask) {
            outbuf_set_bit();
        }
        if (outbuf_next_bit()) {
            write_to_file(fOutput);
        }
    }
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

void write_file_header(FILE *fOutput, unsigned file_size, Tree *root) {
    outbuf_reset(); //re-consider
    //write file's size to the header
    write_int_to_outbuf(fOutput, file_size);
    //write the code tree to the file
    write_tree(fOutput, root);
}

void encode(FILE *fInput, FILE *fOutput) {
    inbuf_reset(); //re-consider
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
    write_to_file(fOutput); //re-consider
}

void encode_file(FILE *fInput, FILE *fOutput) {
    //get characters' frequences & size of the file
    unsigned file_size = analyze_file(fInput);
    //build code tree & table
    Tree *root = build_code_tree();
    build_code_table(root);
    //write file header
    write_file_header(fOutput, file_size, root);
    //write encoded symbols to the buffer
    encode(fInput, fOutput);
    //free resources
    tree_destroy(root);
}

//decoding

unsigned read_file_header(FILE *fInput, Tree **root) {
    //read from file
    read_from_file(fInput); //re-consider
    //get the input file's size
    unsigned file_size = read_int_from_inbuf(fInput);
    if (file_size > 0) {
        //read the code tree
        *root = read_tree(fInput);
    }
    return file_size;
}

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

unsigned decode_file(FILE *fInput, FILE *fOutput) {
    Tree *root = NULL;
    //read file header
    unsigned file_size = read_file_header(fInput, &root);
    //decode the input file
    decode(fInput, fOutput, file_size, root);
    //free resources
    tree_destroy(root);

    return file_size;
}
