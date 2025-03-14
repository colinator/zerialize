#pragma once

#include <any>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <span>
#include <concepts>
#include <sstream>
#include <functional>

constexpr bool DEBUG_TRACE_CALLS = false;

namespace zerialize {

using std::string, std::string_view, std::vector, std::map, std::set, std::pair, std::span;
using std::any, std::any_cast, std::initializer_list, std::function;
using std::convertible_to, std::same_as, std::is_convertible_v, std::enable_if_t, std::declval;
using std::runtime_error;
using std::cout, std::endl, std::stringstream;


// --------------
// Error types thrown by various functions.

class SerializationError : public runtime_error {
public:
    SerializationError(const string& msg) : runtime_error(msg) { }
};

class DeserializationError : public runtime_error {
public:
    DeserializationError(const string& msg) : runtime_error(msg) { }
};


// --------------
// Deserializable Concept
//
// The Deserializer concept defines a compile-time interface
// that compliant classes must satisfy. Each 'node' in a dynamic
// tree of Deserializer must define these operations, and
// are encouraged to do so in a zero-copy way, if possible.
// It's just the union of BlobDeserialiable and NonBlobDeserialiable.

// Requires classes to implement deserialization into primitive
// types, maps, and vectors. Blob constraints defined below.
template<typename V>
concept NonBlobDeserializable = requires(const V& v, const string& key, size_t index) {

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
};

// Requires classes to implement a valid asBlob method
// that can return owning or non-owning array types.
template <typename T>
concept BlobDeserializable = requires(T t) {
    { t.asBlob() };
} && (convertible_to<decltype(declval<T>().asBlob()), vector<uint8_t>> ||
      same_as<decltype(declval<T>().asBlob()), span<const uint8_t>>);

// We just use the union because of c++20 constraints - we cannot
// define the asBlob as returning either span or vector otherwise.
template <typename T>
concept Deserializable = BlobDeserializable<T> && NonBlobDeserializable<T>;


// --------------
// The DataBuffer class exists to:
// 1. enforce a common compile-time concept interface for derived types: they
//    must conform to Deserializable.
// 2. enforce a common interface for root-node buffers (that are themselves
//    derived types, but also manage the actual byte buffer).

template <typename Derived>
class DataBuffer {
public:
    DataBuffer() {
        static_assert(Deserializable<Derived>, "Derived must satisfy Deserializable concept");
    }
    
    virtual const vector<uint8_t>& buf() const = 0;
    size_t size() const { return buf().size(); }
    virtual string to_string() const { return "<DataBuffer size: " + std::to_string(size()) + ">"; }

    template<typename T>
    T as() const {
        const Derived* d = static_cast<const Derived*>(this);
        if constexpr (std::is_same_v<T, int8_t>) {
            return d->asInt8();
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return d->asInt16();
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return d->asInt32();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return d->asInt64();
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            return d->asUInt8();
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            return d->asUInt16();
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return d->asUInt32();
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return d->asUInt64();
        } else if constexpr (std::is_same_v<T, float>) {
            return d->asFloat();
        } else if constexpr (std::is_same_v<T, double>) {
            return d->asDouble();
        } else if constexpr (std::is_same_v<T, bool>) {
            return d->asBool();
        } else if constexpr (std::is_same_v<T, string>) {
            return d->asString();
        } else if constexpr (std::is_same_v<T, string_view>) {
            return d->asStringView();
        } else {
            static_assert(false, "Unsupported type in as<T>()");
        }
    }
};


// --------------
// The Serializing concept defines a compile-time interface
// that compliant classes must satisfy, to be considered
// a 'Serializer'.

template<typename V>
concept Serializing = requires(V& v, 
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

    // Must support v.serialize(key, any)
    { v.serialize(key, a) } -> std::same_as<void>;
};


struct SerializingCallable {
    template <typename T>
    requires Serializing<std::remove_cvref_t<T>>
    void operator()(T&&) { }
};

// Lame: _another_ concept to define serializeMap and Vector. These methods
// take as arguments SerializingCallable objects: 'sub-serializers' that
// are responsible for serializing key/value pairs or vector elements.
template <typename V>
concept SerializingComposite = requires(V& v, const std::string& key) {
    typename V::Serializer;
    { v.serialize(SerializingCallable{}) } -> std::same_as<void>;
    { v.serializeMap(SerializingCallable{}) } -> std::same_as<void>;
    { v.serializeVector(SerializingCallable{}) } -> std::same_as<void>;
};

// Finally, the concept we really want: just the union of the above.
template <typename T>
concept SerializingConcept = Serializing<T> && SerializingComposite<T>;

// Defines the concept type of a function taking some arg.
// NOTE: Why can't we add && SerializingConcept<Arg> ?
template <typename F, typename Arg>
concept InvocableSerializer = std::invocable<F, Arg>; 


// --------------
// A CRTP-base class for serializers. Requires child classes to 
// implement the SerializingConcept. Provides several convenience
// methods.

template <typename Derived>
class Serializer {
public:

