#pragma once

#include <any>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <span>


constexpr bool DEBUG_TRACE_CALLS = true;

namespace zerialize {

class Buffer {
public:
    virtual const std::vector<uint8_t>& buf() const = 0;
    virtual std::string to_string() const = 0;
    size_t size() const { return buf().size(); }
};

// Trait to get serializer name at compile-time:
// each serializer must have unique name, defined
// in it's .hpp file
template<typename T>
struct SerializerName;


template <typename SerializerType>
typename SerializerType::BufferType serialize() {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize()" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize();
}

template <typename SerializerType>
typename SerializerType::BufferType serialize(const std::any& value) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(any: " << value.type().name() << ")" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(value);
}

template <typename SerializerType>
typename SerializerType::BufferType serialize(std::initializer_list<std::any> list) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(initializer list)" << std::endl;
    }
    SerializerType serializer;
    return serializer.serialize(list);
}

template <typename SerializerType>
typename SerializerType::BufferType serialize(std::initializer_list<std::pair<std::string, std::any>> list) {
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
typename SerializerType::BufferType serialize(std::pair<const char*, ValueTypes>&&... values) {
    if constexpr (DEBUG_TRACE_CALLS) {
        std::cout << "serialize(&& value pairs)" << std::endl;
    }
    SerializerType serializer;
    return serializer.serializeMap(std::forward<std::pair<const char*, ValueTypes>>(values)...);
}

}