#pragma once
#include <stdlib.h>

void* alloc(size_t bytes);
void dealloc(void* ptr);

void add_thread_to_garbage_collector();
void delete_thread_from_garbage_collector();
void collect_garbage();

void debug_log();

