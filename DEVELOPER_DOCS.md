# Developer Docs

This is a small guide for developers wanting to contribute to the code base.

## C as a bridge between Go and C++

Go can integrate C code, but not C++ code. V8 is a C++ API, so it cannot
immediately be consumed by Go. So this code base is to a large part a
C-compatible subset of C++ be callable from Go. Many types defined here are
intended to carry information between Go and C++, and are restricted to C struct
types rather than C++ classes. When C/C++ pointers are transferred, the
underlying type often has two different forms, depending on whether it's Go or
C++ code being compiled.

### Same type, two declarations

As an example, `isolate.h` defines the type `v8Isolate`. When compiled in a C++
context, it's a `typedef` of `v8::Isolate`. When compiled in a C context, it's a
struct declaration.

Note that in both cases, the header file doesn't provide any information about
what the type contains. In C++ by using a forward declaration. Only pointer
values are passed to Go code. So Go code cannot access any data on an `Isolate`,
but it can pass the pointer value back to C code in a type-safe manner.

```c++
#ifndef V8GO_ISOLATE_H
#define V8GO_ISOLATE_H

#include "unbound_script.h"

#ifdef __cplusplus

namespace v8 {
class Isolate; 
}
typedef v8::Isolate v8Isolate;

extern "C" {
#else
typedef struct v8Isolate v8Isolate;
#endif

extern v8Isolate* NewIsolate();

#ifdef __cplusplus
}  // extern "C"
#endif
#endif
```

## `Value`s and `Handle<>`s.


