![CI](https://github.com/vcthuster/armel/actions/workflows/ci.yml/badge.svg)
![Version](https://img.shields.io/badge/version-v1.0.0-yellow)
![Platform Support](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-blue)
![License: zlib](https://img.shields.io/badge/license-zlib-lightgrey)

# Armel – Fast linear memory allocator for C

**Armel** is a tiny, fast and portable arena (bump-pointer) memory allocator for C.  
It provides a simple and predictable alternative to `malloc/free`, optimized for performance-critical code, temporary allocations, and embedded use cases.

---

## ✨ Features

- ⚡ Ultra-fast linear allocations (zero overhead)
- 📦 Release-all-at-once with `reset` or `rewind`
- 🎯 Alignment-aware and platform-tuned
- 🧱 Static or dynamic allocation (mmap/VirtualAlloc or user-provided buffer)
- 🔄 Optional zero-fill or soft-fail behavior
- 🧪 Fully tested, minimal API
- 💻 Works on macOS, Linux, and Windows (x64 / ARM64)

---

## 📦 Getting Started

Just copy the following files into your project:

- `includes/Armel/armel.h`
- `src/armel.c`
- `src/armel_sys.c`

No dependencies. No setup. Drop-in ready.

```c
#include <Armel/armel.h>

Armel armel;
arl_new(&armel, 64 * ARL_KB, ARL_ALIGN, ARL_NOFLAG);

int* a = arl_make(&armel, int);
*a = 42;

arl_reset(&armel);  // reuse all memory
arl_free(&armel);   // release memory (unless static)
```

Need a static armel with no system calls?

```c
ARL_STATIC(temp, 4096);
char* data = arl_array(&temp, char, 128);
```

---

## 🔧 Allocation Helpers

```c
MyStruct *obj = arl_make(&armel, MyStruct);    // allocate one object
float *arr = arl_array(&armel, float, 32);     // allocate array of floats
size_t size = arl_size(MyStruct, 100, 16);     // compute required size
```

---

## 🛠️ Configuration Flags

| Flag             | Description                               |
|------------------|-------------------------------------------|
| `ARL_NOFLAG`     | Default behavior                          |
| `ARL_ZEROS`      | Zero-initialize all allocations           |
| `ARL_SOFTFAIL`   | Return NULL on OOM instead of abort       |

---

## 📐 Alignment and arena size

Use `ARL_ALIGN` for best performance on your platform (e.g. 16 bytes on ARM64, 8 on others).

The library automatically sets this value based on your system architecture  
(e.g. ARM64, x86_64, etc.) for optimal memory access and CPU performance.

You can override it manually **before including `armel.h`** by defining the macro yourself:

```c
#define ARL_ALIGN 32
#include <Armel/armel.h>

The alignment must be a power of 2 (e.g. 8, 16, 32).

You can also use these helpers for defining sizes clearly:

```c
ARL_KB  // 1024 bytes
ARL_MB  // 1024 * 1024 bytes
ARL_GB  // 1024 * 1024 * 1024 bytes

arl_new(&armel, 8 * ARL_KB, ARL_ALIGN, ARL_NOFLAG);

---

## 📤 API Overview

```c
void arl_new(Armel*, size_t size, size_t alignment, uint8_t flags);
void arl_free(Armel*);
void arl_reset(Armel*);

void* arl_alloc(Armel*, size_t size);

uintptr_t arl_offset(Armel*);
void arl_rewind_to(Armel*, uintptr_t offset);

size_t arl_used(Armel*);
size_t arl_remaining(Armel*);
void arl_print_info(Armel*);
```

For static use:
```c
void arl_new_static(Armel*, void* buffer, size_t size, size_t alignment, uint8_t flags);
#define ARL_STATIC(name, size)
```

---

## 🧪 Tests and Benchmarks

This library includes internal unit tests and a benchmark against `malloc/free`.

```bash
# example benchmark output
=== Benchmark (N = 10000000) ===
malloc/free:    0.000000 s
armel_alloc:    0.009111 s
malloc individual: 0.127543 s
armel individual:  0.023865 s
```

---

## ⚠️ Caveats

- Not thread-safe
- Memory is not individually freed (use `reset()` or `rewind_to()`)
- Use responsibly in memory-constrained environments

---

## 📄 License

zlib – do whatever you want, just don't forget to give credit 😉

---

## ✍️ Author

Made with ❤️ by Vincent Huster
