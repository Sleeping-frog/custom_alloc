#include "vector.h"
#include <stdio.h>

void vector_init(vector_stack* vec) {
    vec->ptr = alloc(4 * sizeof(stack_attr));
    vec->capacity = 4;
    vec->size = 0;
}

void vector_destroy(vector_stack* vec) {
    dealloc(vec->ptr);
    vec->ptr = NULL;
    vec->capacity = 0;
    vec->size = 0;
}

void vector_push_back(vector_stack* vec, stack_attr attr) {
    if (vec->size == vec->capacity) {  // if vector is full make new allocation
        stack_attr* tmp = alloc(2 * vec->capacity * sizeof(stack_attr));
        for (size_t i = 0; i < vec->capacity; ++i)
            tmp[i] = vec->ptr[i];
        dealloc(vec->ptr);
        vec->ptr = tmp;
        vec->capacity *= 2;
    }
    vec->ptr[vec->size++] = attr;
}

void vector_delete(vector_stack* vec, void* address) {
    for (size_t i = 0; i < vec->size; ++i) {
        if (vec->ptr[i].addr == address) {
            for (size_t j = i; j < vec->size - 1; ++j)
                vec->ptr[j] = vec->ptr[j + 1];
            vec->capacity--;
            return;
        }
    }
}

stack_attr* vector_at(vector_stack* vec, size_t index) {
    if (index >= vec->size) {
        fprintf(stderr, "vector_at() error: invalid index");
        abort();
    }
    return &vec->ptr[index];
}
