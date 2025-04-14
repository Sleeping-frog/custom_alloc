#define _GNU_SOURCE     /* To get pthread_getattr_np() declaration */
#include "alloc.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdbool.h> 
#include <limits.h>
#include <assert.h>
#include "hashTable.h"
#include "vector.h"
#include <pthread.h>
 
typedef struct block_mini_metadata {
    size_t block_size;
    struct block_mini_metadata* next_block;
} block_mini_metadata;

typedef size_t mini_metadata;

typedef struct block_metadata {
    size_t block_size;
    struct block_metadata* next_block;
    size_t chunks_count;
} block_metadata;
 
typedef struct chunk_metadata {
    void* start;
    size_t size;
} chunk_metadata;
 
void* first_block_ptr = NULL;
void* first_mini_block_ptr = NULL;

void** mini_blocks_arr = NULL;  // array of pointers to blocks of data sorted from highest to lowest
size_t mini_blocks_cap = 0;
size_t mini_blocks_size = 0;
bool realloc_mini_blocks_arr = false;

void** blocks_arr = NULL;  // array of pointers to blocks of data sorted from highest to lowest
size_t blocks_cap = 0;
size_t blocks_size = 0;
bool realloc_blocks_arr = false;
 
void* alloc_init_block(size_t size) {
    void* ptr;
    const size_t size_2MB = 2 * 1024 * 1024;
    if (size + sizeof(chunk_metadata) <= (4096 - sizeof(block_metadata)) / 4) {  // if size is less than 1/4 of default free space
        ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        block_metadata* meta = (block_metadata*)ptr;
        meta->block_size = 4096;
        meta->next_block = NULL;
        meta->chunks_count = 0;
        return ptr;
    }
    if (size + sizeof(chunk_metadata) < 512 * 1024 - sizeof(block_metadata)) {  // if size is less than 512kB
        size_t block_size = ((size + sizeof(chunk_metadata) + 4095) / 4096) * 4096 * 4;  // size of block is 4 sizes of data
        ptr = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        block_metadata* meta = (block_metadata*)ptr;
        meta->block_size = block_size;
        meta->next_block = NULL;
        meta->chunks_count = 0;
        return ptr;
    }
    if (size < __PTRDIFF_MAX__) {  // if size is less than maxinum pointer difference
        size_t block_size = ((size + sizeof(chunk_metadata) + size_2MB - 1) / size_2MB) * size_2MB;  // size of block is size of data
        ptr = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        block_metadata* meta = (block_metadata*)ptr;
        meta->block_size = block_size;
        meta->next_block = NULL;
        meta->chunks_count = 0;
        return ptr;
    }
    return NULL;
}

void* alloc_mini_init_block() {
    void* ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    block_mini_metadata* meta = (block_mini_metadata*)ptr;
    meta->block_size = 4096;
    meta->next_block = NULL;
    return ptr;
}

int get_mini_type(mini_metadata* meta) {
    return (*meta >> 62) & 0b11;
}

