#include "deque.h"
#include "../alloc.h"
#include <stdio.h>

void deque_init(deque* deq) {
    deq->ptr = alloc(32 * sizeof(void*));
    deq->capacity = 32;
    deq->size = 0;
    deq->start = 0;
}

void deque_destroy(deque* deq) {
    dealloc(deq->ptr);
    deq->capacity = 0;
    deq->size = 0;
    deq->start = 0;
}

bool deque_empty(deque* deq) {
    return deq->size == 0;
}

void resize(deque* deq) {
    void** tmp = alloc(2 * deq->capacity * sizeof(void*));
    size_t offt = deq->start;
    size_t size = deq->size;
    size_t cap = deq->capacity;
    for (size_t i = 0; i < deq->size; ++i)
        tmp[i] = deq->ptr[(offt + i) % cap];
    dealloc(deq->ptr);
    deq->ptr = tmp;
    deq->capacity *= 2;
    deq->start = 0;
}

void deque_push_back(deque* deq, void* val) {
    if (deq->size == deq->capacity)
        resize(deq);
    size_t offt = deq->start;
    size_t size = deq->size;
    size_t cap = deq->capacity;
    deq->ptr[(offt + size) % cap] = val;
    ++deq->size;
}

void deque_pop_back(deque* deq) {
    if (deq->size == 0) {
        frintf(stderr, "Error: popping empty deque\n");
        abort();
    }
    --deq->size;
}

void deque_push_front(deque* deq, void* val) {
    if (deq->size == deq->capacity)
        resize(deq);
    size_t offt = deq->start;
    size_t cap = deq->capacity;
    offt = (offt + cap - 1) % cap;
    deq->ptr[offt] = val;
    ++deq->size;
    deq->start = offt;
}

void deque_pop_front(deque* deq) {
    if (deq->size == 0) {
        frintf(stderr, "Error: popping empty deque\n");
        abort();
    }
    --deq->size;
    size_t offt = deq->start;
    size_t cap = deq->capacity;
    offt = (offt + 1) % cap;
    deq->start = offt;
}
