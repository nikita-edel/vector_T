/*
Author:	 Nikita Edel, nikita.edel.dev@gmail.com, on GitHub:https://github.com/nikita-edel
File:	 vector_T.h is a compile time typed dynamic/static stack/array
	 from: https://github.com/nikita-edel/vector_T
License: Zlib Copyright (c) 2026 Nikita Edel

NOTE: this expands to around 350 - 700LOC (including the header without noop like (void)0)

to navigate the source file, do a search for:
{section name}START or END
MAKROS -> HELPERS HEAD SEMANTICS ALLOCERR ALLOC ERRORS
HEADER -> STRUCT API // all the functions are here, just seach for _fnname
	  IMPL (the implementation)
UNDEFINES

            _,'|             _.-''``-...___..--';)
           /_ \'.      __..-' ,      ,--...--'''
          <\    .`--'''       `     /'
           `-';'               ;   ; ;
     __...--''     ___...--_..'  .;.'
    (,__....----'''       (,..--''  

dependencies:	stdint.h //SIZE_MAX and uintptr_t (when custom T_MOVE)
		stdlib.h //libc allocators unless STATIC_SIZE or custom allocators,
			 //abort() for ASSERT_ALWAYS, size_t  
		string.h //memcpy and memmove (when not using custom T_MOVE)
		(stdio.h printf, fprintf when ERR_LOG or ASSERT_ALWAYS)
*/

// helper for text highliting
// #define T int, veci
// #define VECTOR_IMPL

//MAKROSSTART
//HELPERSSTART
#define VEC__MAX(x, y) ((x) > (y) ? (x) : (y))
#define VEC__MIN(x, y) ((x) > (y) ? (y) : (x))

#if !defined(__cplusplus) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)
	#error C99 or later required
#endif

#define VEC__STIN static inline

#ifdef VECTOR_DO_STIN
	#define VEC__FN VEC__STIN
#else
	#define VEC__FN
#endif


#if defined(__cplusplus)
	#if defined(__GNUC__) || defined(__clang__)
		#define VEC__RESTRICT __restrict__

	#elif defined(_MSC_VER)
		#define VEC__RESTRICT __restrict

	#else
		#define VEC__RESTRICT
	#endif
#else
//assume c99+
	#define VEC__RESTRICT restrict
#endif

#define VEC__XCAT(a, b) a##b
#define VEC__CAT(a, b)  VEC__XCAT(a, b)

#define VEC__XSTR(a) #a
#define VEC__STR(a) VEC__XSTR(a)

#if (defined(__GNUC__) || defined(__clang__))  && !defined(VECTOR_NO_BRANCH_HINTS)
	#define LIKELY(x)   __builtin_expect(!!(x), 1)
	#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
	#define LIKELY(x)   (x)
	#define UNLIKELY(x) (x)
#endif

#ifndef VECTOR_NO_STATIC_ASSERT
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
	#define VEC__STATIC_ASSERT(cond, ns) _Static_assert(cond, #ns)
#elif defined(__COUNTER__)
	//counter is there to prevent colliding of typedefs which is not allowed in c99
	#define VEC__STATIC_ASSERT(cond, ns) \
		typedef char VEC__CAT(FN_NS(_static_assert_), VEC__CAT(VEC__CAT(ns, _), __COUNTER__))[(cond) ? 1 : -1]
#else
	#define VEC__STATIC_ASSERT(cond, ns) \
		typedef char VEC__CAT(FN_NS(_static_assert_), VEC__CAT(VEC__CAT(ns, _), __LINE__))[(cond) ? 1 : -1]
#endif
#endif

#define VEC__ARGC(...) VEC__ARGC_(__VA_ARGS__, 5, 4, 3, 2, 1, 0)
#define VEC__ARGC_(a,b,c,d,e,N,...) N
	
#define VEC__GET_1(...) VEC__GET_1_(__VA_ARGS__,)
#define VEC__GET_1_(a, ...) a
	
#define VEC__GET_2(...) VEC__GET_2_(__VA_ARGS__,)
#define VEC__GET_2_(a, b, ...) b
	
#define VEC__GET_3(...) VEC__GET_3_(__VA_ARGS__,)
#define VEC__GET_3_(a, b, c, ...) c

#define VEC__GET_4(...) VEC__GET_4_(__VA_ARGS__,)
#define VEC__GET_4_(a, b, c, d, ...) d
//HELPERSEND

//HEADSTART
//needs to be modified foreach ds
#ifdef T
//custom code per ds
	#if VEC__ARGC(T) == 1
		#if !defined(VECTOR_NS)
			#error T: VECTOR_NS (namespace)(2nd argument) is missing
		#endif
	#elif VEC__ARGC(T) == 2
		#define VECTOR_T   VEC__GET_1(T)
		#define VECTOR_NS    VEC__GET_2(T)
		#define VECTOR_FN_NS VECTOR_NS

	#elif VEC__ARGC(T) == 3
		#define VECTOR_T   VEC__GET_1(T)
		#define VECTOR_NS    VEC__GET_2(T)
		#define VECTOR_FN_NS VEC__GET_3(T)

	#else
		#error T: too many arguments (max 3: type, ns, fn_ns)

	#endif
#endif

#ifndef VECTOR_T
	#error VECTOR_T (type) must be defined
#endif

#ifndef VECTOR_NS
	#error VECTOR_NS (namespace) must be defined
#endif

#ifndef VECTOR_FN_NS
	#define VECTOR_FN_NS VECTOR_NS
#endif

#define FN_NS(fnname) VEC__CAT(VECTOR_FN_NS, fnname)
//HEADEND

//
//SEMANTICSSTART
///
#ifdef VECTOR_T_DEINIT
	#ifdef VECTOR_ALLOCATORS_STATEFUL
		#define VEC__T_DEINIT(ctx, that) VECTOR_T_DEINIT((ctx), (that))
	#else
		#define VEC__T_DEINIT(ctx, that) VECTOR_T_DEINIT((that))
	#endif
	
	#define VEC__T_DEINIT_N(ctx, here, amount)\
		do { \
			VECTOR_T* de__here = (here); \
			size_t de__n = (size_t)(amount); \
			for(size_t de__i = 0; de__i < de__n; ++de__i) { \
				VEC__T_DEINIT((ctx), de__here + de__i); \
			} \
		} while(0)
	
#else
	#define VEC__T_DEINIT(ctx, that) ((void)0)
	#define VEC__T_DEINIT_N(ctx, here, amount) ((void)0)
#endif

#ifdef VECTOR_T_MOVE

 //single asignment
	#define VEC__T_MOVE(dst, src) VECTOR_T_MOVE((dst), (const VECTOR_T*)(src))