void* alloc_mini(size_t bytes) {
    if (first_mini_block_ptr == NULL) {
        first_mini_block_ptr = alloc_mini_init_block(NULL);
        mini_blocks_arr = alloc(4 * sizeof(void*));
        mini_blocks_arr[0] = first_mini_block_ptr;
        mini_blocks_cap = 4;
        mini_blocks_size = 1;
        
    }

    size_t size;
    if (bytes <= 4)
        size = 4;
    else 
        size = 8 * ((bytes + 7) / 8);
    size_t size_8b = size / 8;
    
    block_mini_metadata* block_meta = first_mini_block_ptr;
    while (true) {
        mini_metadata* meta = (mini_metadata*)((void*)block_meta + sizeof(block_mini_metadata));
        while ((void*)block_meta + block_meta->block_size - (void*)meta >= sizeof(meta) + size * 62) {
            if (*meta == 0) {
                if (size_8b == 0)
                    *meta = 1;  // code 00 in the first two digits and non-zero bit not to misinterpret as free space
                else 
                    *meta = size_8b << 62;  // set code (size_8b) to the first two digits
                *meta |= (size_t)1 << 61;
                return (void*)meta + sizeof(meta);
            }
            int type = get_mini_type(meta);  // left two bits that represent type
            if (size_8b != type || (*meta | ((size_t)0b11 << 62)) == ULLONG_MAX) {  // if wrong type or bitmap is full
                size_t data_size  = type == 0 ? 4 : type * 8;
                meta = (mini_metadata*)((void*)meta + sizeof(meta) + data_size * 62);
            }
            else
                for (int position = 61; position >= 0; --position) {
                    bool is_busy = (*meta >> position) & 1;
                    if (!is_busy) {
                        *meta |= (size_t)1 << position;
                        return (void*)meta + sizeof(meta) + (61 - position) * size;
                    }
                }
        }
        if (block_meta->next_block == NULL) {
            if (mini_blocks_cap - mini_blocks_size < 2 && !realloc_mini_blocks_arr) {
                realloc_mini_blocks_arr = true;
                void** tmp = alloc(2 * mini_blocks_cap * sizeof(void*));
                realloc_mini_blocks_arr = false;
                mini_blocks_cap *= 2;
                for (size_t i = 0; i < mini_blocks_size; ++i) {
                    tmp[i] = mini_blocks_arr[i];    
                }
                dealloc(mini_blocks_arr);
                mini_blocks_arr = tmp;
            }
            block_meta->next_block = alloc_mini_init_block();
            void* const new_ptr = block_meta->next_block;
            for (int i = mini_blocks_size; i > 0; --i) {
                if (new_ptr > mini_blocks_arr[i - 1])
                    mini_blocks_arr[i] = mini_blocks_arr[i - 1];
                else {
                    mini_blocks_arr[i] = new_ptr;
                    break;
                }
            }
            if (mini_blocks_arr[0] == mini_blocks_arr[1])  // if we moved every pointer to the left (if new_ptr is bigger than any other)
                mini_blocks_arr[0] = new_ptr;
            ++mini_blocks_size;
        }
        block_meta = block_meta->next_block;
    }
}
 
