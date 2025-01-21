#pragma once

#include <any>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <span>

namespace zerialize {

class Buffer {
public:
    virtual const std::vector<uint8_t>& buf() const = 0;
    virtual std::string to_string() const = 0;
};

template <typename SerializerType>
typename SerializerType::BufferType serialize() {
    std::cout << "serialize nothing" << std::endl;
    SerializerType serializer;
    return serializer.serialize();
}

template <typename SerializerType>
typename SerializerType::BufferType serialize(const std::any& value) {
    std::cout << "serialize any: " << value.type().name() << std::endl;
    SerializerType serializer;
    return serializer.serialize(value);
}

template <typename SerializerType>
typename SerializerType::BufferType serialize(std::initializer_list<std::any> list) {
    std::cout << "serialize list " << std::endl;
    SerializerType serializer;
    return serializer.serialize(list);
}

template <typename SerializerType>
typename SerializerType::BufferType serialize(std::initializer_list<std::pair<std::string, std::any>> list) {
    std::cout << "serialize list " << std::endl;
    SerializerType serializer;
    return serializer.serialize(list);
}

template<typename SerializerType, typename ValueType>
typename SerializerType::BufferType serialize(ValueType&& value) {
    std::cout << "serialize generic one" << std::endl;
    SerializerType serializer;
    return serializer.serialize(std::forward<ValueType>(value));
}

template<typename SerializerType, typename... ValueTypes>
typename SerializerType::BufferType serialize(ValueTypes&&... values) {
    std::cout << "serialize generic many " << std::endl;
    SerializerType serializer;
    return serializer.serialize(std::forward<ValueTypes>(values)...);
}

template<typename SerializerType, typename... ValueTypes>
typename SerializerType::BufferType serialize(std::pair<const char*, ValueTypes>&&... values) {
    std::cout << "serialize generic many pairs" << std::endl;
    SerializerType serializer;
    return serializer.serializeMap(std::forward<std::pair<const char*, ValueTypes>>(values)...);
}

}