//custom memcpy
	#define VEC__T_MCPY(dst, src, amount)\
		do { \
			VECTOR_T* VEC__RESTRICT cpy__dst = (dst); \
			const VECTOR_T* VEC__RESTRICT cpy__src = (const VECTOR_T*) (src); \
			size_t cpy__n = (size_t)(amount); \
			\
			if(LIKELY(cpy__n != 0 && (const VECTOR_T*) cpy__dst != cpy__src)) { \
				for(size_t cpy__i = 0; cpy__i < cpy__n; ++cpy__i) { \
					VEC__T_MOVE(cpy__dst + cpy__i, cpy__src + cpy__i); \
				} \
			} \
		} while(0)

	#define VEC__T_MMOV(dst, src, amount)\
		do { \
			VECTOR_T* mov__dst = (dst); \
			VECTOR_T* mov__src = (src); \
			size_t mov__n = (size_t)(amount); \
			\
			if(LIKELY(mov__n != 0 && mov__dst != mov__src)) { \
				if((uintptr_t) mov__dst <= (uintptr_t) mov__src \
						|| (uintptr_t)mov__dst >= (uintptr_t)mov__src + mov__n) { \
					for(size_t mov__i = 0; mov__i < mov__n; ++mov__i) { \
						VEC__T_MOVE(mov__dst + mov__i, mov__src + mov__i); \
					} \
					\
				} else { \
					while(mov__n) { \
						--mov__n; \
						VEC__T_MOVE(mov__dst + mov__n, mov__src + mov__n); \
					} \
				} \
			} \
		} while(0)

	#define VEC__NO_REALLOC
#else
	#define VEC__T_MOVE(dst, src) *(dst) = *(src)
	#define VEC__T_MCPY(dst, src, amount) memcpy((dst), (src), (amount) * sizeof(VECTOR_T))
	#define VEC__T_MMOV(dst, src, amount) memmove((dst), (src), (amount) * sizeof(VECTOR_T))
#endif

//shallow copy of val into an array
#define VEC__T_MSET(dst, pVal, amount) \
	do { \
		VECTOR_T* VEC__RESTRICT set__dst = (dst); \
		VECTOR_T* set__val = (pVal); \
		size_t set__n = (size_t)(amount); \
		if(LIKELY(set__n != 0)) { \
			for(size_t set__i = 0; set__i < set__n; ++set__i) { \
				VEC__T_MOVE(set__dst + set__i, set__val); \
			} \
		} \
	} while (0)

#ifdef VECTOR_T_INIT
//pSuc := pointer to an int that 0 represents success
	#ifdef VECTOR_ALLOCATORS_STATEFUL
		#define VEC__T_XINIT(ctx, dst, src) VECTOR_T_INIT((ctx), (dst), (src))
	#else
		#define VEC__T_XINIT(ctx, dst, src) VECTOR_T_INIT((dst), (src))
	#endif
	

	#ifdef VECTOR_NO_ERR_T_INIT
		#define VEC__T_INIT(ctx, dst, src, pSuc) do { *(pSuc) = 0; VEC__T_XINIT((ctx), (dst), (src)); } while(0)


		#define VEC__T_INIT_N(ctx, dst, src, amount, pSuc) \
			do { \
				*(pSuc) = 0;\
				VECTOR_T* in__dst = (dst); \
				VECTOR_T* in__src = (src); \
				size_t in__n = (size_t)(amount); \
				\
				if(LIKELY(in__n > 0 &&  in__dst != in__src)) {\
					for(size_t in__i = 0; in__i < in__n; ++in__i) { \
						VEC__T_XINIT((ctx), in__dst + in__i, in__src + in__i); \
					} \
				}\
			} while(0)
	#else
		#define VEC__T_INIT(ctx, dst, src, pSuc) \
			do { \
				*(pSuc) = VEC__T_XINIT((ctx), (dst), (src)); \
			} while(0)

		#define VEC__T_INIT_N(ctx, dst, src, amount, pSuc) \
			do { \
				*(pSuc) = 0; \
				VECTOR_T* in__dst = (dst); \
				VECTOR_T* in__src = (src); \
				size_t in__n = (size_t)(amount); \
				\
				if(LIKELY(in__n > 0 &&  in__dst != in__src)) { \
					for(size_t in__i = 0; in__i < in__n; ++in__i) { \
						if(UNLIKELY(VEC__T_XINIT((ctx),  \
										in__dst + in__i, in__src + in__i) != 0)) { \
							\
							*(pSuc) = 1; \
							VEC__T_DEINIT_N((ctx), in__dst, in__i); \
							break; \
						} \
					} \
				} \
			} while(0)

	#endif

#else
	#define VEC__T_INIT(ctx, dst, src, pSuc) do { *(pSuc) = 0; VEC__T_MOVE((dst), (src)); } while(0)
	#define VEC__T_INIT_N(ctx, dst, src, amount, pSuc) do { *(pSuc) = 0; VEC__T_MCPY((dst), (src), (amount)); } while(0)
	#define VECTOR_NO_ERR_T_INIT

#endif
///
//SEMANTICSEND

//ALLOCERRSTART
//
#if defined(VECTOR_STATIC_SIZE) && !(VECTOR_STATIC_SIZE > 0)
	#error VECTOR_STATIC_SIZE must be greater than 0
#endif

#if defined(VECTOR_FREE) && defined(VECTOR_NO_FREE)
	#error defined both free and no_free
#endif

#if defined(VECTOR_ALLOC) && !(defined(VECTOR_FREE) || defined(VECTOR_NO_FREE))
	#error didnt define free for allocator define VECTOR_FREE or VECTOR_NO_FREE
#endif

#if !defined(VECTOR_ALLOC) && (defined(VECTOR_FREE) || defined(VECTOR_NO_FREE))
	#error tried to define free but no alloc was defined
#endif

#if defined(VECTOR_ALLOCATORS_STATEFUL) || defined(VECTOR_ALIGN_SIZE)\
	|| defined(VECTOR_REALLOC)

	#if !defined(VECTOR_ALLOC) || !(defined(VECTOR_FREE) || defined(VECTOR_NO_FREE))
		#error for both alloc or free (or no_free) needs to be defined
	#endif
#endif

#if defined(VECTOR_NO_FREE) 
	#if !(defined(VECTOR_ALLOC) || defined(VECTOR_ALLOCATORS_STATEFUL) || defined(VECTOR_ALIGN_SIZE)\
		|| defined(VECTOR_REALLOC))

		#error no_free, while not using custom allocators
	#endif
#endif

#if defined(VECTOR_STATIC_SIZE)
	#if defined(VECTOR_ALLOC) || defined(VECTOR_FREE) || defined(VECTOR_ALIGN_SIZE) \
	   || defined(VECTOR_ALLOCATORS_STATEFUL) || defined(VECTOR_REALLOC)
		#error static_size defined dont define anything allocator specific
	#endif

	#if defined(VECTOR_NO_RESIZE)
		#error both no_resize and static_size defined, static VECTOR wont resize anyways
	#endif
#endif

#if defined(VECTOR_REALLOC)

	#if defined(VECTOR_NO_RESIZE)
		#error realloc not needed for VECTOR with no_resize
	#endif
	
	#if defined(VEC__NO_REALLOC)
		#error no realloc needed (not used or cant use because e.g. move defined for type)
	#endif

#endif
//ALLOCERREND

//ALLOCSTART
//this is required for functions that need to take in a state when required

#ifdef VECTOR_ALLOCATORS_STATEFUL
	#ifdef VECTOR_CTX_T
		#define VEC__CTX VECTOR_CTX_T
	#else
		#define VEC__CTX void*
	#endif

	#define VEC__CTX_PARAM , VEC__CTX ctx