void* alloc(size_t bytes) {
    if (bytes == 0)
        bytes = 4;
    if (bytes <= 24) 
        return alloc_mini(bytes);

    const size_t size = ((bytes + 7) / 8) * 8;  // make size divisible by 8

    if (first_block_ptr == NULL) {
        first_block_ptr = alloc_init_block(size);
        blocks_arr = alloc(4 * sizeof(void*));
        blocks_arr[0] = first_block_ptr;
        blocks_cap = 4;
        blocks_size = 1;
    }
    
    block_metadata* block_meta = first_block_ptr;

    while (true) {
        if (size + sizeof(chunk_metadata) <= block_meta->block_size - sizeof(block_metadata)) {
            chunk_metadata* meta = (chunk_metadata*)((void*)block_meta + sizeof(block_metadata));
            void* end_ptr = (void*)block_meta + block_meta->block_size;
            void* start_ptr = (void*)block_meta + sizeof(block_metadata) + block_meta->chunks_count * sizeof(chunk_metadata);
            if (block_meta->chunks_count == 0) {
                block_meta->chunks_count = 1;
                meta->size = size;
                meta->start = (void*)end_ptr - size;
                return meta->start;
            }
            size_t last_index = block_meta->chunks_count - 1;
            if ((void*)meta[last_index].start - (void*)&meta[last_index + 1] >= sizeof(chunk_metadata)) {  // check if new chunk_metadata fits
                if (end_ptr - (meta[0].start + meta[0].size) >= size) {  // check if data fits in the right last bytes
                    ++block_meta->chunks_count;
                    for (int i = last_index; i >= 0; --i)
                        meta[i + 1] = meta[i];
                    meta[0].size = size;
                    meta[0].start = end_ptr - size;
                    return meta[0].start;
                }
                for (int i = 0; i < last_index; ++i) {
                    if (meta[i].start - (meta[i + 1].start + meta[i + 1].size) >= size) {
                        ++block_meta->chunks_count;
                        for (int j = last_index; j > i; --j)
                            meta[j + 1] = meta[j];
                        meta[i + 1].size = size;
                        meta[i + 1].start = meta[i].start - size;
                        return meta[i + 1].start;
                    }
                }
                if ((void*)meta[last_index].start - (void*)&meta[last_index + 1] >= sizeof(chunk_metadata) + size) {  // check if data fits to the left of the leftest data
                    ++block_meta->chunks_count;
                    meta[last_index + 1].size = size;
                    meta[last_index + 1].start = (void*)meta[last_index].start - size;
                    return meta[last_index + 1].start;
                }
            }
        }
        if (block_meta->next_block == NULL) {
            if (blocks_cap - blocks_size < 2 && !realloc_blocks_arr) {
                realloc_blocks_arr = true;
                void** tmp = alloc(2 * blocks_cap * sizeof(void*));
                realloc_blocks_arr = false;
                blocks_cap *= 2;
                for (size_t i = 0; i < blocks_size; ++i) {
                    tmp[i] = blocks_arr[i];    
                }
                dealloc(blocks_arr);
                blocks_arr = tmp;
            }
            if (block_meta->next_block == NULL) {
                block_meta->next_block = alloc_init_block(size);
                void* const new_ptr = block_meta->next_block;
                for (int i = blocks_size; i > 0; --i) {
                    if (new_ptr > blocks_arr[i - 1])
                        blocks_arr[i] = blocks_arr[i - 1];
                    else {
                        blocks_arr[i] = new_ptr;
                        break;
                    }
                }
                if (blocks_arr[0] == blocks_arr[1])  // if we moved every pointer to the left (if new_ptr is bigger than any other)
                    blocks_arr[0] = new_ptr;
                ++blocks_size;
            }
        }
        block_meta = block_meta->next_block;
    }

    return NULL;
}
 
void dealloc(void* ptr) {
    // part for mini allocations
    if (mini_blocks_arr != NULL) {
        size_t start = 0;
        size_t end = mini_blocks_size;
        if (ptr > mini_blocks_arr[0])
            end = 0;
        else
            while (end - start != 1) {
                size_t mid = (start + end) / 2;
                if (ptr < mini_blocks_arr[mid])
                    start = mid;
                else
                    end = mid;
            }
        if (end == mini_blocks_size) {
            return;
        }
        block_mini_metadata* block_meta = mini_blocks_arr[end];
        if (ptr < (void*)block_meta + sizeof(block_mini_metadata) || ptr > (void*)block_meta + block_meta->block_size) {
            return;
        }
        mini_metadata* meta = (void*)block_meta + sizeof(block_mini_metadata);
        while (*meta != 0) {
            int type = get_mini_type(meta);
            size_t data_size = type == 0 ? 4 : type * 8;
            if (ptr < (void*)meta + sizeof(mini_metadata)) {
                fprintf(stderr, "dealloc() error: incorrect address\n");
                abort();
            }
            if (ptr > (void*)meta + sizeof(mini_metadata) + data_size * 62) {  // if ptr is not in our mini chank
                meta = (void*)meta + sizeof(mini_metadata) + data_size * 62;
                continue;
            }
            size_t data_index = 61 - (ptr - ((void*)meta + sizeof(mini_metadata))) / data_size;
            if (type == 0 && data_index == 0) {
                fprintf(stderr, "dealloc() error: incorrect address\n");
                abort();
            }
            size_t mask = ULLONG_MAX ^ (1ULL << data_index);
            *meta &= mask;
            return;
        }
    }

    // part for big allocations
    if (blocks_arr != NULL) {
        size_t start = 0;
        size_t end = blocks_size;
        if (ptr > blocks_arr[0])
            end = 0;
        else 
            while (end - start != 1) {
                size_t mid = (start + end) / 2;
                if (ptr < blocks_arr[mid])
                    start = mid;
                else
                    end = mid;
            }
        if (end == blocks_size) {  // if ptr is less than minimal block
            fprintf(stderr, "dealloc() error: incorrect address\n");
            abort();
        }
        block_metadata* block_meta = blocks_arr[end];
        if (block_meta->chunks_count == 0) {  // if block is empty
            fprintf(stderr, "dealloc() error: incorrect address\n");
            abort();
        }
        if (ptr < (void*)block_meta + sizeof(block_metadata) || ptr > (void*)block_meta + block_meta->block_size) {  // if ptr is outside of block
            fprintf(stderr, "dealloc() error: incorrect address\n");
            abort();
        }
        chunk_metadata* meta = (void*)block_meta + sizeof(block_metadata);
        size_t meta_index = block_meta->chunks_count;
        start = 0;
        end = block_meta->chunks_count;
        if (ptr == meta[0].start) {
            meta_index = 0;
        }
        else while (end - start > 1) {
            size_t mid = (start + end) / 2;
            if (ptr == meta[mid].start) {
                meta_index = mid;
                break;
            }
            else if (ptr > meta[mid].start)
                end = mid;
            else
                start = mid;
        }
        if (meta_index == block_meta->chunks_count) {
            fprintf(stderr, "dealloc() error: incorrect address\n");
            abort();
        }
        --block_meta->chunks_count;
        for (int i = meta_index; i < block_meta->chunks_count; ++i) {
            meta[i] = meta[i + 1];
        }
        return;
    }
    fprintf(stderr, "dealloc() error: incorrect address\n");
    abort();
}


