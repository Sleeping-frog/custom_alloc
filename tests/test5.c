#include "tests.h"
#include <pthread.h>
#include "unistd.h"

void* new_thread(void*) {
    add_thread_to_garbage_collector();

    int* arr[10];
    for (int i = 0; i < 10; ++i) {
        arr[i] = alloc(32);
    }
    sleep(3);
    delete_thread_from_garbage_collector();
    return NULL;
}

void test5() {
    pthread_t tids[10];
    for (int i = 0; i < 10; ++i) {
        pthread_create(&tids[i], NULL, new_thread, NULL);
    }

    sleep(1);
    log_memory();
    collect_garbage();
    log_memory();

    for (int i = 0; i < 10; ++i) {
        pthread_join(tids[i], NULL);
    }

    collect_garbage();
    log_memory();
}