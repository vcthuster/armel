/**
 * @file armel.h
 * @brief Fast linear memory allocator (arena/bump allocator) for C projects.
 *
 * This library provides a lightweight, low-overhead memory allocation system
 * based on the arena (a.k.a. bump pointer) strategy.
 *
 * Memory is allocated linearly from a pre-allocated buffer using a cursor.
 * This allows extremely fast allocations, predictable performance, and
 * minimal overhead compared to malloc/free.
 *
 * Key features:
 *   - Fast linear allocation with optional zeroing
 *   - Support for alignment-aware allocation
 *   - Reset and rewind capabilities
 *   - Optional soft-fail mode instead of abort()
 *   - Static and dynamic arena support
 *   - Cross-platform support (macOS, Linux, Windows)
 *
 * Use cases:
 *   - Temporary memory (e.g. per-frame or per-task)
 *   - Memory pools for performance-critical code
 *   - Allocation of short-lived objects or arrays
 *
 * The arena does not track individual allocations: memory is released all at once
 * using arl_reset(), arl_rewind_to(), or arl_free().
 *
 * The API is minimal by design and meant to be embedded directly into your project.
 *
 * Author: Vincent 
 * License: MIT
 */

#ifndef ARMEL_H
#define ARMEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

/**
 * @def ARL_KB
 * @brief Number of bytes in a kilobyte (1 KB = 1024 bytes)
 */
#ifndef ARL_KB
    #define ARL_KB ((size_t)1024)
#endif

/**
 * @def ARL_MB
 * @brief Number of bytes in a megabyte (1 MB = 1024 * 1024 bytes)
 */
#ifndef ARL_MB
    #define ARL_MB (ARL_KB * 1024)
#endif

/**
 * @def ARL_GB
 * @brief Number of bytes in a gigabyte (1 GB = 1024 * 1024 * 1024 bytes)
 */
#ifndef ARL_GB
    #define ARL_GB (ARL_MB * 1024)
#endif

/**
 * @def ARL_ALIGN
 * @brief Default memory alignment used if none is specified.
 *
 * Automatically adapts to the architecture for optimal performance.
 * 16 bytes on ARM64/x86_64, 8 bytes otherwise.
 */
#ifndef ARL_ALIGN
	#if defined(__aarch64__) || defined(__arm64__)
		#define ARL_ALIGN 16
	#elif defined(__x86_64__)
		#define ARL_ALIGN 16
	#else
		#define ARL_ALIGN 8
	#endif
#endif

/**
 * @def ARL_DEFAULT_ALIGNMENT
 * @brief Internal alias to ARL_ALIGN.
 */
#define ARL_DEFAULT_ALIGNMENT ARL_ALIGN

/**
 * @def ARL_NOFLAG
 * @brief No special behavior (default)
 */
#define ARL_NOFLAG 0

/**
 * @def ARL_SOFTFAIL
 * @brief Instead of aborting on allocation failure, return NULL.
 *
 * Useful if you want to handle out-of-memory cases manually.
 */
#define ARL_SOFTFAIL 0x01

/**
 * @def ARL_ZEROS
 * @brief Zero-fill memory during allocation (like calloc).
 *
 * Slightly slower, but ensures memory safety for uninitialized reads.
 */
#define ARL_ZEROS 0x02

/**
 * @brief Computes the total size needed to allocate N items of type T with alignment.
 *
 * This macro is useful to calculate the buffer size required before creating an arena,
 * especially when using arl_static or preallocating memory manually.
 *
 * @param T     Type of the element
 * @param N     Number of elements to allocate
 * @param ALIGN Desired memory alignment (typically ARL_ALIGN)
 *
 * @return Size in bytes to allocate
 *
 * Example:
 *     size_t size = arl_size(MyStruct, 32, ARL_ALIGN);
 */
#define arl_size(T, N, ALIGN) \
	(arl_align_up(sizeof(T), (ALIGN)) * (N))

