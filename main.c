#include <stdio.h>
#include <Armel/armel.h>

// cmd + maj + B pour compiler 

int main() {

	ARL_STATIC(armel, ARL_KB);

	int *i = arl_make(&armel, int);

	arl_print_info(&armel);

	printf("%zu \n", (uintptr_t)armel.base % ARL_ALIGN);
	printf("%zu \n", (uintptr_t)armel.cursor % ARL_ALIGN);
	printf("%zu \n", (uintptr_t)i % ARL_ALIGN);

	return 0;
}
