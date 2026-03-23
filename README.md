# vector_T.h

a compiletime configurable, single header, dynamic/static array/stack written in C (99).<br>
A cut-out of a library I am writing, most of it is just merged headers and query replaced names.

## Features
- comptime type + namespace
- comptime custom alocators
  - *global* as well as *stateful* + aligned allocators support (for both)
  - *statically* allocated/ comptime sized
- automatic memory manegement, defined at comptime for the type
- support for non trivially movable types
- comptime configurable, fully covered, and safe error handling
- c++ compilers supported

## Usage
Here is a small snippet. Else look into all of the [examples](https://github.com/nikita-edel/vector_T/tree/main/examples), which are also serving as documentation.
```c++
#include <stdint.h>
#include <stdio.h>

#define T int32_t, veci32
#define VECTOR_IMPL
#include "vector_T.h"

int main(void) {
  veci32 ints;
  if (veci32_init(&ints, 16)) {
    fprintf("Out of Memory, couldn't allocate vector\n");
    return -1;
  }

  for(int32_t i = 0; i < 32; ++i) {
    veci32_push_back(&ints, i);
  }

  vector_foreach(&ints, int32_t i) {
    printf("%d\n", i);
  }

  veci32_deinit(&ints);
  return 0;
}
```

## Performance
0 Overhead and as fast as possible. very easily vectorizable.<br>
look into the [benchmarks](https://github.com/nikita-edel/vector_T/blob/main/benchmarks.txt) compared to libc++ and notice that it performs as fast (and faster in debug/non optimized builds, test it yourself).<br>
snippets (-O3):<br>
C:
```
operation            type               ns/op       total_ops
----------------------------------------------------------------
push_back_1          uint64_t            0.19     10446164116
push_back_64         uint64_t            0.40      5100274176
insert_front         uint64_t           42.54        47018965
insert_64_front      uint64_t            0.69      2952790528
replace              uint64_t            0.37      5432979957
```
lib C++:
```
operation            type               ns/op       total_ops
----------------------------------------------------------------
push_back_1          uint64_t            0.21      9505661667
push_back_64         uint64_t            0.41      4966056448
insert_front         uint64_t           43.27        46259254
insert_64_front      uint64_t            1.24      1677722112
replace              uint64_t            0.17     12007243784
```
