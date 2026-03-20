// here is the source code to understand how to configure errors:

#if defined(VECTOR_ASSERT)
#include <assert.h>
#define VEC__ASSERT(cond, msg) assert((cond) && (msg))

// independent of NDEBUG, + better message
#elif defined(VECTOR_ASSERT_ALWAYS)
#include <stdio.h>
#define VEC__ASSERT(cond, msg)                                                 \
	do {                                                                   \
		if (UNLIKELY(!(cond))) {                                       \
			fprintf(stderr,                                        \
				"Assertion failed (%s):  %s, file: %s, line: " \
				"%d, fn: %s\n",                                \
				(msg), #cond, __FILE__, __LINE__, __func__);   \
			abort();                                               \
		}                                                              \
	} while (0)
#else
#define VEC__ASSERT(cond, msg) ((void)0)
#endif

#if defined(VECTOR_ERR_LOG)
#include <stdio.h>
#define VEC__LOG(msg)                                                 \
	printf("%s, in file:%s, line: %d, fn: %s\n", (msg), __FILE__, \
	       __LINE__, __func__)
#else
#define VEC__LOG(msg) ((void)0)
#endif

#ifdef VECTOR_NO_ERR
#define VECTOR_ERR_LVL 67
#endif

// all of the defines can be used separetely
// there is no initializer check/err, if none is specified

#if VECTOR_ERR_LVL == 1
#define VECTOR_NO_ERR_UNDERFLOW
#define VECTOR_NO_ERR_OVERFLOW

#elif VECTOR_ERR_LVL == 2
#define VECTOR_NO_ERR_NULL
#define VECTOR_NO_ERR_BOUNDS
#define VECTOR_NO_ERR_UNDERFLOW
#define VECTOR_NO_ERR_OVERFLOW

#elif VECTOR_ERR_LVL == 3
#define VECTOR_NO_ERR_NULL
#define VECTOR_NO_ERR_BOUNDS
#define VECTOR_NO_ERR_UNDERFLOW
#define VECTOR_NO_ERR_OVERFLOW
#define VECTOR_NO_ERR_T_INIT

#elif VECTOR_ERR_LVL > 3
#define VECTOR_NO_ERR_NULL
#define VECTOR_NO_ERR_BOUNDS
#define VECTOR_NO_ERR_UNDERFLOW
#define VECTOR_NO_ERR_OVERFLOW
#define VECTOR_NO_ERR_T_INIT
#define VECTOR_NO_ERR_ALLOC
#endif

// if you define NO_ERR, assume this error state is not reachable
#if defined(__clang__)
#define VEC__UNREACHABLE() __builtin_unreachable()

#elif defined(__GNUC__)
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define VEC__UNREACHABLE() __builtin_unreachable()
#else
#define VEC__UNREACHABLE() ((void)0)
#endif

#elif defined(_MSC_VER)
#define VEC__UNREACHABLE() __assume(0)

#else
#define VEC__UNREACHABLE() ((void)0)

#endif

// and use a happy path, which means this branch will not ever exist
// and compiler can do decent optimizations
#ifndef VECTOR_NO_UNREACHABLE
#define VEC__HAPPY_PATH(cond)               \
	do {                                \
		if (UNLIKELY(!(cond))) {    \
			VEC__UNREACHABLE(); \
		}                           \
	} while (0)
#else
#define VEC__HAPPY_PATH(cond) ((void)0)
#endif

// if you define ABORT, than it crashes with no logs
// else use ASSERT or ASSERT_ALWAYS or ERR_LOG
#if defined(VECTOR_ASSERT) || defined(VECTOR_ASSERT_ALWAYS)
#ifdef VECTOR_ERR_ABORT
#error ABORT and ASSERT defined at the same time, choose one
#endif
#define VEC__ASR_OR_RET(cond, msg, reterr) VEC__ASSERT((cond), (msg))

#elif defined(VECTOR_ERR_ABORT)
#define VEC__ASR_OR_RET(cond, msg, reterr) \
	if (!(cond)) {                     \
		abort();                   \
	}

#else
// by default you just return the error (noop), no crash.
#define VEC__ASR_OR_RET(cond, msg, reterr)                \
	do {                                              \
		if (UNLIKELY(!(cond))) {                  \
			VEC__LOG(VEC__STR(cond) " " msg); \
			return (reterr);                  \
		}                                         \
	} while (0)
#endif
