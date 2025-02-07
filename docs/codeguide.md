<!--
SPDX-PackageName: "ACTS"
SPDX-FileCopyrightText: 2016 CERN
SPDX-License-Identifier: MPL-2.0
-->

# Code guidelines

The following guidelines must be followed by all new code. Existing code that does not yet follow should be adapted when possible.
You might disagree with some guidelines, but in a large code base as this one consistency is more important than personal opinion.
All guidelines have a short identifier label, e.g. N.1, for easier reference in discussions.

For cases and constructs not explicitly mentioned here, code should fall back to the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines).

## ACTS-specific

### A.indices: Always use enum values to access vector/matrix components

Always use the appropriate enum values to access components. This clearly states
the semantic meaning and is easier to understand.

Example:

```cpp
// local surface coordinate vector
Vector2 loc;
loc[ePos0] = 2.0;

// space-time coordinate vector
Vector4 pos4;
pos4[ePos0] = 0.1;
...
pos4[eTime] = 12.3;

// bound parameters vector
BoundVector pars;
pars[eBoundLoc0] = 1.2;
...
pars[eBoundPhi] = std::numbers::pi;
...
```

**Do not** use plain numbers as this can lead to inconsistencies if the order
changes. It reduces the readability and makes it harder to spot errors.

**Do not** use the `.{x,y,z,w}()` accessors available for small fixed-size
vectors. While this should produce correct results in most cases, the accessor
name does not always match the semantic meaning, e.g. for momentum vectors, and
it only works for vectors but not for matrices. Always use the entries in
`enum CoordinateIndices` for a consistent way of accessing both vectors and matrices.

## Naming

### N.1: Namespaces use CamelCase

Namespaces use CamelCase with an upper case initial.

Example:

```cpp
namespace Something {
...
}
```

The only exception is the nested `detail` namespace with always uses the lower-case name. There is no particular strong reason but existing practice for this. The `detail` namespace must be never be the top-most namespace.

Example:

```cpp
namespace Foo {
namespace detail {
...
}
}
```

### N.2: Concrete types use CamelCase

All concrete types, i.e. classes, structs, enums, and typedefs for both fundamental and external types, use CamelCase with an upper case initial.

Example:

```cpp
class ComplicatedNamedClass;
struct AnotherStruct;
enum EnumType;
using SemanticName = int32;
```

Prefer semantic names over descriptive names. This applies in particular to typedefs of external types or standard containers. Types are checked by the compiler, while semantic meaning is only checked by the user; optimize for the latter.

Example:

```cpp
// avoid: purely descriptive typename
using ConstThingPtrVector = std::vector<const Thing*>;
// prefer: semantic typename the describes the usage
using InputThings = std::vector<const Thing*>;
```

Avoid abbreviations and prefer full names. If abbreviations must be used, only use upper case for the initial letter of the abbreviation. The abbreviation is semantically a single word and this provides better visual consistency with the CamelCase naming.

Example:

```cpp
// VertexSomething should be abbreviated as
struct VtxSomething; // not VTXSomething.
// This is bad example since the name should not be abbreviated here anyway.
```

### N.3: Functions and methods use mixedCase

All functions and methods use mixedCase with a lower case initial. This applies also to private helper functions, e.g. in the `detail` namespace. These are not fundamentally different from other functions and do not warrant a separate convention just by virtue of being e.g. in the `detail` namespace.

Example:

```cpp
doSomething(...);
thing.doSomething(...);
detail::anImplementationDetail(...);
```

### N.4: Variables use mixedCase with appropriate prefix

Local variables, including function arguments, and public member variables use mixedCase with lower case initial.

Example:

```cpp
int aVariable;
struct Thing {
  double aPublicVariable;
  float anotherPublicOne;
};
```

Private member variables have an additional `m_` prefix.

Example:

```cpp
class Foo {
private:
  m_privateMember;
};
```

Static variables, i.e. local, global, and member ones, have an additional `s_` prefix to mark them as the satanic tools that they are.

```cpp
static int s_thisIsBadAndYouShouldFeelBad = -42;
class Bar {
    static double s_notMuchBetter = 3.14;
};
```

### N.5: Constant values use kCamelCase

Variables representing constant values use CamelCase with a `k` prefix. Only variables marked as `constexpr` are considered constant values.

Example:

```cpp
static constexpr double kMagic = 1.23;
```

Variables defined in the `Acts::UnitConstants` namespace are exempted for usability reasons and use regular variable naming instead.

### N.6: Enum values use eCamelCase

Enum values use CamelCase with a `e` prefix. They are not really constants but symbolic values, e.g. they can never have an address, and warant a separate convention.

Example:

```cpp
enum Bar {
  eBla,
  eBlub,
};
```

### N.7: Template parameter types use snake_case_t

Template parameter types use lower case words, separated by underscores, with a `_t` suffix. The separate naming convention from concrete types clarifies that these are variable types and avoids naming collisions when reexporting the type.

Example:

```cpp
template<typename large_vector_t>
struct Algorithm {
  // make the template type publicly available
  using LargeVector = large_vector_t;
  ...
};
```

### N.8: Macros use ACTS_ALL_UPPER_CASE

Do not use macros. You want to use macros? Do not use them! Are you even listening? Ok, you are not: macros use `UPPER_CASE` with underscores as word separators and a mandatory `ACTS_` prefix.

Example:

```
#define ACTS_I_DID_NOT_LISTEN(...) ...
```

### N.9: Files use CamelCase

Files use CamelCase with upper case initial. If the file defines a single class/struct, the filename must match the typename. Otherwise, a common name describing the shared intent should be used.

Source files use the `.cpp` extension, Header files use the `.hpp` extension, inline implementation files use the `.ipp` extensions.

Files that contain only symbols in the `detail` namespace should be moved in a`detail/` directory in the module directory. This should not be done for corresponding source files.

## Formatting

See [](howto_format)
