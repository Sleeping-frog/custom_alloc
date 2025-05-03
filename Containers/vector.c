#include "vector.h"
#include <stdio.h>

extern void* alloc_sub(size_t);
extern void dealloc_sub(void*);

void vector_init(vector* vec) {
    vec->ptr = alloc_sub(4 * sizeof(vec_data));
    vec->capacity = 4;
    vec->size = 0;
}

void vector_destroy(vector* vec) {
    dealloc_sub(vec->ptr);
    vec->ptr = NULL;
    vec->capacity = 0;
    vec->size = 0;
}

void vector_push_back(vector* vec, vec_data attr) {
    if (vec->size == vec->capacity) {  // if vector is full make new allocation
        vec_data* tmp = alloc_sub(2 * vec->capacity * sizeof(vec_data));
        for (size_t i = 0; i < vec->capacity; ++i)
            tmp[i] = vec->ptr[i];
        dealloc_sub(vec->ptr);
        vec->ptr = tmp;
        vec->capacity *= 2;
    }
    vec->ptr[vec->size++] = attr;
}

void vector_delete(vector* vec, void* address) {
    for (size_t i = 0; i < vec->size; ++i) {
        if (vec->ptr[i].addr == address) {
            for (size_t j = i; j < vec->size - 1; ++j)
                vec->ptr[j] = vec->ptr[j + 1];
            vec->capacity--;
            return;
        }
    }
}

vec_data* vector_at(vector* vec, size_t index) {
    if (index >= vec->size) {
        fprintf(stderr, "vector_at() error: invalid index");
        abort();
    }
    return &vec->ptr[index];
}
