#pragma once

#include <stdexcept>
#include <string>

namespace zerialize {

using std::runtime_error, std::string;

class SerializationError : public runtime_error {
public:
    SerializationError(const string& msg) : runtime_error(msg) { }
};

class DeserializationError : public runtime_error {
public:
    DeserializationError(const string& msg) : runtime_error(msg) { }
};

} // namespace zerialize
