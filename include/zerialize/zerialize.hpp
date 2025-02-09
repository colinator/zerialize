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

// The Deserializer concept defines a compile-time interface
// that child classes must satisfy. Each 'node' in a dynamic
// tree of Deserializer must define these operations, and
// are encouraged to do so in a zero-copy way, if possible.
template<typename V>
concept Deserializer = requires(const V& v, const std::string& key, size_t index) {

    { v.isInt8() } -> std::convertible_to<bool>;
    { v.isInt16() } -> std::convertible_to<bool>;
    { v.isInt32() } -> std::convertible_to<bool>;
    { v.isInt64() } -> std::convertible_to<bool>;
    { v.isArray() } -> std::convertible_to<bool>;
    { v.isUInt8() } -> std::convertible_to<bool>;
    { v.isUInt16() } -> std::convertible_to<bool>;
    { v.isUInt32() } -> std::convertible_to<bool>;
    { v.isUInt64() } -> std::convertible_to<bool>;
    { v.isFloat() } -> std::convertible_to<bool>;
    { v.isDouble() } -> std::convertible_to<bool>;
    { v.isString() } -> std::convertible_to<bool>;
    { v.isBool() } -> std::convertible_to<bool>;
    { v.isMap() } -> std::convertible_to<bool>;
    { v.isArray() } -> std::convertible_to<bool>;
    
    { v.asInt8() } -> std::convertible_to<int8_t>;
    { v.asInt16() } -> std::convertible_to<int16_t>;
    { v.asInt32() } -> std::convertible_to<int32_t>;
    { v.asInt64() } -> std::convertible_to<int64_t>;
    { v.asUInt8() } -> std::convertible_to<uint8_t>;
    { v.asUInt16() } -> std::convertible_to<uint16_t>;
    { v.asUInt32() } -> std::convertible_to<uint32_t>;
    { v.asUInt64() } -> std::convertible_to<uint64_t>;
    { v.asFloat() } -> std::convertible_to<float>;
    { v.asDouble() } -> std::convertible_to<double>;
    { v.asString() } -> std::convertible_to<std::string_view>;
    { v.asBool() } -> std::convertible_to<bool>;

    { v.mapKeys() } -> std::convertible_to<std::vector<std::string_view>>;
    { v.arraySize() } -> std::convertible_to<int64_t>;
    { v[key] } -> std::same_as<V>;
    { v[index] } -> std::same_as<V>;
};


// The DataBuffer class exists to:
// 1. enforce a common compile-time concept interface for derived types
// 2. enforce a common interface for root-node buffers (that are themselves
//    derived types, but also manage the actual byte buffer).
template <typename Derived>
class DataBuffer {
public:
    DataBuffer() {
        static_assert(Deserializer<Derived>, "Derived must satisfy Deserializer concept");
    }
    virtual const std::vector<uint8_t>& buf() const = 0;
    size_t size() const { return buf().size(); }
    virtual std::string to_string() const = 0;
};



// Trait to get serializer name at compile-time:
// each serializer must have unique name, defined
// in it's .hpp file
template<typename T>
struct SerializerName;


// Define various serializers: the 7 canonical forms.

template <typename SerializerType>
//requires DataValueType<typename SerializerType::BufferType>
typename SerializerType::BufferType serialize() {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize()" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize();
}

template <typename SerializerType>
//requires DataValueType<typename SerializerType::BufferType>
typename SerializerType::BufferType serialize(const std::any& value) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(any: " << value.type().name() << ")" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(value);
}

template <typename SerializerType>
//requires DataValueType<typename SerializerType::BufferType>
typename SerializerType::BufferType serialize(std::initializer_list<std::any> list) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(initializer list)" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(list);
}

template <typename SerializerType>
//requires DataValueType<typename SerializerType::BufferType>
typename SerializerType::BufferType serialize(std::initializer_list<std::pair<std::string, std::any>> list) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(initializer list/map)" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(list);
}

template<typename SerializerType, typename ValueType>
//requires DataValueType<typename SerializerType::BufferType>
typename SerializerType::BufferType serialize(ValueType&& value) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(&&value)" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(std::forward<ValueType>(value));
}

template<typename SerializerType, typename... ValueTypes>
//requires DataValueType<typename SerializerType::BufferType>
typename SerializerType::BufferType serialize(ValueTypes&&... values) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(&&values" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(std::forward<ValueTypes>(values)...);
}

template<typename SerializerType, typename... ValueTypes>
//requires DataValueType<typename SerializerType::BufferType>
typename SerializerType::BufferType serialize(std::pair<const char*, ValueTypes>&&... values) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(&& value pairs)" << std::endl;
    }
    SerializerType serializer;
    return serializer.serializeMap(std::forward<std::pair<const char*, ValueTypes>>(values)...);
}

class DeserializationError : public std::runtime_error {
public:
    DeserializationError(const std::string& msg) : std::runtime_error(msg) { }
};

}
