#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// signature int fn(T* dst, T* src);
static inline int clone(char** dst, char** src) {
	*dst = strdup(*src);
	return *dst ? 0 : -1;
}

// signature void fn(T* that);
static inline void dealloc(char** that) { free(*that); }

#define VECTOR_T_INIT clone
#define VECTOR_T_DEINIT dealloc
// use assert on error
#define VECTOR_ASSERT
#define VECTOR_IMPL

#define T char*, vecStr
#include "../vector_T.h"

int main(void) {
	vecStr strs;
	vecStr_init(&strs, 32);

	for (size_t i = 0; i < 100; ++i) {
		vecStr_push_back(&strs, "test");
	}

	vecStr_replace(&strs, "replaced0", 0);
	vecStr_replace(&strs, "replaced1", vecStr_count(&strs) - 1);

	vecStr_insert(&strs, "insert0", 0);
	vecStr_insert(&strs, "insert1", 0);
	vecStr_insert(&strs, "insert2", 0);

	vecStr_swap(&strs, 0, 7);

	vector_foreach(&strs, char* i) { printf("%s, ", i); }

	printf("\n");

	vecStr_deinit(&strs);
	return 0;
}
