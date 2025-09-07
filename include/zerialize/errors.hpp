#pragma once

#include <stdexcept>
#include <string>

// Serialization and Deserialization exception types. 
// These really require no further explanation.

namespace zerialize {

class SerializationError : public std::runtime_error {
public:
    SerializationError(const std::string& msg) : std::runtime_error(msg) { }
};

class DeserializationError : public std::runtime_error {
public:
    DeserializationError(const std::string& msg) : std::runtime_error(msg) { }
};

} // namespace zerialize
