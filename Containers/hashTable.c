#include "hashTable.h"
#include "../alloc.h"

extern void* alloc_sub(size_t);
extern void dealloc_sub(void*);

#define LOAD_FACTOR 0.6

unsigned primes[] = {53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843, 50331653};
// there are 21 prime numbers

unsigned hash1(void* ptr) {
    unsigned key = (unsigned)(size_t)ptr ^ (unsigned)((size_t)ptr >> 32);
    key = ((key >> 16) ^ key) * 0x45d9f3b;
    key = ((key >> 16) ^ key) * 0x45d9f3b;
    key = ((key >> 16) ^ key);
    return key;
}

unsigned hash2(void* ptr) {
    unsigned key = (unsigned)(size_t)ptr ^ (unsigned)((size_t)ptr >> 32) ^ 0b101010101010101010;
    key = ((key >> 16) ^ key) * 0x45d9f3b;
    key = ((key >> 16) ^ key) * 0x45d9f3b;
    key = ((key >> 16) ^ key);
    return key;
}

void hash_init(hashTable* table) {
    table->size = primes[0];
    table->count = 0;
    table->table_ptr = alloc_sub(table->size * sizeof(void*));
    table->states = alloc_sub(table->size * sizeof(bool));
    for (unsigned i = 0; i < table->size; ++i) {
        table->states[i] = 0;  // 0 means empty
    }
}

void hash_destroy(hashTable* table) {
    dealloc_sub(table->table_ptr);
    table->table_ptr = NULL;
    dealloc_sub(table->states);
    table->states = NULL;
    table->count = 0;
}

void rehash(hashTable* table) {
    int new_size;
    int i = 0;
    while (table->size != primes[i])
        ++i;
    new_size = primes[i + 1];
    hashTable tmp_table;
    tmp_table.size = new_size;
    tmp_table.count = 0;
    tmp_table.table_ptr = alloc_sub(new_size * sizeof(void*));
    tmp_table.states = alloc_sub(new_size * sizeof(bool));
    for (int i = 0; i < new_size; ++i) {
        tmp_table.states[i] = 0;
    }
    for (unsigned i = 0; i < table->size; ++i) {
        if (table->states[i] != 0)
            hash_insert(&tmp_table, table->table_ptr[i]);
    }
    dealloc_sub(table->table_ptr);
    table->table_ptr = tmp_table.table_ptr;
    dealloc_sub(table->states);
    table->states = tmp_table.states;
    table->size = tmp_table.size;
    hash_insert(table, table->states);
    hash_insert(table, table->table_ptr);
}

void hash_insert(hashTable* table, void* val) {
    if ((float)table->count / (float)table->size > LOAD_FACTOR)
        rehash(table);
    unsigned h1 = hash1(val) % table->size;
    unsigned h2 = (hash2(val) % (table->size - 1)) + 1;
    for (int i = 0; i < table->size; ++i) {
        unsigned index = (h1 + i * h2) % table->size;
        if (table->states[index] != 0) {
            if (table->table_ptr[index] == val)
                return;
        }
        else {
            table->states[index] = 1;
            table->table_ptr[index] = val;
            table->count++;
            break;
        }
    }
}

bool hash_contains(hashTable* table, void* val) {
    unsigned h1 = hash1(val) % table->size;
    unsigned h2 = (hash2(val) % (table->size - 1)) + 1;
    for (int i = 0; i < table->size; ++i) {
        unsigned index = (h1 + i * h2) % table->size;
        if (table->states[index] == 0)
            return false;
        else if (val == table->table_ptr[index])
            return true;
    }
}