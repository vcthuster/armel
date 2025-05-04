#include <stdio.h>
#include <Armel/armel.h>

// cmd + maj + B pour compiler 

int main() {

	Armel armel;

	arl_new(&armel, 1024, 8, 0);

	arl_free(&armel);
	return 0;
}