/**
 * @brief Allocates and returns a pointer to a single instance of type T.
 *
 * This is a shorthand macro to allocate one object of type T from the arena.
 * The returned pointer is cast to (T*).
 *
 * @param A Pointer to the Arena
 * @param T Type of the object to allocate
 * @return Pointer to a newly allocated object of type T
 *
 * Example:
 *     MyStruct* obj = arl_make(&arl, MyStruct);
 */
#define arl_make(A, T) (T*)arl_alloc(A, sizeof(T))

/**
 * @brief Allocates and returns a pointer to an array of N items of type T.
 *
 * This macro is used to allocate a contiguous block for an array of type T
 * with the correct alignment. The returned pointer is cast to (T*).
 *
 * @param A  Pointer to the Arena
 * @param T  Type of the array elements
 * @param N  Number of elements to allocate
 * @return Pointer to the start of the allocated array
 *
 * Example:
 *     int* numbers = arl_array(&arl, int, 64);
 */
#define arl_array(A, T, NB) (T*)arl_alloc(A, sizeof(T)*(size_t)NB)

/**
 * @def ARL_CHECK
 * @brief Internal safety check. Aborts the program with a message if condition fails.
 *
 * Can be disabled by defining `ARL_NO_CHECKS` before including Armel.
 *
 * Example:
 * ```c
 * ARL_CHECK(ptr != NULL, "allocation failed");
 * ```
 */
#ifndef ARL_NO_CHECKS
    #define ARL_CHECK(cond, msg)                                       \
        do {                                                           \
            if (!(cond)) {                                             \
                fprintf(stderr, "Armel error: %s (%s:%d)\n",           \
                        (msg), __FILE__, __LINE__);                    \
                abort();                                               \
            }                                                          \
        } while (0)
#else
    #define ARL_CHECK(cond, msg) ((void)0)
#endif

/**
 * @brief Always-on fatal assertion. Crashes the program if the condition is false.
 *
 * Unlike ARL_CHECK, this macro is never disabled (even with ARL_NO_CHECKS).
 * Used for critical failures (e.g. system allocation errors).
 *
 * @param cond Condition that must be true
 * @param msg  Message to print if the condition fails
 *
 * Example:
 *     ARL_ASSERT_FATAL(ptr != NULL, "VirtualAlloc failed");
 */
#define ARL_ASSERT_FATAL(cond, msg)                                          \
    ((cond) ? (void)0 :                                                      \
     (fprintf(stderr, "Fatal Armel error: %s\n", msg), abort(), (void)0))

/**
 * @struct Armel
 * @brief A linear (bump) memory allocator.
 *
 * This structure holds all state needed to manage a linear memory region
 * allocated with mmap (or platform equivalent). Users allocate memory using
 * arl_alloc() and related macros. Memory is released in bulk via reset or free.
 *
 * Fields:
 *   - base:    Start of the allocated memory buffer (read-only)
 *   - cursor:  Current allocation pointer (moves forward)
 *   - end:     End of the buffer (used for overflow checks)
 *   - alignment: Alignment in bytes (power of 2, typically 8 or 16)
 *   - flags:   Configuration flags (ARL_ZEROS, SOFTFAIL, etc.)
 *
 * Do not modify fields manually unless you know what you're doing.
 */
typedef struct {
    void* base;
    void* cursor;
    void* end;
	size_t alignment;
    size_t mask;
	uint8_t flags;
} Armel;

/**
 * @brief Initializes an arena using a user-provided memory buffer.
 *
 * This function allows you to create an arena without dynamic allocation (e.g., mmap).
 * You must provide a buffer and its size. This is useful for embedding arenas into
 * structures, using stack memory, or avoiding system-level allocations.
 *
 * @param armel     Pointer to the Armel struct to initialize
 * @param buffer    Pointer to the memory block to use
 * @param size      Size of the memory block (in bytes)
 * @param alignment Alignment in bytes (must be a power of 2, e.g. 8 or 16)
 * @param flags     Arena flags (e.g. ARL_ZEROS, ARL_SOFTFAIL)
 *
 * Example:
 *     uint8_t buffer[1024];
 *     Armel armel;
 *     arl_new_static(&armel, buffer, sizeof(buffer), 8, ARL_NOFLAG);
 */
