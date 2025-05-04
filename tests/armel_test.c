#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <Armel/armel.h>

typedef void (*fn) (void);

#define ARMEL_TEST(name) void name(void)

#define RUN_TEST(test) 									\
	do { 												\
		printf("⚡️ Running %-30s ... ", #test); 		\
		test(); 										\
		printf("PASSED ✅\n"); 							\
	} while (0)


ARMEL_TEST(test_arl_static_alloc) {
	Armel armel;
	void* buffer[1024];

	arl_new_static(&armel, buffer, sizeof(buffer), 8, 0);

	void* ptr1 = arl_alloc(&armel, 12);
	void* ptr2 = arl_alloc(&armel, 24);

	assert(ptr1 != NULL);
	assert(ptr2 != NULL);
	assert((uintptr_t)ptr1 % 8 == 0);
	assert((uintptr_t)ptr2 % 8 == 0);
}


ARMEL_TEST(test_arl_alloc_int) {
	Armel armel;

	arl_new(&armel, 32, 8, ARL_NOFLAG);

	int* i = arl_make(&armel, int);
	int* j = arl_make(&armel, int);

	*i = 10;
	*j = 15;

	assert(*i == 10);
	assert(*j == 15);
	assert(*i != *j);
	assert(i != j);

	arl_free(&armel);
}

ARMEL_TEST(test_arl_free) {
	Armel armel;
	arl_new(&armel, 8, 8, ARL_NOFLAG);

	int *i = arl_make(&armel, int);
	*i = 12;

	assert(*i == 12);
	arl_free(&armel);
	assert(armel.base == NULL);
	assert(armel.cursor == NULL);

	armel.flags = ARL_SOFTFAIL;
	void* x = arl_alloc(&armel, 8);
	assert(x == NULL);
}

ARMEL_TEST(test_arl_cursor) {
	Armel armel;
	size_t size = arl_size(int, 2, ARL_ALIGN);
	arl_new(&armel, size, ARL_ALIGN, ARL_NOFLAG);

	int* a = arl_make(&armel, int);
	int* b = arl_make(&armel, int);

	assert((uintptr_t)a % armel.alignment == 0);
	assert((uintptr_t)b % armel.alignment == 0);

	uintptr_t expected_b = ((uintptr_t)a + sizeof(int) + armel.alignment - 1) & armel.mask;
	assert((uintptr_t)b == expected_b);

	arl_free(&armel);
}

ARMEL_TEST(test_arl_alloc_array) {
	Armel armel;

	arl_new(&armel, 32, 8, ARL_ZEROS);

	int* arr = arl_array(&armel, int, 4);

	for (int i = 0; i < 4; i++) {
		assert(arr[i] == 0); // AVANT affectation, si tu veux tester ARENA_ZEROS
	}

	for (int i = 0; i < 4; i++) {
		arr[i] = i;
	}

	for (int i = 0; i < 4; i++) {
		assert(arr[i] == i);
	}

	arl_free(&armel);
}

ARMEL_TEST(test_arl_used) {
	Armel armel;
	arl_new(&armel, 64, ARL_ALIGN, ARL_NOFLAG);

	uintptr_t before = (uintptr_t)armel.cursor;

	int* i = arl_make(&armel, int);
	*i = 42;

	uintptr_t after = (uintptr_t)armel.cursor;
	size_t expected = sizeof(int);

	assert((uintptr_t)i % armel.alignment == 0);
	assert(after - before == expected);
	assert(arl_used(&armel) == after - (uintptr_t)armel.base);

	arl_free(&armel);
}

ARMEL_TEST(test_arl_remaining) {
	Armel armel;
	size_t total_size = arl_size(int, 3, ARL_ALIGN);

	arl_new(&armel, total_size, ARL_ALIGN, ARL_NOFLAG);

	int *i = arl_make(&armel, int);
	*i = 10;

	uintptr_t aligned_cursor = arl_align_up((uintptr_t)armel.cursor, armel.alignment);
	size_t expected_remaining = (uintptr_t)armel.end - aligned_cursor;

	assert(arl_remaining(&armel) == expected_remaining);
	assert(arl_remaining(&armel) <= total_size);
	assert((uintptr_t)armel.cursor >= (uintptr_t)armel.base);
	assert(arl_used(&armel) + arl_remaining(&armel) <= total_size);

	arl_free(&armel);
}

ARMEL_TEST(test_arl_offset_rewind) {
	Armel armel;
	arl_new(&armel, arl_size(int, 4, ARL_ALIGN), ARL_ALIGN, ARL_NOFLAG);

	int* a = arl_make(&armel, int);
	*a = 42;

	uintptr_t mark = arl_offset(&armel);  // snapshot de la position

	int* b = arl_make(&armel, int);
	*b = 99;

	arl_rewind_to(&armel, mark);  // on revient à la position `mark`

	int* c = arl_make(&armel, int);
	*c = 77;

	assert(*a == 42);                 // l'ancien reste valide
	assert(*c == 77);                 // la nouvelle zone fonctionne
	assert((void*)c == (void*)b);     // même adresse que celle libérée
	assert((uintptr_t)armel.cursor > (uintptr_t)a); // on ne revient pas avant le début

	arl_free(&armel);
}

ARMEL_TEST(test_arl_reset) {
	Armel armel;
	arl_new(&armel, 64, ARL_ALIGN, ARL_NOFLAG);

	int* a = arl_make(&armel, int);
	*a = 42;

	uintptr_t after_a = arl_offset(&armel);

	arl_reset(&armel);

	int* b = arl_make(&armel, int);
	*b = 24;

	assert(*b == 24);
	assert((void*)b == armel.base);
	assert(arl_offset(&armel) + (uintptr_t)armel.base == (uintptr_t)armel.cursor);
	assert(arl_offset(&armel) <= after_a);

	arl_free(&armel);
}

// Test: alignment is respected
ARMEL_TEST (test_arl_alignment) {
    Armel arena;
    arl_new(&arena, 64, 16, ARL_NOFLAG);

    void *ptr = arl_alloc(&arena, sizeof(double));
    assert(((uintptr_t)ptr % 16) == 0);

    arl_free(&arena);
}

// Test: rewind and reallocation
ARMEL_TEST (test_arl_rewind_and_reuse) {
    Armel arena;
    arl_new(&arena, 128, ARL_ALIGN, ARL_NOFLAG);

    uintptr_t mark = arl_offset(&arena);
    int *temp = arl_array(&arena, int, 10);
    for (int i = 0; i < 10; i++) temp[i] = i;

    arl_rewind_to(&arena, mark);
    int *next = arl_make(&arena, int);
    *next = 77;

    assert(*next == 77);
	assert(next == arena.base);
    arl_free(&arena);
}

// Test: ARL_SOFTFAIL returns NULL instead of crashing
ARMEL_TEST (test_arl_softfail) {
    Armel arena;
    arl_new(&arena, 16, ARL_ALIGN, ARL_SOFTFAIL);

    void *p1 = arl_alloc(&arena, 16);
    void *p2 = arl_alloc(&arena, 16);

    assert(p1 != NULL);
    assert(p2 == NULL); // out of memory but no crash

    arl_free(&arena);
}

// Test: ARL_ZEROS fills memory with zero
ARMEL_TEST (test_arl_zeros) {
    Armel arena;
    arl_new(&arena, 64, ARL_ALIGN, ARL_ZEROS);

    int *arr = arl_array(&arena, int, 4);
    for (int i = 0; i < 4; i++) {
        assert(arr[i] == 0);
    }

    arl_free(&arena);
}

ARMEL_TEST(test_arl_print_info) {
    Armel arena;
    arl_new(&arena, 64, ARL_ALIGN, ARL_NOFLAG);

    (void)arl_make(&arena, int);
    (void)arl_array(&arena, int, 3);

    arl_print_info(&arena); // Doit simplement s’exécuter sans crash

    arl_free(&arena);
}

// ⚠️ This test is designed to deliberately fail
// Uncomment to verify ARL_CHECK behavior (will abort the program)
/*
ARMEL_TEST(test_arl_check_fail) {
    Armel arena;
    arena.base = NULL;

    // This should trigger ARL_CHECK and abort due to null base
    void *p = arl_alloc(&arena, 8);
    (void)p;

    // If we reach this line, ARL_CHECK did not abort as expected
    assert(0 && "ARL_CHECK should have aborted the program");
}
*/


// ------------------------------------------------------------------------------------- //
// Pour lancer les tests : cmd + maj + P -> run tasks -> exec armel tests

int main (void) {
	RUN_TEST(test_arl_static_alloc);
	RUN_TEST(test_arl_alloc_int);
	RUN_TEST(test_arl_free);
	RUN_TEST(test_arl_cursor);
	RUN_TEST(test_arl_alloc_array);
	RUN_TEST(test_arl_used);
	RUN_TEST(test_arl_remaining);
	RUN_TEST(test_arl_offset_rewind);
	RUN_TEST(test_arl_reset);
	RUN_TEST(test_arl_alignment);
	RUN_TEST(test_arl_rewind_and_reuse);
	RUN_TEST(test_arl_softfail);
	RUN_TEST(test_arl_zeros);
	RUN_TEST(test_arl_print_info);
	// 

	return 0;
}