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
- 📦 Release-all-at-once with `reset` or `rewind_to`
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
arl_new(&armel, 64 * ARL_KB);
// or : 
// arl_new_custom(&armel, 64 * ARL_KB, ARL_ALIGN, ARL_NOFLAG);
// for customization

int* a = arl_make(&armel, int);
*a = 42;

arl_reset(&armel);  // reuse all memory
arl_free(&armel);   // release memory (unless static)
```

Need a static arena with no system calls?

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
```

The alignment must be a power of 2 (e.g. 8, 16, 32).

You can also use these helpers for defining sizes clearly:

```c
ARL_KB  // 1024 bytes
ARL_MB  // 1024 * 1024 bytes
ARL_GB  // 1024 * 1024 * 1024 bytes

arl_new(&armel, 8 * ARL_KB);
```

---

## 📤 API Overview

```c
void arl_new(Armel*, size_t size);
void arl_new_custom(Armel*, size_t size, size_t alignment, uint8_t flags);
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
// or just :
ARL_STATIC(name, size);
```

---

## 🧪 Tests and Benchmarks

### ✅ Coverage Report

The Armel library is thoroughly tested with high coverage on all core components.
All tests are located in [`tests/armel_test.c`](tests/armel_test.c) and are executed via:

```bash
make tests
```

Coverage is measured using Clang’s --coverage instrumentation and llvm-cov, with support for expect_abort() to test abort() paths.
Each test run is followed by a coverage report generated with:

```
make coverage
````

> 📌 All benchmarks were compiled with `-O2` and run on an **Apple M4 (ARM64)**.  
> Benchmarks were designed to reflect **real-world usage patterns** with allocation, zeroing, and reuse loops.

Here is the current coverage summary:

| Filename                      | Function Coverage     | Line Coverage         | Region Coverage        | Branch Coverage         |
|------------------------------|------------------------|------------------------|-------------------------|--------------------------|
| includes/Armel/armel.h       | 100.00% (10/10)        | 79.01% (64/81)         | 76.79% (43/56)          | 68.75% (11/16)           |
| includes/Armel/armel_test.h  | 100.00% (1/1)          | 55.56% (10/18)         | 77.78% (14/18)          | 50.00% (4/8)             |
| src/armel.c                  | 100.00% (3/3)          | 80.49% (33/41)         | 53.33% (16/30)          | 38.46% (5/13)            |
| src/armel_sys.c              | 100.00% (2/2)          | 100.00% (10/10)        | 71.43% (15/21)          | 50.00% (2/4)             |
| tests/armel_test.c           | 87.50% (28/32)         | 94.08% (302/321)       | 58.30% (418/717)        | 62.26% (99/159)          |
| **Totals**                   | **91.67% (44/48)**     | **88.96% (419/471)**   | **60.10% (506/842)**    | **60.50% (121/200)**     |

> 💡 Function and line coverage are near full on all core logic.
> Branch and region coverage include deep safety checks, abort() handling, softfail paths, and alignment control logic.


### 📊 Benchmarks

A series of rigorous benchmarks were conducted to compare **Armel** with standard `malloc`/`free` under realistic conditions.

Each test performs **10 million operations (`N = 10,000,000`)**, repeated **20 times**.  
To ensure meaningful results:

- `volatile` variables are used to prevent compiler optimizations.
- Each test writes and reads actual memory contents.
- Outliers are removed: the **fastest and slowest** measurements are discarded.
- The final value is the **average time per operation**, computed over the remaining 18 runs.

Benchmark logic is defined in [`arl_bench.h`](includes/Armel/arl_bench.h), and an example suite is available in [`bench.c`](bench.c).

#### Results (Apple M4, compiled with `-O2`)

| Operation               | Time (avg, ns/op) | Notes                                |
|-------------------------|------------------:|----------------------------------------|
| `malloc + memset`       |         143.33 ns | Standard allocation + zeroing         |
| `arl_array (ZEROS)`     | **105.56 ns**     | Faster + zeroing via `ARL_ZEROS` flag |
| `malloc array`          |         234.33 ns | Allocate, write, read, free           |
| `arl_array`             | **193.06 ns**     | Linear alloc + fast reuse             |
| `malloc single`         |           0.17 ns | Likely inlined or optimized by system |
| `arl_make`              | **0.00 ns**       | Pure bump allocation                  |
| `arl_new_custom`        |         282.56 ns | Includes `mmap` / `VirtualAlloc`      |
| `arl_new`               |         283.67 ns | Default alignment, same backend call  |

---

> ⚠️ **Caveats**
> 
> - Not thread-safe  
> - Memory is not individually freed (use `arl_reset()` or `arl_rewind_to()`)  
> - Use responsibly in memory-constrained environments

---

## 📄 License

zlib – do whatever you want, just don't forget to give credit 😉

---

## ✍️ Author

Made with ❤️ by Vincent Huster


> 🙌 Contributions, improvements or feedback are welcome!  
> Open an issue or submit a pull request.