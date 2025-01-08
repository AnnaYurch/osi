#include "library.h"

#define MIN_BLOCK_SIZE 32

typedef struct Block {
    size_t size; //размер блока
    struct Block *next; //указатель на следующий блок
    bool is_free; 
} Block;

typedef struct Allocator {
    Block *free_list_start_from; //указатель на начало списка свободных блоков
    void *memory_start_from; //указатель на начало выделенной памяти
    size_t general_size; //общий размер выделенной памяти
} Allocator;

Allocator *allocator_create(void *memory, size_t size) {
    if (!memory || size < sizeof(Allocator)) {
        return NULL;
    }

    Allocator *allocator = (Allocator *)memory;

    allocator->memory_start_from = (char *)memory + sizeof(Allocator);
    allocator->general_size = size - sizeof(Allocator);
    allocator->free_list_start_from = (Block *)allocator->memory_start_from;

    allocator->free_list_start_from->size = allocator->general_size - sizeof(Block);
    allocator->free_list_start_from->next = NULL;
    allocator->free_list_start_from->is_free = true;

    return allocator;
}

void allocator_destroy(Allocator *allocator) {
    if (allocator) {
        memset(allocator, 0, allocator->general_size);
    }
}

void *allocator_alloc(Allocator *allocator, size_t size) {
    if (!allocator || size == 0) {
        return NULL;
    }

    size = (size + MIN_BLOCK_SIZE - 1) / MIN_BLOCK_SIZE * MIN_BLOCK_SIZE;
    Block *current = allocator->free_list_start_from;

    while (current) {
        if (current->is_free && current->size >= size) {
            // Если оставшийся размер блока достаточно для нового блока
            if (current->size >= size + sizeof(Block) + MIN_BLOCK_SIZE) {
                //Создание нового указателя new_block, который указывает на начало нового свободного блока, 
                //который будет создан после выделенного блока. 
                Block *new_block = (Block *)((char *)current + sizeof(Block) + size);
                new_block->size = current->size - size - sizeof(Block);
                new_block->is_free = true;
                new_block->next = current->next;

                current->next = new_block;
                current->size = size;
            }

            current->is_free = false;
            if (current == allocator->free_list_start_from) {
                allocator->free_list_start_from = current->next;
            } else {
                Block *prev = allocator->free_list_start_from;
                while (prev && prev->next != current) {
                    prev = prev->next;
                }
                if (prev) {
                    prev->next = current->next;
                }
            }

            return (void *)((char *)current + sizeof(Block));
        }
        current = current->next;
    }

    return NULL;
}

void allocator_free(Allocator *allocator, void *ptr_to_memory) {
    if (!allocator || !ptr_to_memory) {
        return;
    }

    Block *block_to_free = (Block *)((char *)ptr_to_memory - sizeof(Block));
    block_to_free->is_free = true;

    // Объединение с соседними свободными блоками
    Block *current = allocator->free_list_start_from;

    if (block_to_free < current) {
        block_to_free->next = current;
        allocator->free_list_start_from = block_to_free;
        current = block_to_free;
    } else {
        while (current && current->next && current->next < block_to_free) {
            current = current->next;
        }
        block_to_free->next = current->next;
        current->next = block_to_free;
    }

    // Объединение с предыдущим блоком
    if (current && current->is_free) {
        current->size += block_to_free->size + sizeof(Block);
        current->next = block_to_free->next;
        block_to_free = current;
    }
    //Проверяем, свободен ли предыдущий блок (т.е. current):
    //Если да, то объединяем его с освобождаемым блоком, увеличивая размер предыдущего блока на размер 
    //освобождаемого блока и размер заголовка.
    //Обновляем указатель next предыдущего блока, чтобы он указывал на следующий блок после освобождаемого.
    //Обновляем block_to_free, чтобы он указывал на объединенный блок.


    // Объединение с следующим блоком
    if (block_to_free->next && block_to_free->next->is_free) {
        block_to_free->size += block_to_free->next->size + sizeof(Block);
        block_to_free->next = block_to_free->next->next;
    }
    //Проверяем, свободен ли следующий блок (т.е. block_to_free->next):
    //Если да, то объединяем текущий блок с следующим, увеличивая его размер на размер следующего блока и 
    //размер заголовка.
    //Обновляем указатель next текущего блока, чтобы он указывал на следующий блок после следующего.
}
