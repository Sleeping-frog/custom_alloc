#include "tests.h"

void test6() {
    int* arr[1000];
    for (int i = 0; i < 900; ++i)
        arr[i] = alloc(4);
    log_memory();
    for (int i = 900; i < 1000; ++i)
        arr[i] = alloc(4);
    log_memory();
}