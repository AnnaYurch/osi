#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define MAX_THREADS 30

typedef struct {
    int num_points;
    int inside_circle;
    pthread_mutex_t* mutex; // Указатель на мьютекс
    int* total_inside_circle; // Указатель на общее количество точек внутри окружности
    double radius;
    unsigned int seed;
} ThreadData;

void* monte_carlo(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int inside = 0;

    double radius = data->radius;

    for (int i = 0; i < data->num_points; i++) {
        double x = ((double)rand_r(&data->seed) / RAND_MAX) * 2 * radius - radius;
        double y = ((double)rand_r(&data->seed) / RAND_MAX) * 2 * radius - radius;
        if (x * x + y * y <= radius * radius) {
            inside++;
        }
    }

    pthread_mutex_lock(data->mutex);
    *(data->total_inside_circle) += inside; //обновляем количество точек
    pthread_mutex_unlock(data->mutex);

    data->inside_circle = inside; // Сохраняем количество точек внутри окружности для данного потока
    return NULL;
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
    ThreadData thread_data[MAX_THREADS];
    pthread_mutex_t mutex; 


    if (pthread_mutex_init(&mutex, NULL) != 0) {
        char err[] = "Problems with init mutex\n";
        write(STDERR_FILENO, err, sizeof(err));
        return 1;
    }

    //clock_t start_time = clock();

    // Создание потоков
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].num_points = points_per_thread;
        thread_data[i].mutex = &mutex;
        thread_data[i].total_inside_circle = &total_inside_circle;
        thread_data[i].radius = radius;
        thread_data[i].seed = time(NULL);
        pthread_create(&threads[i], NULL, monte_carlo, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL); //блокирует выполнение до тех пор, пока указанный поток не завершится
    }

    //clock_t end_time = clock();

    pthread_mutex_destroy(&mutex);

    double estimated_pi = (4.0 * total_inside_circle) / num_points;
    double area = estimated_pi * radius * radius; 
    
    char* result = (char*)malloc(50 * sizeof(char));
    if (result == NULL) {
        char err[] = "Memory allocation error\n";
        write(STDERR_FILENO, err, strlen(err));
        return 1;
    }

    if (sprintf(result, "Square = %f\n", area) < 0) { 
        char err[] = "Overflow\n";                   
        write(STDERR_FILENO, err, sizeof(err));
        free(result);
        return 1;
    }
    write(STDOUT_FILENO, result, strlen(result));
    free(result);

    //double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    //printf("Time taken with %d threads: %f seconds\n", num_threads, time_taken);

    return 0;
}
