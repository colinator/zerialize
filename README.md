# zerialize

Zero-copy multi-format serialization/deserialization for c++.

## Goals

1. Ergonomic and performant serialization and deserialization of c++ data structures and primitives.
2. Support as many dynamic types as possible (JSON, Flexbuffers, MessagePack, etc).
3. For underlying types that support it (FlexBuffers, MessagePack, JSON in some cases), provided support for zero-copy deserialization. For serialization, minimize copies.
4. Support easy conversion between types, at least to the extent it's possible.
5. Transparently support serialization and deserialization into xtensor tensors and eigen matrices. Do this with zero-copy, if possible.
6. Support serialization and deserialization into 'statically-typed' formats, such as Protobuf and Flatbuffers.

## Current support

JSON

Flexbuffers

## Building

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