///////////////////////////// garbage collcetor /////////////////////////////


vector_stack stacks = {0};

void add_thread_to_garbage_collector() {
    if (stacks.capacity == 0)
        vector_init(&stacks);
    stack_attr attr;
    pthread_attr_t pth_attr;
    pthread_attr_init(&pth_attr);
    pthread_getattr_np(pthread_self(), &pth_attr);
    pthread_attr_getstack(&pth_attr, &attr.addr, &attr.size);
    pthread_attr_destroy(&pth_attr);
    vector_push_back(&stacks, attr);
}

void delete_thread_from_garbage_collector() {
    stack_attr attr;
    pthread_attr_t pth_attr;
    pthread_attr_init(&pth_attr);
    pthread_getattr_np(pthread_self(), &pth_attr);
    pthread_attr_getstack(&pth_attr, &attr.addr, &attr.size);
    pthread_attr_destroy(&pth_attr);
    vector_delete(&stacks, attr.addr);
}

void collect_garbage() {
    hashTable found_ptrs;
    hash_init(&found_ptrs);
    hash_insert(&found_ptrs, blocks_arr);
    hash_insert(&found_ptrs, mini_blocks_arr);

    // searching for all stored pointers
    if (stacks.size > 0)
    for (size_t i = 0; i < stacks.size; ++i) {
        void* stack_addr = vector_at(&stacks, i)->addr;
        size_t stack_size = vector_at(&stacks, i)->size;
        for (void** ptr_to_ptr = stack_addr; ptr_to_ptr < (void**)(stack_addr + stack_size); ++ptr_to_ptr) {
            void* ptr = *ptr_to_ptr;
            if (blocks_arr != NULL) {
                size_t start = 0;
                size_t end = blocks_size;
                if (ptr > blocks_arr[0])
                    end = 0;
                else 
                    while (end - start != 1) {
                        size_t mid = (start + end) / 2;
                        if (ptr < blocks_arr[mid]) 
                            start = mid;
                        else
                            end = mid;
                    }
                if (end == blocks_size) {  // if ptr is less than minimal block
                    continue;
                }
                block_metadata* block_meta = blocks_arr[end];
                if (block_meta->chunks_count == 0) {  // if block is empty
                    continue;
                }
                if (ptr < (void*)block_meta + sizeof(block_metadata) || ptr > (void*)block_meta + block_meta->block_size) {  // if ptr is outside of block
                    continue;
                }
                chunk_metadata* meta = (void*)block_meta + sizeof(block_metadata);
                size_t meta_index = block_meta->chunks_count;
                start = 0;
                end = block_meta->chunks_count;
                unsigned i = 0;
                if (ptr == meta[0].start) {
                    meta_index = 0;
                }
                else while (end - start > 1) {
                    size_t mid = (start + end) / 2;
                    if (ptr == meta[mid].start) {
                        meta_index = mid;
                        break;
                    }
                    else if (ptr > meta[mid].start)
                        end = mid;
                    else
                        start = mid;
                }
                if (meta_index == block_meta->chunks_count) {
                    continue;
                }
                hash_insert(&found_ptrs, meta[meta_index].start);
                printf("Found pointer: %p\t\t%p\n", meta[meta_index].start, ptr_to_ptr);
            }
        }
    }

    hash_insert(&found_ptrs, found_ptrs.table_ptr);
    hash_insert(&found_ptrs, found_ptrs.states);

    // deleting unused memory

    // big blocks
    for (block_metadata* block_meta = first_block_ptr; block_meta != NULL; block_meta = block_meta->next_block) {
        size_t chunks_count = block_meta->chunks_count;
        if (chunks_count == 0)
            continue;
        chunk_metadata* meta = (void*)block_meta + sizeof(block_metadata);
        unsigned i = 0, j = 0;
        while (j < chunks_count) {
            if (hash_contains(&found_ptrs, meta[j].start)) {
                if (i != j)
                    meta[i] = meta[j];
                ++i;
            }
            ++j;
        }
        block_meta->chunks_count = i;
    }

    // mini blocks
    for (block_mini_metadata* block_meta = first_mini_block_ptr; block_meta != NULL; block_meta = block_meta->next_block) {
        mini_metadata* meta = (void*)block_meta + sizeof(block_mini_metadata);
        while (*meta != 0) {
            int type = get_mini_type(meta);
            size_t data_size = type == 0 ? 4 : type * 8;
            for (int position = 61; position >= (type == 0 ? 1 : 0); --position) {  // if type is 00 then last bit is reserved and must not be deleted
                if ((*meta & (1ULL << position) != 0) && !hash_contains(&found_ptrs, (void*)meta + sizeof(meta) + (61 - position) * data_size)) {
                    size_t mask = ULLONG_MAX ^ (1ULL << position);
                    *meta &= mask;
                }
            }
            meta = (void*)meta + sizeof(mini_metadata) + data_size * 62;
        }
    }

    hash_destroy(&found_ptrs);
}

