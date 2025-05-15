#include <Armel/armel.h>
#include <stdio.h>

int main (void) {
    Armel arena;
    arl_new(&arena, 4 * ARL_KB);

    int* a = arl_make(&arena, int);
    int* b = arl_make(&arena, int);
    *a = 10;
    *b = 42;

    printf("a = %d, b = %d\n", *a, *b);

    arl_free(&arena);
    return 0;
}