    using SerializingFunction = function<void(Derived& s)>;

    Serializer() {
        static_assert(SerializingConcept<Derived>, "Derived must satisfy Serializing concept");
    }

    void serializeFunction(function<void(Derived& s)> f) {
        f(*static_cast<Derived*>(this));
    }

    void serializeAny(const any& val) {
        Derived* d = static_cast<Derived*>(this);

        // Cache hash codes for each type.
        static const std::size_t FunctionTypeHash           = typeid(function<void(Derived&)>).hash_code();
        static const std::size_t MapStringAnyTypeHash       = typeid(map<string, any>).hash_code();
        static const std::size_t VectorAnyTypeHash          = typeid(vector<any>).hash_code();
        static const std::size_t NullptrTypeHash            = typeid(std::nullptr_t).hash_code();
        static const std::size_t Int8TypeHash               = typeid(int8_t).hash_code();
        static const std::size_t Int16TypeHash              = typeid(int16_t).hash_code();
        static const std::size_t Int32TypeHash              = typeid(int32_t).hash_code();
        static const std::size_t Int64TypeHash              = typeid(int64_t).hash_code();
        static const std::size_t UInt8TypeHash              = typeid(uint8_t).hash_code();
        static const std::size_t UInt16TypeHash             = typeid(uint16_t).hash_code();
        static const std::size_t UInt32TypeHash             = typeid(uint32_t).hash_code();
        static const std::size_t UInt64TypeHash             = typeid(uint64_t).hash_code();
        static const std::size_t BoolTypeHash               = typeid(bool).hash_code();
        static const std::size_t DoubleTypeHash             = typeid(double).hash_code();
        static const std::size_t FloatTypeHash              = typeid(float).hash_code();
        static const std::size_t SpanConstUInt8TypeHash     = typeid(span<const uint8_t>).hash_code();
        static const std::size_t ConstCharPtrTypeHash       = typeid(const char*).hash_code();
        static const std::size_t StringTypeHash             = typeid(string).hash_code();
        static const std::size_t VectorInt8TypeHash         = typeid(vector<int8_t>).hash_code();
        static const std::size_t VectorInt16TypeHash        = typeid(vector<int16_t>).hash_code();
        static const std::size_t VectorInt32TypeHash        = typeid(vector<int32_t>).hash_code();
        static const std::size_t VectorInt64TypeHash        = typeid(vector<int64_t>).hash_code();
        static const std::size_t VectorUInt8TypeHash        = typeid(vector<uint8_t>).hash_code();
        static const std::size_t VectorUInt16TypeHash       = typeid(vector<uint16_t>).hash_code();
        static const std::size_t VectorUInt32TypeHash       = typeid(vector<uint32_t>).hash_code();
        static const std::size_t VectorUInt64TypeHash       = typeid(vector<uint64_t>).hash_code();
        static const std::size_t VectorFloatTypeHash        = typeid(vector<float>).hash_code();
        static const std::size_t VectorDoubleTypeHash       = typeid(vector<double>).hash_code();
        static const std::size_t VectorBoolTypeHash         = typeid(vector<bool>).hash_code();
        static const std::size_t VectorStringTypeHash       = typeid(vector<string>).hash_code();
        static const std::size_t MapStringInt8TypeHash      = typeid(map<string, int8_t>).hash_code();
        static const std::size_t MapStringInt16TypeHash     = typeid(map<string, int16_t>).hash_code();
        static const std::size_t MapStringInt32TypeHash     = typeid(map<string, int32_t>).hash_code();
        static const std::size_t MapStringInt64TypeHash     = typeid(map<string, int64_t>).hash_code();
        static const std::size_t MapStringUInt8TypeHash     = typeid(map<string, uint8_t>).hash_code();
        static const std::size_t MapStringUInt16TypeHash    = typeid(map<string, uint16_t>).hash_code();
        static const std::size_t MapStringUInt32TypeHash    = typeid(map<string, uint32_t>).hash_code();
        static const std::size_t MapStringUInt64TypeHash    = typeid(map<string, uint64_t>).hash_code();
        static const std::size_t MapStringFloatTypeHash     = typeid(map<string, float>).hash_code();
        static const std::size_t MapStringDoubleTypeHash    = typeid(map<string, double>).hash_code();
        static const std::size_t MapStringBoolTypeHash      = typeid(map<string, bool>).hash_code();
        static const std::size_t MapStringStringTypeHash    = typeid(map<string, string>).hash_code();

        // Get the hash code of the runtime type.
        const std::size_t vt_hash = val.type().hash_code();

        if (vt_hash == FunctionTypeHash) {
            serializeFunction(any_cast<function<void(Derived&)>>(val));
        } else if (vt_hash == MapStringAnyTypeHash) {
            auto m = any_cast<map<string, any>>(val);
            d->serializeMap([&](SerializingConcept auto& s) {
                for (const auto& [key, value] : m)
                    s.serialize(key, value);
            });
        } else if (vt_hash == VectorAnyTypeHash) {
            auto v = any_cast<vector<any>>(val);
            d->serializeVector([&](SerializingConcept auto& s) {
                for (const any& value : v)
                    s.serializeAny(value);
            });
        } else if (vt_hash == NullptrTypeHash) {
            d->serialize(nullptr);
        } else if (vt_hash == Int8TypeHash) {
            d->serialize(static_cast<int64_t>(any_cast<int8_t>(val)));
        } else if (vt_hash == Int16TypeHash) {
            d->serialize(static_cast<int64_t>(any_cast<int16_t>(val)));
        } else if (vt_hash == Int32TypeHash) {
            d->serialize(static_cast<int64_t>(any_cast<int32_t>(val)));
        } else if (vt_hash == Int64TypeHash) {
            d->serialize(any_cast<int64_t>(val));
        } else if (vt_hash == UInt8TypeHash) {
            d->serialize(static_cast<uint64_t>(any_cast<uint8_t>(val)));
        } else if (vt_hash == UInt16TypeHash) {
            d->serialize(static_cast<uint64_t>(any_cast<uint16_t>(val)));
        } else if (vt_hash == UInt32TypeHash) {
            d->serialize(static_cast<uint64_t>(any_cast<uint32_t>(val)));
        } else if (vt_hash == UInt64TypeHash) {
            d->serialize(static_cast<uint64_t>(any_cast<uint64_t>(val)));
        } else if (vt_hash == BoolTypeHash) {
            d->serialize(any_cast<bool>(val));
        } else if (vt_hash == DoubleTypeHash) {
            d->serialize(any_cast<double>(val));
        } else if (vt_hash == FloatTypeHash) {
            d->serialize(static_cast<double>(any_cast<float>(val)));
        } else if (vt_hash == SpanConstUInt8TypeHash) {
            d->serialize(any_cast<span<const uint8_t>>(val));
        } else if (vt_hash == ConstCharPtrTypeHash) {
            d->serialize(any_cast<const char*>(val));
        } else if (vt_hash == StringTypeHash) {
            d->serialize(any_cast<string>(val));
        } else if (vt_hash == VectorInt8TypeHash) {
            d->serialize(any_cast<vector<int8_t>>(val));
        } else if (vt_hash == VectorInt16TypeHash) {
            d->serialize(any_cast<vector<int16_t>>(val));
        } else if (vt_hash == VectorInt32TypeHash) {
            d->serialize(any_cast<vector<int32_t>>(val));
        } else if (vt_hash == VectorInt64TypeHash) {
            d->serialize(any_cast<vector<int64_t>>(val));
        } else if (vt_hash == VectorUInt8TypeHash) {
            d->serialize(any_cast<vector<uint8_t>>(val));
        } else if (vt_hash == VectorUInt16TypeHash) {
            d->serialize(any_cast<vector<uint16_t>>(val));
        } else if (vt_hash == VectorUInt32TypeHash) {
            d->serialize(any_cast<vector<uint32_t>>(val));
        } else if (vt_hash == VectorUInt64TypeHash) {
            d->serialize(any_cast<vector<uint64_t>>(val));
        } else if (vt_hash == VectorFloatTypeHash) {
            d->serialize(any_cast<vector<float>>(val));
        } else if (vt_hash == VectorDoubleTypeHash) {
            d->serialize(any_cast<vector<double>>(val));
        } else if (vt_hash == VectorBoolTypeHash) {
            d->serialize(any_cast<vector<bool>>(val));
        } else if (vt_hash == VectorStringTypeHash) {
            d->serialize(any_cast<vector<string>>(val));
        } else if (vt_hash == MapStringInt8TypeHash) {
            d->serialize(any_cast<map<string, int8_t>>(val));
        } else if (vt_hash == MapStringInt16TypeHash) {
            d->serialize(any_cast<map<string, int16_t>>(val));
        } else if (vt_hash == MapStringInt32TypeHash) {
            d->serialize(any_cast<map<string, int32_t>>(val));
        } else if (vt_hash == MapStringInt64TypeHash) {
            d->serialize(any_cast<map<string, int64_t>>(val));
        } else if (vt_hash == MapStringUInt8TypeHash) {
            d->serialize(any_cast<map<string, uint8_t>>(val));
        } else if (vt_hash == MapStringUInt16TypeHash) {
            d->serialize(any_cast<map<string, uint16_t>>(val));
        } else if (vt_hash == MapStringUInt32TypeHash) {
            d->serialize(any_cast<map<string, uint32_t>>(val));
        } else if (vt_hash == MapStringUInt64TypeHash) {
            d->serialize(any_cast<map<string, uint64_t>>(val));
        } else if (vt_hash == MapStringFloatTypeHash) {
            d->serialize(any_cast<map<string, float>>(val));
        } else if (vt_hash == MapStringDoubleTypeHash) {
            d->serialize(any_cast<map<string, double>>(val));
        } else if (vt_hash == MapStringBoolTypeHash) {
            d->serialize(any_cast<map<string, bool>>(val));
        } else if (vt_hash == MapStringStringTypeHash) {
            d->serialize(any_cast<map<string, string>>(val));
        } else {
            throw SerializationError("-- Unsupported type in any");
        }
    }

