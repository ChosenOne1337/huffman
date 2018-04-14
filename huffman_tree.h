#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

typedef struct Tag {
    unsigned sym;
    unsigned freq;
} Tag;

Tag make_tag(unsigned sym, unsigned freq);

typedef struct Tree {
    Tag label;
    //left <=> 0, right <=> 1
    struct Tree *left, *right;
} Tree;

Tree *tree_create(Tag label);

Tree *build_code_tree(void);

void build_code_table(Tree *root);

char *get_code(unsigned char sym);

void print_code_table(void);

void tree_destroy(Tree *root);

#endif // HUFFMAN_TREE_H
