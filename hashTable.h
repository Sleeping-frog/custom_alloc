#pragma once
#include <stdbool.h>

typedef struct {
    unsigned size;
    unsigned count;
    bool* states;
    void** table_ptr;
} hashTable;

void hash_init(hashTable* table);
void hash_destroy(hashTable* table);
void hash_insert(hashTable* table, void* val);
bool hash_contains(hashTable* table, void* val);

