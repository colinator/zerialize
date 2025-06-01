#pragma once

#include <concepts>
#include <span>
#include <string>
#include <string_view>
#include <set>
#include <functional>
#include <any>
#include <type_traits>

namespace zerialize {

//
// Defines the two main concepts:
//  Deserializable, which defines typechecking and conversion requirements.
//  Serialiable, which defines serialize function requirements.
//

using std::string, std::string_view, std::set, std::span, std::any;
using std::convertible_to, std::same_as, std::is_convertible_v, std::enable_if_t;

// --------------
// Deserializable Concept
//
// The Deserializer concept defines a compile-time interface
// that compliant classes must satisfy. Each 'node' in a dynamic
// tree of Deserializer must define these operations, and
// are encouraged to do so in a zero-copy way, if possible.
// It's just the union of BlobDeserialiable and NonBlobDeserialiable.

// Requires classes to implement deserialization into primitive
// types, maps, vectors, and blobs.


// A helper for 'blob-like' things...
template <typename C>
concept Blobby = requires(const C& c) {
    { c.data() } -> std::convertible_to<const uint8_t*>; 
    { c.size() } -> std::convertible_to<std::size_t>;
};

template<typename V>
concept Deserializable = requires(const V& v, const string_view key, size_t index) {

    // Type checking: everything that conforms to this concept needs
    // to be able to check it's type.
    { v.isNull() } -> convertible_to<bool>;
    { v.isInt() } -> convertible_to<bool>;
    { v.isUInt() } -> convertible_to<bool>;
    { v.isFloat() } -> convertible_to<bool>;
    { v.isString() } -> convertible_to<bool>;
    { v.isBlob() } -> convertible_to<bool>;
    { v.isBool() } -> convertible_to<bool>;
    { v.isMap() } -> convertible_to<bool>;
    { v.isArray() } -> convertible_to<bool>;

    // Deserialization: everything that conforms to this concept needs
    // to be able to read itself as any type of value (which could fail
    // by throwing DeserializationError).
    { v.asInt8() } -> convertible_to<int8_t>;
    { v.asInt16() } -> convertible_to<int16_t>;
    { v.asInt32() } -> convertible_to<int32_t>;
    { v.asInt64() } -> convertible_to<int64_t>;
    { v.asUInt8() } -> convertible_to<uint8_t>;
    { v.asUInt16() } -> convertible_to<uint16_t>;
    { v.asUInt32() } -> convertible_to<uint32_t>;
    { v.asUInt64() } -> convertible_to<uint64_t>;
    { v.asFloat() } -> convertible_to<float>;
    { v.asDouble() } -> convertible_to<double>;
    { v.asString() } -> convertible_to<string>;
    { v.asStringView() } -> convertible_to<string_view>;
    { v.asBool() } -> convertible_to<bool>;

    // Composite handling: everything that conforms to this concept needs
    // to be able to index itself as a map...
    { v.mapKeys() } -> convertible_to<set<string_view>>;
    { v[key] } -> same_as<V>;

    // ... or as an array
    { v.arraySize() } -> convertible_to<size_t>;
    { v[index] } -> same_as<V>;

    // ... or as a blob
    { v.asBlob() } -> Blobby;
};


// --------------
// The Serializing concept defines a compile-time interface
// that compliant classes must satisfy, to be considered
// a 'Serializer'.

template<typename V>
concept SerializingValue = requires(V& v, 
    const any& a, 
    int64_t b, 
    uint64_t c, 
    bool d, 
    double e, 
    const string& f, 
    const span<const uint8_t>& g, 
    const string_view& h, 
    const char* i,
    std::nullptr_t j,
    const string& key
) {
    //{ v.serialize(a) } -> std::same_as<void>;
    { v.serialize(b) } -> std::same_as<void>;
    { v.serialize(c) } -> std::same_as<void>;
    { v.serialize(c) } -> std::same_as<void>;
    { v.serialize(d) } -> std::same_as<void>;
    { v.serialize(e) } -> std::same_as<void>;
    { v.serialize(f) } -> std::same_as<void>;
    { v.serialize(g) } -> std::same_as<void>;
    { v.serialize(h) } -> std::same_as<void>;
    { v.serialize(i) } -> std::same_as<void>;
    { v.serialize(j) } -> std::same_as<void>;
    { v.serialize(key, a) } -> std::same_as<void>;
};

template <typename T>
using remove_volref_t = std::remove_volatile_t<std::remove_reference_t<T>>;

struct SerializingCallable {
    template <typename SerializerType>
    requires SerializingValue<remove_volref_t<SerializerType>> 
    void operator()(SerializerType&&) noexcept { }
};

// Lame: _another_ concept to define serializeMap and Vector. These methods
// take as arguments SerializingCallable objects: 'sub-serializers' that
// are responsible for serializing key/value pairs or vector elements.
// TODO: Unify these
// TBH: it would be better for SerializingCallable could require Serializer
// instead of SerializingValue. But there's no recursion between concepts
// like that, without forward-declaration anyway...
template <typename V>
concept SerializingComposite = requires(V& v, const std::string& key) {
    typename V::Serializer;
    { v.serialize(SerializingCallable{}) } -> std::same_as<void>;
    { v.serializeMap(SerializingCallable{}) } -> std::same_as<void>;
    { v.serializeVector(SerializingCallable{}) } -> std::same_as<void>;
};

// Finally, the concept we really want: just the union of the above.
template <typename T>
concept Serializable = SerializingValue<T> && SerializingComposite<T>;

// Defines the concept type of a function taking some arg.
// NOTE: Why can't we add && Serializable<Arg> ?
template <typename F, typename Arg>
concept InvocableSerializer = std::invocable<F, Arg>; 

} // namespace zerialize
