#pragma once

#include <any>
#include <functional>
#include <algorithm>
#include <ranges>
#include <vector>
#include <unordered_map>
#include <string>
#include <span>
#include "concepts.hpp"
#include "errors.hpp"
#include "any_dispatch.hpp"

namespace zerialize {

using std::any, std::function, std::vector, std::unordered_map, std::string, std::span;
using std::string_view, std::enable_if_t, std::is_convertible_v;

// --------------
// A CRTP-base class for serializers. Requires child classes to 
// implement the Serializable. Provides several convenience
// methods.

template <typename Derived>
class Serializer {
public:

    using SerializingFunction = function<void(Derived& s)>;

    Serializer() {
        static_assert(Serializable<Derived>, "Derived must satisfy Serializing concept");
    }

    inline void serializeFunction(function<void(Derived& s)> f) noexcept {
        f(*static_cast<Derived*>(this));
    }

    inline void serializeAny(const any& val) {

        // Get the hash code of the runtime type
        const std::size_t vt_hash = val.type().hash_code();
        
        // Get the type handlers array
        const auto& typeHandlers = getTypeHandlers<Derived>();
        
        // Binary search for the handler
        auto it = std::lower_bound(typeHandlers.begin(), typeHandlers.end(), vt_hash, 
            [](const TypeEntry<Derived>& entry, std::size_t hash) {
                return entry.hash < hash;
            });
        
        // If found, call the handler
        if (it != typeHandlers.end() && it->hash == vt_hash) {
            it->handler(this, val);
        } else {
            throw SerializationError("-- Unsupported type in any");
        }
    }

    // Used for format conversion, from any other Deserializable.
    template<Deserializable SourceBufferType>
    void serialize(const SourceBufferType& value) {
        Derived* d = static_cast<Derived*>(this);
        if (value.isMap()) {
            d->serializeMap([&](Serializable auto& s){
                for (string_view key: value.mapKeys()) {
                    auto keySerializer = s.serializerForKey(key);
                    keySerializer.Serializer::serialize(value[key]);
                }
            });
        } else if (value.isArray()) {
            d->serializeVector([&](Serializable auto& s){
                for (size_t i=0; i<value.arraySize(); i++) {
                    s.Serializer::serialize(value[i]);
                }
            });
        } else if (value.isNull()) {
            d->serialize(nullptr);
        } else if (value.isInt()) {
            d->serialize(value.asInt64());
        } else if (value.isUInt()) {
            d->serialize(value.asUInt64());
        }  else if (value.isFloat()) {
            d->serialize(value.asDouble());
        } else if (value.isBool()) {
            d->serialize(value.asBool());
        } else if (value.isString()) {
            d->serialize(value.asString());
        } else if (value.isBlob()) {
            auto blob = value.asBlob();
            std::span<const uint8_t> blob_span{blob.data(), blob.size()};
            d->serialize(blob_span);
        } else {
            throw SerializationError("Unsupported source buffer value type");
        }
    }

    // Serialize any vector-like container that we can iterate over.
    // Tested with array and vector
    template <typename Container>
    requires std::ranges::range<Container>
    inline void serialize(const Container& v) {
        Derived* d = static_cast<Derived*>(this);
        d->serializeVector([&v](Serializable auto& s) {
            for (auto&& val : v) {
                s.serialize(val);
            }
        });
    }

    // Serialize any map-like container that we can iterate over,
    // and whose key is convertible to string_view.
    template <typename Map>
    requires std::ranges::range<Map> &&
    requires(const typename Map::value_type kv) {
        { kv.first } -> std::convertible_to<std::string_view>;
        { kv.second };
    }
    inline void serialize(const Map& m) {
        Derived* d = static_cast<Derived*>(this);
        d->serializeMap([&m](Serializable auto& s){
            for (const auto& [key, value] : m) {
                auto keySerializer = s.serializerForKey(key);
                keySerializer.serialize(value);
            }
        });
    }
};


// --------------
// A 'counting serializer': a 'Serializer' that doesn't 
// serialize; instead it counts serialize invocations.
// Useful for encoder types that need to know array or
// map sizes in advance, like messagepack does. 
//
// Also: an example of how to write a serializer.
struct SerializeCounter: public Serializer<SerializeCounter> {
    size_t count = 0;

    using Serializer<SerializeCounter>::serialize;

    void serialize(std::nullptr_t) { count += 1; }
    void serialize(int8_t) { count += 1; }
    void serialize(int16_t) { count += 1; }
    void serialize(int32_t) { count += 1; }
    void serialize(int64_t) { count += 1; }
    void serialize(uint8_t) { count += 1; }
    void serialize(uint16_t) { count += 1; }
    void serialize(uint32_t) { count += 1; }
    void serialize(uint64_t) { count += 1; }
    void serialize(bool) { count += 1; }
    void serialize(double) { count += 1; }
    void serialize(const char*) { count += 1; }
    void serialize(const string&) { count += 1; }
    void serialize(const span<const uint8_t>&) { count += 1; }
    void serialize(const string&, const any&) { count += 1; }
    void serialize(const unordered_map<string, any>&) { count += 1; }
    void serialize(const vector<any>&) { count += 1; }

    template<typename T, typename = enable_if_t<is_convertible_v<T, string_view>>>
    void serialize(T&&) { count += 1; }

    template <typename F> requires InvocableSerializer<F, SerializeCounter&>
    void serialize(F&&) { count += 1; }

    template <typename F> requires InvocableSerializer<F, SerializeCounter&>
    void serializeMap(F&&) { count += 1; }

    template <typename F> requires InvocableSerializer<F, SerializeCounter&>
    void serializeVector(F&&) { count += 1; }

    // Handle any std::function type during counting - just execute it to count properly
    template <typename F>
    void serialize(const std::function<void(F&)>& func) { 
        // Create a temporary instance of F to execute the function for counting
        if constexpr (std::is_same_v<F, SerializeCounter>) {
            func(*this);  // Execute with this counter
        } else {
            count += 1;   // Fallback - just count as 1 element
        }
    }

    SerializeCounter serializerForKey(const string_view&) { count += 1; return *this; }
};

} // namespace zerialize
