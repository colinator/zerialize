#pragma once

#include "zerialize.hpp"
#include <xtensor/xtensor.hpp>
#include <xtensor/xadapt.hpp>
#include <xtensor/xarray.hpp>
#include <xtl/xhalf_float.hpp>
#include <xtensor/xio.hpp>

namespace zerialize {
namespace xtensor {


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
//template<> inline constexpr int tensor_dtype_index<intptr_t> = 8;
//template<> inline constexpr int tensor_dtype_index<uintptr_t> = 9;
template<> inline constexpr int tensor_dtype_index<float> = 10;
template<> inline constexpr int tensor_dtype_index<double> = 11;
//template<> inline constexpr int tensor_dtype_index<complex64> = 12;
//template<> inline constexpr int tensor_dtype_index<complex128> = 13;
template<> inline constexpr int tensor_dtype_index<xtl::half_float> = 14;


template <typename S, typename T, size_t D>
S::SerializingFunction serializer(const xt::xtensor<T, D>& t) {

    // get a vector of the shape as a vector of any.
    // we really should do something about that...
    // We should be able to pass vector<T> to serialize...
    std::vector<std::any> shape;
    shape.reserve(shape.size());
    std::transform(t.shape().begin(), t.shape().end(), std::back_inserter(shape),
        [](uint64_t x) { return std::any(x); });

    // get the raw bytes
    const T* actual_data = t.data();
    const uint8_t* byte_data = (const uint8_t*)actual_data;
    size_t num_bytes = t.size() * sizeof(T);
    auto blob = std::span<const uint8_t>(byte_data, num_bytes);
    
    auto r = [shape, blob](S::Serializer& s) {
        s.serializeMap([&shape, &blob](S::Serializer& ser) {
            ser.serialize("shape", shape);
            ser.serialize("dtype", tensor_dtype_index<T>);
            ser.serialize("data", blob);
        });
    };

    return r;
}

template <typename S, typename T>
S::SerializingFunction serializer(const xt::xarray<T>& t) {

    // get a vector of the shape as a vector of any.
    // we really should do something about that...
    // We should be able to pass vector<T> to serialize...
    std::vector<std::any> shape;
    shape.reserve(shape.size());
    std::transform(t.shape().begin(), t.shape().end(), std::back_inserter(shape),
        [](uint64_t x) { return std::any(x); });

    // get the raw bytes
    const T* actual_data = t.data();
    const uint8_t* byte_data = (const uint8_t*)actual_data;
    size_t num_bytes = t.size() * sizeof(T);
    auto blob = std::span<const uint8_t>(byte_data, num_bytes);

    return [&shape, &blob](S::Serializer& s) {
        s.serializeMap([&shape, &blob](S::Serializer& ser) {
            ser.serialize("shape", shape);
            ser.serialize("dtype", tensor_dtype_index<T>);
            ser.serialize("data", blob);
        });
    };
}

template <typename T>
bool isTensor(const Deserializable auto& buf) {
    if (!buf.isMap()) return false;
    set<string_view> keys = buf.mapKeys();
    return keys.contains("shape") && buf["shape"].isArray() && 
           keys.contains("dtype") && buf["dtype"].isInt() && buf["dtype"].asInt8() == tensor_dtype_index<T> &&
           keys.contains("data") && buf["data"].isBlob();
}

using TensorShape = vector<uint32_t>;

inline TensorShape tensor_shape(const Deserializable auto& d) {
    if (!d.isArray()) {
        return {};
    }
    TensorShape vshape;
    for (size_t i=0; i<d.arraySize(); i++) {
        uint64_t shape_element = d[i].asUInt64();
        vshape.push_back(shape_element);
    }
    return vshape;
}

// The actual type that the call to xt::adapt returns is this.
// (the 'auto' keyword works for the return type as well, but it's
// good to know).
template <typename T>
using flextensor_adaptor = xt::xarray_adaptor<
    xt::xbuffer_adaptor<
        T *&,
        xt::no_ownership,
        std::allocator<T>
    >,
    xt::layout_type::row_major,
    std::vector<uint32_t, std::allocator<uint32_t>>,
    xt::xtensor_expression_tag
>;

template <typename T>
auto asXTensor(const Deserializable auto& buf) {
    if (!isTensor<T>(buf)) { throw DeserializationError("not a tensor"); }

    auto shape = buf["shape"];
    auto dtype = buf["dtype"];
    auto blob = buf["data"].asBlob();

    TensorShape vshape = tensor_shape(shape);

    const uint8_t * data_bytes = blob.data();
    const T * data_typed_const = (const T*)data_bytes;
    T* data_typed = const_cast<T*>(data_typed_const);

    flextensor_adaptor<T> in_place_xtensor = xt::adapt(
        data_typed,
        blob.size() / sizeof(T),
        xt::no_ownership(),
        vshape
    );

    if constexpr (std::is_same_v<decltype(blob), std::vector<uint8_t>>) {
        // We've gotta copy it...
        return xt::xarray<T>(in_place_xtensor);
    } else if constexpr (std::is_same_v<decltype(blob), std::span<const uint8_t>>) {
        return in_place_xtensor;
    }
}

} // namespace xtensor
} // namespace zerialize
