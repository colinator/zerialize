# <i>z</i>erialize

Zero-copy multi-format serialization/deserialization for c++20.

## Goals

1. **Ergonomic and performant** serialization and deserialization of C++ data structures and primitives.
2. **Support as many dynamic protocols as possible** (JSON, Flexbuffers, MessagePack, CBOR. More to come).
3. For underlying protocols that support it (FlexBuffers, MessagePack, CBOR, JSON in some cases), provide support for **zero-copy, zero-work, lazy deserialization**. For serialization, minimize copies.
4. Support **easy conversion between protocols**.
5. Transparently support serialization and deserialization into **xtensor tensors and eigen matrices**. Do this with zero-copy, if possible.
6. Still to come: support serialization and deserialization into 'statically-typed' formats, such as Protobuf and Flatbuffers.

## Current Support

*   **JSON** (via yyjson)
*   **Flexbuffers** (Google's schema-less binary format)
*   **MessagePack** (compact binary serialization)
*   **CBOR** (via jsoncons)
*   **NOTE:** Zerialize supports the least-common-denominator of serializeable objects: arrays, maps with string keys, and primitives: integers, floats, strings, but also blobs.

## Building

This is a **header-only library**, and contains nothing to build. Well, sort of: the core library is header-only. Each supported protocol relies on 3rd-party libraries, which may or may not be header-only. Also, if using modules, the project must be built. The `examples/`, `test/`, and `benchmark_compare/` directories contain executables with examples of how to organize CMake projects.

If using modules, the module is named `zerialize`.

See [CMAKEHOWTO.md](CMAKEHOWTO.md) for details.

## Usage

See `examples/` for the basics and `test/` for tests and many more examples, including xtensor, eigen, and custom structures.

The basics:

```cpp
#include <iostream>
#include <zerialize/zerialize.hpp>
#include <zerialize/protocols/json.hpp>
#include <zerialize/protocols/flex.hpp>
#include <zerialize/protocols/cbor.hpp>
#include <zerialize/translate.hpp>

namespace z = zerialize;

int main() {

    // Serialize and deserialize a map in Json format. 
    // Can also be z::Flex, z::MsgPack, z::CBOR, more to come.

    // Serialize into a data buffer.
    z::ZBuffer databuf = z::serialize<z::JSON>(
        z::zmap<"name", "age">("James Bond", 37)
    );
    
    // Get the raw bytes
    std::span<const uint8_t> rawBytes = databuf.buf();

    // Deserialize from a span or vector of bytes.
    auto d = z::JSON::Deserializer(rawBytes);

    // Read attributes dynamically and lazily. You provide the type.
    std::cout << "JSON AGENT " 
              << "agent name: " << d["name"].asString()
              <<       " age: " << d["age"].asUInt16() << std::endl;

    // Translate from one format to another.
    z::Flex::Deserializer f = z::translate<z::Flex>(d);
}
```

### Arrays and Nested Structures

```cpp
// Arrays are easy
auto array_buffer = zerialize::serialize<zerialize::JSON>(
    zerialize::zvec(1, 2, 3, "hello", true)
);

// Nesting? No problem!
auto nested_buffer = zerialize::serialize<zerialize::JSON>(
    zerialize::zmap<"users", "metadata">(
        zerialize::zvec(
            zerialize::zmap<"id", "name">(1, "Alice"),
            zerialize::zmap<"id", "name">(2, "Bob")
        ),
        zerialize::zmap<"version", "timestamp">(
            "1.0", 
            1234567890
        )
    )
);
```

### A note on blobs

Blobs are stored as 'blobs' in protocols that support this (flex, msgpack). Protocols that don't (JSON) store blobs as arrays of ["~b", <base64-encoded data as a string>, "base64"]

### Working with Tensors (xtensor)

Zerialize has first-class support for xtensor with zero-copy where possible.

Tensors (both xtensor and eigen) are stored as arrays of size 3, where the type code is the numpy-compatible primitive type code. See include/zerialize/tensor/utils.hpp.

    [type code, [dimension 1 size, dimension 2 size, etc], blob]


```cpp
#include <zerialize/tensor/xtensor.hpp>
#include <xtensor/xtensor.hpp>

int main() {
    // Create a tensor
    auto my_tensor = xt::xtensor<double, 2>{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    
    // Serialize it alongside other data
    auto buffer = zerialize::serialize<zerialize::JSON>(
        zerialize::zmap<"experiment_id", "data", "timestamp">(
            "exp_001",
            my_tensor,  // Tensor serialized directly!
            1234567890
        )
    );
    
    // Deserialize
    zerialize::JSON::Deserializer deserializer(buffer.buf());
    
    std::string exp_id = deserializer["experiment_id"].asString();
    auto restored_tensor = zerialize::xtensor::asXTensor<double, 2>(deserializer["data"]);
    int64_t timestamp = deserializer["timestamp"].asInt64();
    
    // restored_tensor == my_tensor (element-wise)
    
    return 0;
}
```

### Working with Eigen Matrices

```cpp
#include <zerialize/tensor/eigen.hpp>
#include <Eigen/Dense>

int main() {
    // Create an Eigen matrix
    Eigen::Matrix<double, 3, 2> eigen_mat;
    eigen_mat << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0;
    
    // Serialize
    auto buffer = zerialize::serialize<zerialize::MsgPack>(
        zerialize::zmap<"matrix", "label">(eigen_mat, "test_matrix")
    );
    
    // Deserialize
    zerialize::MsgPack::Deserializer deserializer(buffer.buf());
    auto restored_matrix = zerialize::eigen::asEigenMatrix<double, 3, 2>(
        deserializer["matrix"]
    );
    
    return 0;
}
```

### Deserialization API

All deserializers provide a consistent interface:

```cpp
// Create deserializer from buffer
zerialize::JSON::Deserializer data(buffer.buf());

// Type checking
if (data.isMap()) {
    // Access map elements
    auto value = data["key"];
    
    // Iterate over keys
    for (auto key : data.mapKeys()) {
        std::cout << "Key: " << key << std::endl;
    }
}

if (data.isArray()) {
    // Access array elements
    auto first = data[0];
    std::size_t size = data.arraySize();
}

// Type conversion
int32_t i = data.asInt32();
double d = data.asDouble();
std::string s = data.asString();
bool b = data.asBool();
```

### Cross-Format Translation

Convert between formats effortlessly:

```cpp
// Serialize to JSON
auto json_buffer = zerialize::serialize<zerialize::JSON>(
    zerialize::zmap<"data">(zerialize::zvec(1, 2, 3))
);

// Deserialize from JSON
zerialize::JSON::Deserializer json_data(json_buffer.buf());

// Translate to MessagePack
auto msgpack_data = zerialize::translate<zerialize::MsgPack>(json_data);

// Now you have the same data in MessagePack format!
```

### Multiple Protocols

Switch between protocols by changing the template parameter:

```cpp
// Same data structure, different formats
auto json_buf = zerialize::serialize<zerialize::JSON>(my_data);
auto flex_buf = zerialize::serialize<zerialize::Flex>(my_data);
auto msgpack_buf = zerialize::serialize<zerialize::MsgPack>(my_data);

// All contain the same logical data, just in different binary formats
```

## Key Features

- **Compile-time keys**: `zmap<"key1", "key2">()` generates no runtime string allocations
- **Zero-copy deserialization**: Where supported by the underlying format
- **Tensor support**: First-class xtensor and Eigen integration
- **Format agnostic**: Write once, serialize to any supported format
- **Header-only**: No build dependencies, just include and go
- **Exception-safe**: Clear error messages when things go wrong
- **Modules support**: Offers support for modules, for C++20 onwards

## Advanced Usage

For more complex scenarios, see `test/test_zerialize.cpp` which contains comprehensive examples including:

- Custom type serialization via ADL
- Binary blob handling
- Unicode string support
- Large array serialization
- Cross-format translation patterns

## Why Zerialize?

Zerialize is the fastest multi-format dynamic serialization library, and the only one to offer zero-copy, zero-work deserialization (if supported by the underlying protocols).
