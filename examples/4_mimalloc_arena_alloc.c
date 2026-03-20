#include <mimalloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// necessary for switch of function definitions
#define VECTOR_ALLOCATORS_STATEFUL

// these functions are for demonstration and output
// there is no need to make it that complex
// look at the block below for simple definitions
#if 1
static inline void* myarena_alloc(void* ctx, size_t size) {
	printf("allocated %zu bytes\n", size);
	// or you can define:
	// #define VECTOR_CTX_T mi_heap_t* to surpress warnings
	return mi_heap_malloc(
	    (mi_heap_t*)ctx,
	    size);  // make it compilable with c++, no unemplicit conversions
}
#define VECTOR_ALLOC myarena_alloc

// no need to define realloc, it will alloc and (free if defined or not if
// defined NO_FREE)
static inline void* myarena_realloc(void* ctx, void* ptr, size_t newSize) {
	printf("reallocated block %p with newSize %zu\n", ptr, newSize);
	return mi_heap_realloc((mi_heap_t*)ctx, ptr, newSize);
}
#define VECTOR_REALLOC myarena_realloc

// or like said, #define VECTOR_NO_FREE for actual arena allocators
static inline void myarena_free(void* ctx, void* ptr) {
	printf("deallocated block %p\n", ptr);
	(void)ctx;  // mimalloc doesnt need ctx, but its still given
	mi_free(ptr);
}

#define VECTOR_FREE myarena_free

#else
// typename of the context. not necessary, but will
// remove any unimplicit conversion warnings (in c++)
#define VECTOR_CTX_T mi_heap_t*
// alternative: (will give warnings in c++)
#define VECTOR_ALLOC mi_heap_malloc

// again, not necessary
#define VECTOR_REALLOC mi_heap_realloc

// #define VECTOR_NO_FREE

// or you will need a function that discards ctx
static inline void myarena_free(void* ctx, void* ptr) {
	(void)ctx;
	mi_free(ptr);
}
#define VECTOR_FREE myarena_free
#endif

#define T int64_t, veci64
#define VECTOR_DO_STIN
#define VECTOR_IMPL
#include "../vector_T.h"

static uint8_t arena_buf[1024 * 1024] = {0};

int main(void) {
	mi_arena_id_t arena_id;
	mi_manage_os_memory_ex(arena_buf, sizeof(arena_buf), true, false, false,
			       -1, true, &arena_id);

	mi_heap_t* heap = mi_heap_new_in_arena(arena_id);

	veci64 vec;
	// allocated 8 * 16 bytes
	veci64_init(&vec, heap, 16);

	for (size_t i = 0; i < 32; ++i) {
		// after 16 push backs
		// reallocated block 0xXXX with new size 8 * 32 bytes
		// or allocated 8 * 32bytes (and deallocated block 0xXXX)
		veci64_insert(&vec, i, 0);
	}

	// reallocated block 0xXXX' with new size 8 * 1000 bytes
	// or allocated 8 * 1000bytes (and deallocated block 0xXXX')
	veci64_resize_exact(&vec, 1000);

	vector_foreach(&vec, int64_t i) { printf("%lld ", (long long)i); }
	printf("\n");

	//(deallocated block 0xXXX'')
	veci64_deinit(&vec);

	return 0;
}
