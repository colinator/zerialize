#pragma once

#include <any>
#include <array>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <span>

namespace zerialize {

using std::string, std::string_view, std::vector, std::map, std::span;
using std::any, std::any_cast, std::function;

// Forward declaration of Serializer class
template <typename Derived>
class Serializer;

// Type handler function type
template <typename Derived>
using TypeHandler = void (*)(Serializer<Derived>*, const any&);

// Structure to hold hash code and handler function pairs
template <typename Derived>
struct TypeEntry {
    std::size_t hash;
    TypeHandler<Derived> handler;
    
    // Comparison operators for binary search
    constexpr bool operator<(const TypeEntry& other) const {
        return hash < other.hash;
    }
    
    constexpr bool operator<(std::size_t hashValue) const {
        return hash < hashValue;
    }
};

// Handler functions for different types
template <typename Derived>
void serializeAnyFunction(Serializer<Derived>* self, const any& v) {
    self->serializeFunction(any_cast<function<void(Derived&)>>(v));
}

template <typename Derived>
void serializeAnyMapStringAny(Serializer<Derived>* self, const any& v) {
    auto m = any_cast<map<string, any>>(v);
    static_cast<Derived*>(self)->serializeMap([&](auto& s) {
        for (const auto& [key, value] : m)
            s.serialize(key, value);
    });
}

template <typename Derived>
void serializeAnyVectorAny(Serializer<Derived>* self, const any& v) {
    auto vec = any_cast<vector<any>>(v);
    static_cast<Derived*>(self)->serializeVector([&](auto& s) {
        for (const any& value : vec)
            s.serializeAny(value);
    });
}

template <typename Derived>
void serializeAnyNullptr(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(nullptr);
}

template <typename Derived>
void serializeAnyInt8(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<int64_t>(any_cast<int8_t>(v)));
}

template <typename Derived>
void serializeAnyInt16(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<int64_t>(any_cast<int16_t>(v)));
}

template <typename Derived>
void serializeAnyInt32(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<int64_t>(any_cast<int32_t>(v)));
}

template <typename Derived>
void serializeAnyInt64(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<int64_t>(v));
}

template <typename Derived>
void serializeAnyUInt8(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<uint64_t>(any_cast<uint8_t>(v)));
}

template <typename Derived>
void serializeAnyUInt16(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<uint64_t>(any_cast<uint16_t>(v)));
}

template <typename Derived>
void serializeAnyUInt32(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<uint64_t>(any_cast<uint32_t>(v)));
}

template <typename Derived>
void serializeAnyUInt64(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<uint64_t>(any_cast<uint64_t>(v)));
}

template <typename Derived>
void serializeAnyBool(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<bool>(v));
}

template <typename Derived>
void serializeAnyDouble(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<double>(v));
}

template <typename Derived>
void serializeAnyFloat(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<double>(any_cast<float>(v)));
}

template <typename Derived>
void serializeAnySpanConstUInt8(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<span<const uint8_t>>(v));
}

template <typename Derived>
void serializeAnyConstCharPtr(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<const char*>(v));
}

template <typename Derived>
void serializeAnyString(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<string>(v));
}

template <typename Derived>
void serializeAnyVectorInt8(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<int8_t>>(v));
}

template <typename Derived>
void serializeAnyVectorInt16(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<int16_t>>(v));
}

template <typename Derived>
void serializeAnyVectorInt32(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<int32_t>>(v));
}

template <typename Derived>
void serializeAnyVectorInt64(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<int64_t>>(v));
}

template <typename Derived>
void serializeAnyVectorUInt8(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<uint8_t>>(v));
}

template <typename Derived>
void serializeAnyVectorUInt16(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<uint16_t>>(v));
}

template <typename Derived>
void serializeAnyVectorUInt32(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<uint32_t>>(v));
}

template <typename Derived>
void serializeAnyVectorUInt64(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<uint64_t>>(v));
}

template <typename Derived>
void serializeAnyVectorFloat(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<float>>(v));
}

template <typename Derived>
void serializeAnyVectorDouble(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<double>>(v));
}

template <typename Derived>
void serializeAnyVectorBool(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<bool>>(v));
}

template <typename Derived>
void serializeAnyVectorString(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<string>>(v));
}

template <typename Derived>
void serializeAnyMapStringInt8(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, int8_t>>(v));
}

template <typename Derived>
void serializeAnyMapStringInt16(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, int16_t>>(v));
}

template <typename Derived>
void serializeAnyMapStringInt32(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, int32_t>>(v));
}

template <typename Derived>
void serializeAnyMapStringInt64(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, int64_t>>(v));
}

template <typename Derived>
void serializeAnyMapStringUInt8(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, uint8_t>>(v));
}

template <typename Derived>
void serializeAnyMapStringUInt16(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, uint16_t>>(v));
}

template <typename Derived>
void serializeAnyMapStringUInt32(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, uint32_t>>(v));
}