static inline void arl_new_static(Armel *armel, void *buffer, size_t size, size_t alignment, uint8_t flags) {
	armel->base = buffer;
	armel->cursor = buffer;
	armel->end = (uint8_t*)buffer + size;
	armel->alignment = alignment;
	armel->flags = flags;
}

/**
 * @brief Declares and initializes a static arena using a local buffer.
 *
 * This macro creates both a static buffer and a static Arena instance,
 * and calls `arl_new_static()` to initialize it.
 *
 * Useful for quick, stack-free, zero-allocation arenas with fixed capacity.
 * The alignment is set to `ARL_ALIGN`, and flags are set to `ARL_NOFLAG`.
 *
 * @param name Name of the arena (used as variable name and to prefix the buffer)
 * @param size Size in bytes of the arena buffer
 *
 * Example:
 *     ARL_STATIC(temp_arena, 4096);
 *     int* values = arl_array(&temp_arena, int, 128);
 */
#define ARL_STATIC(name, size) 				     		\
	static uint8_t name##_buffer[size]; 				\
	static Armel name; 									\
	arl_new_static(&name, name##_buffer, size, 8, 0)

/**
 * @brief Rounds a size up to the next multiple of the given alignment.
 *
 * Ensures that allocations respect the required alignment.
 * The alignment must be a power of 2.
 *
 * @param size      The original size in bytes
 * @param alignment Desired alignment (e.g. 8, 16)
 * @return Aligned size (≥ size)
 *
 * Example:
 *     arl_align_up(13, 8) => 16
 */
static inline size_t arl_align_up(size_t size, size_t align) {
    size_t a = align - 1;
    return (size + a) & ~a;
}

/**
 * @brief Allocates a memory region of the given size using system-specific calls.
 *
 * On UNIX: uses mmap. 
 * On Windows: uses VirtualAlloc.
 * This function is intended for internal use by arl_new().
 *
 * @param size Number of bytes to allocate (must already be aligned)
 * @return Pointer to the allocated memory (never NULL or MAP_FAILED — use ARL_ASSERT_FATAL)
 */
void* arl_sys_alloc (size_t size);

/**
 * @brief Frees a memory region allocated by arl_sys_alloc.
 *
 * On UNIX: uses munmap. 
 * On Windows: uses VirtualFree.
 * This function is intended for internal use by arl_free().
 *
 * @param ptr  Pointer to the memory block to free
 * @param size Original size of the memory block
 */
void arl_sys_free (void *ptr, size_t size);

/**
 * @brief Create a new Arena with size bytes capacity.
 * @param armel Pointer to an arena to initialise
 * @param size Capacity of the Arena in bytes
 * @param alignment The alignment to be applied, must be a power of 2
 */
void arl_new (Armel* armel, size_t size, size_t alignment, uint8_t flags);

/**
 * @brief Releases the memory used by the arena.
 *
 * Frees the memory allocated by arl_new() via system-level deallocation
 * (e.g., munmap or VirtualFree). Also clears the arena's internal pointers and flags.
 *
 * Do not use the arena after calling this function unless reinitialized.
 *
 * If the arena was initialized via arl_new_static(), calling this is invalid.
 *
 * @param armel Pointer to the arena to free
 *
 * Example:
 *     arl_free(&armel);
 */
void arl_free (Armel *armel);

/**
 * @brief Resets the arena by moving the cursor back to the beginning.
 *
 * This function effectively "frees" all memory allocated so far,
 * without releasing the underlying memory buffer.
 * Use it to reuse the arena for new allocations without any overhead.
 *
 * This is a constant-time operation.
 *
 * @param armel Pointer to the arena to reset
 *
 * Example:
 *     arl_reset(&armel);
 */
void arl_reset (Armel *armel);

/**
 * @brief Request an allocation of size bytes in the arena armel, 
 * with a align bytes alignment. The flags allows the caller to 
 * customize the behavior of this allocation request (see flags above)
 * @param armel The arena where to request memory allocation
 * @param size Size in bytes of the allocation request
 * @return A pointer to the allocated memory
 */
void* arl_alloc (Armel *armel, size_t size);

/**
 * @brief Allocates zero-initialized memory from the arena.
 *
 * This function is equivalent to `arl_alloc`, but it ensures that the allocated
 * memory is cleared (filled with zeroes). It does not require setting the
 * `ARL_ZEROS` flag, and is useful when you only want zeroed memory for some
 * allocations, without affecting the whole arena's behavior.
 *
 * @param armel Pointer to the arena.
 * @param size  Number of bytes to allocate.
 * @return A pointer to a zero-initialized block of memory, or NULL if allocation fails.
 *
 * Example:
 * ```c
 * int* arr = (int*)arl_alloc_zeroed(&arena, 4 * sizeof(int));
 * ```
 */
static inline void* arl_alloc_zeroed (Armel *armel, size_t size) {
    void* ptr = arl_alloc(armel, size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/**
 * @brief Returns the current cursor position in the arena, as an offset in bytes from the base.
 *
 * Use this function to mark a point in the arena's memory, allowing you to rewind to it later
 * using arl_rewind_to(). This is useful for temporary allocations or controlled rollback.
 *
 * @param armel Pointer to the arena.
 * @return Offset (in bytes) from armel->base to armel->cursor.
 *
 * @note This is especially useful for temporary memory scopes, simulated sub-arenas,
 *       or canceling allocations without resetting the entire arena.
 */
static inline uintptr_t arl_offset (Armel *armel) {
	return (uintptr_t)armel->cursor - (uintptr_t)armel->base;
}

/**
 * @brief Rewinds the arena cursor to a previously saved offset.
 *
 * This effectively "frees" all allocations made after the given offset.
 * Typically used in conjunction with arl_offset().
 *
 * @param armel  Pointer to the arena.
 * @param offset Offset (in bytes) from armel->base.
 *
 * @note The offset must not exceed the arena's total size.
 *       If the offset is invalid, this will trigger ARL_CHECK().
 */
static inline void arl_rewind_to (Armel *armel, uintptr_t offset) {
	uintptr_t limit = (uintptr_t)armel->end - (uintptr_t)armel->base;
    ARL_CHECK(offset <= limit, "arl_rewind_to : offset out of bounds");

	armel->cursor = (uint8_t*)armel->base + offset;
}

/**
 * @brief Returns the number of bytes currently used in the arena.
 *
 * This includes any internal padding due to alignment. Useful for
 * tracking memory usage or debugging.
 *
 * @param armel Pointer to the arena
 * @return Number of bytes allocated so far
 */
static inline size_t arl_used (Armel *armel) {
	return (uintptr_t)armel->cursor - (uintptr_t)armel->base;
}

/**
 * @brief Returns the number of bytes available for allocation.
 *
 * This accounts for internal alignment and padding, and gives
 * the maximum number of bytes you can still allocate.
 *
 * @param armel Pointer to the arena
 * @return Number of bytes left
 */
static inline size_t arl_remaining (Armel *armel) {
	return (uintptr_t)armel->end - arl_align_up((uintptr_t)armel->cursor, armel->alignment);
}

/**
 * @brief Prints the internal state of the arena to stdout.
 *
 * Useful for debugging memory usage, capacity, and current offset.
 * Output includes the memory range, used/remaining bytes, alignment, and flags.
 *
 * @param armel Pointer to the arena to inspect
 */
void arl_print_info (Armel *armel);

#endif