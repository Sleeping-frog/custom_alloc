#include <stdio.h>
#include <limits.h>
#include "alloc.h"

int main() {
    size_t size = 2 * 100;
    void* arr[size];
    for (int i = 0; i < size; ++i) {
        arr[i] = alloc(32);
        //printf("%d)\t%p\n", i + 1, arr[i]);
    }
    for (int i = 0; i < 100; ++i) {
        dealloc(arr[2 * i]);
    }
    for (int i = 0; i < 100; ++i) {
        arr[i] = alloc(32);
        //printf("%d)\t%p\n", size + i + 1, arr[i]);
    }
    debug_log();
}