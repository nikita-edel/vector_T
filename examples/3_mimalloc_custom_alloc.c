// for simplicity reasons i just give an example for mimalloc.h,
// you will need to install it to compile with -lmimalloc
#include <mimalloc.h>
#define VECTOR_ALLOC mi_malloc
// realloc is not necessary. it will do an internal alloc, memcpy
// and free if you dont define it

// #define VECTOR_REALLOC mi_realloc

// will only allocate once, when setting with init
// needing resize returns VECTOR_ERR_ALLOC
// no resize/reserve functions can be used
#define VECTOR_NO_RESIZE

// is also not necessary. you can define VECTOR_NO_FREE if you
// dont want a deallocation.
#define VECTOR_FREE mi_free

#define CSTL_IMPL_FALLTHRU

#include <stdint.h>
#define T uint64_t, vecu64
#define VECTOR_DO_STIN
#include "../vector_T.h"

#include <stdio.h>

static inline void* myalloc(size_t size) {
	printf("allocated %zu bytes\n", size);
	return mi_malloc(size);
}
#define VECTOR_ALLOC myalloc
/*
static inline void* myrealloc(void* ptr, size_t newSize) {
	printf("reallocated block %p with new size %zu bytes\n", ptr, newSize);
	return mi_realloc(ptr, newSize);
}

#define VECTOR_REALLOC myrealloc
*/

static inline void myfree(void* ptr) {
	printf("deallocated block %p\n", ptr);
	mi_free(ptr);
}

#define VECTOR_FREE myfree

#define T int64_t, veci64
#define VECTOR_DO_STIN
#include "../vector_T.h"

// this example explicitly doesnt use the first
// created vector, scince it doesnt do anything that shows sth
int main(void) {
	veci64 vec;
	// allocated 8 * 32bytes
	veci64_init(&vec, 32);
	for (int64_t i = 0; i < 64; ++i) {
		// after 32 pushbacks:
		// allocated 8 * 64 bytes
		// deallocated block 0xXXX
		//(or with reallloc reallocated block 0xXXX, with new size 8 *
		//64bytes)
		veci64_push_back(&vec, i);
	}

	// allocated 8 * 128 bytes
	// deallocated block 0xXXX'
	veci64_reserve_min(&vec, 65);

	// allocated 8 * 65 bytes
	// deallocated block 0xXXX''
	veci64_resize_exact(&vec, 65);

	// deallocated block 0xXXX'''
	veci64_deinit(&vec);

	return 0;
}
