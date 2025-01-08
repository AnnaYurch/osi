#include "library.h"

typedef struct Allocator {
    void *memory; 
    size_t size; 
    uint8_t *bitmap; 
    size_t block_size; 
} Allocator;


Allocator* allocator_create(void *const memory, const size_t size) {
    if (!memory || size == 0) return NULL;

    Allocator *allocator = (Allocator *)memory;
    allocator->memory = (void *)((uint8_t *)memory + sizeof(Allocator));
    allocator->size = size - sizeof(Allocator);
    allocator->block_size = 32;
    allocator->bitmap = (uint8_t *)allocator->memory;

    size_t bitmap_size = allocator->size / allocator->block_size / 8;
    memset(allocator->bitmap, 0, bitmap_size);
    allocator->memory = (uint8_t *)allocator->bitmap + bitmap_size;

    return allocator;
}

void allocator_destroy(Allocator *const allocator) {
    if (allocator) {
        memset(allocator, 0, allocator->size);
    }
}

void* allocator_alloc(Allocator *const allocator, const size_t size) {
    if (!allocator || size == 0 || size > allocator->size) return NULL;

    size_t blocks_needed = (size + allocator->block_size - 1) / allocator->block_size;
    size_t total_blocks = allocator->size / allocator->block_size;
    uint8_t *bitmap = allocator->bitmap;

    size_t free_blocks = 0;
    for (size_t i = 0; i < total_blocks; ++i) {
        if (!(bitmap[i / 8] & (1 << (i % 8)))) {
            ++free_blocks;

            if (free_blocks == blocks_needed) {
                size_t start_block = i - blocks_needed + 1;
                for (size_t j = start_block; j <= i; ++j) {
                    bitmap[j / 8] |= (1 << (j % 8));
                }

                return (uint8_t *)allocator->memory + start_block * allocator->block_size;
            }
        } else {
            free_blocks = 0;
        }
    }

    return NULL;
}

void allocator_free(Allocator *const allocator, void *const memory) {
    if (!allocator || !memory) return;

    size_t offset = (uint8_t *)memory - (uint8_t *)allocator->memory;
    if (offset % allocator->block_size != 0) return;
    size_t block_index = offset / allocator->block_size;
    allocator->bitmap[block_index / 8] &= ~(1 << (block_index % 8));
}