template <typename Derived>
void serializeAnyMapStringUInt64(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, uint64_t>>(v));
}

template <typename Derived>
void serializeAnyMapStringFloat(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, float>>(v));
}

template <typename Derived>
void serializeAnyMapStringDouble(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, double>>(v));
}

template <typename Derived>
void serializeAnyMapStringBool(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, bool>>(v));
}

template <typename Derived>
void serializeAnyMapStringString(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, string>>(v));
}

// Create and return a sorted array of type entries (hash code + handler function)
template <typename Derived>
const std::array<TypeEntry<Derived>, 46>& getTypeHandlers() {
    static const std::array<TypeEntry<Derived>, 46> typeHandlers = []() {
        std::array<TypeEntry<Derived>, 46> handlers = {{
            { typeid(function<void(Derived&)>).hash_code(), serializeAnyFunction<Derived> },
            { typeid(map<string, any>).hash_code(), serializeAnyMapStringAny<Derived> },
            { typeid(vector<any>).hash_code(), serializeAnyVectorAny<Derived> },
            { typeid(std::nullptr_t).hash_code(), serializeAnyNullptr<Derived> },
            { typeid(int8_t).hash_code(), serializeAnyInt8<Derived> },
            { typeid(int16_t).hash_code(), serializeAnyInt16<Derived> },
            { typeid(int32_t).hash_code(), serializeAnyInt32<Derived> },
            { typeid(int64_t).hash_code(), serializeAnyInt64<Derived> },
            { typeid(uint8_t).hash_code(), serializeAnyUInt8<Derived> },
            { typeid(uint16_t).hash_code(), serializeAnyUInt16<Derived> },
            { typeid(uint32_t).hash_code(), serializeAnyUInt32<Derived> },
            { typeid(uint64_t).hash_code(), serializeAnyUInt64<Derived> },
            { typeid(bool).hash_code(), serializeAnyBool<Derived> },
            { typeid(double).hash_code(), serializeAnyDouble<Derived> },
            { typeid(float).hash_code(), serializeAnyFloat<Derived> },
            { typeid(span<const uint8_t>).hash_code(), serializeAnySpanConstUInt8<Derived> },
            { typeid(const char*).hash_code(), serializeAnyConstCharPtr<Derived> },
            { typeid(string).hash_code(), serializeAnyString<Derived> },
            { typeid(vector<int8_t>).hash_code(), serializeAnyVectorInt8<Derived> },
            { typeid(vector<int16_t>).hash_code(), serializeAnyVectorInt16<Derived> },
            { typeid(vector<int32_t>).hash_code(), serializeAnyVectorInt32<Derived> },
            { typeid(vector<int64_t>).hash_code(), serializeAnyVectorInt64<Derived> },
            { typeid(vector<uint8_t>).hash_code(), serializeAnyVectorUInt8<Derived> },
            { typeid(vector<uint16_t>).hash_code(), serializeAnyVectorUInt16<Derived> },
            { typeid(vector<uint32_t>).hash_code(), serializeAnyVectorUInt32<Derived> },
            { typeid(vector<uint64_t>).hash_code(), serializeAnyVectorUInt64<Derived> },
            { typeid(vector<float>).hash_code(), serializeAnyVectorFloat<Derived> },
            { typeid(vector<double>).hash_code(), serializeAnyVectorDouble<Derived> },
            { typeid(vector<bool>).hash_code(), serializeAnyVectorBool<Derived> },
            { typeid(vector<string>).hash_code(), serializeAnyVectorString<Derived> },
            { typeid(map<string, int8_t>).hash_code(), serializeAnyMapStringInt8<Derived> },
            { typeid(map<string, int16_t>).hash_code(), serializeAnyMapStringInt16<Derived> },
            { typeid(map<string, int32_t>).hash_code(), serializeAnyMapStringInt32<Derived> },
            { typeid(map<string, int64_t>).hash_code(), serializeAnyMapStringInt64<Derived> },
            { typeid(map<string, uint8_t>).hash_code(), serializeAnyMapStringUInt8<Derived> },
            { typeid(map<string, uint16_t>).hash_code(), serializeAnyMapStringUInt16<Derived> },
            { typeid(map<string, uint32_t>).hash_code(), serializeAnyMapStringUInt32<Derived> },
            { typeid(map<string, uint64_t>).hash_code(), serializeAnyMapStringUInt64<Derived> },
            { typeid(map<string, float>).hash_code(), serializeAnyMapStringFloat<Derived> },
            { typeid(map<string, double>).hash_code(), serializeAnyMapStringDouble<Derived> },
            { typeid(map<string, bool>).hash_code(), serializeAnyMapStringBool<Derived> },
            { typeid(map<string, string>).hash_code(), serializeAnyMapStringString<Derived> }
        }};
        
        // Sort by hash code for binary search
        std::sort(handlers.begin(), handlers.end());
        return handlers;
    }();
    
    return typeHandlers;
}

} // namespace zerialize
