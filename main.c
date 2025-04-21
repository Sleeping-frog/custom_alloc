#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include "alloc.h"
#include "Containers/hashTable.h"

const size_t size = 2 * 100;
void* arr[200];

void foo(void***** ptr) {
    *ptr = alloc(32);
    **ptr = alloc(32);
    ***ptr = alloc(32);
    ****ptr = alloc(32);
}

int main() {
    add_thread_to_garbage_collector();

    void**** ptr;
    foo(&ptr);

    printf("ptr: %p\n", ptr);
    printf("*ptr: %p\n", *ptr);
    printf("**ptr: %p\n", **ptr);
    printf("***ptr: %p\n", ***ptr);

    debug_log();
    collect_garbage();
    debug_log();
    delete_thread_from_garbage_collector();
}
