#pragma once

#include <any>
#include <array>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <span>
#include "concepts.hpp"
#include "errors.hpp"

/*
 * This file implements a compile-time type map for efficient serialization of std::any values.
 * 
 * Instead of using a long if-else chain to check types (O(n) complexity), we:
 * 1. Create a static array of {type_hash, handler_function} pairs at compile time
 * 2. Sort this array by type hash codes
 * 3. Use binary search (O(log n)) to find the appropriate handler for a given type
 * 
 * This approach significantly improves performance for type lookups, especially
 * with many supported types. For 46 types, it reduces the maximum number of
 * comparisons from 46 to about 6.
 * 
 * Template functions are used to reduce code duplication for similar types,
 * making the code more maintainable and easier to extend.
 */

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

// Generic template function for simple types
template <typename Derived, typename T>
void serializeAnySimple(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<T>(v));
}

// Template function for integer types that need casting to int64_t
template <typename Derived, typename T>
void serializeAnyIntegral(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<int64_t>(any_cast<T>(v)));
}

// Template function for unsigned integer types that need casting to uint64_t
template <typename Derived, typename T>
void serializeAnyUnsigned(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<uint64_t>(any_cast<T>(v)));
}

// Template function for float that needs casting to double
template <typename Derived>
void serializeAnyFloat(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(static_cast<double>(any_cast<float>(v)));
}

// Template function for vector types
template <typename Derived, typename T>
void serializeAnyVector(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<vector<T>>(v));
}

// Template function for map<string, T> types
template <typename Derived, typename T>
void serializeAnyMapString(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<map<string, T>>(v));
}

// Special case handlers that can't be templated easily
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
void serializeAnyNullptr(Serializer<Derived>* self, const any&) {
    static_cast<Derived*>(self)->serialize(nullptr);
}

template <typename Derived>
void serializeAnySpanConstUInt8(Serializer<Derived>* self, const any& v) {
    static_cast<Derived*>(self)->serialize(any_cast<span<const uint8_t>>(v));
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
            { typeid(int8_t).hash_code(), serializeAnyIntegral<Derived, int8_t> },
            { typeid(int16_t).hash_code(), serializeAnyIntegral<Derived, int16_t> },
            { typeid(int32_t).hash_code(), serializeAnyIntegral<Derived, int32_t> },
            { typeid(int64_t).hash_code(), serializeAnySimple<Derived, int64_t> },
            { typeid(uint8_t).hash_code(), serializeAnyUnsigned<Derived, uint8_t> },
            { typeid(uint16_t).hash_code(), serializeAnyUnsigned<Derived, uint16_t> },
            { typeid(uint32_t).hash_code(), serializeAnyUnsigned<Derived, uint32_t> },
            { typeid(uint64_t).hash_code(), serializeAnySimple<Derived, uint64_t> },
            { typeid(bool).hash_code(), serializeAnySimple<Derived, bool> },
            { typeid(double).hash_code(), serializeAnySimple<Derived, double> },
            { typeid(float).hash_code(), serializeAnyFloat<Derived> },
            { typeid(span<const uint8_t>).hash_code(), serializeAnySpanConstUInt8<Derived> },
            { typeid(const char*).hash_code(), serializeAnySimple<Derived, const char*> },
            { typeid(string).hash_code(), serializeAnySimple<Derived, string> },
            { typeid(vector<int8_t>).hash_code(), serializeAnyVector<Derived, int8_t> },
            { typeid(vector<int16_t>).hash_code(), serializeAnyVector<Derived, int16_t> },
            { typeid(vector<int32_t>).hash_code(), serializeAnyVector<Derived, int32_t> },
            { typeid(vector<int64_t>).hash_code(), serializeAnyVector<Derived, int64_t> },
            { typeid(vector<uint8_t>).hash_code(), serializeAnyVector<Derived, uint8_t> },
            { typeid(vector<uint16_t>).hash_code(), serializeAnyVector<Derived, uint16_t> },
            { typeid(vector<uint32_t>).hash_code(), serializeAnyVector<Derived, uint32_t> },
            { typeid(vector<uint64_t>).hash_code(), serializeAnyVector<Derived, uint64_t> },
            { typeid(vector<float>).hash_code(), serializeAnyVector<Derived, float> },
            { typeid(vector<double>).hash_code(), serializeAnyVector<Derived, double> },
            { typeid(vector<bool>).hash_code(), serializeAnyVector<Derived, bool> },
            { typeid(vector<string>).hash_code(), serializeAnyVector<Derived, string> },
            { typeid(map<string, int8_t>).hash_code(), serializeAnyMapString<Derived, int8_t> },
            { typeid(map<string, int16_t>).hash_code(), serializeAnyMapString<Derived, int16_t> },
            { typeid(map<string, int32_t>).hash_code(), serializeAnyMapString<Derived, int32_t> },
            { typeid(map<string, int64_t>).hash_code(), serializeAnyMapString<Derived, int64_t> },
            { typeid(map<string, uint8_t>).hash_code(), serializeAnyMapString<Derived, uint8_t> },
            { typeid(map<string, uint16_t>).hash_code(), serializeAnyMapString<Derived, uint16_t> },
            { typeid(map<string, uint32_t>).hash_code(), serializeAnyMapString<Derived, uint32_t> },
            { typeid(map<string, uint64_t>).hash_code(), serializeAnyMapString<Derived, uint64_t> },
            { typeid(map<string, float>).hash_code(), serializeAnyMapString<Derived, float> },
            { typeid(map<string, double>).hash_code(), serializeAnyMapString<Derived, double> },
            { typeid(map<string, bool>).hash_code(), serializeAnyMapString<Derived, bool> },
            { typeid(map<string, string>).hash_code(), serializeAnyMapString<Derived, string> }
        }};
        
        // Sort by hash code for binary search
        std::sort(handlers.begin(), handlers.end());
        return handlers;
    }();
    
    return typeHandlers;
}

} // namespace zerialize
