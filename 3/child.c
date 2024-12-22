#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>

#define SHM_NAME "/my_shm"
#define SEM_NAME "/my_sem"
#define BUFFER_SIZE 256

bool is_prime(int num) {
    if (num <= 1) return false;
    for (int i = 2; i * i <= num; i++) {
        if (num % i == 0) return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        const char msg[] = "error: not enough arg\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, BUFFER_SIZE);
    int *shared_memory = mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);

    int number;
    while (1) {
        sem_wait(sem); 

        number = shared_memory[0]; 
        if (number < 0) {
            sem_post(sem);
            break;
        }

        if (is_prime(number) || number == 1 || number == 0) {
            shared_memory[0] = number; 
        } else {
            char buffer[20];
            int len = snprintf(buffer, sizeof(buffer), "%d\n", number);
            int file_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
            write(file_fd, buffer, len);
            close(file_fd);
            shared_memory[0] = number; 
        }

        sem_post(sem); 
    }

    munmap(shared_memory, BUFFER_SIZE);
    shm_unlink(SHM_NAME);
    sem_close(sem);
    sem_unlink(SEM_NAME);
    exit(0);
}
