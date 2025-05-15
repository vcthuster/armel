#include <Armel/armel.h>
#include <stdio.h>

int main (void) {
    Armel arena;
    arl_new(&arena, 8 * ARL_KB);

    uintptr_t mark = arl_offset(&arena);

    int* temp = arl_array(&arena, int, 5);
    for (int i = 0; i < 5; ++i) temp[i] = i * 2;

    printf("temp[2] = %d\n", temp[2]);

    arl_rewind_to(&arena, mark); // temp now invalid, nothing has happened

    arl_free(&arena);
    return 0;
}