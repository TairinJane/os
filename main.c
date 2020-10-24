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

int file_size = E * 1024 * 1024;
pthread_mutex_t mutex;
pthread_cond_t condition;
int isFree = 1;

typedef struct {
    ulong start_i;
    int count;
    int *heap;
    FILE *file_pointer;
} threadData;

void write_to_heap(int *heap, ulong start, int count, FILE *fs) {
    fread(&heap[start], sizeof(int), count, fs);
//    printf("Write Heap[%lu] = %d\n", start, heap[start]);
}

void *thread_write_to_heap(void *thread_data) {
    threadData *data = (threadData *) thread_data;
//    printf("Thread start %lu\n", data->start_i);
    write_to_heap(data->heap, data->start_i, data->count, data->file_pointer);
    return NULL;
}

void *sum_from_file(void *thread_data) {
    pthread_mutex_lock(&mutex);
    while (isFree != 1) pthread_cond_wait(&condition, &mutex);
    isFree = 0;
    FILE *sum_file = fopen(HEAP_FILE, "r");
    if (sum_file != NULL) {
        ulong sum = 0;
        int next_int = 0;
        for (int i = 0; i < file_size; ++i) {
            fread(&next_int, sizeof(int), 1, sum_file);
            sum += next_int;
        }
        printf("Sum in file = %lu\n", sum);
    } else printf("Error reading %s", HEAP_FILE);
    isFree = 1;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void write_to_file(int *heap, int int_count) {
    FILE *heap_file = fopen(HEAP_FILE, "w+");
    if (heap_file != NULL) {
        printf("Start file write\n");
        fwrite(heap, sizeof(int), int_count, heap_file);
        printf("End file write\n");
        rewind(heap_file);
    } else printf("Error open heap_file");
}

int main() {
    int *heap_area = (int *) malloc(A * 1024 * 1024);
    ulong chunk_size = A * 1024 * 1024 / D;
    ulong ints_in_chunk = chunk_size / sizeof(int);

    printf("Heap Addr: %p\n", heap_area);
    printf("Size of int: %lu\n", sizeof(int));
    printf("A / D = %d mb in chunk\n", A / D);
    printf("chunk size = %lu bytes\n", chunk_size);
    printf("ints in chunk = %lu\n", ints_in_chunk);

    FILE *urandom_p = fopen(URANDOM, "r");

    pthread_t *threads = (pthread_t *) malloc(D * sizeof(pthread_t));
    threadData *threadsData = (threadData *) malloc(D * sizeof(threadData));

    for (int i = 0; i < D; ++i) {
        threadsData[i].start_i = ints_in_chunk * i;
        threadsData[i].count = ints_in_chunk;
        threadsData[i].file_pointer = urandom_p;
        threadsData[i].heap = heap_area;
        pthread_create(&threads[i], NULL, thread_write_to_heap, &threadsData[i]);
    }

    for (int i = 0; i < D; ++i) {
        pthread_join(threads[i], NULL);
    }

    FILE *heap_file = fopen(HEAP_FILE, "w+");

    if (heap_file != NULL) {
        printf("Start file write\n");
        fwrite(heap_area, sizeof(int), file_size, heap_file);
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
    free(threadsData);
    return 0;
}
