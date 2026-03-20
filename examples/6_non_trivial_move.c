#include <stdio.h>

// for non trivially movable structs/data
// e.g. self referencing struct, which would be a dangling pointer
// on e.g. reallocation

typedef struct {
	size_t var;
	size_t* pVar;
} self_ref;

// signature void fn(T* dst, const T* src);
// a.k.a shallow copy.
static inline void m_move(self_ref* dst, const self_ref* src) {
	dst->var = src->var;
	dst->pVar = &dst->var;
}

#define VECTOR_T_MOVE m_move
#define VECTOR_ASSERT
#define VECTOR_IMPL
#define T self_ref, vecSr
#include "../vector_T.h"

static inline void sr_from_sz(self_ref* out, size_t var) {
	out->var = var;
	out->pVar = &out->var;
}

int main(void) {
	vecSr vars;
	vecSr_init(&vars, 8);
	// on realloc its going to use the move
	self_ref sr;
	for (size_t i = 0; i < 32; ++i) {
		sr_from_sz(&sr, i);

		// and for initialization (by ptr obviously not needed, but good
		// for bigger structs)
		vecSr_push_back_ptr(&vars, &sr);
	}

	sr_from_sz(&sr, 67);
	// same here its going to backwards move.
	vecSr_insert_ptr(&vars, &sr, 0);

	vector_foreach_ptr(&vars, self_ref * var) {
		printf("var: %zu, deref ptr: %zu \n", var->var, *var->pVar);
	}

	// same for such operations, it will use shallow copy every where
	vecSr_at(&vars, &sr, 0);
	printf("_at 0:\n var: %zu, deref ptr: %zu \n", sr.var, *sr.pVar);

	vecSr_deinit(&vars);
	return 0;
}
