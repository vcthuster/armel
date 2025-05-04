#include <stdio.h>
#include <stdlib.h>
#include <Armel/armel.h>

#define N 10000000

void bench_alloc_plain() {
    Armel arena;
    arl_new(&arena, arl_size(int, N, ARL_ALIGN), ARL_ALIGN, ARL_NOFLAG);
    for (size_t i = 0; i < N; i++) {
        int* p = arl_make(&arena, int);
        *p = (int)i;
    }
    arl_free(&arena);
}

void bench_alloc_zeroed() {
    Armel arena;
    arl_new(&arena, arl_size(int, N, ARL_ALIGN), ARL_ALIGN, ARL_ZEROS);
    for (size_t i = 0; i < N; i++) {
        int* p = arl_make(&arena, int);
        *p = (int)i;
    }
    arl_free(&arena);
}

void bench_reset() {
    Armel arena;
    arl_new(&arena, arl_size(int, N, ARL_ALIGN), ARL_ALIGN, ARL_NOFLAG);
    for (int cycle = 0; cycle < 100; cycle++) {
        for (size_t i = 0; i < 100000; i++) {
            int* p = arl_make(&arena, int);
            *p = (int)i;
        }
        arl_reset(&arena);
    }
    arl_free(&arena);
}

void bench_rewind() {
    Armel arena;
    arl_new(&arena, arl_size(int, N, ARL_ALIGN), ARL_ALIGN, ARL_NOFLAG);
    for (size_t i = 0; i < N; i += 2) {
        uintptr_t offset = arl_offset(&arena);
        int* a = arl_make(&arena, int);
        int* b = arl_make(&arena, int);
        *a = (int)i;
        *b = (int)i + 1;
        arl_rewind_to(&arena, offset);
    }
    arl_free(&arena);
}

int main(void) {
    printf("Running Armel benchmarks...\n");
    bench_alloc_plain();       // 🔍 Focus sur arl_alloc sans memset
    bench_alloc_zeroed();      // 🔍 Focus sur arl_alloc + memset
    bench_reset();             // 🔁 Test arl_reset
    bench_rewind();            // ↩️ Test arl_offset + rewind_to
    return 0;
}