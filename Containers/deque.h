#pragma once
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    size_t size;
    size_t capacity;
    size_t start;
    void** ptr;
} deque;

void deque_init(deque* deq);
void deque_destroy(deque* deq);
bool deque_empty(deque* deq);
void deque_push_back(deque* deq, void* val);
void deque_pop_back(deque* deq);
void deque_push_front(deque* deq, void* val);
void deque_pop_front(deque* deq);
