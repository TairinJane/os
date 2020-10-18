#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define URANDOM "/dev/urandom"
#define A 118
#define D 58

int E = 105;
int G = 108;
int I = 77;

typedef struct {
    ulong start_i;
    int count;
    int *heap;
    FILE *file_pointer;
} threadData;

void write_to_heap(int *heap, ulong start, int count, FILE *fs) {
    fread(&heap[start], sizeof(int), count, fs);
    printf("Write Heap[%lu] = %d\n", start, heap[start]);
}

void *thread_write_to_heap(void *thread_data) {
    threadData *data = (threadData *) thread_data;
    printf("Thread start %lu\n", data->start_i);
    write_to_heap(data->heap, data->start_i, data->count, data->file_pointer);
    return NULL;
}

int main() {
    int *heap_area = (int *) malloc(A * 1024 * 1024);
    ulong chunk_size = A * 1024 * 1024 / D;
    ulong ints_in_chunk = chunk_size / sizeof(int);
    printf("Addr: %d\n", heap_area);
    printf("Size of int: %lu\n", sizeof(int));
    printf("A / D = %d mb in chunk\n", A / D);
    printf("chunk size = %lu bytes\n", chunk_size);
    printf("ints in chunk = %lu\n", ints_in_chunk);
    FILE *urandom_p = fopen(URANDOM, "r");

    pthread_t *threads = (pthread_t *) malloc(D * sizeof(pthread_t));
    threadData *threadsData = (threadData *) malloc(D * sizeof(threadData));

    for (int i = 0; i < D; ++i) {
        threadsData[i].count = ints_in_chunk;
        threadsData[i].file_pointer = urandom_p;
        threadsData[i].heap = heap_area;
        pthread_create(&(threads[i]), NULL, thread_write_to_heap, &threadsData[i]);
    }

    for (int i = 0; i < D; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("\n\n-- CONTROL --\n\n");
    for (int i = 0; i < D; ++i) {
        printf("Heap[%lu] = %d\n", threadsData[i].start_i, heap_area[threadsData[i].start_i]);
    }

    free(heap_area);
    free(threads);
    free(threadsData);
    return 0;
}