void debug_log() {
    if (first_mini_block_ptr == NULL)
        printf("No mini allocations\n");
    else
        printf("There are %zu mini blocks\n", mini_blocks_size);
    if (first_block_ptr == NULL)
        printf("No big allocations\n");
    else {
        /*printf("There are %zu big blocks\n", blocks_size);
        block_metadata* block_meta = first_block_ptr;
        for (int i = 0; i < blocks_size; ++i) {
            printf("Block %d:\n", i + 1);
            chunk_metadata* meta = (void*)block_meta + sizeof(block_metadata);
            for (int j = 0; j < block_meta->chunks_count; ++j) {
                printf("%d)\taddress: %p\n", j + 1, meta[j].start);
                printf("\tsize: %zu\n", meta[j].size);
            }
            printf("\n");
            block_meta = block_meta->next_block;
        }*/
        printf("There are %zu big blocks\n", blocks_size);
        block_metadata* block_meta = first_block_ptr;
        for (int i = 0; i < blocks_size; ++i) {
            const int group_size = 32;
            printf("Block %d:\n", i + 1);
            chunk_metadata* meta = (void*)block_meta + sizeof(block_metadata);
            int display_index = 0;
            printf("\033[0;31m");  // set red color
            while (group_size * display_index + group_size <= sizeof(block_metadata) + block_meta->chunks_count * sizeof(chunk_metadata)) {  // occupied by metadata
                printf("0");
                ++display_index;
            }
            printf("\033[0;37m");  // set white color
            void* cur_index_first_byte = (void*)block_meta + group_size * display_index;
            int meta_index = block_meta->chunks_count - 1;
            if (block_meta->chunks_count > 0) {
                if ((void*)meta[meta_index].start - cur_index_first_byte > 0 && group_size * display_index != sizeof(block_metadata) + block_meta->chunks_count * sizeof(chunk_metadata)) {
                    printf("\033[0;33m");  // set yellow color
                    printf("0");
                    printf("\033[0;37m");  // set white color
                    ++display_index;
                    cur_index_first_byte += group_size;
                }
                while (cur_index_first_byte + group_size <= meta[meta_index].start) {
                    printf("\033[0;32m");  // set green color
                    printf("0");
                    printf("\033[0;37m");  // set white color
                    ++display_index;
                    cur_index_first_byte += group_size;
                }
                if (cur_index_first_byte != meta[meta_index].start) {
                    printf("\033[0;33m");  // set yellow color
                    printf("0");
                    printf("\033[0;37m");  // set white color
                    ++display_index;
                    cur_index_first_byte += group_size;
                }
                for ( ; meta_index > 0; --meta_index) {
                    while (cur_index_first_byte + group_size <= (void*)meta[meta_index].start + meta[meta_index].size) {
                        printf("\033[0;31m");  // set red color
                        printf("0");
                        printf("\033[0;37m");  // set white color
                        ++display_index;
                        cur_index_first_byte += group_size;
                    }
                    if (cur_index_first_byte != (void*)meta[meta_index].start + meta[meta_index].size) {
                        if ((void*)meta[meta_index - 1].start - ((void*)meta[meta_index].start + meta[meta_index].size) > 0) {
                            printf("\033[0;33m");  // set yellow color
                            printf("0");
                            printf("\033[0;37m");  // set white color
                            ++display_index;
                            cur_index_first_byte += group_size;
                        }
                        else {
                            printf("\033[0;31m");  // set red color
                            printf("0");
                            printf("\033[0;37m");  // set white color
                            ++display_index;
                            cur_index_first_byte += group_size;
                        }
                    }
                    while (cur_index_first_byte + group_size <= meta[meta_index - 1].start) {
                        printf("\033[0;32m");  // set green color
                        printf("0");
                        printf("\033[0;37m");  // set white color
                        ++display_index;
                        cur_index_first_byte += group_size;
                    }
                }
                while (cur_index_first_byte + group_size <= (void*)meta[0].start + meta[0].size) {
                    printf("\033[0;31m");  // set red color
                    printf("0");
                    printf("\033[0;37m");  // set white color
                    ++display_index;
                    cur_index_first_byte += group_size;
                }
                if (cur_index_first_byte != (void*)meta[0].start + meta[0].size) {
                    printf("\033[0;33m");  // set yellow color
                    printf("0");
                    printf("\033[0;37m");  // set white color
                    ++display_index;
                    cur_index_first_byte += group_size;
                }
                while (cur_index_first_byte + group_size <= ((void*)block_meta + block_meta->block_size)) {
                    printf("\033[0;32m");  // set green color
                    printf("0");
                    printf("\033[0;37m");  // set white color
                    ++display_index;
                    cur_index_first_byte += group_size;
                }
            }
            else {
                if (cur_index_first_byte < (void*)block_meta + sizeof(block_metadata)) {
                    printf("\033[0;33m");  // set yellow color
                    printf("0");
                    printf("\033[0;37m");  // set white color
                    ++display_index;
                    cur_index_first_byte += group_size;
                }
                while (cur_index_first_byte + group_size <= ((void*)block_meta + block_meta->block_size)) {
                    printf("\033[0;32m");  // set green color
                    printf("0");
                    printf("\033[0;37m");  // set white color
                    ++display_index;
                    cur_index_first_byte += group_size;
                }
            }
            printf("\033[0;37m");  // set white color
            printf("\n");
            block_meta = block_meta->next_block;
        }
    }
}
