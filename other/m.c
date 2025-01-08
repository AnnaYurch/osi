#include "library.h"

//Определяет размер пула памяти (MEMORY_POOL_SIZE), который будет использоваться для аллокатора
#define MEMORY_POOL_SIZE 1024

//Определяет функцию-заменитель allocator_create_stub, которая будет использоваться, 
//если оригинальная функция не найдена
static Allocator *allocator_create_stub(void *const memory, const size_t size) {
	const char msg[] = "We use mmap\n";
	write(STDERR_FILENO, msg, sizeof(msg) - 1);

	void *mapped_memory = mmap(memory, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	//PROT_READ | PROT_WRITE: Указывает, что память будет доступна для чтения и записи
	//MAP_PRIVATE: Создает частное отображение, изменения не будут видны другим процессам.
	//Это полезно, когда вы хотите, чтобы изменения не затрагивали другие части программы или другие процессы.
    //MAP_ANONYMOUS: Указывает, что память не связана с файлом, т.е. будет выделена "анонимная" память.
	//Это позволяет выделять память, которая не будет сохраняться на диске.
    //MAP_FIXED: Указывает, что память должна быть размещена по указанному адресу (если memory не NULL). 
	//Это может быть полезно, если вы хотите контролировать, где именно будет выделена память, 
	//но может привести к ошибкам, если указанный адрес уже занят.
	//-1: Указывает, что не используется файл для отображения.
	//0: Смещение в файле (не используется, так как файл не указан)

	//Анонимная память — это память, которая не связана с каким-либо файлом на диске. 
	//Она создается в оперативной памяти и используется для временного хранения данных.
	if (mapped_memory == MAP_FAILED) {
		const char err_msg[] = "mmap failed 1\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
		return NULL;
	}

	return (Allocator *)mapped_memory;
}

static void allocator_destroy_stub(Allocator *const allocator) {
	const char msg[] = "We use munmap\n";
	write(STDERR_FILENO, msg, sizeof(msg) - 1);

	if (allocator) {
		if (munmap(allocator, MEMORY_POOL_SIZE) == -1) {
			//allocator: Указатель на область памяти, которую нужно освободить.
			//MEMORY_POOL_SIZE: Размер области памяти, которую нужно освободить.
			//munmap освобождает ранее выделенную память, и если указатель или размер неверные, возвращает -1
			const char err_msg[] = "munmap failed 2\n";
			write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
		}
	}
}

static void *allocator_alloc_stub(Allocator *const allocator, const size_t size) {
	const char msg[] = "We use mmap\n";
	write(STDERR_FILENO, msg, sizeof(msg) - 1);

	void *mapped_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		//PROT_READ | PROT_WRITE: Указывает, что память будет доступна для чтения и записи
		//MAP_PRIVATE: Создает частное отображение, изменения не будут видны другим процессам.
		//MAP_ANONYMOUS: Указывает, что память не связана с файлом, т.е. будет выделена "анонимная" память.
		//-1: Указывает, что не используется файл для отображения.
		//0: Смещение в файле (не используется, так как файл не указан)
	if (mapped_memory == MAP_FAILED) {
		const char err_msg[] = "mmap failed 3\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
		return NULL;
	}

	return mapped_memory;
}

static void allocator_free_stub(Allocator *const allocator, void *const memory) {
	const char msg[] = "We use munmap\n";
	write(STDERR_FILENO, msg, sizeof(msg) - 1);

	if (memory && munmap(memory, sizeof(memory)) == -1) {
		const char err_msg[] = "munmap failed 4\n";
		write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
	}
}

//Объявляет указатели на функции для работы с аллокатором
static allocator_create_f *allocator_create; 
static allocator_destroy_f *allocator_destroy;
static allocator_alloc_f *allocator_alloc;
static allocator_free_f *allocator_free;
//Ключевое слово static означает, что переменная видима только в пределах файла, в котором она объявлена. 
//Это помогает избежать конфликтов имен и защищает внутренние функции и переменные от доступа из других файлов.

int main(int argc, char **argv) {
	if (argc < 2) {
		const char msg[] = "Usage: ./a.out(main) <library_path>\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		return 1;
	}

	//Загружает библиотеку по указанному пути
	void *library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);
    //RTLD_LOCAL: Указывает, что символы, определенные в загруженной библиотеке, не будут видны другим библиотекам, 
	//загруженным в тот же процесс. Это помогает избежать конфликтов имен.
    //RTLD_NOW: Указывает, что все символы должны быть разрешены сразу при загрузке. 
	//Если какой-либо символ не может быть разрешен, dlopen вернет ошибку.

	if (library) {
		//Получает адреса функций из библиотеки
		allocator_create = dlsym(library, "allocator_create");
		allocator_destroy = dlsym(library, "allocator_destroy");
		allocator_alloc = dlsym(library, "allocator_alloc");
		allocator_free = dlsym(library, "allocator_free");

		if (!allocator_create || !allocator_destroy || !allocator_alloc || !allocator_free) {
			allocator_create = allocator_create_stub;
			allocator_destroy = allocator_destroy_stub;
			allocator_alloc = allocator_alloc_stub;
			allocator_free = allocator_free_stub;
		}
	} else {
		const char msg[] = "error: failed to open custom library\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		return 1;
	}


  	//Teсты библиотеки
	//Выделяет память для аллокатора с помощью mmap
	size_t size = MEMORY_POOL_SIZE;
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
		dlclose(library);
        char message[] = "mmap failed\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        return 1;
    }

	//Создает аллокатор, вызывая функцию allocator_create
	Allocator *allocator = allocator_create(addr, MEMORY_POOL_SIZE);

	if (!allocator) {
		const char msg[] = "Failed to initialize allocator\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		munmap(addr, size);
		dlclose(library);
  
		return 1;
	}

	//Выделяет память для целочисленного блока и устанавливает его значение.
	int *int_block = (int *)allocator_alloc(allocator, sizeof(int));
	if (int_block) {
		*int_block = 36;
		const char msg[] = "Allocated int_block with value 36\n";
		write(STDOUT_FILENO, msg, sizeof(msg));
	} else {
		const char msg[] = "Failed to allocate memory\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	}

	if (int_block) {
		allocator_free(allocator, int_block);
		const char msg[] = "Free 36\n";
		write(STDOUT_FILENO, msg, sizeof(msg));
	}

	allocator_destroy(allocator);
	const char msg[] = "Allocator destroyed\n";
	write(STDOUT_FILENO, msg, sizeof(msg));

	//Закрывает загруженную библиотеку и освобождает память, выделенную для аллокатора
	if (library) dlclose(library);
	munmap(addr, size);
	
	return EXIT_SUCCESS;
}