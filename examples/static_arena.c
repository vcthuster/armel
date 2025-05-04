#include <Armel/armel.h>
#include <stdio.h>

int main (void) {
    ARL_STATIC(temp, 1024);

    float* values = arl_array(&temp, float, 16);
    for (int i = 0; i < 16; ++i)
        values[i] = i + 0.5f;

    printf("values[10] = %.1f\n", values[10]);

    return 0; // arl_free not needed
}