#pragma once
#include <alloc.h>

typedef struct {
    void* addr;
    size_t size;
} stack_attr;

typedef struct {
    size_t size;
    size_t capacity;
    stack_attr* ptr;
} vector_stack;

void vector_init(vector_stack* vec);
void vector_destroy(vector_stack* vec);
void vector_push_back(vector_stack* vec, stack_attr attr);
void vector_delete(vector_stack* vec, void* address);
stack_attr* vector_at(vector_stack* vec, size_t index);