    // Used for format conversion, from any other Deserializable.
    template<Deserializable SourceBufferType>
    void serialize(const SourceBufferType& value) {
        Derived* d = static_cast<Derived*>(this);
        if (value.isMap()) {
            d->serializeMap([&](SerializingConcept auto& s){
                for (string_view key: value.mapKeys()) {
                    const string k(key);    // DO NOT LIKE COPY CONSTRUCTION
                    auto keySerializer = s.serializerForKey(k);
                    keySerializer.Serializer::serialize(value[k]);
                }
            });
        } else if (value.isArray()) {
            d->serializeVector([&](SerializingConcept auto& s){
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
            d->serialize(value.asBlob()); // ERROR HERE!!!! Howdoweknow?
        } else {
            throw SerializationError("Unsupported source buffer value type");
        }
    }

    // Serialize any vector-like container that we can iterate over.
    // Tested with array and vector
    template <typename Container>
    requires std::ranges::range<Container>
    void serialize(const Container& v) {
        Derived* d = static_cast<Derived*>(this);
        d->serializeVector([&](SerializingConcept auto& s) {
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
    void serialize(const Map& m) {
        Derived* d = static_cast<Derived*>(this);
        d->serializeMap([&m](SerializingConcept auto& s){
            for (const auto& [key, value] : m) {
                const string k(key);    // DO NOT LIKE COPY CONSTRUCTION
                auto keySerializer = s.serializerForKey(k);
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
    void serialize(const map<string, any>&) { count += 1; }
    void serialize(const vector<any>&) { count += 1; }

    template<typename T, typename = enable_if_t<is_convertible_v<T, string_view>>>
    void serialize(T&&) { count += 1; }

    template <typename F> requires InvocableSerializer<F, SerializeCounter&>
    void serialize(F&&) { count += 1; }

    template <typename F> requires InvocableSerializer<F, SerializeCounter&>
    void serializeMap(F&&) { count += 1; }

    template <typename F> requires InvocableSerializer<F, SerializeCounter&>
    void serializeVector(F&&) { count += 1; }

    SerializeCounter serializerForKey(const string&) { count += 1; return *this; }
};


// --------------
// Value Type: exists to provide support for introspection
// on Deserializable.

enum ValueType {
    Bool, Int, UInt, Float,
    String, Blob,
    Map, Array
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


// --------------
// debug streaming

inline string repeated_string(int num, const string& input) {
    string ret;
    ret.reserve(input.size() * num);
    while (num--)
        ret += input;
    return ret;
}

void debug_stream(stringstream & s, int tabLevel, const Deserializable auto& v) {
    auto valueType = to_value_type(v);
    auto tab = "  ";
    auto tabString = repeated_string(tabLevel, tab);

    if (v.isMap()) {
        s << "<Map> {" << endl;
        for (string_view key: v.mapKeys()) {
            const string sk(key);
            s << tabString << tab << "\"" << key << "\": ";
            debug_stream(s, tabLevel+1, v[sk]);
        }
        s << tabString << "}" << endl;
    } else if (v.isArray()) {
        s << "<Array> [" << endl;
        for (size_t i=0; i<v.arraySize(); i++) {
            s << tabString << tab;
            debug_stream(s, tabLevel+1, v[i]);
        }
        s << tabString << "]" << endl;
    } else if (is_primitive(valueType)) {
        if (v.isUInt()) {
            s << v.asUInt64();
        } else if (v.isInt()) {
            s << v.asInt64();
        } else if (v.isFloat()) {
            s << v.asDouble();
        } else if (v.isString()) {
            s << "\"" << v.asString() << "\"";
        } else if (v.isNull()) {
            s << "<null/>";
        } else if (v.isBool()) {
            s << v.asBool();
        } else if (v.isBlob()) {
            auto blob = v.asBlob();
            s << "<" << blob.size() << " bytes>";
        }
        s << " <" << value_type_to_string(valueType) << ">" << endl;
    }
}

string debug_string(const Deserializable auto& v) {
    stringstream s;
    debug_stream(s, 0, v);
    return s.str();
}

inline std::string blob_to_string(vector<uint8_t>& s) {
    std::string result = "<vec " + std::to_string(s.size()) + ": ";
    for (uint8_t v : s) {
        result += " " + std::to_string(v);
    }
    result += ">";
    return result;
}

inline std::string blob_to_string(std::span<const uint8_t> s) {
    std::string result = "<span " + std::to_string(s.size()) + ": ";
    for (uint8_t v : s) {
        result += " " + std::to_string(v);
    }
    result += ">";
    return result;
}


// --------------
// Define various serializer functions: the 7 canonical forms, so
// we can pass nothing, an any, initializer lists for
// vectors and maps, a perfect-forwarded value, and 
// perfect-forwarded lists and maps.

// Serialize nothing.
template <typename SerializerType>
typename SerializerType::BufferType serialize() {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "serialize()" << endl;
    }
    typename SerializerType::RootSerializer rootSerializer;
    typename SerializerType::Serializer serializer(rootSerializer);
    return rootSerializer.finish();
}

// Serialize anything, as an 'any' value.
template <typename SerializerType>
typename SerializerType::BufferType serialize(const any& value) {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "serialize(any: " << value.type().name() << ")" << endl;
    }
    typename SerializerType::RootSerializer rootSerializer;
    typename SerializerType::Serializer serializer(rootSerializer);
    serializer.serializeAny(value);
    return rootSerializer.finish();
}

// Serialize a vector from an initializer list of any.
template <typename SerializerType>
typename SerializerType::BufferType serialize(initializer_list<any> list) {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "serialize(initializer list)" << endl;
    }
    typename SerializerType::RootSerializer rootSerializer;
    typename SerializerType::Serializer serializer(rootSerializer);
    serializer.serializeVector([&list](SerializingConcept auto& s){
        for (const any& val : list) {
            s.serializeAny(val);
        }           
    });
    return rootSerializer.finish();
}

// Serialize a map from an initializer list of pair<string, any>.
template <typename SerializerType>
typename SerializerType::BufferType serialize(initializer_list<pair<string, any>> l) {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "serialize(initializer list/map)" << endl;
    }
    typename SerializerType::RootSerializer rootSerializer;
    typename SerializerType::Serializer serializer(rootSerializer);
    serializer.serializeMap([&l](SerializingConcept auto& s){
        for (const auto& [key, val] : l) {
            s.serialize(key, val);
        }           
    });
    return rootSerializer.finish();
}

// Serialize a perfect-forwarded value.
template<typename SerializerType, typename ValueType>
typename SerializerType::BufferType serialize(ValueType&& value) {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "serialize(&&value)" << endl;
    }
    typename SerializerType::RootSerializer rootSerializer;
    typename SerializerType::Serializer serializer(rootSerializer);
    serializer.serialize(std::forward<ValueType>(value));
    return rootSerializer.finish();
}

// Serialize a map from pairs of string keys and  perfectly-forwarded values.
template<typename SerializerType, typename... ValueTypes>
typename SerializerType::BufferType serialize(pair<const char*, ValueTypes>&&... values) {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "serialize(&& value pairs)" << endl;
    }
    typename SerializerType::RootSerializer rootSerializer;
    typename SerializerType::Serializer serializer(rootSerializer);
    serializer.serializeMap([&](SerializingConcept auto& s){
        ([&](auto&& pair) {
            string key(pair.first); // don't like copy!
            SerializingConcept auto ks = s.serializerForKey(key);
            ks.serialize(std::forward<decltype(pair.second)>(pair.second));
        } (std::forward<decltype(values)>(values)), ...);
    });
    return rootSerializer.finish();
}

// Serialize a vector from a perfectly-forwarded parameter pack.
template<typename SerializerType, typename... ValueTypes>
typename SerializerType::BufferType serialize(ValueTypes&&... values) {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "serialize(&&values)" << endl;
    }
    typename SerializerType::RootSerializer rootSerializer;
    typename SerializerType::Serializer serializer(rootSerializer);
    serializer.serializeVector([&](SerializingConcept auto& s){
        (s.serialize(std::forward<ValueTypes>(values)), ...);      
    });
    return rootSerializer.finish();
}


// --------------
// ...and a special handle-it-all generic conversion function,
// to convert from one serialization format to another.

template<typename SrcSerializerType, typename DstSerializerType>
typename DstSerializerType::BufferType convert(const typename SrcSerializerType::BufferType& src) {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "convert(" << SrcSerializerType::Name << ") to: " << DstSerializerType::Name << endl;
    }
    typename DstSerializerType::RootSerializer rootSerializer;
    typename DstSerializerType::Serializer serializer(rootSerializer);
    serializer.Serializer::serialize(src);
    return rootSerializer.finish();
}

}
