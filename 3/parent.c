#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SHM_NAME "/my_shm"
#define SEM_WRITE_NAME "/sem_write"
#define SEM_READ_NAME "/sem_read"

int main() {
    char filename[256];
    write(STDOUT_FILENO, "Enter the filename to store composite numbers: ", 47);
    int len = read(STDIN_FILENO, filename, sizeof(filename));
    if (len <= 1) {
        const char msg[] = "error: invalid filename\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    filename[len - 1] = '\0';

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    
    ftruncate(shm_fd, sizeof(int) * 256);

    int *shared_memory = mmap(0, sizeof(int) * 256, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_t *sem_write = sem_open(SEM_WRITE_NAME, O_CREAT, 0666, 1);
    sem_t *sem_read = sem_open(SEM_READ_NAME, O_CREAT, 0666, 0);

    pid_t pid = fork();
    
    if (pid == -1) {
        const char msg[] = "error: fork failed\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Дочерний процесс
        execl("./child", "./child", filename, NULL);
        _exit(1);
    } else {
        // Родительский процесс
        int number;
        while (1) {
            write(STDOUT_FILENO, "Enter a number (negative to exit): ", 35);
            char buffer[256];
            int len = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (len <= 1) break;

            buffer[len - 1] = '\0';
            number = atoi(buffer);

            if (number < 0) {
                sem_wait(sem_write);
                shared_memory[0] = number;  
                sem_post(sem_read);
                break;
            }

            sem_wait(sem_write);
            shared_memory[0] = number; 
            sem_post(sem_read);
        }

        wait(NULL);

        munmap(shared_memory, sizeof(int) * 256); 
        shm_unlink(SHM_NAME);
        sem_close(sem_write);
        sem_close(sem_read); 
        sem_unlink(SEM_WRITE_NAME); 
        sem_unlink(SEM_READ_NAME); 

        
        write(STDOUT_FILENO, "Parent process exiting.\n", 24);
    }

    return 0;
}
