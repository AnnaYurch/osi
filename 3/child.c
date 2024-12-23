#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <semaphore.h>

#define SHM_NAME "/my_shm" 
#define SEM_WRITE_NAME "/sem_write"
#define SEM_READ_NAME "/sem_read"

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

    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);

    int *shared_memory = mmap(0, sizeof(int) * 256, PROT_READ, MAP_SHARED, shm_fd, 0);

    sem_t *sem_write = sem_open(SEM_WRITE_NAME, 0);
    sem_t *sem_read = sem_open(SEM_READ_NAME, 0);

    int file_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644); 
    if (file_fd < 0) {                                                 
        const char msg[] = "error with open requested file\n";         
        write(STDERR_FILENO, msg, sizeof(msg));                   
        exit(EXIT_FAILURE);
    }

    int number;
    while (1) {
        sem_wait(sem_read); 
        number = shared_memory[0]; 

        if (number < 0) {
            write(STDOUT_FILENO, "Child indicated to terminate\n", 29);
            break;
        }

        if (!(is_prime(number) || number == 1 || number == 0)) {
            char buffer[20];
            int len = snprintf(buffer, sizeof(buffer), "%d\n", number);
            write(file_fd, buffer, len);
        }

        sem_post(sem_write);
    }

    close(file_fd); 
    munmap(shared_memory, sizeof(int) * 256); 
    shm_unlink(SHM_NAME); 
    sem_close(sem_write); 
    sem_close(sem_read); 
    
    return 0;
}
