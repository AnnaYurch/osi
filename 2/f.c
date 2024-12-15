//мьютексы нужны для разграничения доступа потоков к общим ресурсам (нужны если в программе если 
//потоки используют общие ресурсы: общие переменные, файлы...)
//мьютекс - это обьект, который может находить в двух состояниях: заблокированном и разблокированном
//он представлен в си типом pthread_mutex_t
//для перевода мьютекса из одного состояния в другое используются функции:
//  1)pthread_mutex_lock() - из разблокированного в разблокированное 
//  2)pthread_mutex_unlock() - наоборот (после этого другие потоки могут пользоваться мьютексом)
//если мьютекс разлокирован, то поток должен ждать, когда другой поток его разблокирует
//
//Общая схема работы с мьютексами состоит в следующем. Поток, который хочет работать с некоторым 
//общим ресурсом, блокирует мьютекс с помощью метода pthread_mutex_lock(). 
//В это время все остальные потоки ожидают. После завершения работы с общим ресурсом поток, 
//захвативший мьютекс, разблокирует этот мьютекс с помощью метода pthread_mutex_unlock(). 
//После этого мьютекс (и соответственно общий ресурс) свободен, пока его не захватит другой поток.
//
//pthread_mutex_init() нужен для инициализации мьютекса. Первый параметр функции - указатель на мьютекс, 
//а второй - атрибуты (NULL - атрибуты по умолчанию или указатель на объект phread_mutexattr_t, который 
//указывает атрибуты, которые требуется использовать для мьютекса)
//после инициализации мьютекст в разблокированном состоянии
//Пример: pthread_mutex_init(&m, NULL); 
//- возвращает 0, если инициализации успешна
//
//pthread_mutex_destroy - нужен для уничножения мьютекса. 0 в случае успеха или код ошибки
//
//pthread_create - создает новый поток
//пример: int pthread_create(*ptherad_t, const pthread_attr_t *attr, void* (*start_routine)(void*), void *arg);
//  Функция получает в качестве аргументов указатель на поток, переменную типа pthread_t, в которую, 
//  в случае удачного завершения сохраняет id потока. pthread_attr_t – атрибуты потока. 
//  В случае если используются атрибуты по умолчанию, то можно передавать NULL. 
//  start_routin – это непосредственно та функция, которая будет выполняться в новом потоке. 
//  arg – это аргументы, которые будут переданы функции. 
//
//вернет 0 в случае успешного выполнения или:
//    EAGAIN – у системы нет ресурсов для создания нового потока, или система не может больше создавать
//             потоков, так как количество потоков превысило значение PTHREAD_THREADS_MAX
//    EINVAL – неправильные атрибуты потока (переданные аргументом attr)
//    EPERM – Вызывающий поток не имеет должных прав для того, чтобы задать нужные параметры или 
//            политики планировщика.
//
//
//pthread_join - откладывает выполнение вызывающего (эту функцию) потока, до тех пор, 
//               пока не будет выполнен поток thread
//пример: int pthread_join(pthread_t thread, void **value_ptr);
//Когда pthread_join выполнилась успешно, то она возвращает 0. 
//Если поток явно вернул значение (return .. из нашей функции), то оно будет помещено в переменную value_ptr.
//Возможные ошибки, которые возвращает pthread_join:
//    EINVAL – thread указывает на не объединяемый поток
//    ESRCH – не существует потока с таким идентификатором, который хранит переменная thread
//    EDEADLK – был обнаружен дедлок (взаимная блокировка), или же в качестве объединяемого потока 
//              указан сам вызывающий поток.
//  Взаимоблокировка происходит, когда два потока захватывают мьютексы в различном порядке и каждый поток 
//  ждет освобождения мьютекса, захваченного другим потоком.
//
//
//
//Расчет ускорения: Ускорение (Speedup) можно рассчитать как отношение времени выполнения программы с 
//одним потоком к времени выполнения программы с n потоками.
//
//Расчет эффективности: Эффективность (Efficiency) можно рассчитать как отношение ускорения 
//к количеству потоков.
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define MAX_THREADS 4

typedef struct {
    int num_points;
    int inside_circle;
    pthread_mutex_t* mutex; // Указатель на мьютекс
    int* total_inside_circle; // Указатель на общее количество точек внутри окружности
    double radius;
} ThreadData;

void* monte_carlo(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int inside = 0;

    srand(time(NULL)); //Инициализация генератора случайных чисел с использованием текущего времени, 
                       //чтобы каждый запуск программы генерировал разные случайные числа.

    double radius = data->radius;

    for (int i = 0; i < data->num_points; i++) {
        double x = ((double)rand() / RAND_MAX) * 2 * radius - radius;
        double y = ((double)rand() / RAND_MAX) * 2 * radius - radius;
        if (x * x + y * y <= radius * radius) { //если точка попала в окружность
            inside++;
        }
    }

    pthread_mutex_lock(data->mutex);
    *(data->total_inside_circle) += inside; //обновляем количество точек
    pthread_mutex_unlock(data->mutex);

    data->inside_circle = inside; // Сохраняем количество точек внутри окружности для данного потока
    return NULL; //стандартный способ завершить поток, который не возвращает никакого значения
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        char err[] = "Usage: <radius> <num_threads> <num_points>\n";
        write(STDERR_FILENO, err, sizeof(err));
        return 1;
    }

    double radius = atof(argv[1]);
    int num_threads = atoi(argv[2]);
    int num_points = atoi(argv[3]);

    if (radius < 0 || num_threads > MAX_THREADS || num_points <= 0) {
        char err[] = "Invalid INPUT\n";
        write(STDERR_FILENO, err, sizeof(err));
        return 1;
    }

    int points_per_thread = num_points / num_threads;
    int total_inside_circle = 0;

    pthread_t threads[MAX_THREADS]; // Массив потоков
    ThreadData thread_data[MAX_THREADS]; // Массив структур для передачи данных потокам
    pthread_mutex_t mutex; 


    if (pthread_mutex_init(&mutex, NULL) != 0) {
        char err[] = "Problems with init mutex\n";
        write(STDERR_FILENO, err, sizeof(err));
        return 1;
    }

    // Создание потоков
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].num_points = points_per_thread;
        thread_data[i].mutex = &mutex; // Передаем указатель на мьютекс
        thread_data[i].total_inside_circle = &total_inside_circle;
        thread_data[i].radius = radius;
        pthread_create(&threads[i], NULL, monte_carlo, &thread_data[i]);
        //вызывается pthread_create, чтобы создать поток, который будет выполнять функцию monte_carlo
    }

    // ожидание завершения всех потоков блокирует выполнение до тех пор, пока указанный поток не завершится
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL); //блокирует выполнение до тех пор, пока указанный поток не завершится
    }

    pthread_mutex_destroy(&mutex);

    double estimated_pi = (4.0 * total_inside_circle) / num_points;
    //Оценка числа π на основе количества точек внутри круга и общее количество точек
    double area = estimated_pi * radius * radius; // Площадь круга
    
    char* result = (char*)malloc(50 * sizeof(char));
    if (result == NULL) {
        char err[] = "Memory allocation error\n";
        write(STDERR_FILENO, err, strlen(err));
        return 1;
    }

    if (sprintf(result, "Square = %f\n", area) < 0) { //форматирует строку и записывает ее в result
        char err[] = "Overflow\n";                    //возвращает количество символов, записанных в строку
        write(STDERR_FILENO, err, sizeof(err));
        free(result);
        return 1;
    }
    write(STDOUT_FILENO, result, strlen(result));
    free(result);
    return 0;
}