#else
	#define VEC__CTX_PARAM
#endif

//information for ds/library
#if defined(VECTOR_ALLOC) && !defined(VECTOR_REALLOC)
	#define VEC__NO_REALLOC
#endif

#if defined(VECTOR_ALLOCATORS_STATEFUL)
	#if defined(VECTOR_ALIGN_SIZE)
		#define VEC__ALLOC(ctx, size) VECTOR_ALLOC((ctx), (size), (VECTOR_ALIGN_SIZE))
		
		#if defined(VECTOR_REALLOC)
			#define VEC__REALLOC(ctx, ptr, size) VECTOR_REALLOC((ctx), (ptr), (size), (VECTOR_ALIGN_SIZE))
		#endif
		
	#else
		#define VEC__ALLOC(ctx, size) VECTOR_ALLOC((ctx), (size))

		#if defined(VECTOR_REALLOC)
			#define VEC__REALLOC(ctx, ptr, size) VECTOR_REALLOC((ctx), (ptr), (size))
		#endif

	#endif

	#if defined(VECTOR_NO_FREE)
		#define VEC__FREE(ctx, ptr) ((void)0)

	#else
		#define VEC__FREE(ctx, ptr) VECTOR_FREE((ctx), (ptr))

	#endif
	
	
#else
	#if defined(VECTOR_ALIGN_SIZE)
		#define VEC__ALLOC(ctx, size) VECTOR_ALLOC((size), (VECTOR_ALIGN_SIZE))

		#if defined(VECTOR_NO_FREE)
			#define VEC__FREE(ctx, ptr) ((void)0)

		#else
			#define VEC__FREE(ctx, ptr) VECTOR_FREE((ptr))
		#endif

		#if defined(VECTOR_REALLOC)
			#define VEC__REALLOC(ctx, ptr, size) VECTOR_REALLOC((ptr), (size), (VECTOR_ALIGN_SIZE))
		#endif
		
	#else
		#if defined(VECTOR_ALLOC)
			#define VEC__ALLOC(ctx, size) VECTOR_ALLOC((size))

			#if defined(VECTOR_NO_FREE)
				#define VEC__FREE(ctx, ptr) ((void)0)

			#else
				#define VEC__FREE(ctx, ptr) VECTOR_FREE((ptr))

			#endif

			#if defined(VECTOR_REALLOC)
				#define VEC__REALLOC(ctx, ptr, size) VECTOR_REALLOC((ptr), (size))
			#endif
		#else
			#define VEC__ALLOC(ctx, size) malloc((size))

			#define VEC__FREE(ctx, ptr) free((ptr))
			//code is not depandant on having realloc but on VEC__NO_REALLOC or yes
			#define VEC__REALLOC(ctx, ptr, size) realloc((ptr), (size))


		#endif
	#endif
#endif
//ALLOCEND

//ERRORSSTART
#if defined(VECTOR_ASSERT)
	#include <assert.h>
	#define VEC__ASSERT(cond, msg) assert((cond) && (msg))

