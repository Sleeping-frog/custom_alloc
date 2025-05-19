#include "tests.h"

void test3() {
    add_thread_to_garbage_collector();

    for (int i = 0; i < 10; ++i) 
        alloc(50 * sizeof(int));

    log_memory();
    collect_garbage();
    log_memory();

    delete_thread_from_garbage_collector();
}