#include "library.h"

#define MY_MEMORY_SIZE 1024

static Allocator *allocator_create_other(void *const memory, const size_t size) {
	const char mes[] = "We use mmap\n";
	write(STDERR_FILENO, mes, sizeof(mes) - 1);

	void *new_memory = mmap(memory, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	if (new_memory == MAP_FAILED) {
		const char err[] = "mmap failed 1\n";
		write(STDERR_FILENO, err, sizeof(err) - 1);
		return NULL;
	}

	return (Allocator *)new_memory;
}

static void allocator_destroy_other(Allocator *const allocator) {
	const char mes[] = "We use munmap\n";
	write(STDERR_FILENO, mes, sizeof(mes) - 1);

	if (allocator) {
		if (munmap(allocator, MY_MEMORY_SIZE) == -1) {
			const char err[] = "munmap failed 2\n";
			write(STDERR_FILENO, err, sizeof(err) - 1);
		}
	}
}

static void *allocator_alloc_other(Allocator *const allocator, const size_t size) {
	const char mes[] = "We use mmap\n";
	write(STDERR_FILENO, mes, sizeof(mes) - 1);

	void *new_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (new_memory == MAP_FAILED) {
		const char err[] = "mmap failed 3\n";
		write(STDERR_FILENO, err, sizeof(err) - 1);
		return NULL;
	}

	return new_memory;
}

static void allocator_free_other(Allocator *const allocator, void *const memory) {
	const char mes[] = "We use munmap\n";
	write(STDERR_FILENO, mes, sizeof(mes) - 1);

	if (memory && munmap(memory, sizeof(memory)) == -1) {
		const char err[] = "munmap failed 4\n";
		write(STDERR_FILENO, err, sizeof(err) - 1);
	}
}

static allocator_create_f *allocator_create; 
static allocator_destroy_f *allocator_destroy;
static allocator_alloc_f *allocator_alloc;
static allocator_free_f *allocator_free;

int main(int argc, char **argv) {
	if (argc < 2) {
		const char mes[] = "./a.out(main) <library_path>\n";
		write(STDERR_FILENO, mes, sizeof(mes));
		return 1;
	}

	void *library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);

	if (library) {
		allocator_create = dlsym(library, "allocator_create");
		allocator_destroy = dlsym(library, "allocator_destroy");
		allocator_alloc = dlsym(library, "allocator_alloc");
		allocator_free = dlsym(library, "allocator_free");

		if (!allocator_create || !allocator_destroy || !allocator_alloc || !allocator_free) {
			allocator_create = allocator_create_other;
			allocator_destroy = allocator_destroy_other;
			allocator_alloc = allocator_alloc_other;
			allocator_free = allocator_free_other;
		}
	} else {
		const char mes[] = "failed to open custom library\n";
		write(STDERR_FILENO, mes, sizeof(mes));
		return 1;
	}

	size_t size = MY_MEMORY_SIZE;
    void *new_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_addr == MAP_FAILED) {
		dlclose(library);
        char mes[] = "mmap failed\n";
        write(STDERR_FILENO, mes, sizeof(mes) - 1);
        return 1;
    }

	Allocator *allocator = allocator_create(new_addr, MY_MEMORY_SIZE);

	if (!allocator) {
		const char mes[] = "Failed to initialize allocator\n";
		write(STDERR_FILENO, mes, sizeof(mes));
		munmap(new_addr, size);
		dlclose(library);
  
		return 1;
	}

	int *num = (int *)allocator_alloc(allocator, sizeof(int));
	if (num) {
		*num = 36;
		const char mes[] = "Allocated block with int value 36\n";
		write(STDOUT_FILENO, mes, sizeof(mes));
	} else {
		const char mes[] = "Failed to allocate memory\n";
		write(STDERR_FILENO, mes, sizeof(mes));
	}

	if (num) {
		allocator_free(allocator, num);
		const char mes[] = "Free 36\n";
		write(STDOUT_FILENO, mes, sizeof(mes));
	}

	allocator_destroy(allocator);
	const char mes[] = "Allocator destroyed\n";
	write(STDOUT_FILENO, mes, sizeof(mes));

	if (library) dlclose(library);
	munmap(new_addr, size);
	
	return EXIT_SUCCESS;
}