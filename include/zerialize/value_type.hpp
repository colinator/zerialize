#pragma once

#include <string>
#include "concepts.hpp"

namespace zerialize {

using std::string;

// --------------
// Value Type: exists to provide support for introspection
// on Deserializable.

enum ValueType {
    Bool, 
    Int, 
    UInt, 
    Float,
    String, 
    Blob,
    Map, 
    Array
};

template <Deserializable T>
ValueType to_value_type(const T& v) {
    return 
        v.isInt() ? ValueType::Int : 
        v.isUInt() ? ValueType::UInt : 
        v.isFloat() ? ValueType::Float : 
        v.isString() ? ValueType::String : 
        v.isBlob() ? ValueType::Blob : 
        v.isBool() ? ValueType::Bool : 
        v.isMap() ? ValueType::Map : 
        ValueType::Array;
}

inline string value_type_to_string(ValueType v) {
    switch (v) {
        case ValueType::Int: return "Int";
        case ValueType::UInt: return "UInt";
        case ValueType::Float: return "Float";
        case ValueType::String: return "String";
        case ValueType::Blob: return "Blob";
        case ValueType::Bool: return "Bool";
        case ValueType::Map: return "Map";
        case ValueType::Array: return "Array";
    }
}

inline bool is_composite(ValueType v) {
    return v == ValueType::Map || v == ValueType::Array;
}

inline bool is_primitive(ValueType v) {
    return !is_composite(v);
}

} // namespace zerialize
