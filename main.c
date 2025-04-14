#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include "alloc.h"
#include "hashTable.h"

int main() {
    add_thread_to_garbage_collector();
    size_t size = 2 * 100;
    void* arr[size];
    for (int i = 0; i < 100; ++i) {
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