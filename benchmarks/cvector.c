#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// compile with -D_POSIX_C_SOURCE=200809L if you get an error
#include <time.h>

#define T char, charvec
#define VECTOR_NO_ERR
#define VECTOR_IMPL
#include "../vector_T.h"

#define T int, intvec
#define VECTOR_NO_ERR
#define VECTOR_IMPL
#include "../vector_T.h"

#define T uint64_t, u64vec
#define VECTOR_NO_ERR
#define VECTOR_IMPL
#include "../vector_T.h"

typedef struct {
	char bytes[256];
} Blob256;

#define T Blob256, blobvec
#define VECTOR_NO_ERR
#define VECTOR_IMPL
#include "../vector_T.h"

// compile with -D_POSIX_C_SOURCE=200809L if you get an error
static inline double now_sec(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

#define MAX_RESULTS 256

typedef struct {
	char label[32];
	char type_name[16];
	uint64_t ops;
	double sec;
} Result;

static Result g_results[MAX_RESULTS];
static int g_nresults = 0;

typedef struct {
	uint64_t batch;
	uint64_t total;
	double elapsed;
	double t0;
} BenchState;

static inline void bench_start(BenchState* b) {
	b->batch = 8;
	b->total = 0;
	b->elapsed = 0.0;
	b->t0 = now_sec();
}

static inline void bench_update(BenchState* b, uint64_t ops) {
	b->total += ops;
	b->elapsed = now_sec() - b->t0;

	double npo = b->total ? b->elapsed * 1e9 / (double)b->total : 1.0;
	uint64_t want = (uint64_t)(5e6 / (npo > 0.0 ? npo : 1.0));

	if (want < 8) want = 8;
	if (want > (1u << 20)) want = (1u << 20);

	b->batch = want;
}

static inline void bench_finish(BenchState* b, const char* label,
				const char* type) {
	Result* r = &g_results[g_nresults++];
	snprintf(r->label, sizeof r->label, "%s", label);
	snprintf(r->type_name, sizeof r->type_name, "%s", type);
	r->ops = b->total;
	r->sec = b->elapsed;
}

static inline void print_results(void) {
	const char* last = "";

	printf("\n%-20s %-10s  %12s  %14s\n", "operation", "type", "ns/op",
	       "total_ops");
	printf(
	    "----------------------------------------------------------------"
	    "\n");

	for (int i = 0; i < g_nresults; ++i) {
		Result* r = &g_results[i];

		if (strcmp(r->type_name, last) && i)
			printf(
			    "--------------------------------------------------"
			    "--------------\n");

		last = r->type_name;

		double ns = r->ops ? r->sec * 1e9 / (double)r->ops : 0.0;
		printf("%-20s %-10s  %12.2f  %14llu\n", r->label, r->type_name,
		       ns, (unsigned long long)r->ops);
	}

	printf("\n");
}

static inline void bench_char(double S) {
	charvec v;
	charvec_init(&v, 4);
	BenchState b;
	static char arr[64];

	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		charvec_clear(&v);
		for (uint64_t i = 0; i < n; ++i) charvec_push_back(&v, 'x');

		bench_update(&b, n);
	}
	bench_finish(&b, "push_back_1", "char");

	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		charvec_clear(&v);
		for (uint64_t i = 0; i < n; ++i)
			charvec_push_back_arr(&v, arr, 64);

		bench_update(&b, n * 64);
	}
	bench_finish(&b, "push_back_64", "char");

	charvec_clear(&v);
	while (v.count < 512) charvec_push_back(&v, 'x');
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		for (uint64_t i = 0; i < n; ++i) {
			if (v.count >= 1024) v.count = 512;
			charvec_insert(&v, 'x', 0);
		}

		bench_update(&b, n);
	}
	bench_finish(&b, "insert_front", "char");

	charvec_clear(&v);
	while (v.count < 512) charvec_push_back(&v, 'x');
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		for (uint64_t i = 0; i < n; ++i) {
			if (v.count + 64 > 1024) v.count = 512;
			charvec_insert_arr(&v, arr, 64, 0);
		}

		bench_update(&b, n * 64);
	}
	bench_finish(&b, "insert_64_front", "char");

	charvec_clear(&v);
	while (v.count < 1024) charvec_push_back(&v, 'x');
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		uint64_t pos = 0;
		for (uint64_t i = 0; i < n; ++i) {
			charvec_replace(&v, 'x', pos);
			pos = (pos + 1) & 1023;
		}

		bench_update(&b, n);
	}
	bench_finish(&b, "replace", "char");

	charvec_deinit(&v);
}

static inline void bench_int(double S) {
	intvec v;
	intvec_init(&v, 4);
	BenchState b;
	static int arr[64];

	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		intvec_clear(&v);
		for (uint64_t i = 0; i < n; ++i) intvec_push_back(&v, 42);

		bench_update(&b, n);
	}
	bench_finish(&b, "push_back_1", "int");

	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		intvec_clear(&v);
		for (uint64_t i = 0; i < n; ++i)
			intvec_push_back_arr(&v, arr, 64);

		bench_update(&b, n * 64);
	}
	bench_finish(&b, "push_back_64", "int");

	intvec_clear(&v);
	while (v.count < 512) intvec_push_back(&v, 42);
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		for (uint64_t i = 0; i < n; ++i) {
			if (v.count >= 1024) v.count = 512;
			intvec_insert(&v, 42, 0);
		}

		bench_update(&b, n);
	}
	bench_finish(&b, "insert_front", "int");

	intvec_clear(&v);
	while (v.count < 512) intvec_push_back(&v, 42);
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		for (uint64_t i = 0; i < n; ++i) {
			if (v.count + 64 > 1024) v.count = 512;
			intvec_insert_arr(&v, arr, 64, 0);
		}

		bench_update(&b, n * 64);
	}
	bench_finish(&b, "insert_64_front", "int");

	intvec_clear(&v);
	while (v.count < 1024) intvec_push_back(&v, 42);
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		uint64_t pos = 0;
		for (uint64_t i = 0; i < n; ++i) {
			intvec_replace(&v, 42, pos);
			pos = (pos + 1) & 1023;
		}

		bench_update(&b, n);
	}
	bench_finish(&b, "replace", "int");

	intvec_deinit(&v);
}

