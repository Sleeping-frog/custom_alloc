#pragma once
#include "alloc.h"

typedef struct {
    void* addr;
    size_t size;
} vec_data;

typedef struct {
    size_t size;
    size_t capacity;
    vec_data* ptr;
} vector;

void vector_init(vector* vec);
void vector_destroy(vector* vec);
void vector_push_back(vector* vec, vec_data attr);
void vector_delete(vector* vec, void* address);
vec_data* vector_at(vector* vec, size_t index);