#elif defined(VECTOR_ASSERT_ALWAYS)
	#include <stdio.h>
	#define VEC__ASSERT(cond, msg) \
		do {\
			if(UNLIKELY(!(cond))) {\
				fprintf(stderr, "Assertion failed (%s):  %s, file: %s, line: %d, fn: %s\n", \
					(msg), #cond, __FILE__, __LINE__, __func__); \
				abort();\
			}\
		} while(0)
#else
	#define VEC__ASSERT(cond, msg) ((void)0)
#endif

#if defined(VECTOR_ERR_LOG)
	#include <stdio.h>
	#define VEC__LOG(msg) printf("%s, in file:%s, line: %d, fn: %s\n", (msg), __FILE__, __LINE__, __func__)
#else
	#define VEC__LOG(msg) ((void)0)
#endif

#define VEC__LOG_ASSERT(cond, msg)\
	do {\
		VEC__LOG(VEC__STR((cond)) " " (msg));\
		VEC__ASSERT((cond), (msg));\
	} while(0)

#ifdef VECTOR_NO_ERR
	#define VECTOR_ERR_LVL 69
#endif

#if !defined(VECTOR_ERR_LVL) || VECTOR_ERR_LVL == 0
	//nothing
#elif VECTOR_ERR_LVL == 1
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


#ifndef VECTOR_NO_UNREACHABLE
	#define VEC__HAPPY_PATH(cond) do { if(UNLIKELY(!(cond))) { VEC__UNREACHABLE(); } } while(0)
#else
	#define VEC__HAPPY_PATH(cond) ((void)0)
#endif


#if defined(VECTOR_ASSERT) || defined(VECTOR_ASSERT_ALWAYS)
	#ifdef VECTOR_ERR_ABORT
		#error ABORT and ASSERT defined at the same time, choose one
	#endif
	#define VEC__ASR_OR_RET(cond, msg, reterr) \
		VEC__ASSERT((cond), (msg))

#elif defined(VECTOR_ERR_ABORT)
	#define VEC__ASR_OR_RET(cond, msg, reterr) \
		if(!(cond)) { abort(); }

#else
	#define VEC__ASR_OR_RET(cond, msg, reterr) \
		do { \
			if(UNLIKELY(!(cond))) { \
				VEC__LOG(VEC__STR(cond) " " msg); \
				return (reterr); \
			} \
		} while(0)
#endif

#ifndef VECTOR_NO_ERR_NULL
	#define VEC__NULL_CHECK(notnull) \
	VEC__ASR_OR_RET((notnull) != NULL, "NULL ERROR FOR VECTOR", VECTOR_ERR_NULL)

#else 
	#define VEC__NULL_CHECK(notnull) \
	VEC__HAPPY_PATH((notnull) != NULL)

#endif

#ifndef VECTOR_NO_ERR_ALLOC
	#define VEC__ALLOC_CHECK(notnull) \
	VEC__ASR_OR_RET((notnull) != NULL, "ALLOC ERROR FOR VECTOR", VECTOR_ERR_ALLOC)
#else
	#define VEC__ALLOC_CHECK(notnull) \
	VEC__HAPPY_PATH((notnull) != NULL)

#endif

#ifndef VECTOR_NO_ERR_BOUNDS
	#define VEC__BOUNDS_CHECK(start, end, pos)\
	VEC__ASR_OR_RET((pos) >= (start) && (pos) <= (end), "OUT OF BOUNDS ERROR FOR VECTOR", VECTOR_ERR_BOUNDS)

	#define VEC__MAX_CHECK(val, max)\
	VEC__ASR_OR_RET((val) <= (max), "MAX ERROR FOR VECTOR", VECTOR_ERR_BOUNDS)

	#define VEC__MIN_CHECK(val, min)\
	VEC__ASR_OR_RET((val) >= (min), "MIN ERROR FOR VECTOR", VECTOR_ERR_BOUNDS)

#else
	#define VEC__MIN_CHECK(val, atleast) \
	VEC__HAPPY_PATH((val) >= (atleast))

	#define VEC__MAX_CHECK(val, max) \
	VEC__HAPPY_PATH((val) <= (max))

	#define VEC__BOUNDS_CHECK(start, end, pos) \
	VEC__HAPPY_PATH((pos) >= (start) && (pos) <= (end))

#endif

#ifndef VECTOR_NO_ERR_UNDERFLOW
	#define VEC__UNDERFLOW_CHECK(left, minus) \
	VEC__ASR_OR_RET((minus) <= (left), "UNDERFLOW ERROR FOR VECTOR", VECTOR_ERR_UNDERFLOW)

#else
	#define VEC__UNDERFLOW_CHECK(left, minus) \
	VEC__HAPPY_PATH((minus) <= (left))

#endif

#ifndef VECTOR_NO_ERR_OVERFLOW
	#define VEC__ADD_OVERFLOW_CHECK(max, curr, plus) \
	VEC__ASR_OR_RET((curr) <= (max) - (plus), "OVERFLOW ERROR FOR VECTOR", VECTOR_ERR_OVERFLOW)

	#define VEC__MUL_OVERFLOW_CHECK(max, curr, factor) \
	VEC__ASR_OR_RET((curr) <= (max) / (factor), "OVERFLOW ERROR FOR VECTOR", VECTOR_ERR_OVERFLOW)

#else
	#define VEC__ADD_OVERFLOW_CHECK(max, curr, plus) \
	VEC__HAPPY_PATH((curr) <= (max) - (plus))

	#define VEC__MUL_OVERFLOW_CHECK(max, curr, factor) \
	VEC__HAPPY_PATH((curr) <= (max) / (factor))

#endif

#ifndef VECTOR_NO_ERR_T_INIT
	#define VEC__T_INIT_CHECK(succ, recoverblock) \
		do { \
			if(UNLIKELY((succ) != 0)) { \
				recoverblock ; \
				VEC__ASR_OR_RET((succ) == 0, "INIT T ERROR FOR VECTOR", VECTOR_ERR_T_INIT); \
			} \
		} while(0)

#else
//need to explicitly throw that bolean away so the compiler can optimize any use of it away
	#define VEC__T_INIT_CHECK(succ, recoverblock) ((void)succ)

#endif
///
//ERRORSEND
//MAKROSEND

//HEADERSTART
#ifndef VECTOR_BASE_H
#define VECTOR_BASE_H

#include <stdlib.h>	

#define VECTOR_STATUS_TYPES(X)\
X(OK = 0) \
X(ERR_NULL) \
X(ERR_ALLOC) \
X(ERR_BOUNDS) \
X(ERR_UNDERFLOW) \
X(ERR_OVERFLOW) \
X(ERR_T_INIT) \
X(_ERR_COUNT)

typedef enum {
#define X(name) VECTOR_##name,
	VECTOR_STATUS_TYPES(X)
#undef X
} VEC_STATUS;

#endif // VECTOR_BASE_H

//THEADERSTART
#ifdef VECTOR_T

//STRUCTSTART
#define vec_T VECTOR_NS
#define val_T VECTOR_T

#ifndef VECTOR_STATIC_SIZE

typedef struct {
	val_T* data;
	size_t count;
	size_t cap;

#ifdef VECTOR_ALLOCATORS_STATEFUL
	VEC__CTX ctx;
#endif

} vec_T; 

#else //static

typedef struct {
	val_T data[VECTOR_STATIC_SIZE];
	size_t count;
} vec_T; 

#endif 
//STRUCTEND

//APISTART
#if !defined(VECTOR_T_MOVE) && !defined(VECTOR_ALIGN_SIZE)
#define vector_foreach(pVec, declVar)\
	for(size_t vec__i = 0, vec__once = 1; vec__i < (pVec)->count; ++vec__i, vec__once = 1)\
		for( declVar = (pVec)->data[vec__i]; vec__once; vec__once = 0)
#endif

#define vector_foreach_ptr(pVec, pDeclVar)\
	for(size_t vec__i = 0, vec__once = 1; vec__i < (pVec)->count; ++vec__i, vec__once = 1)\
		for( pDeclVar = (pVec)->data + vec__i; vec__once; vec__once = 0)


#ifdef __cplusplus
extern "C" {
#endif


#ifndef VECTOR_STATIC_SIZE

VEC__FN VEC_STATUS FN_NS(_init)(vec_T* vec VEC__CTX_PARAM, size_t startCap);

#ifndef VECTOR_DEBLOAT
VEC__FN VEC_STATUS FN_NS(_init_set)(vec_T* vec VEC__CTX_PARAM, size_t startCap, val_T* val);
#endif

#else

VEC__FN VEC_STATUS FN_NS(_init)(vec_T* vec);

#ifndef VECTOR_DEBLOAT
VEC__FN VEC_STATUS FN_NS(_init_set)(vec_T* vec, val_T* value);
#endif

#endif 

VEC__FN void FN_NS(_deinit)(vec_T* vec);

#if !defined(VECTOR_STATIC_SIZE) && !defined(VECTOR_NO_RESIZE)

VEC__FN VEC_STATUS FN_NS(_resize_exact)(vec_T* vec, size_t newCap);

#ifndef VECTOR_DEBLOAT
//guarantees minCap, exponentially approaches the specified capacity
VEC__FN VEC_STATUS FN_NS(_reserve_min)(vec_T* vec, size_t minCap);

VEC__FN VEC_STATUS FN_NS(_shrink_to_fit)(vec_T* vec);
#endif

#endif 

#ifndef VECTOR_ALIGN_SIZE
VEC__FN VEC_STATUS FN_NS(_push_back)(vec_T* vec, val_T val);
#endif

VEC__FN VEC_STATUS FN_NS(_push_back_ptr)(vec_T* vec, val_T* val);

VEC__FN VEC_STATUS FN_NS(_push_back_arr)(vec_T* vec, val_T* valArr, size_t count);


#ifndef VECTOR_ALIGN_SIZE
VEC__FN VEC_STATUS FN_NS(_insert)(vec_T* vec, val_T val, size_t at);
#endif

VEC__FN VEC_STATUS FN_NS(_insert_ptr)(vec_T* vec, val_T* val, size_t at);

VEC__FN VEC_STATUS FN_NS(_insert_arr)(vec_T* vec, val_T* valArr, size_t count, size_t at);

#ifndef VECTOR_ALIGN_SIZE
VEC__FN VEC_STATUS FN_NS(_replace)(vec_T* vec, val_T val, size_t at);
#endif

#if !defined(VECTOR_ALIGN_SIZE) || defined(VECTOR_NO_ERR_T_INIT)
VEC__FN VEC_STATUS FN_NS(_replace_ptr)(vec_T* vec, val_T* val, size_t at);
#endif

#if !defined(VECTOR_DEBLOAT) && !defined(VECTOR_ALIGN_SIZE)
VEC__FN VEC_STATUS FN_NS(_swap)(vec_T* vec, size_t at1, size_t at2);
#endif

#ifndef VECTOR_DEBLOAT
VEC__FN VEC_STATUS FN_NS(_clear)(vec_T* vec);
#endif

VEC__FN VEC_STATUS FN_NS(_pop)(vec_T* vec);

VEC__FN VEC_STATUS FN_NS(_pop_n)(vec_T* vec, size_t n);

VEC__FN VEC_STATUS FN_NS(_pop_at)(vec_T* vec, size_t at);

#ifndef VECTOR_DEBLOAT
VEC__FN VEC_STATUS FN_NS(_pop_n_at)(vec_T* vec, size_t n, size_t at);
#endif

VEC__FN VEC_STATUS FN_NS(_detach_back)(vec_T* vec, val_T* out);

VEC__FN VEC_STATUS FN_NS(_detach_at)(vec_T* vec, val_T* out, size_t at);

#ifndef VECTOR_ALIGN_SIZE
VEC__FN VEC_STATUS FN_NS(_at)(const vec_T* vec, val_T* out, size_t at);
#endif

VEC__FN VEC_STATUS FN_NS(_at_ptr)(vec_T* vec, val_T** out, size_t at);

#ifndef VECTOR_DEBLOAT
VEC__FN VEC_STATUS FN_NS(_get_count)(const vec_T* vec, size_t* out);

VEC__FN VEC_STATUS FN_NS(_get_cap)(const vec_T* vec, size_t* out);
#endif

VEC__FN size_t FN_NS(_count)(const vec_T* vec);

VEC__FN size_t FN_NS(_cap)(const vec_T* vec);

#ifndef VECTOR_DEBLOAT
VEC__FN size_t FN_NS(_cap_left)(const vec_T* vec);
#endif
//APIEND

#ifdef __cplusplus
}
#endif

//IMPLSTART
#if defined(VECTOR_IMPLEMENTATION) || defined(VECTOR_IMPL) || defined(CSTL_IMPL_FALLTHRU)

#include <string.h>
#include <stdint.h>

#define VEC__MIN_CAP 1

#define VEC__GROWTH_FAC 2

//dirty makros to not pollute namespace with static function when including file with implementation
//also prevents needing to forward the return of an error, scince all checks do a return
#define VEC__NEXT_CAP(currCap, reqCap, pOutCap) \
		do {\
			size_t newCap = (currCap);\
			if(UNLIKELY(newCap == 0)) { \
				newCap = VEC__MIN_CAP * VEC__GROWTH_FAC; \
			} \
			while(newCap < (reqCap)) {\
				VEC__MUL_OVERFLOW_CHECK(SIZE_MAX, newCap, VEC__GROWTH_FAC);\
				newCap *= VEC__GROWTH_FAC;\
			}\
			\
			VEC__MUL_OVERFLOW_CHECK(SIZE_MAX, newCap, sizeof(val_T));\
			*(pOutCap) = newCap;\
			\
		} while(0)

#if defined(VECTOR_FORCE_REALLOC) && !defined(VEC__REALLOC) 
	#error tried to force use realloc, but no realloc was found
#endif

#if !(defined(VECTOR_STATIC_SIZE) || defined(VECTOR_NO_RESIZE))

	#if !(defined(VEC__NO_REALLOC) || defined(VECTOR_T_MOVE)) || (defined(VECTOR_FORCE_REALLOC) && defined(VEC__REALLOC))//trivially movable, have realloc
		#define VEC__FORCE_RESIZE(pVec, newCap) \
			do { \
				val_T* temp_ = (val_T*) VEC__REALLOC((pVec)->ctx, (pVec)->data, (newCap) * sizeof(val_T)); \
				VEC__ALLOC_CHECK(temp_); \
				\
				(pVec)->data = temp_; \
				(pVec)->cap = (newCap); \
			} while(0)

	#else
		#define VEC__FORCE_RESIZE(pVec, newCap) \
			do { \
				val_T* temp_ = (val_T*) VEC__ALLOC((pVec)->ctx, (newCap) * sizeof(val_T)); \
				VEC__ALLOC_CHECK(temp_); \
				\
				VEC__T_MCPY(temp_, (pVec)->data, (pVec)->count); \
				VEC__FREE((pVec)->ctx, (pVec)->data); \
				\
				(pVec)->data = temp_; \
				(pVec)->cap = (newCap); \
			} while(0)
	#endif

	#define VEC__MAYBE_RESIZE_N(pVec, addCap)\
		if(UNLIKELY((pVec)->count + (addCap) > (pVec)->cap)) { \
			size_t newCap_; \
			VEC__NEXT_CAP((pVec)->cap, (pVec)->count + (addCap), &newCap_); \
			VEC__FORCE_RESIZE((pVec), (newCap_)); \
		}

//this was added when i was benchmarking, because i thought the NEXT_CAP
//with an arbitrary additional length is overhead, 
//but it does technically save one asm instruction (an addition)
//and one less branch for the while loop

	#define VEC__MAYBE_RESIZE_ONE(pVec) \
		if(UNLIKELY((pVec)->count == (pVec)->cap)) { \
			VEC__MUL_OVERFLOW_CHECK(SIZE_MAX, (pVec)->cap, VEC__GROWTH_FAC); \
			size_t newCap = (pVec)->cap * VEC__GROWTH_FAC; \
			if(UNLIKELY(newCap == 0)) { \
				newCap = VEC__MIN_CAP * VEC__GROWTH_FAC ; \
			} \
			VEC__MUL_OVERFLOW_CHECK(SIZE_MAX, newCap, sizeof(val_T)); \
			VEC__FORCE_RESIZE((pVec), (newCap)); \
		}

#elif defined(VECTOR_NO_RESIZE)
#ifdef VECTOR_NO_ERR_ALLOC
	#define VEC__MAYBE_RESIZE_N(pVec, addCap) \
		VEC__HAPPY_PATH((pVec)->count + addCap <= (pVec)->cap)
#else
	#define VEC__MAYBE_RESIZE_N(pVec, addCap)\
		VEC__ASR_OR_RET((pVec)->count + addCap <= (pVec)->cap, "RAN OOMEMORY FOR A NOT RESIZABLE VECTOR", VECTOR_ERR_ALLOC)
#endif

//im wayyy too lazy for this
	#define VEC__MAYBE_RESIZE_ONE(pVec) VEC__MAYBE_RESIZE_N((pVec), 1)

#else // static size
#ifdef VECTOR_NO_ERR_ALLOC
	#define VEC__MAYBE_RESIZE_N(pVec, addCap) \
		VEC__HAPPY_PATH((pVec)->count + addCap <= VECTOR_STATIC_SIZE)
#else
	#define VEC__MAYBE_RESIZE_N(pVec, addCap)\
		VEC__ASR_OR_RET((pVec)->count + (addCap) <= VECTOR_STATIC_SIZE , "RAN OOMEMORY FOR A STATIC VECTOR", VECTOR_ERR_ALLOC)
#endif

	#define VEC__MAYBE_RESIZE_ONE(pVec) VEC__MAYBE_RESIZE_N((pVec), 1)

#endif


#ifndef VECTOR_STATIC_SIZE

VEC__FN VEC_STATUS FN_NS(_init)(vec_T* vec VEC__CTX_PARAM, size_t startCap) {
	VEC__NULL_CHECK(vec);

	VEC__MUL_OVERFLOW_CHECK(SIZE_MAX, startCap, sizeof(val_T));
	startCap = VEC__MAX(startCap, VEC__MIN_CAP);

	val_T* allocated = (val_T*) VEC__ALLOC(ctx, startCap * sizeof(val_T));
	if(UNLIKELY(!allocated)) {
			VEC__ASR_OR_RET(allocated != NULL, 
					"ERROR ALLOC WHILE INIT FOR VECTOR",
					VECTOR_ERR_ALLOC);
	}

	vec->data = allocated;
	vec->cap = startCap;
	vec->count = 0;
	
#ifdef VECTOR_ALLOCATORS_STATEFUL
	vec->ctx = ctx;
#endif
	return VECTOR_OK;
}

#if !defined(VECTOR_DEBLOAT)
VEC__FN VEC_STATUS FN_NS(_init_set)(vec_T* vec VEC__CTX_PARAM, size_t startCap, val_T* val) {
	VEC__NULL_CHECK(vec);

	VEC__MUL_OVERFLOW_CHECK(SIZE_MAX, startCap, sizeof(val_T));
	startCap = VEC__MAX(startCap, VEC__MIN_CAP);

	val_T* allocated = (val_T*) VEC__ALLOC(ctx, startCap * sizeof(val_T));
	VEC__ALLOC_CHECK(allocated);

	VEC__T_MSET(allocated, val, startCap);

	vec->data = allocated;
	vec->cap = startCap;
	vec->count = 0;
	
#ifdef VECTOR_ALLOCATORS_STATEFUL
	vec->ctx = ctx;
#endif

	return VECTOR_OK;
}
#endif
#else

VEC__FN VEC_STATUS FN_NS(_init)(vec_T* vec) {
	VEC__NULL_CHECK(vec);
	vec->count = 0;
	return VECTOR_OK;
}


#if !defined(VECTOR_DEBLOAT) 
VEC__FN VEC_STATUS FN_NS(_init_set)(vec_T* vec, val_T* val) {
	VEC__NULL_CHECK(vec);
	VEC__T_MSET(vec->data, val, VECTOR_STATIC_SIZE);
	vec->count = 0;
	return VECTOR_OK;
}
#endif

#endif


#ifndef VECTOR_STATIC_SIZE

VEC__FN void FN_NS(_deinit)(vec_T* vec) {
	if(UNLIKELY(!vec)) {
		return;
	}
	
	VEC__T_DEINIT_N(vec->ctx, vec->data, vec->count);

	if(LIKELY(vec->data))
		VEC__FREE(vec->ctx, vec->data);

	vec->data = NULL;
	vec->cap = vec->count = 0;
}

#else

VEC__FN void FN_NS(_deinit)(vec_T* vec) {
	if(UNLIKELY(!vec)) {
		return;
	}

	VEC__T_DEINIT_N(vec->ctx, vec->data, vec->count);
	vec->count = 0;
}

#endif

#if !defined(VECTOR_STATIC_SIZE)  && !defined(VECTOR_NO_RESIZE)

VEC__FN VEC_STATUS FN_NS(_resize_exact)(vec_T* vec, size_t newCap) {
	VEC__NULL_CHECK(vec);

	if(UNLIKELY(vec->cap == newCap)) {
		return VECTOR_OK;
	}

	if(UNLIKELY(newCap < vec->count)) {

		VEC__T_DEINIT_N(vec->ctx, vec->data + newCap, vec->count - newCap);
		vec->count = newCap;
	}
	
	newCap = VEC__MAX(newCap, VEC__MIN_CAP);

	VEC__FORCE_RESIZE(vec, newCap);

	return VECTOR_OK;
}

#ifndef VECTOR_DEBLOAT
//exponentially approach minCap
VEC__FN VEC_STATUS FN_NS(_reserve_min)(vec_T* vec, size_t minCap) {
	VEC__NULL_CHECK(vec);
	
	if(minCap <= vec->cap) {
		return VECTOR_OK;
	}

	size_t nextCap;
	VEC__NEXT_CAP(vec->cap, minCap, &nextCap);
	VEC__FORCE_RESIZE(vec, nextCap);

	return VECTOR_OK;
}


VEC__FN VEC_STATUS FN_NS(_shrink_to_fit)(vec_T* vec) {
	VEC__NULL_CHECK(vec);

	if(UNLIKELY(vec->cap == vec->count)) {
		return  VECTOR_OK;
	}

	size_t nextCap = VEC__MAX(vec->count, VEC__MIN_CAP);
	VEC__FORCE_RESIZE(vec, nextCap);

	return VECTOR_OK;
}
#endif
#endif

#ifndef VECTOR_ALIGN_SIZE

VEC__FN VEC_STATUS FN_NS(_push_back)(vec_T* vec, val_T val) {
	VEC__NULL_CHECK(vec);
	VEC__ADD_OVERFLOW_CHECK(SIZE_MAX, vec->count, 1);
	
	VEC__MAYBE_RESIZE_ONE(vec);

	int succ;
	VEC__T_INIT(vec->ctx, vec->data + vec->count, &val, &succ);
	VEC__T_INIT_CHECK(succ, (void)0 );

	++vec->count;

	return VECTOR_OK;
}

#endif

VEC__FN VEC_STATUS FN_NS(_push_back_ptr)(vec_T* vec, val_T* val) {
	VEC__NULL_CHECK(vec);
	VEC__NULL_CHECK(val);
	VEC__ADD_OVERFLOW_CHECK(SIZE_MAX, vec->count, 1);
	
	VEC__MAYBE_RESIZE_ONE(vec);

	int succ;
	VEC__T_INIT(vec->ctx, vec->data + vec->count, val, &succ);
	VEC__T_INIT_CHECK(succ, (void)0);

	++vec->count;

	return VECTOR_OK;
}

VEC__FN VEC_STATUS FN_NS(_push_back_arr)(vec_T* vec, val_T* valArr, size_t count) {
	VEC__NULL_CHECK(vec);
	VEC__NULL_CHECK(valArr);
	VEC__ADD_OVERFLOW_CHECK(SIZE_MAX, vec->count, count);
	
	VEC__MAYBE_RESIZE_N(vec, count);

	int succ;
	VEC__T_INIT_N(vec->ctx, vec->data + vec->count, valArr, count, &succ);
	VEC__T_INIT_CHECK(succ, (void)0);

	vec->count += count;

	return VECTOR_OK;
}

#ifndef VECTOR_ALIGN_SIZE 

VEC__FN VEC_STATUS FN_NS(_insert)(vec_T* vec, val_T val, size_t at) {
	VEC__NULL_CHECK(vec);
	VEC__MAX_CHECK(at, vec->count);

	VEC__ADD_OVERFLOW_CHECK(SIZE_MAX, vec->count, 1);
	
	VEC__MAYBE_RESIZE_ONE(vec);

	val_T* base = vec->data + at;
	size_t tail = vec->count - at;

	VEC__T_MMOV(base + 1, base, tail);

	int succ;
	VEC__T_INIT(vec->ctx, base, &val, &succ);
	VEC__T_INIT_CHECK(succ, VEC__T_MMOV(base, base + 1, tail));

	++vec->count;

	return VECTOR_OK;
}

#endif


VEC__FN VEC_STATUS FN_NS(_insert_ptr)(vec_T* vec, val_T* val, size_t at) {
	VEC__NULL_CHECK(vec);
	VEC__NULL_CHECK(val);
	VEC__MAX_CHECK(at, vec->count);

	VEC__ADD_OVERFLOW_CHECK(SIZE_MAX, vec->count, 1);
	
	VEC__MAYBE_RESIZE_ONE(vec);

	val_T* base = vec->data + at;
	size_t tail = vec->count - at;

	VEC__T_MMOV(base + 1, base, tail);

	int succ;
	VEC__T_INIT(vec->ctx, base, val, &succ);
	VEC__T_INIT_CHECK(succ, VEC__T_MMOV(base, base + 1, tail));

	++vec->count;

	return VECTOR_OK;
}

VEC__FN VEC_STATUS FN_NS(_insert_arr)(vec_T* vec, val_T* valArr, size_t count, size_t at) {
	VEC__NULL_CHECK(vec);
	VEC__NULL_CHECK(valArr);
	VEC__MAX_CHECK(at, vec->count);
	VEC__ADD_OVERFLOW_CHECK(SIZE_MAX, vec->count, count);

	VEC__MAYBE_RESIZE_N(vec, count);

	val_T* base = vec->data + at;
	size_t tail = vec->count - at;

	VEC__T_MMOV(base + count, base, tail);

	int succ;
	VEC__T_INIT_N(vec->ctx, base, valArr, count, &succ);
	VEC__T_INIT_CHECK(succ, VEC__T_MMOV(base, base + count, tail));

	vec->count += count;

	return VECTOR_OK;
}

#ifndef VECTOR_ALIGN_SIZE

VEC__FN VEC_STATUS FN_NS(_replace)(vec_T* vec, val_T val, size_t at) {
	VEC__NULL_CHECK(vec);
	VEC__UNDERFLOW_CHECK(vec->count, 1);
	VEC__MAX_CHECK(at, vec->count - 1);

	val_T* base = vec->data + at;
	
#ifndef VECTOR_NO_ERR_T_INIT

	val_T temp;

	int succ;
	VEC__T_INIT(vec->ctx, &temp, &val, &succ);
	VEC__T_INIT_CHECK(succ, (void)0);

	VEC__T_DEINIT(vec->ctx, base);

	VEC__T_MOVE(base, &temp);

#else

	VEC__T_DEINIT(vec->ctx, base);

	int succ;
	VEC__T_INIT(vec->ctx, base, &val, &succ);

#endif

	return VECTOR_OK;
}

#endif

#if !defined(VECTOR_ALIGN_SIZE) || defined(VECTOR_NO_ERR_T_INIT)
VEC__FN VEC_STATUS FN_NS(_replace_ptr)(vec_T* vec, val_T* val, size_t at) {
	VEC__NULL_CHECK(vec);
	VEC__UNDERFLOW_CHECK(vec->count, 1);
	VEC__MAX_CHECK(at, vec->count - 1);

	val_T* base = vec->data + at;
	
#ifndef VECTOR_NO_ERR_T_INIT

	val_T temp;

	int succ;
	VEC__T_INIT(vec->ctx, &temp, val, &succ);
	VEC__T_INIT_CHECK(succ, (void)0);

	VEC__T_DEINIT(vec->ctx, base);

	VEC__T_MOVE(base, &temp);

#else
	VEC__T_DEINIT(vec->ctx, base);

	int succ;
	VEC__T_INIT(vec->ctx, base, val, &succ);

#endif

	return VECTOR_OK;
}
#endif

#if !defined(VECTOR_DEBLOAT) && !defined(VECTOR_ALIGN_SIZE)
VEC__FN VEC_STATUS FN_NS(_swap)(vec_T* vec, size_t at1, size_t at2) {
	VEC__NULL_CHECK(vec);
	VEC__UNDERFLOW_CHECK(vec->count, 2);
	VEC__MAX_CHECK(at1, vec->count - 1);
	VEC__MAX_CHECK(at2, vec->count - 1);

	if(UNLIKELY(at1 == at2)) {
		return VECTOR_OK;
	}

	val_T temp;
	VEC__T_MOVE(&temp, vec->data + at1);
	VEC__T_MOVE(vec->data + at1, vec->data + at2);
	VEC__T_MOVE(vec->data + at2, &temp);

	return VECTOR_OK;
}
#endif

#ifndef VECTOR_DEBLOAT

VEC__FN VEC_STATUS FN_NS(_clear)(vec_T* vec) {
	VEC__NULL_CHECK(vec);
	
	VEC__T_DEINIT_N(vec->ctx, vec->data, vec->count);

	vec->count = 0;

	return VECTOR_OK;
}

#endif

VEC__FN VEC_STATUS FN_NS(_pop)(vec_T* vec) {
	VEC__NULL_CHECK(vec);
	VEC__UNDERFLOW_CHECK(vec->count, 1);
	
	VEC__T_DEINIT(vec->ctx, vec->data + vec->count - 1);

	--vec->count;
	return VECTOR_OK;
}

VEC__FN VEC_STATUS FN_NS(_pop_n)(vec_T* vec, size_t n) {
	VEC__NULL_CHECK(vec);
	VEC__UNDERFLOW_CHECK(vec->count, n);
	
	VEC__T_DEINIT_N(vec->ctx, vec->data + vec->count - n, n);

	vec->count -= n;
	return VECTOR_OK;
}

VEC__FN VEC_STATUS FN_NS(_pop_at)(vec_T* vec, size_t at) {
	VEC__NULL_CHECK(vec);
	VEC__UNDERFLOW_CHECK(vec->count, 1);
	VEC__MAX_CHECK(at, vec->count - 1);

	val_T* base = vec->data + at;

	VEC__T_DEINIT(vec->ctx, base);
	VEC__T_MMOV(base, base + 1, vec->count - at - 1);

	--vec->count;
	return VECTOR_OK;
}

#ifndef VECTOR_DEBLOAT
VEC__FN VEC_STATUS FN_NS(_pop_n_at)(vec_T* vec, size_t n, size_t at) {
	VEC__NULL_CHECK(vec);
	VEC__UNDERFLOW_CHECK(vec->count, n);
	VEC__MAX_CHECK(at, vec->count - n);

	val_T* base = vec->data + at;

	VEC__T_DEINIT_N(vec->ctx, base, n);

	VEC__T_MMOV(base, base + n, vec->count - (at + n));

	vec->count -= n;

	return VECTOR_OK;
}
#endif

VEC__FN VEC_STATUS FN_NS(_detach_back)(vec_T* vec, val_T* out) {
	VEC__NULL_CHECK(vec);
	VEC__NULL_CHECK(out);
	VEC__UNDERFLOW_CHECK(vec->count, 1);

	VEC__T_MOVE(out, vec->data + vec->count - 1);

	--vec->count;

	return VECTOR_OK;
}

VEC__FN VEC_STATUS FN_NS(_detach_at)(vec_T* vec, val_T* out, size_t at) {
	VEC__NULL_CHECK(vec);
	VEC__NULL_CHECK(out);
	VEC__UNDERFLOW_CHECK(vec->count, 1);
	VEC__MAX_CHECK(at, vec->count - 1);
	
	val_T* base = vec->data + at;

	VEC__T_MOVE(out, base);

	VEC__T_MMOV(base, base + 1, vec->count - at - 1);

	--vec->count;
	return VECTOR_OK;

}

#ifndef VECTOR_ALIGN_SIZE
VEC__FN VEC_STATUS FN_NS(_at)(const vec_T* vec, val_T* out, size_t at) {
	VEC__NULL_CHECK(vec);
	VEC__NULL_CHECK(out);
	VEC__UNDERFLOW_CHECK(vec->count, 1);
	VEC__MAX_CHECK(at, vec->count - 1);

	VEC__T_MOVE(out, vec->data + at);
	return VECTOR_OK;

}
#endif

VEC__FN VEC_STATUS FN_NS(_at_ptr)(vec_T* vec, val_T** out, size_t at) {
	VEC__NULL_CHECK(vec);
	VEC__NULL_CHECK(out);
	VEC__UNDERFLOW_CHECK(vec->count, 1);
	VEC__MAX_CHECK(at, vec->count - 1);

	*out = vec->data + at;
	return VECTOR_OK;

}

#ifndef VECTOR_DEBLOAT
VEC__FN VEC_STATUS FN_NS(_get_count)(const vec_T* vec, size_t* out) {
	VEC__NULL_CHECK(vec);
	*out = vec->count;
	return VECTOR_OK;
}

VEC__FN VEC_STATUS FN_NS(_get_cap)(const vec_T* vec, size_t* out) {
	VEC__NULL_CHECK(vec);
#ifndef VECTOR_STATIC_SIZE
	*out = vec->cap;
#else
	*out = VECTOR_STATIC_SIZE;
#endif
	return VECTOR_OK;
}
#endif

VEC__FN size_t FN_NS(_count)(const vec_T* vec) {
	VEC__LOG_ASSERT(vec != NULL, "NULL ERROR FOR VECTOR");
	return vec->count;
}

VEC__FN size_t	   FN_NS(_cap)(const vec_T* vec) {
	VEC__LOG_ASSERT(vec != NULL, "NULL ERROR FOR VECTOR");
#ifndef VECTOR_STATIC_SIZE
	return vec->cap;
#else
	(void) vec;
	return VECTOR_STATIC_SIZE;
#endif
}

#ifndef VECTOR_DEBLOAT
VEC__FN size_t FN_NS(_cap_left)(const vec_T* vec) {
	VEC__LOG_ASSERT(vec != NULL, "NULL ERROR FOR VECTOR");
#ifndef VECTOR_STATIC_SIZE
	return vec->cap - vec->count;
#else
	(void) vec;
	return VECTOR_STATIC_SIZE - vec->count;
#endif
}

#endif
#endif //IMPLEMENTATION
//IMPLEND

#endif // VECTOR_T
//HEADEREND

//VECTOR specific undefines
#undef vec_T
#undef val_T
#undef VECTOR_STATUS_TYPES
#undef VEC__NEXT_CAP
#undef VEC__FORCE_RESIZE
#undef VEC__MAYBE_RESIZE_N
#undef VEC__MAYBE_RESIZE_ONE
#undef VEC__MIN_CAP
#undef VEC__GROWTH_FAC

//UNDEFINESSTART
#undef VECTOR_IMPLEMENTATION
#undef VECTOR_IMPL

//UDHEADSTART---------------------------------------
//public
///
#undef VECTOR_T
///
#undef VECTOR_NS
#undef VECTOR_FN_NS

//private
#undef FN_NS

#undef VECTOR_T
#undef VECTOR_NS
#undef VECTOR_FN_NS
#undef T
//UDHEADEND-----------------------------------------

//UDALLOCATORSSTART---------------------------------
//public
#undef VECTOR_ALLOCATORS_STATEFUL
#undef VECTOR_ALIGN_SIZE
#undef VECTOR_ALLOC
#undef VECTOR_FREE
#undef VECTOR_NO_FREE

#undef VECTOR_REALLOC
#undef VECTOR_CTX_T
#undef VECTOR_NO_RESIZE
#undef VECTOR_STATIC_SIZE

//private
#undef VEC__ALLOC
#undef VEC__FREE
#undef VEC__CTX
#undef VEC__CTX_PARAM
#undef VEC__NO_REALLOC

#undef VEC__REALLOC
//UDALLOCATORSEND-----------------------------------

//UDERRORSSTART-------------------------------------
//public
#undef VECTOR_ASSERT
#undef VECTOR_ASSERT_ALWAYS
#undef VECTOR_ERR_LOG
#undef VECTOR_ERR_ABORT
#undef VECTOR_NO_UNREACHABLE

#undef VECTOR_NO_ERR
#undef VECTOR_ERR_LVL
#undef VECTOR_NO_ERR_NULL
#undef VECTOR_NO_ERR_BOUNDS
#undef VECTOR_NO_ERR_UNDERFLOW
#undef VECTOR_NO_ERR_OVERFLOW

#undef VECTOR_NO_ERR_ALLOC

///
#undef VECTOR_NO_ERR_T_INIT
///

//private
#undef VEC__ASSERT
#undef VEC__LOG
#undef VEC__LOG_ASSERT
#undef VEC__ASR_OR_RET

#undef VEC__UNREACHABLE
#undef VEC__HAPPY_PATH

#undef VEC__NULL_CHECK

#undef VEC__MIN_CHECK
#undef VEC__MAX_CHECK
#undef VEC__BOUNDS_CHECK

#undef VEC__UNDERFLOW_CHECK
#undef VEC__ADD_OVERFLOW_CHECK
#undef VEC__MUL_OVERFLOW_CHECK

#undef VEC__ALLOC_CHECK
///
#undef VEC__T_INIT_CHECK
///
//UDERRORSEND---------------------------------------

//UDHELPERSSTART------------------------------------
//public
#undef VECTOR_DO_STIN
#undef VECTOR_NO_BRANCH_HINTS
#undef VECTOR_NO_STATIC_ASSERT

//private
#undef VEC__MIN
#undef VEC__MAX
#undef VEC__STIN
#undef VEC__FN
#undef VEC__RESTRICT
#undef LIKELY
#undef UNLIKELY

#undef VEC__STATIC_ASSERT

#undef VEC__XSTR
#undef VEC__STR
#undef VEC__XCAT
#undef VEC__CAT

#undef VEC__ARGC
#undef VEC__ARGC_
#undef VEC__GET_1
#undef VEC__GET_1_
#undef VEC__GET_2
#undef VEC__GET_2_
#undef VEC__GET_3
#undef VEC__GET_3_
#undef VEC__GET_4
#undef VEC__GET_4_
//UDHELPERSEND--------------------------------------

//UDSEMANTICSSTART----------------------------------
///
//public
#undef VECTOR_T_INIT
#undef VECTOR_T_MOVE
#undef VECTOR_T_DEINIT

//private
#undef VEC__T_XINIT

#undef VEC__T_INIT
#undef VEC__T_INIT_N

#undef VEC__T_MOVE
#undef VEC__T_MCPY
#undef VEC__T_MMOV
#undef VEC__T_MSET

#undef VEC__T_DEINIT
#undef VEC__T_DEINIT_N
///
//UDSEMANTICSEND------------------------------------
//UNDEFINESEND
