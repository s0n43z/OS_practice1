#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <libgen.h>

#define BUFFER_SIZE 8192
#define NUM_THREADS 3

typedef void (*set_key_func)(char);
typedef void (*caesar_func)(void*, void*, int);

pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
int copied_files_count = 0;
int current_file_index = 0; 

char **files_to_copy;
int total_files = 0;
const char *out_dir;
caesar_func caesar;

void* worker_thread(void* arg) {
    (void)arg;
    
    while (1) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;
        
        int lock_res = pthread_mutex_timedlock(&counter_mutex, &ts);
        if (lock_res == ETIMEDOUT) {
            printf("Возможная взаимоблокировка: поток %lu ожидает мьютекс более 5 секунд\n", pthread_self());
            pthread_exit(NULL); 
        }
        
        if (current_file_index >= total_files) {
            pthread_mutex_unlock(&counter_mutex);
            break; 
        }
        
        int my_file_idx = current_file_index;
        current_file_index++;
        pthread_mutex_unlock(&counter_mutex);
        
        const char *input_path = files_to_copy[my_file_idx];
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        char path_copy[1024];
        strncpy(path_copy, input_path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';
        char *filename = basename(path_copy);
        
        char out_path[2048];
        snprintf(out_path, sizeof(out_path), "%s/%s", out_dir, filename);
        
        int is_success = 0;
        FILE *in = fopen(input_path, "rb");
        if (in) {
            FILE *out = fopen(out_path, "wb");
            if (out) {
                unsigned char buffer[BUFFER_SIZE];
                size_t bytes_read;
                is_success = 1;
                while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, in)) > 0) {
                    caesar(buffer, buffer, bytes_read);
                    fwrite(buffer, 1, bytes_read, out);
                }
                fclose(out);
            }
            fclose(in);
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;
        lock_res = pthread_mutex_timedlock(&counter_mutex, &ts);
        if (lock_res == ETIMEDOUT) {
            printf("Возможная взаимоблокировка: поток %lu ожидает мьютекс более 5 секунд\n", pthread_self());
            pthread_exit(NULL);
        }
        
        if (is_success) {
            copied_files_count++;
        }
        
        FILE *log_file = fopen("log.txt", "a");
        if (log_file) {
            time_t now = time(NULL);
            char *time_str = ctime(&now); // Используем ctime, как рекомендовано в PDF
            time_str[strcspn(time_str, "\n")] = 0; // Удаляем лишний перенос строки
            
            fprintf(log_file, "[%s] Process/Thread: %lu, File: %s, Result: %s, Time: %.4f sec\n",
                    time_str, pthread_self(), filename, is_success ? "Success" : "Error", time_spent);
            fclose(log_file);
        }
        
        pthread_mutex_unlock(&counter_mutex);
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Использование: ./secure_copy file1.txt file2.txt ... output_dir/ key\n");
        return 1;
    }
    
    total_files = argc - 3;
    files_to_copy = (char **)&argv[1];
    out_dir = argv[argc - 2];
    char key = (char)atoi(argv[argc - 1]);
    
    void *handle = dlopen("./libcaesar.so", RTLD_LAZY);
    if (!handle) {
        printf("Ошибка загрузки библиотеки: %s\n", dlerror());
        return 1;
    }
    
    set_key_func set_key = (set_key_func)dlsym(handle, "set_key");
    caesar = (caesar_func)dlsym(handle, "caesar");
    
    if (!set_key || !caesar) {
        printf("Ошибка загрузки символов: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }
    set_key(key);
    
    struct stat st = {0};
    if (stat(out_dir, &st) == -1) {
        mkdir(out_dir, 0700);
    }
    
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    dlclose(handle);
    printf("Счетчик скопированных файлов: %d\n", copied_files_count);
    
    return 0;
}