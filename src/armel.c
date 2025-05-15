#include <stdint.h>
#include <stddef.h>

#include <Armel/armel_sys.h>
#include <Armel/armel.h>

void arl_new_custom (Armel* armel, size_t size, size_t alignment, uint8_t flags) {
	if ((alignment == 0) || (alignment & (alignment - 1)) != 0) {
		ARL_FATAL("Armel arena error : Alignment must be a power of 2 and non-zero.");
		return;
	}

	size_t padded_size = arl_align_up(size, alignment);
    void* ptr = arl_sys_alloc(padded_size);

    armel->base = ptr;
    armel->cursor = ptr;
    armel->end = (char*)ptr + padded_size;
	armel->alignment = alignment;
	armel->mask = alignment - 1;
	armel->flags = flags;

	if (flags & ARL_ZEROS) {
		memset(ptr, 0, padded_size);
	}
}


void arl_free (Armel *armel) {
	size_t size = (uintptr_t)armel->end - (uintptr_t)armel->base;

	arl_sys_free(armel->base, size);

	armel->base = NULL;
	armel->cursor = NULL;
	armel->end  = NULL;
	armel->flags = 0;
	armel->alignment = 0;
}


void arl_print_info (Armel *armel) {
	printf("[Arena]\n");
	printf("  base      = %p\n", armel->base);
	printf("  cursor    = %p\n", armel->cursor);
	printf("  end       = %p\n", armel->end);
	printf("  used      = %zu bytes\n", arl_used(armel));
	printf("  remaining = %zu bytes\n", arl_remaining(armel));
	printf("  alignment = %zu\n", (size_t)armel->alignment);
	printf("  flags     = 0x%02X", armel->flags);

	if (armel->flags) {
		printf(" (");
		if (armel->flags & ARL_SOFTFAIL) printf("ARL_SOFTFAIL ");
		if (armel->flags & ARL_ZEROS) printf("ARL_ZEROS ");
		printf(")");
	}
	printf("\n\n");
}
