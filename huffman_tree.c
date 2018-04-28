#include <stdlib.h>
#include <string.h>
#include "priority_queue.h"
#include "huffman_tree.h"
#include "huffman_coding.h"

#define MAX_CODE_LEN 64

char code_table[ALPH_SIZE][MAX_CODE_LEN + 1] = {0};

//huffman tree

Tag make_tag(unsigned sym, unsigned freq) {
    Tag t = {.sym = sym, .freq = freq};
    return t;
}

Tree *tree_create(Tag label) {
    Tree *node = (Tree*)malloc(sizeof(Tree));
    if (node != NULL) {
        node->label = label;
        node->left = node->right = NULL;
    }
    return node;
}

static void init_heap(Heap *heap) {
    Tag label;
    Tree *node = NULL;
    for (unsigned sym = 0, freq = 0; sym < ALPH_SIZE; ++sym) {
        freq = get_sym_freq(sym);
        if (freq > 0) {
            label = make_tag(sym, freq);
            node = tree_create(label);
            heap_insert(heap, node);
        }
    }
}

Tree *build_code_tree(void) {
    Tag label;
    Tree *node = NULL;
    Tree *node1 = NULL, *node2 = NULL;
    //init heap
    Heap *heap = heap_create(ALPH_SIZE);
    init_heap(heap);
    //build tree
    while (!heap_empty(heap)) {
        if (heap_size(heap) == 1) {
            //huffman tree has been built
            return heap_extract(heap);
        }
        //extract two nodes
        node1 = heap_extract(heap);
        node2 = heap_extract(heap);
        //identify node1 and node2
        label = make_tag(UINT_MAX, node1->label.freq + node2->label.freq);
        node = tree_create(label);
        //connect nodes
        node->left = node1;
        node->right = node2;
        //push to the heap
        heap_insert(heap, node);
    }
    //destroy heap
    heap_destroy(heap);
    return NULL;
}

void tree_traversal(Tree *node) {
    //fill code table while traversing the tree
    static char buf[MAX_CODE_LEN + 1] = {0};
    static int depth = -1;

    ++depth;
    if (node->left == NULL && node->right == NULL) {
        //a node with a symbol has been found
        snprintf(code_table[node->label.sym], MAX_CODE_LEN + 1, buf);
        --depth;
        return;
    }
    buf[depth] = '0'; buf[depth + 1] = '\0';
    tree_traversal(node->left);
    buf[depth] = '1'; buf[depth + 1] = '\0';
    tree_traversal(node->right);
    --depth;
}

void tree_destroy(Tree *root) {
    if (root != NULL) {
        tree_destroy(root->left);
        tree_destroy(root->right);
        free(root);
    }
}

//code table

void reset_code_table(void) {
    memset(code_table, 0, ALPH_SIZE * (MAX_CODE_LEN + 1));
}

void build_code_table(Tree *root) {
    reset_code_table();
    if (root == NULL) {
        return;
    }
    if (root->left == NULL && root->right == NULL) {
        //the case of a single node in the tree
        snprintf(code_table[root->label.sym], MAX_CODE_LEN + 1, "0");
        return;
    }
    tree_traversal(root);
}

char *get_code(unsigned char sym) {
    return code_table[sym];
}

void print_code_table(void) {
    for (unsigned i = 0; i < ALPH_SIZE; ++i) {
        if (code_table[i][0]) {
            printf("%c) %s\n", i, code_table[i]);
        }
    }
    printf("\n");
}
