#pragma once

#include <any>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <span>
#include <concepts>


constexpr bool DEBUG_TRACE_CALLS = true;

namespace zerialize {

using std::string, std::string_view, std::vector, std::pair, std::runtime_error;
using std::any, std::initializer_list;
using std::convertible_to, std::same_as;

class DeserializationError : public runtime_error {
public:
    DeserializationError(const string& msg) : runtime_error(msg) { }
};

// The Deserializer concept defines a compile-time interface
// that child classes must satisfy. Each 'node' in a dynamic
// tree of Deserializer must define these operations, and
// are encouraged to do so in a zero-copy way, if possible.
template<typename V>
concept Deserializable = requires(const V& v, const string& key, size_t index) {

    // Type checking: everything that conforms to this concept needs
    // to be able to check it's type.
    { v.isInt8() } -> convertible_to<bool>;
    { v.isInt16() } -> convertible_to<bool>;
    { v.isInt32() } -> convertible_to<bool>;
    { v.isInt64() } -> convertible_to<bool>;
    { v.isArray() } -> convertible_to<bool>;
    { v.isUInt8() } -> convertible_to<bool>;
    { v.isUInt16() } -> convertible_to<bool>;
    { v.isUInt32() } -> convertible_to<bool>;
    { v.isUInt64() } -> convertible_to<bool>;
    { v.isFloat() } -> convertible_to<bool>;
    { v.isDouble() } -> convertible_to<bool>;
    { v.isString() } -> convertible_to<bool>;
    { v.isBool() } -> convertible_to<bool>;
    { v.isMap() } -> convertible_to<bool>;
    { v.isArray() } -> convertible_to<bool>;

    // Deserialization: everything that conforms to this concept needs
    // to be able to read itself as any type of value (which could fail
    // by throwing DeserializationError).
    { v.asInt8() } -> convertible_to<int8_t>;
    { v.asInt16() } -> convertible_to<int16_t>;
    { v.asInt32() } -> convertible_to<int32_t>;
    { v.asInt64() } -> convertible_to<int64_t>;
    { v.asUInt8() } -> convertible_to<uint8_t>;
    { v.asUInt16() } -> convertible_to<uint16_t>;
    { v.asUInt32() } -> convertible_to<uint32_t>;
    { v.asUInt64() } -> convertible_to<uint64_t>;
    { v.asFloat() } -> convertible_to<float>;
    { v.asDouble() } -> convertible_to<double>;
    { v.asString() } -> convertible_to<string_view>;
    { v.asBool() } -> convertible_to<bool>;

    // Composite handling: everything that conforms to this concept needs
    // to be able to index itself as a map...
    { v.mapKeys() } -> convertible_to<vector<string_view>>;
    { v[key] } -> same_as<V>;

    // ... or as an array
    { v.arraySize() } -> convertible_to<size_t>;
    { v[index] } -> same_as<V>;
};


// The DataBuffer class exists to:
// 1. enforce a common compile-time concept interface for derived types
// 2. enforce a common interface for root-node buffers (that are themselves
//    derived types, but also manage the actual byte buffer).
template <typename Derived>
class DataBuffer {
public:
    DataBuffer() {
        static_assert(Deserializable<Derived>, "Derived must satisfy Deserializable concept");
    }
    virtual const vector<uint8_t>& buf() const = 0;
    size_t size() const { return buf().size(); }
    virtual string to_string() const = 0;
};



// Trait to get serializer name at compile-time:
// each serializer must have unique name, defined
// in it's .hpp file
template<typename T>
struct SerializerName;


// Define various serializers: the 7 canonical forms.

template <typename SerializerType>
typename SerializerType::BufferType serialize() {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize()" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize();
}

template <typename SerializerType>
typename SerializerType::BufferType serialize(const any& value) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(any: " << value.type().name() << ")" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(value);
}

template <typename SerializerType>
typename SerializerType::BufferType serialize(initializer_list<any> list) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(initializer list)" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(list);
}

template <typename SerializerType>
typename SerializerType::BufferType serialize(initializer_list<pair<string, any>> list) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(initializer list/map)" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(list);
}

template<typename SerializerType, typename ValueType>
typename SerializerType::BufferType serialize(ValueType&& value) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(&&value)" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(std::forward<ValueType>(value));
}

template<typename SerializerType, typename... ValueTypes>
typename SerializerType::BufferType serialize(ValueTypes&&... values) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(&&values" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(std::forward<ValueTypes>(values)...);
}

template<typename SerializerType, typename... ValueTypes>
typename SerializerType::BufferType serialize(pair<const char*, ValueTypes>&&... values) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(&& value pairs)" << std::endl;
    }
    SerializerType serializer;
    return serializer.serializeMap(std::forward<pair<const char*, ValueTypes>>(values)...);
}


// A special handle-it-all generic conversion function

template<typename SrcSerializerType, typename DstSerializerType>
typename DstSerializerType::BufferType convert(const typename SrcSerializerType::BufferType& src) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "convert(" << SerializerName<SrcSerializerType>::value << ") to: " << SerializerName<DstSerializerType>::value << std::endl;
    }

    // Create destination serializer, use it to serialize the src
    DstSerializerType dstSerializer;
    return dstSerializer.serialize(src);
}

}
