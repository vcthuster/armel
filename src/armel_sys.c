#include <Armel/armel.h>

#ifdef _WIN32
	#include <windows.h>

	void* arl_sys_alloc (size_t size) {
		void *ptr = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		ARL_ASSERT_FATAL(ptr != NULL, "arl_sys_alloc: VirtualAlloc allocation failed");
		return ptr;
	}

	void arl_sys_free (void *ptr, size_t size) {
		(void)size;
		BOOL ok = VirtualFree(ptr, 0, MEM_RELEASE);
    	ARL_ASSERT_FATAL(ok != 0, "arl_sys_free: VirtualFree failed");
	}
#else 
	#include <sys/mman.h>

	void* arl_sys_alloc (size_t size) {
		void* ptr = mmap(NULL, size, 
			PROT_READ | PROT_WRITE, 
			MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		ARL_ASSERT_FATAL(ptr != MAP_FAILED, "arl_sys_alloc: mmap allocation failed");
		return ptr;
	}

	void arl_sys_free (void *ptr, size_t size) {
		int result = munmap(ptr, size);
		ARL_ASSERT_FATAL(result == 0, "arl_sys_free : Unable to deallocate memory");
	}
#endif