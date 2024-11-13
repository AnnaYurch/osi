#include <unistd.h> //fork, pipe, dup2, execve
#include <fcntl.h> //open
#include <sys/types.h> //*waitpid
#include <sys/wait.h>
#include <stdlib.h> //exit, atoi
#include <errno.h> //*
//зачем нам тут использовать fork, pipe, dup2, waitpid я нисего в этом не понимаю обясни подробнее?
//execve - заменяет текущий процесс на новый, если нет ошибок, то не вернет ничего
//fork - создает новый дочерний процесс, для запуска child
//pipe - сосдает канал для межпроцессорного ваимодействия, через который происходит обмен данными
//pipe(pipe1) создает массив из 2 файловых дескрипторов
//dup2 - перенаправляет файловые дискрипторы. Мы перенаправляем ввод и вывод дочернего процесса,
//чтоб он читал данные из pipe1 и писал в pipe2
//waitpid - чтоб родительский процесс не завершился до завершения дочернего
//


//pid_t - тип данных для представления идентификаторов процессов
//файловый дискриптор - целочисленное значение для управления открытыми файлами и другими потоками
//0 - стандартный ввод, 1 - стандартный вывод
//Дочерний процесс - получает копию всех данных родительского и выполняет отдельные задачи параллельнос ним
//нам он нужен для выполнения программы child
int main() {
    char filename[256];
    write(STDOUT_FILENO, "Enter the filename to store composite numbers: ", 47);
    int len = read(STDIN_FILENO, filename, sizeof(filename));
    if (len <= 0) { //когда эта ошибка всплывет?
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

    pid_t pid = fork(); //а зачем нам копия родительского процесса?
    if (pid == -1) {
        const char msg[] = "error: fork failed\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE); 
    }

    if (pid == 0) { //тоесть у нас работает и if и else одновременно?
        //Дочерний процесс какие данные от родительского получает?
        close(pipe1[1]); //для r
        close(pipe2[0]); //для w

        dup2(pipe1[0], STDIN_FILENO);  // Перенаправляем pipe1 на стандартный ввод
        dup2(pipe2[1], STDOUT_FILENO);

        char *args[] = {"./child", filename, NULL}; //зачем передавать эти аргументы, мы же 2 раза передаем ./child?
        execve("./child", args, NULL); //что такое переменные окружения и зачем они?

        _exit(1);
    } else {
        // Родительский процесс
        close(pipe1[0]); //родительский отправляет числа
        close(pipe2[1]); //дочерний отправляет через это число в родительский

        int number;
        char buffer[256];
        while (1) {
            write(STDOUT_FILENO, "Enter a number (negative to exit): ", 35);
            int len = read(STDIN_FILENO, buffer, sizeof(buffer)); //тут считываем, что пишет пользователь?
            if (len <= 0) break;  // Почему именно break? в каком случае он сработает?

            buffer[len - 1] = '\0';
            number = atoi(buffer);

            // Отправляем число дочернему процессу
            write(pipe1[1], &number, sizeof(number)); //куда что и сколько, пишем число в в канал pipe, 
                                                    //чтоб передать его дочернему процессу 

            // Ожидаем ответ от дочернего процесса
            int child_response;
            if (read(pipe2[0], &child_response, sizeof(child_response)) > 0) { //считывает данные из pipe2
                if (child_response < 0) { //что такое child_response и что оно содержит?
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