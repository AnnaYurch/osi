#include "library.h"

#define MIN_BLOCK_SIZE 32 

//элемент списка свободных блоков памяти
typedef struct Block { 
    size_t size;
    struct Block *next;
    bool is_free;
} Block;

//Определяет структуру Allocator, которая управляет памятью
typedef struct Allocator {
    Block *free_list; //указатель на первый блок в списке свободных блоков
    void *memory_start; //указатель на начало области памяти, выделенной для аллокатора
    size_t total_size; //общий размер выделенной памяти
} Allocator;

//создает аллокатор, принимая указатель на область памяти и ее размер
Allocator *allocator_create(void *memory, size_t size) { 
    //Проверяет, что указатель на память не равен NULL и что выделенный 
    //размер больше или равен размеру структуры Allocator
    if (!memory || size < sizeof(Allocator)) {
        return NULL;
    }

    Allocator *allocator = (Allocator *)memory;

    //Устанавливает memory_start на адрес, следующий за структурой Allocator в выделенной памяти
    allocator->memory_start = (char *)memory + sizeof(Allocator);
    allocator->total_size = size - sizeof(Allocator);
    allocator->free_list = (Block *)allocator->memory_start;

    //Устанавливает размер первого блока равным доступному размеру памяти за вычетом размера структуры Block
    allocator->free_list->size = allocator->total_size - sizeof(Block);
    allocator->free_list->next = NULL;
    allocator->free_list->is_free = true;

    return allocator;
}

//обнуляет память, занятую аллокатором
void allocator_destroy(Allocator *allocator) {
    if (allocator) {
        memset(allocator, 0, allocator->total_size);
    }
}

//выделяет блок памяти заданного размера
void *allocator_alloc(Allocator *allocator, size_t size) {
    if (!allocator || size == 0) {
        return NULL;
    }

    size = (size + MIN_BLOCK_SIZE - 1) / MIN_BLOCK_SIZE * MIN_BLOCK_SIZE; //50 -> 64
    //Это добавляет MIN_BLOCK_SIZE - 1 к size, чтобы гарантировать, что любое значение, 
    //которое меньше кратного MIN_BLOCK_SIZE, будет округлено вверх.
    //Эта формула используется для округления size до ближайшего большего кратного MIN_BLOCK_SIZE. 
    //Это может помочь улучшить выравнивание данных в памяти и уменьшить фрагментацию, когда свободная 
    //память разбивается на небольшие кусочки, которые не могут быть эффективно использованы.
    Block *best = NULL;
    Block *prev_best = NULL;
    Block *current = allocator->free_list;
    Block *prev = NULL;

    while (current) {
        if (current->is_free && current->size >= size) {
            if (best == NULL || current->size < best->size) {
               best = current;
               prev_best = prev;
            }
        }
        prev = current;
        current = current->next;
    }

    if (best) {
        size_t remain_size = best->size - size;
        if (remain_size >= sizeof(Block) + MIN_BLOCK_SIZE) {
                //Вычисляет адрес нового блока, который будет создан после выделенного блока
                Block *new_block = (Block *)((char *)best + sizeof(Block) + size);
                new_block->size = remain_size - sizeof(Block);
                new_block->is_free = true;
                new_block->next = best->next;

                best->next = new_block;
                best->size = size;
            }

        best->is_free = false;

        //Обновляет список свободных блоков, удаляя best из него
        if (prev_best == NULL) {
            allocator->free_list = best->next;
        } else {
            prev_best->next = best->next;
        }

        //Возвращает указатель на память, начиная с адреса, следующего за заголовком блока best
        return (void *)((char *)best + sizeof(Block));
    }

    return NULL;
}


//освобождает ранее выделенный блок памяти
void allocator_free(Allocator *allocator, void *ptr_to_memory) {
    if (!allocator || !ptr_to_memory) {
        return;
    }

    //Вычисляет адрес заголовка блока
    Block *head = (Block *)((char *)ptr_to_memory - sizeof(Block));
    //if (!head) return;

    //Помещает освобожденный блок в начало списка свободных блоков и помечает его как свободный
    head->next = allocator->free_list;
    head->is_free = true;
    allocator->free_list = head;

    Block *current = allocator->free_list;

    //Начинает цикл по списку свободных блоков, чтобы объединить соседние свободные блоки
    while (current && current->next) {
        //Проверяет, являются ли текущий блок и следующий блок соседними в памяти
        if (((char *)current + sizeof(Block) + current->size) == (char *)current->next) {
            current->size += current->next->size + sizeof(Block);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}