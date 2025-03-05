#pragma once

#include "zerialize.hpp"
#include <complex>

// boooo... xtensor dependency...
#include <xtl/xhalf_float.hpp>

namespace zerialize {

using std::complex;

template<typename T>
inline constexpr int tensor_dtype_index = -1;
template<> inline constexpr int tensor_dtype_index<int8_t>   = 0;
template<> inline constexpr int tensor_dtype_index<int16_t>  = 1;
template<> inline constexpr int tensor_dtype_index<int32_t>  = 2;
template<> inline constexpr int tensor_dtype_index<int64_t>  = 3;
template<> inline constexpr int tensor_dtype_index<uint8_t>  = 4;
template<> inline constexpr int tensor_dtype_index<uint16_t> = 5;
template<> inline constexpr int tensor_dtype_index<uint32_t> = 6;
template<> inline constexpr int tensor_dtype_index<uint64_t> = 7;
template<> inline constexpr int tensor_dtype_index<float> = 10;
template<> inline constexpr int tensor_dtype_index<double> = 11;
template<> inline constexpr int tensor_dtype_index<complex<float>> = 12;
template<> inline constexpr int tensor_dtype_index<complex<double>> = 13;
template<> inline constexpr int tensor_dtype_index<xtl::half_float> = 14;

template<typename T>
inline constexpr string_view tensor_dtype_name = "";
template<> inline constexpr string_view tensor_dtype_name<int8_t>   = "int8";
template<> inline constexpr string_view tensor_dtype_name<int16_t>  = "int16";
template<> inline constexpr string_view tensor_dtype_name<int32_t>  = "int32";
template<> inline constexpr string_view tensor_dtype_name<int64_t>  = "int64";
template<> inline constexpr string_view tensor_dtype_name<uint8_t>  = "uint8";
template<> inline constexpr string_view tensor_dtype_name<uint16_t> = "uint16";
template<> inline constexpr string_view tensor_dtype_name<uint32_t> = "uint32";
template<> inline constexpr string_view tensor_dtype_name<uint64_t> = "uint64";
template<> inline constexpr string_view tensor_dtype_name<float> = "float";
template<> inline constexpr string_view tensor_dtype_name<double> = "double";
template<> inline constexpr string_view tensor_dtype_name<complex<float>> = "complex<float>";
template<> inline constexpr string_view tensor_dtype_name<complex<double>> = "complex<double>";
template<> inline constexpr string_view tensor_dtype_name<xtl::half_float> = "xtl::half_float";

// Unsupported...
//template<> inline constexpr int tensor_dtype_index<intptr_t> = 8;
//template<> inline constexpr int tensor_dtype_index<uintptr_t> = 9;


inline string_view type_name_from_code(int type_code) {
    switch (type_code) {
        case tensor_dtype_index<int8_t>: return tensor_dtype_name<int8_t>;
        case tensor_dtype_index<int16_t>: return tensor_dtype_name<int16_t>;
        case tensor_dtype_index<int32_t>: return tensor_dtype_name<int32_t>;
        case tensor_dtype_index<int64_t>: return tensor_dtype_name<int64_t>;
        case tensor_dtype_index<uint8_t>: return tensor_dtype_name<uint8_t>;
        case tensor_dtype_index<uint16_t>: return tensor_dtype_name<uint16_t>;
        case tensor_dtype_index<uint32_t>: return tensor_dtype_name<uint32_t>;
        case tensor_dtype_index<uint64_t>: return tensor_dtype_name<uint64_t>;
        case tensor_dtype_index<float>: return tensor_dtype_name<float>;
        case tensor_dtype_index<double>: return tensor_dtype_name<double>;
        case tensor_dtype_index<complex<float>>: return tensor_dtype_name<complex<float>>;
        case tensor_dtype_index<complex<double>>: return tensor_dtype_name<complex<double>>;
        case tensor_dtype_index<xtl::half_float>: return tensor_dtype_name<xtl::half_float>;
    }
    return "unknown";
}

constexpr char ShapeKey[] = "shape";
constexpr char DTypeKey[] = "dtype";
constexpr char DataKey[] = "data";

using TensorShapeElement = uint32_t;
using TensorShape = vector<TensorShapeElement>;

constexpr bool TensorIsMap = false;

inline vector<any> shape_of_any(const auto& tshape) {
    // get a vector of the shape as a vector of any.
    // we really should do something about that...
    // We should be able to pass vector<T> to serialize...
    vector<any> shape;
    shape.reserve(tshape.size());
    std::transform(tshape.begin(), tshape.end(), std::back_inserter(shape),
        [](TensorShapeElement x) { return any(x); });
    return shape;
}

TensorShape tensor_shape(const Deserializable auto& d) {
    if (!d.isArray()) {
        return {};
    }
    TensorShape vshape;
    vshape.reserve(d.arraySize());
    for (size_t i=0; i<d.arraySize(); i++) {
        vshape.push_back(d[i].asUInt64());
    }
    return vshape;
}

template <typename C>
concept HasDataAndSize = requires(const C& c) {
    { c.data() } -> std::convertible_to<const typename C::value_type*>; 
    { c.size() } -> std::convertible_to<std::size_t>;
};

template <HasDataAndSize C>
std::span<const std::uint8_t> span_from_data_of(const C& container) {
    using T = typename C::value_type; 
    const T* actual_data = container.data();
    const std::uint8_t* byte_data = reinterpret_cast<const std::uint8_t*>(actual_data);
    std::size_t num_bytes = container.size() * sizeof(T);
    return std::span<const std::uint8_t>(byte_data, num_bytes);
}

template <typename C>
concept Blobby = requires(const C& c) {
    { c.data() } -> std::convertible_to<const uint8_t*>; 
    { c.size() } -> std::convertible_to<std::size_t>;
};

template <typename T>
T* data_from_blobby(const Blobby auto& blob) {
    const uint8_t * data_bytes = blob.data();
    const T * data_typed_const = (const T*)data_bytes;
    T* data_typed = const_cast<T*>(data_typed_const);
    return data_typed;
}

template <typename T>
bool isTensor(const Deserializable auto& buf) {
    if constexpr (TensorIsMap) {
        if (!buf.isMap()) return false;
        set<string_view> keys = buf.mapKeys();
        return 
            keys.contains(ShapeKey) && buf[ShapeKey].isArray() && 
            keys.contains(DTypeKey) && buf[DTypeKey].isInt() && buf[DTypeKey].asInt8() == tensor_dtype_index<T> &&
            keys.contains(DataKey) && buf[DataKey].isBlob();
    } else {
       if (!buf.isArray()) return false;
        if (buf.arraySize() < 3) return false;
        return 
            buf[0].isInt() && buf[0].asInt8() == tensor_dtype_index<T> &&
            buf[1].isArray() && 
            buf[2].isBlob();
    }
}

} // namespace zerialize
