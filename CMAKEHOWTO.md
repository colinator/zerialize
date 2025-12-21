Zerialize CMake How‑To

Overview
- Zerialize is a header‑only C++ library with optional protocol backends and math backends.
- Zerialize also includes a built-in, dependency-free protocol backend: `Zer` (`include/zerialize/protocols/zer.hpp`).
- Dependencies are reused if your toolchain already provides CMake targets; otherwise they are fetched via FetchContent.
- You can selectively enable or disable each backend via CMake options.
- Zerialize optionally supports modules for C++20 and onwards, as module `zerialize`.

Configure Options
- ZER (zer): Built-in protocol backend with no external dependencies. Always available (no CMake option).
- ZERIALIZE_ENABLE_FLEXBUFFERS: Enable FlexBuffers (via FlatBuffers). Default: ON
- ZERIALIZE_ENABLE_JSON: Enable JSON (via yyjson). Default: ON
- ZERIALIZE_ENABLE_CBOR: Enable CBOR (via jsoncons). Default: ON
- ZERIALIZE_ENABLE_MSGPACK: Enable MessagePack (via msgpack-c). Default: ON
- ZERIALIZE_ENABLE_XTENSOR: Enable xtensor math backend. Default: ON
- ZERIALIZE_ENABLE_EIGEN: Enable Eigen math backend. Default: ON
- ZERIALIZE_ENABLE_MODULES: Enable C++ modules. Default: OFF

How Dependencies Are Resolved
- FlatBuffers: Uses existing `flatbuffers` or `flatbuffers::flatbuffers` include dirs if present; otherwise fetched (header-only usage via include dirs).
- yyjson: Uses existing `yyjson` or `yyjson::yyjson` if present; otherwise fetched. Internally normalized to `zerialize_yyjson`.
- msgpack-c: Reuses any of `msgpack-c`, `msgpackc-cxx`, or `msgpackc`; otherwise fetched. Internally normalized to `zerialize_msgpack`.
- jsoncons (CBOR): Uses existing `jsoncons` or `jsoncons::jsoncons`; otherwise fetched header‑only and added via include dirs.
- xtensor: Uses existing `xtensor` target if present; otherwise fetches `xtl`, `xsimd`, and `xtensor` headers and wires them up as INTERFACE include directories (no upstream CMake build required).
- Eigen: Tries `Eigen3::Eigen` via find_package; otherwise fetches headers and creates an imported `eigen` interface target.

Compile Definitions Exposed
- ZERIALIZE_HAS_ZER: Always defined (built-in Zer protocol is available).
- ZERIALIZE_HAS_FLEXBUFFERS: Defined when FlexBuffers support is enabled.
- ZERIALIZE_HAS_JSON: Defined when JSON support is enabled.
- ZERIALIZE_HAS_CBOR: Defined when CBOR support is enabled.
- ZERIALIZE_HAS_MSGPACK: Defined when MessagePack support is enabled.
- ZERIALIZE_USE_XTENSOR: Defined when xtensor backend is enabled.
- ZERIALIZE_USE_EIGEN: Defined when Eigen backend is enabled.

Typical Integration
Option A: Add as a subdirectory (recommended for in‑tree builds)
```
add_subdirectory(path/to/zerialize)
target_link_libraries(my_app PRIVATE zerialize)
```

Option B: As a subproject with selective features
```
set(ZERIALIZE_ENABLE_JSON OFF CACHE BOOL "" FORCE)
set(ZERIALIZE_ENABLE_MSGPACK OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/zerialize)
```

Option C: Rely on system/toolchain packages
- Provide your own dependency targets before adding zerialize. For example:
```
find_package(yyjson CONFIG REQUIRED)          # provides yyjson::yyjson
find_package(Flatbuffers CONFIG REQUIRED)     # provides flatbuffers::flatbuffers
find_package(msgpack CONFIG REQUIRED)         # provides msgpack-c or msgpackc-cxx
find_package(jsoncons CONFIG)                 # optional header‑only
add_subdirectory(path/to/zerialize)
```
Zerialize will automatically reuse these targets and avoid fetching duplicates.

Disabling Protocols
Examples:
```
# Only FlexBuffers and CBOR
cmake -S . -B build \
  -DZERIALIZE_ENABLE_JSON=OFF \
  -DZERIALIZE_ENABLE_MSGPACK=OFF

# Disable all optional protocols (Zer is still available)
cmake -S . -B build \
  -DZERIALIZE_ENABLE_FLEXBUFFERS=OFF \
  -DZERIALIZE_ENABLE_JSON=OFF \
  -DZERIALIZE_ENABLE_CBOR=OFF \
  -DZERIALIZE_ENABLE_MSGPACK=OFF
```

Selecting Math Backends
```
# Only Eigen
cmake -S . -B build \
  -DZERIALIZE_ENABLE_XTENSOR=OFF \
  -DZERIALIZE_ENABLE_EIGEN=ON

# Neither (if your code paths don’t need them)
cmake -S . -B build \
  -DZERIALIZE_ENABLE_XTENSOR=OFF \
  -DZERIALIZE_ENABLE_EIGEN=OFF
```

Notes
- Zerialize is header‑only; linking against the target propagates include paths and compile definitions for enabled features.
- When using your own dependency targets, ensure they are visible before calling `add_subdirectory(zerialize)` so zerialize can reuse them.
- If you disable all optional protocols, `Zer` remains available; your code can `#ifdef` on the compile definitions accordingly.
- C++ modules are offered using CMake 3.28 and later. This requires a build generator that supports modules, such as Ninja.
