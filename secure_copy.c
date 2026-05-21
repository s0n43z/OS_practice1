#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <dlfcn.h>
#include <unistd.h>

#define BUFFER_SIZE 8192

volatile int keep_running = 1;

void handle_sigint(int sig) {
    (void)sig; 
    keep_running = 0;
}

typedef void (*set_key_func)(char);
typedef void (*caesar_func)(void*, void*, int);

typedef struct {
    unsigned char buffer[BUFFER_SIZE];
    size_t bytes_in_buffer;
    int eof_reached;
    int data_ready;
    FILE *in_file;
    FILE *out_file;
    
    pthread_mutex_t mutex;
    pthread_cond_t cond_prod;
    pthread_cond_t cond_cons;
    
    caesar_func caesar;
} shared_data_t;

void* producer_thread(void* arg) {
    shared_data_t *data = (shared_data_t*)arg;

    while (keep_running && !data->eof_reached) {
        pthread_mutex_lock(&data->mutex);

        while (data->data_ready && keep_running) {
            pthread_cond_wait(&data->cond_prod, &data->mutex);
        }

        if (!keep_running) {
            pthread_mutex_unlock(&data->mutex);
            break;
        }

        size_t bytes_read = fread(data->buffer, 1, BUFFER_SIZE, data->in_file);

        if (bytes_read < BUFFER_SIZE) {
            if (feof(data->in_file) || ferror(data->in_file)) {
                data->eof_reached = 1;
            }
        }

        if (bytes_read > 0) {
            data->caesar(data->buffer, data->buffer, bytes_read);
            
            data->bytes_in_buffer = bytes_read;
            data->data_ready = 1;
        }

        pthread_cond_signal(&data->cond_cons);
        pthread_mutex_unlock(&data->mutex);
    }
    return NULL;
}

void* consumer_thread(void* arg) {
    shared_data_t *data = (shared_data_t*)arg;

    while (keep_running) {
        pthread_mutex_lock(&data->mutex);

        while (!data->data_ready && keep_running && !data->eof_reached) {
            pthread_cond_wait(&data->cond_cons, &data->mutex);
        }

        if (!keep_running || (!data->data_ready && data->eof_reached)) {
            pthread_mutex_unlock(&data->mutex);
            break;
        }

        fwrite(data->buffer, 1, data->bytes_in_buffer, data->out_file);
        data->data_ready = 0;

        pthread_cond_signal(&data->cond_prod);
        pthread_mutex_unlock(&data->mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <input_file> <output_file> <key>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];
    char key = (char)atoi(argv[3]);

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    void *handle = dlopen("./libcaesar.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Ошибка загрузки библиотеки: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    set_key_func set_key = (set_key_func)dlsym(handle, "set_key");
    caesar_func caesar = (caesar_func)dlsym(handle, "caesar");

    if (!set_key || !caesar) {
        fprintf(stderr, "Ошибка загрузки символов: %s\n", dlerror());
        dlclose(handle);
        return EXIT_FAILURE;
    }

    set_key(key);

    FILE *in = fopen(input_path, "rb");
    if (!in) {
        perror("Ошибка открытия входного файла");
        dlclose(handle);
        return EXIT_FAILURE;
    }

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        perror("Ошибка открытия выходного файла");
        fclose(in);
        dlclose(handle);
        return EXIT_FAILURE;
    }

    shared_data_t data = {0};
    data.in_file = in;
    data.out_file = out;
    data.caesar = caesar;
    
    pthread_mutex_init(&data.mutex, NULL);
    pthread_cond_init(&data.cond_prod, NULL);
    pthread_cond_init(&data.cond_cons, NULL);

    pthread_t prod_tid, cons_tid;
    pthread_create(&prod_tid, NULL, producer_thread, &data);
    pthread_create(&cons_tid, NULL, consumer_thread, &data);

    pthread_join(prod_tid, NULL);
    pthread_join(cons_tid, NULL);

    pthread_mutex_destroy(&data.mutex);
    pthread_cond_destroy(&data.cond_prod);
    pthread_cond_destroy(&data.cond_cons);

    fclose(in);
    fclose(out);
    dlclose(handle);

    if (!keep_running) {
        printf("Операция прервана пользователем\n");
    } else {
        printf("Копирование и шифрование успешно завершено.\n");
    }

    return EXIT_SUCCESS;
}