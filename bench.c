#include <Armel/armel.h>
#include <Armel/armel_bench.h>

#define N 10000000

uint64_t bench_malloc_array() {
    volatile int sink = 0;

    uint64_t start = arl_now_ns();

    for (size_t i = 0; i < N; i++) {
        int* arr = malloc(sizeof(int) * 200);
        for (size_t j = 0; j < 200; j++) {
            arr[j] = (int)j;
            sink += arr[0];  // empêche la suppression complète de la boucle
        }
        free(arr);
    }

    uint64_t end = arl_now_ns();
    return (end - start) / N;
}

uint64_t bench_arl_array() {
    volatile int sink = 0;

    uint64_t start = arl_now_ns();

    Armel armel;
    arl_new(&armel, ARL_MB);

    for (size_t i = 0; i < N; i++) {
        int* arr = arl_array(&armel, int, 200);
        for (size_t j = 0; j < 200; j++) {
            arr[j] = (int)j;
            sink += arr[0];  // empêche la suppression complète de la boucle
        }
        arl_reset(&armel);
    }

    arl_free(&armel);
    uint64_t end = arl_now_ns();
    return (end - start) / N;
}

int main() {
    printf("=== Benchmark (N = %d) ===\n", N);

    arl_bench_avg("malloc array", bench_malloc_array);
    arl_bench_avg("arl_array", bench_arl_array);

    return 0;
}