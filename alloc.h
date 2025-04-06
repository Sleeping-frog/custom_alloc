#pragma once
#include <stdlib.h>

void* alloc(size_t bytes);

void dealloc(void* ptr);

void debug_log();

