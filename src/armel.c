#include <Armel/armel.h>

void arl_new (Armel* armel, size_t size, size_t alignment, uint8_t flags) {
	ARL_ASSERT_FATAL(alignment != 0, "Invalid alignment value");

	size_t padded_size = arl_align_up(size, alignment);
    void* ptr = arl_sys_alloc(padded_size);

    armel->base = ptr;
    armel->cursor = ptr;
    armel->end = (char*)ptr + padded_size;
	armel->alignment = alignment;
	armel->mask = ~(alignment - 1);
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


void arl_reset (Armel *armel) {
	if (armel->flags & ARL_ZEROS) {
		memset(armel->base, 0, (uintptr_t)armel->end - (uintptr_t)armel->base);
	}

    armel->cursor = armel->base;
}


void* arl_alloc (Armel *armel, size_t size) {
	if (armel->base == NULL) {
		if (armel->flags & ARL_SOFTFAIL) {
			return NULL;
		}
		ARL_ASSERT_FATAL(0, "use after arl_free");
	}

	uintptr_t cursor = (uintptr_t)armel->cursor;

	// Align the cursor only if needed
	uintptr_t start = (cursor % armel->alignment == 0)
		? cursor
		: (cursor + armel->alignment - 1) & armel->mask;

	uintptr_t stop = start + size;
	uintptr_t end  = (uintptr_t)armel->end;
	void* ptr = (void*)start;

	if (stop > end) {
		if (armel->flags & ARL_SOFTFAIL) {
			return NULL;
		}

		fprintf(stderr, "Armel arena error: out of memory. requesting %zu, remaining %zu\n", 
			size, (uintptr_t)armel->end - start);
		abort();
	}

	if (armel->flags & ARL_ZEROS) {
		memset(ptr, 0, size);
	}

	armel->cursor = (void*)stop;
    return ptr;
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
