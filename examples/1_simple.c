#include <stdint.h>

// T: typename, namespace of vector, namespace of function
// alternative:
// #define VECTOR_T uint32_t
// #define VECTOR_NS vecU32
//(#define VECTOR_FN_NS vecU32) defaults to VECTOR_NS
//
#define T uint32_t, vecU32
// note: you can split your datastructures in something like:
// vecU32.h and vecU32.c wheree .c includes the .h and declares
// NOTE: if you do that, you need custom include guards,
// scince c doesnt allow the generation of makros based on makros
// this define on top of vecU32.h
// short for VECTOR_IMPLEMENTATION (also possible)
#define VECTOR_IMPL
#include <stdio.h>

#include "../vector_T.h"

int main(void) {
	// no need to 0 initialize if you going to call _init or _init_set
	// but it is safe to do vecU32 ints = {0}; and start using it (any
	// function) (unless you are using stateful allocators and need to pass
	// context)
	vecU32 ints;
	vecU32_init(&ints, 32);	 // start capacity, will always be at least 1
	// runs in a loop and sets initializes every templated value in the
	// array with given templated value vecU32_init_set(&ints, 32, 0);

	for (uint32_t i = 0; i < 69; ++i) {
		vecU32_push_back(&ints, i);
		// usefull for large structs, copys via pointer
		// vecU32_push_back_ptr(&ints, &i);
	}

	// the implementation of this marko is complicated
	// but it gets optimized (>=-Og) to just iterating over an index
	// and assinging ints.data[idx] to i
	// also has a _ptr variant, note it would be uint32_t*
	// it is not null safe.
	vector_foreach(&ints, uint32_t i) { printf("%u,", i); }
	printf("\n");

	// all of these functions have a _ptr prefix variant
	vecU32_insert(&ints, 69, 0);
	vecU32_replace(&ints, 67, 3);
	size_t count = vecU32_count(&ints);

	//_count is unsafe, and will segfault if you pass NULL
	//(unless you assert, read later)
	// all functions apart from deinit return an error code
	// VECTOR_OK == 0
	// VECTOR_ERR_* {NULL, BOUNDS, {UNDER, OVER}FLOW, ALLOC, T_INIT}
	// you can explicitly remove any of those checks or
	// assert/abort/log. by default they are checked and returned
	if (vecU32_get_count(&ints, &count)) {
		printf("error occured\n");
	}

	// also checks if there is at least 2 elements
	// and bounds of the indices etc.
	vecU32_swap(&ints, 0, count - 1);

	uint32_t out;
	vecU32_detach_back(&ints, &out);
	printf("%u\n", out);  // 69, aquivalent to:

	vecU32_insert(&ints, 69, ints.count);
	uint32_t out2;
	vecU32_at(&ints, &out2, ints.count - 1);
	vecU32_pop(&ints);
	printf("%u\n", out2);

	// all of those functions perform bounds, underflow
	// and otherchecks correctly,
	// if you define VECTOR_ASSERT(_ALWAYS)
	// the program will call an assert with the corresponding
	// error message, _ALWAYS also prints the exact function
	// and so does VECTOR_ERR_LOG
	// VECTOR_ERR_ABORT, just crashes the program if they occur
	// VECTOR_NO_ERR removes all checks of any kind
	// VECTOR_ERR_LVL {0, 1, 2, 3, >3}  remove some checks,
	// where as VECTOR_NO_ERR is the maximum
	// note: if you remove those checks, the function will not crash
	// on e.g. out of bounds by itself (but probably segfault)
	vecU32_pop_n(&ints, 3);
	vecU32_pop_at(&ints, ints.count / 2);
	vecU32_pop_n_at(&ints, 4, ints.count / 2);

	count = vecU32_count(&ints);
	for (size_t i = 0; i < count; ++i) {
		printf("%u,", ints.data[i]);  // aquivalent to the foreach makro
	}
	printf("\n");

	// if new size < count, they get discarded
	// later in other examples, if you have a deinitializer
	// they actually get "freed"
	vecU32_resize_exact(&ints, 100);
	// exponentially approaches 200
	// in this case 100 -> 200 -> 400
	vecU32_reserve_min(&ints, 201);

	// aquivalent to vecU32_resize_exact(&ints, ints.count);
	vecU32_shrink_to_fit(&ints);

	// deallocates and set the struct to 0
	// ints can be reused safely after this
	// and so is calling _init, which is needed to set ctx for stateful
	// allocators
	vecU32_deinit(&ints);
}
