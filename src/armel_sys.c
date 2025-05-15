#include <Armel/armel_sys.h>
#include <Armel/armel.h>

#ifdef _WIN32
	#include <windows.h>

	/**
	 * @brief Allocates a block of memory using Windows VirtualAlloc.
	 *
	 * This function reserves and commits a memory region with read/write access.
	 * It aborts the program if the allocation fails.
	 *
	 * @param size The size of memory to allocate in bytes.
	 * @return A pointer to the allocated memory.
	 */
	void* arl_sys_alloc (size_t size) {
		void *ptr = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		ARL_ASSERT_FATAL(ptr != NULL, "arl_sys_alloc: VirtualAlloc allocation failed");
		return ptr;
	}

	/**
	 * @brief Frees a memory block allocated with VirtualAlloc.
	 *
	 * This function releases the memory region allocated previously.
	 * It aborts the program if the deallocation fails.
	 *
	 * @param ptr Pointer to the memory block.
	 * @param size Size of the memory block in bytes (required by Windows API).
	 */
	void arl_sys_free (void *ptr, size_t size) {
		(void)size;
		BOOL ok = VirtualFree(ptr, 0, MEM_RELEASE);
    	ARL_ASSERT_FATAL(ok != 0, "arl_sys_free: VirtualFree failed");
	}


#else 
	#include <sys/mman.h>

	/**
	 * @brief Allocates a block of memory using mmap on POSIX systems.
	 *
	 * This function maps anonymous memory with read/write permissions.
	 * It aborts the program if the allocation fails.
	 *
	 * @param size The size of memory to allocate in bytes.
	 * @return A pointer to the allocated memory.
	 */
	void* arl_sys_alloc (size_t size) {
		void* ptr = mmap(NULL, size, 
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		ARL_ASSERT_FATAL(ptr != MAP_FAILED, "arl_sys_alloc: mmap allocation failed");
		return ptr;
	}

	/**
	 * @brief Frees a memory block allocated with mmap.
	 *
	 * This function unmaps the given memory region.
	 * It aborts the program if the deallocation fails.
	 *
	 * @param ptr Pointer to the memory block.
	 * @param size Size of the memory block in bytes.
	 */
	void arl_sys_free (void *ptr, size_t size) {
		int result = munmap(ptr, size);
		ARL_ASSERT_FATAL(result == 0, "arl_sys_free : Unable to deallocate memory");
	}
#endif