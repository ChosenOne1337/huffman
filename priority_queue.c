#include <stdlib.h>
#include "priority_queue.h"

void swap(Tree **pNode1, Tree **pNode2) {
    Tree *node = *pNode1;
    *pNode1 = *pNode2;
    *pNode2 = node;
}

//heap create/destroy

Heap *heap_create(int size) {
    Heap *heap = (Heap*)malloc(sizeof(Heap));
    heap->ary = (Tree**)calloc(size, sizeof(Tree*));

    heap->ary_size = size;
    heap->size = 0;

    return heap;
}

void *heap_destroy(Heap *heap) {
    if (heap != NULL) {
        if (heap->ary != NULL) {
            free(heap->ary);
        }
        free(heap);
    }
    return NULL;
}

//get heap's size

int heap_empty(Heap *heap) {
    return heap == NULL || heap->size == 0;
}

int heap_size(Heap *heap) {
    return (heap == NULL) ? 0 : heap->size;
}

//get parents/childs

inline int parent(int i) {
    return (i - 1) >> 1;
}

inline int left(int i) {
    return (i << 1) + 1;
}

inline int right(int i) {
    return (i + 1) << 1;
}

//sift up/down

void heap_sift_up(Heap *heap, int i) {
    if (i == 0) {
        return;
    }
    if (heap->ary[i]->label.freq < heap->ary[parent(i)]->label.freq) {
        swap(&heap->ary[i], &heap->ary[parent(i)]);
        heap_sift_up(heap, parent(i));
    }
}

void heap_sift_down(Heap *heap, int i) {
    int l = left(i), r = right(i);
    int lowest = i;

    if (l < heap->size && heap->ary[lowest]->label.freq > heap->ary[l]->label.freq) {
        lowest = l;
    }
    if (r < heap->size && heap->ary[lowest]->label.freq > heap->ary[r]->label.freq) {
        lowest = r;
    }

    if (lowest != i) {
        swap(&heap->ary[i], &heap->ary[lowest]);
        heap_sift_down(heap, lowest);
    }
}

//extract/insert

Tree *heap_top(Heap *heap) {
    return heap->ary[0];
}

Tree *heap_extract(Heap *heap) {
    Tree *node = heap->ary[0];
    heap->ary[0] = heap->ary[heap->size - 1];
    heap->size -= 1;
    heap_sift_down(heap, 0);
    return node;
}

void heap_insert(Heap *heap, Tree *node) {
    if (heap->size == heap->ary_size) {
        return;
    }
    int i = heap->size;
    heap->ary[i] = node;
    heap->size += 1;
    heap_sift_up(heap, i);
}
