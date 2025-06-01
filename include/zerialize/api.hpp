#pragma once

#include <any>
#include <initializer_list>
#include <tuple>
#include <string>
#include <string_view>
#include <iostream>
#include "serializer.hpp"
#include "zbuffer.hpp"

namespace zerialize {

using std::any, std::initializer_list, std::pair, std::string, std::string_view;
using std::cout, std::endl;


constexpr bool DEBUG_TRACE_CALLS = false;


// --------------

// Define various serializer functions: the 7 canonical forms, so
// we can pass nothing, an any, initializer lists for
// vectors and maps, a perfect-forwarded value, and 
// perfect-forwarded lists and maps.
//
// 1. serialize root via a serialization function (all other serializers use this)
// 2. Three 'std::any' path serializers - any, initializer-list any, map of string: any
// 3. Two perfectly-forwarded parameter pack serialization methods, taking a single
//    value, a parameter pack of values, or a map of strings to values
// 4. The zkv/zmap/zvec helper functions, and single associated serialize map function.
//    These helper functions just return lambdas that carry out the actual serialization.

// --- Serialize, as root, via a serialization function ---

template<typename SerializerType, typename F>
ZBuffer serialize_root(F&& func) {
    typename SerializerType::RootSerializer rootSerializer;
    typename SerializerType::Serializer serializer(rootSerializer);
    func(serializer);
    return rootSerializer.finish();
}


// --- The std::any paths, which allow convenient brace-initializer expressions ---

// Serialize anything, as an 'any' value.
template <typename SerializerType>
ZBuffer serialize(const any& v) {
    if constexpr (DEBUG_TRACE_CALLS) { cout << "serialize(any: " << v.type().name() << ")" << endl; }
    return serialize_root<SerializerType>([&v](auto& ser) {
        ser.serializeAny(v);
    });
}

// Serialize a vector from an initializer list of any.
template <typename SerializerType>
ZBuffer serialize(initializer_list<any> l) {
    if constexpr (DEBUG_TRACE_CALLS) { cout << "serialize(initializer list)" << endl; }
    return serialize_root<SerializerType>([&l](auto& ser) {
        ser.serializeVector([&l](Serializable auto& s){
            for (const any& val : l) {
                s.serializeAny(val);
            }           
        });
    });
}

// Serialize a map from an initializer list of pair<string, any>.
template <typename SerializerType>
ZBuffer serialize(initializer_list<pair<string, any>> l) {
    if constexpr (DEBUG_TRACE_CALLS) { cout << "serialize(initializer list/map)" << endl; }
    return serialize_root<SerializerType>([&l](auto& ser) {
        ser.serializeMap([&l](Serializable auto& s){
            for (const auto& [key, val] : l) {
                s.serialize(key, val);
            }
        });
    });
}


// --- Perfectly-forwarded value serialization ---

// Serialize a perfect-forwarded value.
template<typename SerializerType, typename ValueType>
void serialize_impl(SerializerType& ser, ValueType&& value) {
    ser.serialize(std::forward<ValueType>(value));
}

// Serialize a vector from a perfectly-forwarded parameter pack.
template<typename SerializerType, typename... ValueTypes>
void serialize_impl(SerializerType& ser, ValueTypes&&... values) {
    ser.serializeVector([&](Serializable auto& s) constexpr noexcept -> void {
        (s.serialize(std::forward<ValueTypes>(values)), ...);      
    });
}

// serialize a single value or a pack of values...
template<typename SerializerType, typename... ValueTypes>
ZBuffer serialize(ValueTypes&&... values) {
    if constexpr (DEBUG_TRACE_CALLS) { cout << "serialize(&&values generic)" << endl; }
    return serialize_root<SerializerType>([&values...](auto& ser) constexpr noexcept {
        if constexpr (sizeof...(values) > 0) {
            serialize_impl(ser, std::forward<ValueTypes>(values)...);
        }
    });
}

// Serialize a map from pairs of string keys and  perfectly-forwarded values.
template<typename SerializerType, typename... ValueTypes>
requires (sizeof...(ValueTypes) > 0)
ZBuffer serialize(pair<const char*, ValueTypes>&&... values) {
    if constexpr (DEBUG_TRACE_CALLS) { cout << "serialize(&& value pairs)" << endl; }
    return serialize_root<SerializerType>([&values...](auto& ser) {
        ser.serializeMap([&values...](Serializable auto& s) constexpr noexcept -> void {
            ([&](auto&& pair) constexpr noexcept -> void {
                Serializable auto ks = s.serializerForKey(pair.first);
                ks.serialize(std::forward<decltype(pair.second)>(pair.second));
            } (std::forward<decltype(values)>(values)), ...);
        });
    });
}


// --- Special forms: zkv, zmap, zvec; the fastest way to serialize things. ---

namespace zkvdetail {

