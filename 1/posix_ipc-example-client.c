#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h> //
#include <sys/stat.h> //
#include <stdbool.h>
#include <errno.h> //
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
        _exit(1);  // можно ли написать что-то такое:
                    //const char msg[] = "error: failed to open requested file\n";
                    //write(STDERR_FILENO, msg, sizeof(msg));
                    //exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    int pipe1 = STDIN_FILENO;   // Константа (0) указывает на stdin 
    int pipe2 = STDOUT_FILENO;

    // Открываем файл для записи составных чисел
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644); //O_WRONLY - открываем для записи,   
    if (file_fd < 0) {                                                 //O_CREAT - создаем, если нет,
        _exit(1);                                                      //O_APPEND - добавляем в конец,
    }                                                                  //0644 - права доступа на файл (r/w - владелец, r)

    int number;
    while (read(pipe1, &number, sizeof(number)) > 0) { //что такое файловый дискриптор? и этот цикл считывает только одно число?
        if (number <= 1 || is_prime(number)) {
            write(pipe2, &number, sizeof(number));
            break;
        } else {
            char buffer[20];
            int len = snprintf(buffer, sizeof(buffer), "%d\n", number); //переделывает num в строку и записывает в buf

            write(file_fd, buffer, len); //записывает len байт из buf

            write(pipe2, &number, sizeof(number));
        }
    }

    close(file_fd);
    _exit(0);
}