static inline void bench_u64(double S) {
	u64vec v;
	u64vec_init(&v, 4);
	BenchState b;
	static uint64_t arr[64];

	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		u64vec_clear(&v);
		for (uint64_t i = 0; i < n; ++i)
			u64vec_push_back(&v, 0xDEADBEEFULL);

		bench_update(&b, n);
	}
	bench_finish(&b, "push_back_1", "uint64_t");

	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		u64vec_clear(&v);
		for (uint64_t i = 0; i < n; ++i)
			u64vec_push_back_arr(&v, arr, 64);

		bench_update(&b, n * 64);
	}
	bench_finish(&b, "push_back_64", "uint64_t");

	u64vec_clear(&v);
	while (v.count < 512) u64vec_push_back(&v, 0xDEADBEEFULL);
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		for (uint64_t i = 0; i < n; ++i) {
			if (v.count >= 1024) v.count = 512;
			u64vec_insert(&v, 0xDEADBEEFULL, 0);
		}

		bench_update(&b, n);
	}
	bench_finish(&b, "insert_front", "uint64_t");

	u64vec_clear(&v);
	while (v.count < 512) u64vec_push_back(&v, 0xDEADBEEFULL);
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		for (uint64_t i = 0; i < n; ++i) {
			if (v.count + 64 > 1024) v.count = 512;
			u64vec_insert_arr(&v, arr, 64, 0);
		}

		bench_update(&b, n * 64);
	}
	bench_finish(&b, "insert_64_front", "uint64_t");

	u64vec_clear(&v);
	while (v.count < 1024) u64vec_push_back(&v, 0xDEADBEEFULL);
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		uint64_t pos = 0;
		for (uint64_t i = 0; i < n; ++i) {
			u64vec_replace(&v, 0xDEADBEEFULL, pos);
			pos = (pos + 1) & 1023;
		}

		bench_update(&b, n);
	}
	bench_finish(&b, "replace", "uint64_t");

	u64vec_deinit(&v);
}

static inline void bench_blob(double S) {
	static Blob256 blob;
	memset(&blob, 0xAB, sizeof blob);

	blobvec v;
	blobvec_init(&v, 4);

	BenchState b;
	static Blob256 arr[64];

	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		blobvec_clear(&v);
		for (uint64_t i = 0; i < n; ++i)
			blobvec_push_back_ptr(&v, &blob);

		bench_update(&b, n);
	}
	bench_finish(&b, "push_back_1", "Blob256");

	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		blobvec_clear(&v);
		for (uint64_t i = 0; i < n; ++i)
			blobvec_push_back_arr(&v, arr, 64);

		bench_update(&b, n * 64);
	}
	bench_finish(&b, "push_back_64", "Blob256");

	blobvec_clear(&v);
	while (v.count < 512) blobvec_push_back_ptr(&v, &blob);
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		for (uint64_t i = 0; i < n; ++i) {
			if (v.count >= 1024) v.count = 512;
			blobvec_insert_ptr(&v, &blob, 0);
		}

		bench_update(&b, n);
	}
	bench_finish(&b, "insert_front", "Blob256");

	blobvec_clear(&v);
	while (v.count < 512) blobvec_push_back_ptr(&v, &blob);
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		for (uint64_t i = 0; i < n; ++i) {
			if (v.count + 64 > 1024) v.count = 512;
			blobvec_insert_arr(&v, arr, 64, 0);
		}

		bench_update(&b, n * 64);
	}
	bench_finish(&b, "insert_64_front", "Blob256");

	blobvec_clear(&v);
	while (v.count < 1024) blobvec_push_back_ptr(&v, &blob);
	bench_start(&b);
	while (b.elapsed < S) {
		uint64_t n = b.batch;
		uint64_t pos = 0;
		for (uint64_t i = 0; i < n; ++i) {
			blobvec_replace_ptr(&v, &blob, pos);
			pos = (pos + 1) & 1023;
		}

		bench_update(&b, n);
	}
	bench_finish(&b, "replace", "Blob256");

	blobvec_deinit(&v);
}

int main(int argc, char** argv) {
	// per function not bench
	double max_sec = 2.0;
	if (argc >= 2) max_sec = atof(argv[1]);
	if (max_sec <= 0) max_sec = 2.0;

	printf("vector_T C99 benchmark\n");
	printf("max %.1fs per test | compiler: " __VERSION__ "\n", max_sec);

	printf("[char]\n");
	bench_char(max_sec);

	printf("[int]\n");
	bench_int(max_sec);

	printf("[uint64_t]\n");
	bench_u64(max_sec);

	printf("[Blob256]\n");
	bench_blob(max_sec);

	print_results();
	return 0;
}
