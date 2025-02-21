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
};

template <typename T, size_t D>
map<string, any> serialize(const MyComposite<T, D>& c) {
    return map<string, any> {
        {"s", (uint64_t)D}, 
        {"a", c.a}, 
        {"b", c.b}
    };
}

template <size_t D>
bool isMyComposite(const Deserializable auto& buf) {
    if (!buf.isMap()) return false;
    set<string_view> keys = buf.mapKeys();
    return keys.contains("a") && buf["a"].isFloat() && 
           keys.contains("b") && buf["b"].isString() && 
           keys.contains("s") && buf["s"].isUInt() && buf["s"].asUInt64() == D;
}

template <typename T, size_t D>
MyComposite<T, D> asMyComposite(const Deserializable auto& buf) {
    if (!isMyComposite<D>(buf)) { throw DeserializationError("not a MyComposite"); }
    return MyComposite<T, D>(
        buf["a"].asDouble(), 
        buf["b"].asString()
    );
}

} // namespace composite
} // namespace zerialize




// template<typename SerializerType, typename T, size_t D>
// typename SerializerType::BufferType serialize(SerializerType& serializer, const MyComposite<T, D>& c) {
//     if constexpr (DEBUG_TRACE_CALLS) {
//         cout << "serialize(Composite&) up" << endl;
//     }
//     return serializer.serialize(serialize<T, D>(c));
// }

// template<typename SerializerType, typename T, size_t D>
// typename SerializerType::BufferType serialize(const MyComposite<T, D>& c) {
//     if constexpr (DEBUG_TRACE_CALLS) {
//         cout << "serialize(Composite&)" << endl;
//     }   
//     SerializerType serializer;
//     return serialize(serializer, c);
// }