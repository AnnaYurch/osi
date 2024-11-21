#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

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

    int pipe1[2], pipe2[2]; //wr
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        const char msg[] = "error with create pipes\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        const char msg[] = "error: fork failed\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE); 
    }

    if (pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);

        dup2(pipe1[0], STDIN_FILENO); // Перенаправляем pipe1 на стандартный ввод
        dup2(pipe2[1], STDOUT_FILENO);

        char *args[] = {"./child", filename, NULL};
        execve("./child", args, NULL);

        _exit(1);
    } else {
        close(pipe1[0]);
        close(pipe2[1]);

        int number;
        char buffer[256];
        while (1) {
            write(STDOUT_FILENO, "Enter a number (negative to exit): ", 35);
            int len = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (len <= 1) break;

            buffer[len - 1] = '\0';
            number = atoi(buffer);


            write(pipe1[1], &number, sizeof(number));


            int child_response;
            if (read(pipe2[0], &child_response, sizeof(child_response)) > 0) {
                if (child_response < 0) {
                    write(STDOUT_FILENO, "Child indicated to terminate\n", 29);
                    break;
                }
            }
            if (number < 0) break;
        }

        close(pipe1[1]);
        close(pipe2[0]);

        int status;
        waitpid(pid, &status, 0);
        write(STDOUT_FILENO, "Parent process exiting.\n", 24);
    }
 
    return 0;
}