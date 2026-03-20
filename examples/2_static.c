typedef struct {
	int x, y, z;
} vec3i;

// internal structure: struct { vec3f data[512]; size_t count; }
// statically allocate the whole vector, similar to c++ inplace_vector
// basically a static stack
#define BIG_SIZE 512
// all makros declared above the vector are undefined
// no makros will be polluting your namespace
// but you might want to have a makro for you vec/stack size
#define VECTOR_STATIC_SIZE BIG_SIZE
#define T vec3i, vec512v3i
#define CSTL_IMPL_FALLTHRU  // not undefined falls thru do the other vector
#define VECTOR_DO_STIN	    // more on this in next block
#include "../vector_T.h"

#define SMALL_SIZE 8
// you have a per size generated vector
// a static vector expands to about 600 lines of assembly
// this makro marks all functions static inline
// this drastically reduces binary size scince only
// used functions are apart of the binary
// but you cant split into a vec.c file, scince you need proper linkage
// also: you still need to guard the implementation of the same
// namespace with a guard, if its fully included
#define VECTOR_DO_STIN
#define VECTOR_STATIC_SIZE SMALL_SIZE
#define T vec3i, vec8v3i
#include <stdio.h>

#include "../vector_T.h"

// in previous example, i explained this is safe. same for a static vector
// it can hold up to 512 elements. once the limit is reached any function
// will return VECTOR_ERR_ALLOC, basically out of memory
static vec512v3i globalVec3is = {0};

int main(void) {
	// valid c99
	for (int i = 0; i < BIG_SIZE; ++i) {
		vec512v3i_push_back_ptr(&globalVec3is, &(vec3i){i, i, i});
	}

	// allocated on the stack
	// 8bytes (count) + 8 * sizeof(vec3i) = 104bytes (assuming int is 4B)
	// note that too big stack allocations, might overflow it
	vec8v3i smallVec3is;
	// initializes every element with {.x = 0, .y = 0, .z = 0}
	// it does a SHALLOW COPY named as MOVE in my library.
	// note MOVE does not mean the c++ way, we are not changing src
	// but scince we are doing a shallow copy, we are not caring to
	// 0 out the src, scince e.g. wihtin the vector, its not needed.
	//
	// count will still be 0, but might prevent
	// e.g. reading uninitalized data
	vec8v3i_init_set(&smallVec3is, &(vec3i){0});
	for (int i = 0; i < SMALL_SIZE; ++i) {
		// just do demonstarate, that you can do that
		vec8v3i_insert(&smallVec3is, (vec3i){i, i, i}, 0);
	}

	// make space
	vec512v3i_pop_n(&globalVec3is, SMALL_SIZE);

	vec3i* start;
	vec8v3i_at_ptr(&smallVec3is, &start, 0);
	size_t count;
	vec8v3i_get_count(&smallVec3is, &count);
	// note that, because the vector is static,
	// it will never point at invalid memory. unless you dont 0 initalize
	// it, which is the default for global and static variables, but not
	// stack allocated resizing will not cause dangling pointers (scince it
	// cant), but it might start pointing to the wrong "thing", when e.g.
	// insert, replace, pop_at

	vec512v3i_push_back_arr(&globalVec3is, start, count);

	vec512v3i_pop_n_at(&globalVec3is, BIG_SIZE - SMALL_SIZE, 0);

	vector_foreach_ptr(&globalVec3is, vec3i * ptr) {
		printf("Vec x:%d, y:%d, z:%d. \n", ptr->x, ptr->y, ptr->z);
	}

	// no need to release the resources, its static or allocated on the
	// stack but you could call, which in this case would just set the count
	// to 0 for types for which you provided a deinitializer (release of
	// ressources) functions like clear and pop will call it so no memory
	// leaks will happen
	vec8v3i_clear(&smallVec3is);
	return 0;
}
