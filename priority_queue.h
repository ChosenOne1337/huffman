#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "huffman_tree.h"

//heap

typedef struct Heap {
    Tree **ary;
    int ary_size;
    int size;
} Heap;

Heap *heap_create(int size);

void *heap_destroy(Heap *heap);

int heap_empty(Heap *heap);

int heap_size(Heap *heap);

Tree *heap_top(Heap *heap);

Tree *heap_extract(Heap *heap);

void heap_insert(Heap *heap, Tree *node);

#endif // PRIORITY_QUEUE_H
