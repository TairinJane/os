#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define URANDOM "/dev/urandom"
#define A 118
#define D 58
#define E 105
#define I 77
#define G 108
#define HEAP_FILE "heap_file"

int file_size = E * 1024; //E * 1024 * 1024
char *heap_area;
ulong chunk_size = A * 1024 * 1024 / D;
FILE *urandom_p;

FILE *heap_file;
pthread_mutex_t mutex;
pthread_cond_t condition;
int can_write = 1;
int file_position = 0;

typedef struct {
    ulong start_i;
} threadData;

void write_to_heap(char *heap, ulong start, int count, FILE *fs) {
    fread(&heap[start], sizeof(char), count, fs);
//    printf("Write Heap[%lu] = %d\n", start, heap[start]);
}

void *thread_write_to_heap(void *thread_data) {
    ulong start = (ulong) thread_data;
//    printf("Thread start %lu\n", data->start_i);
    fread(&heap_area[start], sizeof(char), chunk_size, urandom_p);
    return NULL;
}

void *sum_from_file() {
    FILE *sum_file = fopen(HEAP_FILE, "r");
    if (sum_file != NULL) {
        ulong sum = 0;
        int next_int = 0;
        for (int i = 0; i < file_size / sizeof(int); ++i) {
            fread(&next_int, sizeof(int), 1, sum_file);
            sum += next_int;
        }
        printf("Sum in file = %lu\n", sum);
    } else printf("Error reading %s", HEAP_FILE);
    return NULL;
}

void *write_chunk_to_file() {
    while (file_position < file_size) {
        pthread_mutex_lock(&mutex);
        while (can_write != 1) pthread_cond_wait(&condition, &mutex);
        can_write = 0;
        if (heap_file != NULL) {
            printf("Start chunk write\n");
            fwrite(heap_area, sizeof(char), G, heap_file);
            file_position += G;
            printf("End chunk write = %d\n", file_position);
        } else printf("Error open heap_file");
        can_write = 1;
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    heap_area = (char *) malloc(A * 1024 * 1024);
    ulong ints_in_chunk = chunk_size / sizeof(int);

    printf("Heap Addr: %p\n", heap_area);
    printf("Wanted size: %d\n", A * 1024 * 1024);
    printf("A / D = %d mb in chunk\n", A / D);
    printf("chunk size = %lu bytes\n", chunk_size);
    printf("ints in chunk = %lu\n", ints_in_chunk);

    urandom_p = fopen(URANDOM, "r");

    pthread_t *threads = (pthread_t *) malloc(D * sizeof(pthread_t));

    for (int i = 0; i < D; ++i) {
        pthread_create(&threads[i], NULL, thread_write_to_heap, (void *) (chunk_size * i));
    }

    for (int i = 0; i < D; ++i) {
        pthread_join(threads[i], NULL);
    }

    heap_file = fopen(HEAP_FILE, "w+");

    if (heap_file != NULL) {
        printf("Start file write\n");

        for (int i = 0; i < D; ++i) {
            pthread_create(&threads[i], NULL, write_chunk_to_file,  NULL);
        }

        for (int i = 0; i < D; ++i) {
            pthread_join(threads[i], NULL);
        }

//        fwrite(heap_area, sizeof(char), file_size, heap_file);
        printf("End file write\n");

        pthread_t *threads_sum = (pthread_t *) malloc(I * sizeof(pthread_t));

        for (int i = 0; i < I; ++i) {
            pthread_create(&(threads_sum[i]), NULL, sum_from_file, NULL);
        }

        for (int i = 0; i < I; ++i) {
            pthread_join(threads_sum[i], NULL);
        }

    } else printf("Can't write heap_file");

    free(heap_area);
    free(threads);
    return 0;
}
