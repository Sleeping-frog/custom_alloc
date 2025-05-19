#include "tests.h"

void foo(void***** ptr) {
    *ptr = alloc(32);
    **ptr = alloc(32);
    ***ptr = alloc(32);
    ****ptr = alloc(32);
}

void test4() {
    add_thread_to_garbage_collector();

    void**** ptr;
    foo(&ptr);

    printf("ptr: %p\n", ptr);
    printf("*ptr: %p\n", *ptr);
    printf("**ptr: %p\n", **ptr);
    printf("***ptr: %p\n", ***ptr);

    log_memory();
    collect_garbage();
    log_memory();
    delete_thread_from_garbage_collector();
}