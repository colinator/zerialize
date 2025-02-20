#pragma once

#include <ranges>
#include "zerialize/zerialize.hpp"

namespace zerialize {
namespace composite {

template <typename T, size_t D>
class MyComposite { //}: public zerialize::BullshitClass {
public:
    T a;
    string b;

    bool operator == (const MyComposite& other) const {
        return a == other.a && b == other.b;
    }

    std::string to_string() const {
        return "<MyComposite a: " + std::to_string(a) + " b: " + b + " />";
    }

    // std::map<std::string, std::any> serialize() const {
    //     return std::map<std::string, std::any> {
    //         {"s", (uint64_t)D}, 
    //         {"a", a}, 
    //         {"b", b}
    //     };
    // }

    // zerialize::ZComposite<MyComposite> serializer() const { 
    //     return zerialize::ZComposite<MyComposite>(*this); 
    // }

    // template<typename SerializerType>
    // void serialize_write(SerializerType& serializer) {
    //     return serializer.serialize({
    //         {"s", (uint64_t)D}, 
    //         {"a", a}, 
    //         {"b", b}
    //     });
    // }

    // template<typename SerializerType>
    // typename SerializerType::BufferType serialize(SerializerType& serializer) {
    //     if constexpr (DEBUG_TRACE_CALLS) {
    //         cout << "MyComposite::serialize" << endl;
    //     }
    //     return serializer.serialize({
    //         {"s", (uint64_t)D}, 
    //         {"a", a}, 
    //         {"b", b}
    //     });
    // }
};

template <typename T, size_t D>
map<string, any> serialize(const MyComposite<T, D>& c) {
    return map<string, any> {
        {"s", (uint64_t)D}, 
        {"a", c.a}, 
        {"b", c.b}
    };
}

template<typename SerializerType, typename T, size_t D>
typename SerializerType::BufferType serialize(SerializerType& serializer, const MyComposite<T, D>& c) {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "serialize(Composite&) up" << endl;
    }
    return serializer.serialize(serialize<T, D>(c));
}

template<typename SerializerType, typename T, size_t D>
typename SerializerType::BufferType serialize(const MyComposite<T, D>& c) {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "serialize(Composite&)" << endl;
    }   
    SerializerType serializer;
    return serialize(serializer, c);
}

inline bool _vectorContains(const vector<string_view>& v, const string_view& c) {
    return std::ranges::find(v, c) != v.end();
}

inline bool isMyComposite(const Deserializable auto& buf) {
    if (buf.isMap()) {
        auto keys = buf.mapKeys();
        return _vectorContains(keys, "a") && buf["a"].isFloat() && 
               _vectorContains(keys, "b") && buf["b"].isString() && 
               _vectorContains(keys, "s") && buf["s"].isUInt();
    }
    return false;
}

template <typename T, size_t D>
MyComposite<T, D> asMyComposite(const Deserializable auto& buf) {
    if (!isMyComposite(buf)) { throw DeserializationError("not a MyComposite"); }
    return MyComposite<T, D>(
        buf["a"].asDouble(), 
        buf["b"].asString()
    );
}

} // namespace composite
} // namespace zerialize
