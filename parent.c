#include <unistd.h> //fork, pipe, dup2, execve
#include <fcntl.h> //open
#include <sys/types.h> //*waitpid
#include <sys/wait.h>
#include <stdlib.h> //exit, atoi
#include <errno.h> //*
//зачем нам тут использовать fork, pipe, dup2, waitpid я нисего в этом не понимаю обясни подробнее
//execve - заменяет текущий процесс на новый, если нет ошибок, то не вернет ничего

//файловый дискриптор - целочисленное значение для управления открытыми файлами и другими потоками
int main() {
    char filename[256];
    write(STDOUT_FILENO, "Enter the filename to store composite numbers: ", 47);
    int len = read(STDIN_FILENO, filename, sizeof(filename));
    if (len <= 0) _exit(1);  // _exit можно изменить на exit?

    filename[len - 1] = '\0';

    int pipe1[2], pipe2[2]; //wr
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        _exit(1);
    }

    //тип данных для представления идентификаторов процессов
    pid_t pid = fork(); //зачем fork и что он выдает
    if (pid == -1) {
        _exit(1); 
    }

    if (pid == 0) { //зачем этот if?
        //Дочерний процесс что за процесс?
        close(pipe1[1]); //для r
        close(pipe2[0]); //для w

        dup2(pipe1[0], STDIN_FILENO);  // Перенаправляем pipe1 на стандартный ввод
        dup2(pipe2[1], STDOUT_FILENO);

        char *args[] = {"./child", filename, NULL}; //зачем передавать эти аргументы?
        execve("./child", args, NULL); //что такое переменные окружения?

        _exit(1);
    } else {
        // Родительский процесс
        close(pipe1[0]); //а что мы собираемся писать в pipe?
        close(pipe2[1]); //что мыбудем считывать из pipe?

        int number;
        char buffer[256];
        while (1) {
            write(STDOUT_FILENO, "Enter a number (negative to exit): ", 35);
            int len = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (len <= 0) break;  // Почему именно break?

            buffer[len - 1] = '\0';
            number = atoi(buffer);

            // Отправляем число дочернему процессу
            write(pipe1[1], &number, sizeof(number)); //какие аргументы? и куда пишем?

            // Ожидаем ответ от дочернего процесса
            int child_response;
            if (read(pipe2[0], &child_response, sizeof(child_response)) > 0) { //что получает read?
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
        waitpid(pid, &status, 0); //Ожидание завершения дочернего процесса? А почему дочерний процесс не завершился раньше
        write(STDOUT_FILENO, "Parent process exiting.\n", 24);
    }

    return 0;
}