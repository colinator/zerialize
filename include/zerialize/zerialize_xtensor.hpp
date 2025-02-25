#pragma once

#include "zerialize.hpp"
#include <xtensor/xtensor.hpp>
#include <xtensor/xadapt.hpp>
#include <xtensor/xarray.hpp>
#include <xtensor/xio.hpp>
#include "zerialize_tensor_utils.h"

namespace zerialize {
namespace xtensor {

template <typename T>
span<const uint8_t> _blob_from_xtensor(const auto& t) {
    const T* actual_data = t.data();
    const uint8_t* byte_data = (const uint8_t*)actual_data;
    size_t num_bytes = t.size() * sizeof(T);
    auto blob = span<const uint8_t>(byte_data, num_bytes);
    return blob;
}

// Serialize an xtensor
template <typename S, typename T, size_t D>
S::SerializingFunction serializer(const xt::xtensor<T, D>& t) {
    return [&t](S::Serializer& s) {
        s.serializeMap([&t](S::Serializer& ser) {
            ser.serialize(ShapeKey, shape_of_any(t.shape()));
            ser.serialize(DTypeKey, tensor_dtype_index<T>);
            ser.serialize(DataKey, _blob_from_xtensor<T>(t));
        });
    };
}

// Serialize an xarray
template <typename S, typename T>
S::SerializingFunction serializer(const xt::xarray<T>& t) {
    return [&t](S::Serializer& s) {
        s.serializeMap([&t](S::Serializer& ser) {
            ser.serialize(ShapeKey, shape_of_any(t.shape()));
            ser.serialize(DTypeKey, tensor_dtype_index<T>);
            ser.serialize(DataKey, _blob_from_xtensor<T>(t));
        });
    };
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

// Deserialize an xtensor/x-adapter
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

    // get the blob data, through old-school c-typing as a T*
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

    // Depending on the blob type (is it owned or not), we might
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
