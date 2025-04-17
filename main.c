#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include "alloc.h"
#include "hashTable.h"

const size_t size = 2 * 100;
void* arr[200];

int main() {
    add_thread_to_garbage_collector();

    for (int i = 0; i < 10; ++i) {
        arr[i] = alloc(32);
    }
    for (int i = 0; i < 100; i += 2) {
        arr[i] = NULL;
    }
    debug_log();
    collect_garbage();
    debug_log();
    delete_thread_from_garbage_collector();
}
