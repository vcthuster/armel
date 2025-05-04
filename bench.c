#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Armel/armel.h>

#define N 10000000

double now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

double benchmark_malloc_array() {
    double start = now();

    int* arr = malloc(N * sizeof(int));
    for (size_t i = 0; i < N; i++) {
        arr[i] = (int)i;
    }
    free(arr);

    return now() - start;
}

double benchmark_arl_array() {
    Armel armel;
    size_t size = arl_size(int, N, ARL_ALIGN);
    arl_new(&armel, size, ARL_ALIGN, ARL_NOFLAG);

    double start = now();

    int* arr = arl_array(&armel, int, N);
    for (size_t i = 0; i < N; i++) {
        arr[i] = (int)i;
    }

    arl_free(&armel);
    return now() - start;
}

double benchmark_malloc_many() {
    double start = now();
    int** ptrs = malloc(sizeof(int*) * N);
    for (size_t i = 0; i < N; i++) {
        ptrs[i] = malloc(sizeof(int));
        *ptrs[i] = (int)i;
    }
    for (size_t i = 0; i < N; i++) {
        free(ptrs[i]);
    }
    free(ptrs);
    return now() - start;
}

double benchmark_arl_many() {
    Armel armel;
    size_t size = arl_size(int, N, ARL_ALIGN);
    arl_new(&armel, size, ARL_ALIGN, ARL_NOFLAG);

    double start = now();
    for (size_t i = 0; i < N; i++) {
        int* p = arl_make(&armel, int);
        *p = (int)i;
    }
    arl_free(&armel);
    return now() - start;
}

int main() {
    printf("=== Benchmark (N = %d) ===\n", N);
    printf("malloc/free:    %.6f s\n", benchmark_malloc_array());
    printf("arl_alloc:    %.6f s\n", benchmark_arl_array());
	printf("malloc individual: %.6f s\n", benchmark_malloc_many());
	printf("armel individual:  %.6f s\n", benchmark_arl_many());
    //printf("Speedup:        x%.2f\n", t_malloc / t_armel);

    return 0;
}