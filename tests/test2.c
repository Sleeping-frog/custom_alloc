#include "tests.h"

void test2() {
    int* arr[10];
    printf("Allocations:\n");
    for (int i = 0; i < 10; ++i) {
        arr[i] = alloc (50 * sizeof(int));
        log_memory();
    }
    printf("\n\nDeallocations:\n");
    for (int i = 0; i < 10; ++i) {
        dealloc(arr[i]);
        log_memory();
    }
}