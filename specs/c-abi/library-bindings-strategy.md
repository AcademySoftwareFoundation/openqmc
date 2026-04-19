[NEW MESSAGE]

I’ve got a C++ library. And I’d like to make bindings to other languages so more can use and access the library. These are the languages I’d like to provide access to:

C
Python
Zig
Rust

I’ve heard nanobind is a good option for Python. And for the others a C wrapper using macros.

That’s a solid plan. Here’s a breakdown of the common approach for each target language:

1. C

Create a minimal C wrapper around your C++ library.

This wrapper should expose a stable extern "C" API with plain structs, enums, and opaque handles.

Use macros carefully to hide versioning/ABI details.

This becomes the “portable” base layer for other languages.

2. Python

If you want direct, modern C++ bindings: use nanobind (cleaner, smaller, modern alternative to pybind11). This avoids the C layer and keeps C++ features (RAII, overloads, etc.).

If you want to reuse the C layer instead: you can use ctypes or cffi in Python, but that’s more verbose and loses some type expressiveness.

3. Zig

Zig has very strong C interop. You can import the C wrapper header (@cImport) directly.

Expose just enough functions/structs in the C layer for Zig users to work comfortably.

No need for a Zig-specific wrapper—Zig can consume the C API directly.

4. Rust

Rust also prefers going through C.

Generate bindings from the C wrapper using bindgen.

Wrap those unsafe bindings with a safe Rust API (newtypes, RAII, Result error handling, etc.).

General strategy
Core library in C++

Thin C wrapper as the universal FFI layer (stable ABI, portable headers).

Language-specific layers:

Python: nanobind (direct to C++), or via C wrapper if you want one unified FFI story.

Zig/Rust: consume the C wrapper, generate idiomatic bindings.

That way:

Only one stable ABI surface (C).

You avoid fighting each language’s quirks directly with C++ templates, exceptions, etc.

Python is the only exception where direct C++ binding is nicer.

Do you want to prioritise ease of maintenance (one C wrapper for all) or maximal language ergonomics (nanobind for Python, C for the rest)?


[NEW MESSAGE]

Should I be employing something like nanobind for the other languages?

No — nanobind (like pybind11) is very Python-specific. It generates CPython extension modules and manages reference counting, the GIL, NumPy interop, etc. None of that translates to Zig or Rust.

For the others:

C: no binding generator needed, you are writing the binding (a clean extern "C" API).

Zig: Zig already has first-class C interop; you just @cImport your C header. No equivalent of nanobind is needed.

Rust: you use bindgen to turn your C header into Rust FFI bindings, then wrap them in safe Rust code.

So:

Python → nanobind (direct C++ binding is worth it).

Everything else → go through your C wrapper.

This keeps things maintainable: only Python gets its own dedicated binding layer, the rest share the universal C ABI.

Would you like me to sketch what a minimal C wrapper would look like for your C++ library (with opaque handles, no templates/exceptions) so you’ve got a base to target?


[NEW MESSAGE]

So I should write the c binding myself and not use some other higher level solution

Exactly 👍

For C, Zig, and Rust you really do want to write the C wrapper yourself. A few reasons:

Stable ABI: Only C gives you a simple, predictable ABI that all those languages can interop with.

Control: You decide what’s exposed, hiding C++ templates, overloads, and exceptions.

Portability: Zig and Rust both “just work” with a C header. They don’t need or want something higher-level.

Maintenance: One thin C layer can serve as the universal interface, instead of trying to maintain per-language binding frameworks.

There are higher-level tools (e.g. SWIG, cppmm, cppbind, etc.), but they usually:

generate noisy or unidiomatic bindings,

don’t cover modern C++ well (templates, RAII, smart pointers, etc.),

make debugging/maintenance harder.

So the usual best practice is:

C++ core library (modern features, templates, etc.).

Hand-written C API (minimal, opaque handles, simple functions).

Language-specific bindings that consume the C API (Zig/Rust directly, Python optionally via C but usually better with nanobind).

Do you want me to outline the pattern for the C API (opaque struct handles, create/destroy functions, error codes) so you’ve got a template to follow?
