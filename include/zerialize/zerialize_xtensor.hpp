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
//template<> inline constexpr string_view tensor_dtype_name<intptr_t> = "intptr";
//template<> inline constexpr string_view tensor_dtype_name<uintptr_t> = "uintptr";
template<> inline constexpr string_view tensor_dtype_name<float> = "float";
template<> inline constexpr string_view tensor_dtype_name<double> = "double";
//template<> inline constexpr string_view tensor_dtype_name<complex64> = "complex64";
//template<> inline constexpr string_view tensor_dtype_name<complex128> = "complex128";
template<> inline constexpr string_view tensor_dtype_name<xtl::half_float> = "xtl::half_float";

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
        case tensor_dtype_index<xtl::half_float>: return tensor_dtype_name<xtl::half_float>;
    }
    return "unknown";
}

using TensorShape = vector<uint32_t>;

inline vector<any> shape_of_any(const auto& tshape) {
    // get a vector of the shape as a vector of any.
    // we really should do something about that...
    // We should be able to pass vector<T> to serialize...
    vector<any> shape;
    shape.reserve(tshape.size());
    std::transform(tshape.begin(), tshape.end(), std::back_inserter(shape),
        [](uint32_t x) { return any(x); });
    return shape;
}

template <typename T>
span<const uint8_t> blob_from_tensor(const auto& t) {
    const T* actual_data = t.data();
    const uint8_t* byte_data = (const uint8_t*)actual_data;
    size_t num_bytes = t.size() * sizeof(T);
    auto blob = span<const uint8_t>(byte_data, num_bytes);
    return blob;
}

constexpr char ShapeKey[] = "shape";
constexpr char DTypeKey[] = "dtype";
constexpr char DataKey[] = "data";

template <typename S, typename T, size_t D>
S::SerializingFunction serializer(const xt::xtensor<T, D>& t) {
    return [&t](S::Serializer& s) {
        s.serializeMap([&t](S::Serializer& ser) {
            ser.serialize(ShapeKey, shape_of_any(t.shape()));
            ser.serialize(DTypeKey, tensor_dtype_index<T>);
            ser.serialize(DataKey, blob_from_tensor<T>(t));
        });
    };
}

template <typename S, typename T>
S::SerializingFunction serializer(const xt::xarray<T>& t) {
    return [&t](S::Serializer& s) {
        s.serializeMap([&t](S::Serializer& ser) {
            ser.serialize(ShapeKey, shape_of_any(t.shape()));
            ser.serialize(DTypeKey, tensor_dtype_index<T>);
            ser.serialize(DataKey, blob_from_tensor<T>(t));
        });
    };
}

template <typename T>
bool isTensor(const Deserializable auto& buf) {
    if (!buf.isMap()) return false;
    set<string_view> keys = buf.mapKeys();
    return keys.contains(ShapeKey) && buf[ShapeKey].isArray() && 
           keys.contains(DTypeKey) && buf[DTypeKey].isInt() && buf[DTypeKey].asInt8() == tensor_dtype_index<T> &&
           keys.contains(DataKey) && buf[DataKey].isBlob();
}

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
using flextensor_adaptor = const xt::xarray_adaptor<
    xt::xbuffer_adaptor<
        T *&,
        xt::no_ownership,
        std::allocator<T>
    >,
    xt::layout_type::row_major,
    std::vector<uint32_t, std::allocator<uint32_t>>,
    xt::xtensor_expression_tag
>;

template <typename T, int D=-1>
auto asXTensor(const Deserializable auto& buf) {
    if (!isTensor<T>(buf)) { throw DeserializationError("not a tensor"); }

    // Check that the serialized dtype matches T
    auto dtype = buf[DTypeKey].asInt32();
    if (dtype != tensor_dtype_index<T>) throw SerializationError(string("asXTensor asked to deserialize a tensor of type ") + string(tensor_dtype_name<T>) + " but found a tensor of type " + string(type_name_from_code(dtype)));

    // get the shape
    TensorShape vshape = tensor_shape(buf[ShapeKey]);

    // perform dimension check
    if constexpr(D >= 0) {
        if (vshape.size() != D) {
            throw SerializationError("asXTensor asked to deserialize a tensor of rank " + std::to_string(D) + " but found a tensor of rank " +  std::to_string(vshape.size()));
        }
    }

    // get the blob data
    auto blob = buf[DataKey].asBlob();
    const uint8_t * data_bytes = blob.data();
    const T * data_typed_const = (const T*)data_bytes;
    T* data_typed = const_cast<T*>(data_typed_const);

    // create an xtensor-adapter that can perform 
    // in-place adaptation of the blob into an xtensor
    flextensor_adaptor<T> in_place_xtensor = xt::adapt(
        data_typed,
        blob.size() / sizeof(T),
        xt::no_ownership(),
        vshape
    );

    // Depending on the blob type (is it owned or passed), we might
    // have to instantiate (copy) the tensor. This will happen for 
    // serializable types that do not support 0-copy blobs (such as Json).
    if constexpr (std::is_same_v<decltype(blob), std::vector<uint8_t>>) {
        // We've gotta copy it...
        return xt::xarray<T>(in_place_xtensor);
    } else if constexpr (std::is_same_v<decltype(blob), std::span<const uint8_t>>) {
        return in_place_xtensor;
    }
}

} // namespace xtensor
} // namespace zerialize
