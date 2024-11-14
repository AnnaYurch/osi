#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

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
    int pipe1 = STDIN_FILENO;   // Константа (0) указывает на stdin 
    int pipe2 = STDOUT_FILENO;

    // Открываем файл для записи составных чисел
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);  //O_WRONLY - открываем для записи,   
    if (file_fd < 0) {                                                  //O_CREAT - создаем, если нет,
        const char msg[] = "error with open requested file\n";          //O_APPEND - добавляем в конец,
        write(STDERR_FILENO, msg, sizeof(msg));            //0644 - права доступа на файл (r/w - владелец, r)
        exit(EXIT_FAILURE);                                            
    }                                               

    int number;
    while (1) {
        if (read(pipe1, &number, sizeof(number)) <= 0) break;
        
        if (number < 0) {
            write(pipe2, &number, sizeof(number));
            break;
        }

        if (is_prime(number) || number == 1 || number == 0) {
            write(pipe2, &number, sizeof(number));
        } else {
            char buffer[20];
            int len = snprintf(buffer, sizeof(buffer), "%d\n", number);
            //переделывает num в строку и записывает в buf

            write(file_fd, buffer, len); //записывает len байт из buf
            write(pipe2, &number, sizeof(number));
        }
    }

    close(file_fd);
    exit(0);
}