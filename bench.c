#include <Armel/armel.h>
#include <Armel/armel_bench.h>

#define N 10000000


////////////////////////////////////////////////////////////////////////////////////
///// BENCHMARK ALLOCS, CUSTOM OR REGULAR, WITH & WITHOUT ARL_ZEROS FLAG

uint64_t bench_malloc_zeroed() {
    volatile int sink = 0;
    uint64_t start = arl_now_ns();

    for (size_t i = 0; i < N; i++) {
        int* ptr = malloc(sizeof(int) * 200);
        memset(ptr, 0, sizeof(int) * 200);
        volatile int sum = 0;
        for (size_t j = 0; j < 200; j++) {
            sum += ptr[j];
        }
        sink += sum;
        sink += ptr[0];
        free(ptr);
    }

    uint64_t end = arl_now_ns();
    return (end - start) / N;
}

uint64_t bench_arl_zeros() {
    volatile int sink = 0;
    Armel armel;
    arl_new_custom(&armel, ARL_KB, 16, ARL_ZEROS);

    uint64_t start = arl_now_ns();

    for (size_t i = 0; i < N; i++) {
        int* ptr = arl_array(&armel, int, 200);
        volatile int sum = 0;
        for (size_t j = 0; j < 200; j++) {
            sum += ptr[j]; // force la lecture mémoire
        }
        sink += sum;
        arl_reset(&armel);
    }

    arl_free(&armel);
    uint64_t end = arl_now_ns();
    return (end - start) / N;
}

uint64_t bench_arl_new_custom() {
    uint64_t start = arl_now_ns();

    for (size_t i = 0; i < N; i++) {
        Armel armel;
        arl_new_custom(&armel, ARL_KB, 8, ARL_NOFLAG);
        arl_free(&armel);
    }

    uint64_t end = arl_now_ns();
    return (end - start) / N;
}

uint64_t bench_arl_new() {
    uint64_t start = arl_now_ns();

    for (size_t i = 0; i < N; i++) {
        Armel armel;
        arl_new(&armel, ARL_KB);
        arl_free(&armel);
    }

    uint64_t end = arl_now_ns();
    return (end - start) / N;
}

////////////////////////////////////////////////////////////////////////////////////
///// BENCHMARK SINGLE ALLOC

uint64_t bench_malloc_single() {
    volatile int sink = 0;
    uint64_t start = arl_now_ns();

    for (size_t i = 0; i < N; i++) {
        int* ptr = malloc(sizeof(int));
        *ptr = (int)i;
        sink += *ptr;
        free(ptr);
    }

    uint64_t end = arl_now_ns();
    return (end - start) / N;
}

uint64_t bench_arl_make_single() {
    volatile int sink = 0;
    Armel armel;
    arl_new(&armel, ARL_KB);

    uint64_t start = arl_now_ns();

    for (size_t i = 0; i < N; i++) {
        int* ptr = arl_make(&armel, int);
        *ptr = (int)i;
        sink += *ptr;
        arl_reset(&armel);
    }

    arl_free(&armel);
    uint64_t end = arl_now_ns();
    return (end - start) / N;
}

////////////////////////////////////////////////////////////////////////////////////
///// BENCHMARK ARRAY

uint64_t bench_malloc_array() {
    volatile int sink = 0;

    uint64_t start = arl_now_ns();

    for (size_t i = 0; i < N; i++) {
        int* arr = malloc(sizeof(int) * 200);
        for (size_t j = 0; j < 200; j++) {
            arr[j] = (int)j;
        }

        volatile int sum = 0;
        for (size_t j = 0; j < 200; j++) {
            sum += arr[j];
        }
        sink += sum;
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
        }
        volatile int sum = 0;
        for (size_t j = 0; j < 200; j++) {
            sum += arr[j];
        }
        sink += sum;
        arl_reset(&armel);
    }

    arl_free(&armel);
    uint64_t end = arl_now_ns();
    return (end - start) / N;
}

int main() {
    printf("=== Benchmark (N = %d) ===\n", N);

    arl_bench_avg("malloc + memset", bench_malloc_zeroed);
    sleep(1);
    arl_bench_avg("arl_array (ZEROS)", bench_arl_zeros);
    sleep(1);
    arl_bench_avg("arl_new_custom", bench_arl_new_custom);
    sleep(1);
    arl_bench_avg("arl_new", bench_arl_new);
    sleep(1);

    arl_bench_avg("malloc single", bench_malloc_single);
    sleep(1);
    arl_bench_avg("arl_make", bench_arl_make_single);
    sleep(1);

    arl_bench_avg("malloc array", bench_malloc_array);
    sleep(1);
    arl_bench_avg("arl_array", bench_arl_array);
    sleep(1);

    return 0;
}