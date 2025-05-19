#include "tests.h"

void test1() {
    int* arr = alloc(2 * sizeof(int));
    arr[0] = 5;
    arr[1] = 10;
    log_memory();
    printf("5 + 10 = %d\n", arr[0] + arr[1]);
}