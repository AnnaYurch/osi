#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define SHM_NAME "/my_shm"
#define SEM_NAME "/my_sem"
#define BUFFER_SIZE 256

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
    ftruncate(shm_fd, BUFFER_SIZE);
    int *shared_memory = mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);

    pid_t pid = fork();
    if (pid == -1) {
        const char msg[] = "error: fork failed\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        char *args[] = {"./child", filename, NULL};
        execve("./child", args, NULL);
        _exit(1);
    } else {
        int number;
        char buffer[256];
        while (1) {
            write(STDOUT_FILENO, "Enter a number (negative to exit): ", 35);
            int len = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (len <= 1) break;

            buffer[len - 1] = '\0';
            number = atoi(buffer);

            sem_wait(sem);
            shared_memory[0] = number; 
            sem_post(sem); 

            if (number < 0) break;

            sem_wait(sem); 
            int child_response = shared_memory[0]; 
            sem_post(sem);

            if (child_response < 0) {
                write(STDOUT_FILENO, "Child indicated to terminate\n", 29);
                break;
            }
        }

        sem_wait(sem); 
        shared_memory[0] = -1; 
        sem_post(sem); 

        munmap(shared_memory, BUFFER_SIZE);
        shm_unlink(SHM_NAME);
        sem_close(sem);
        sem_unlink(SEM_NAME);
        int status;
        waitpid(pid, &status, 0);
        write(STDOUT_FILENO, "Parent process exiting.\n", 24);
    }

    return 0;
}