    // Holds a reference to the value
    template <typename T>
    struct KeyValueRef {
        std::string_view key;
        const T& value;
    };

    // Trait to recognize KVRef types
    template <typename T>
    struct is_kv : std::false_type {};

    template <typename T>
    struct is_kv<KeyValueRef<T>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_kv_v = is_kv<std::remove_cvref_t<T>>::value;

    template <typename... Ts>
    constexpr bool all_are_kv = (is_kv_v<Ts> && ...);

    template<std::size_t I = 0, typename Tuple, typename F>
    constexpr void tuple_for_each(Tuple&& tuple, F&& f) {
        if constexpr (I < std::tuple_size_v<std::remove_reference_t<Tuple>>) {
            f(std::get<I>(std::forward<Tuple>(tuple)));
            tuple_for_each<I + 1>(std::forward<Tuple>(tuple), std::forward<F>(f));
        }
    }
} // namespace zkvdetail


template <typename T>
zkvdetail::KeyValueRef<T> zkv(std::string_view key, const T& value) {
    return zkvdetail::KeyValueRef<T>{key, value};
}

template <typename... KVTypes>
requires zkvdetail::all_are_kv<KVTypes...>
[[nodiscard]]
constexpr auto zmap(KVTypes&&... kvs) noexcept {
    return 
        [data = std::forward_as_tuple(std::forward<KVTypes>(kvs)...)]
        (Serializable auto& ser) constexpr noexcept -> void
    {
        ser.serializeMap([&data](Serializable auto& s) {
            zkvdetail::tuple_for_each(data, [&s](const auto& kv_ref) {
                s.serializerForKey(kv_ref.key).serialize(kv_ref.value);
            });
        });
    };
}

template <typename... ValueTypes>
[[nodiscard]]
constexpr auto zvec(ValueTypes&&... values) noexcept {
    return 
        [data = std::forward_as_tuple(std::forward<ValueTypes>(values)...)]
        (Serializable auto& ser) constexpr noexcept -> void
    {
        ser.serializeVector([&data](Serializable auto& s) constexpr noexcept -> void {
            zkvdetail::tuple_for_each(data, [&s](const auto& value) {
                s.serialize(value);
            });
        });
    };
}

// Map-based serialize overload for KeyValueRef list.
template <typename SerializerType, typename... KVTypes>
requires (std::conjunction_v<std::bool_constant<zkvdetail::is_kv_v<KVTypes>>...>)
ZBuffer serialize(KVTypes&&... kvs) {
    if constexpr (DEBUG_TRACE_CALLS) { cout << "serialize(KVTypes&&...)" << endl; }
    return serialize_root<SerializerType>([&kvs...](auto& ser) {
        ser.serializeMap([&kvs...](Serializable auto& s) {
            (s.serializerForKey(kvs.key).serialize(kvs.value), ...);
        });
    });
}


// --------------
// ...and a special handle-it-all generic conversion function,
// to convert from one serialization format to another.

template<typename SrcSerializerType, typename DstSerializerType>
typename DstSerializerType::BufferType convert(const typename SrcSerializerType::BufferType& srcSerializer) {
    if constexpr (DEBUG_TRACE_CALLS) {
        cout << "convert(" << SrcSerializerType::Name << ") to: " << DstSerializerType::Name << endl;
    }
    typename DstSerializerType::RootSerializer rootSerializer;
    typename DstSerializerType::Serializer serializer(rootSerializer);
    serializer.Serializer::serialize(srcSerializer);
    ZBuffer buf(rootSerializer.finish());
    return typename DstSerializerType::BufferType(buf.to_vector_copy());
}

} // namespace zerialize
