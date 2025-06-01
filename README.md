# zerialize

Zero-copy multi-format serialization/deserialization for C++.

## Goals

1.  Ergonomic and performant serialization and deserialization of C++ data structures and primitives.
2.  Support as many dynamic types as possible (JSON, Flexbuffers, MessagePack, etc).
3.  For underlying types that support it (FlexBuffers, MessagePack, JSON in some cases), provide support for zero-copy deserialization. For serialization, minimize copies.
4.  Support easy conversion between types.
5.  Transparently support serialization and deserialization into xtensor tensors and eigen matrices. Do this with zero-copy, if possible.
6.  Support serialization and deserialization into 'statically-typed' formats, such as Protobuf and Flatbuffers.

## Current support

*   JSON
*   Flexbuffers
*   MessagePack

## Building

This is a header-only library, and contains nothing to build. The `test/`, `benchmark/`, and `composite_example/` directories contain executables with examples of how to organize CMake projects.

## Usage Example

This example demonstrates serializing and deserializing a map containing an `xtensor` using `zkv` for optimized serialization.

There are many more examples in test/test_zerialize.cpp.

```cpp
#include <iostream>
#include "zerialize/zerialize.hpp"
#include "zerialize/protocols/json.hpp" // Or FlexSerializer, MsgpackSerializer
#include "zerialize/tensor/xtensor.hpp" // For xtensor support

int main() {
    // xtensor and eigen are zero-copy where possible.
    auto my_tensor = xt::xtensor<double, 2>{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};

    // --- Serialization ---
    // There are many more forms, including std::any-based forms (for ergonomics) of 
    // serialization. See test/test_zerialize.cpp for more. Below is the fastest.
    auto buffer = zerialize::serialize<zerialize::JsonSerializer>( // or flex or msgpack etc
        zerialize::zkv("id", 12345),
        zerialize::zkv("label", std::string("data_point_alpha")),
        zerialize::zkv("tensor_data", zerialize::xtensor::serializer<zerialize::JsonSerializer>(my_tensor))
    );

    // 'buffer' (a zerialize::ZBuffer) contains the serialized data. 
    // For this example contains json:
    // {
    //      "id", 12345,
    //      "label", "data_point_alpha",
    //      "tensor_data": [
    //          [ 2, 3 ],
    //          11,
    //          "base64 encoded rep of the binary tensor data"
    //      ]
    // }


    // --- Deserialization ---
    zerialize::JsonDeserializer deserialized_data(buffer.buf()); 

    // Accessing values:
    int id = deserialized_data["id"].asInt32();
    std::string label = deserialized_data["label"].asString();
    xt::xtensor<double, 2> deserialized_tensor = zerialize::xtensor::asXTensor<double, 2>(deserialized_data["tensor_data"]);

    return 0;
}

```
This example serializes a map with an `xtensor` and then deserializes it.
Key parts:
*   **Serialization**: `zerialize::serialize<SerializerType>(zmap(zkv(...)))`
*   **Tensor Handling**: `zerialize::xtensor::serializer<SerializerType>(tensor)` for serialization, and `zerialize::xtensor::asXTensor<type, rank>(value)` for deserialization.
*   **Deserialization**: Construct `SerializerType::BufferType` (e.g., `JsonDeserializer`) from `zbuffer.buf()`, then use `[]` and `asType()` methods.
