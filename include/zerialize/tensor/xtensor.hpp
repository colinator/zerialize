#pragma once

#include <xtensor/containers/xtensor.hpp>
#include <xtensor/containers/xadapt.hpp>
#include <xtensor/containers/xarray.hpp>
#include <xtensor/io/xio.hpp>
#include <xtensor/core/xexpression.hpp>
#include <zerialize/zerialize.hpp>
#include <zerialize/tensor/utils.hpp>
#include <zerialize/zbuilders.hpp>

namespace xt {

template <zerialize::HasDataAndSize X, zerialize::Writer W>
void serialize(const X& t, W& w) {
    zerialize::zvec(
        zerialize::tensor_dtype_index<typename X::value_type>,
        zerialize::shape_of_sizet(t.shape()),
        zerialize::span_from_data_of(t)
    )
    (w);
}

} // namespace xt


namespace zerialize {
namespace xtensor {

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
    std::vector<TensorShapeElement, std::allocator<TensorShapeElement>>,
    xt::xtensor_expression_tag
>;

// Deserialize an xtensor/x-adapter
template <typename T, int D=-1, bool TensorIsMap=false>
auto asXTensor(const Reader auto& buf) {
    if (!isTensor<T>(buf)) { throw DeserializationError("not a tensor"); }

    // Check that the serialized dtype matches T
    auto dtype_ref = TensorIsMap ? buf[DTypeKey] : buf[0];
    auto dtype = dtype_ref.asInt32();
    if (dtype != tensor_dtype_index<T>) throw DeserializationError(std::string("asXTensor asked to deserialize a tensor of type ") + std::string(tensor_dtype_name<T>) + " but found a tensor of type " + std::string(type_name_from_code(dtype)));

    // get the shape
    auto shape_ref = TensorIsMap ? buf[ShapeKey] : buf[1];
    TensorShape vshape = tensor_shape(shape_ref);

    // perform dimension check
    if constexpr(D >= 0) {
        if (vshape.size() != D) {
            throw DeserializationError("asXTensor asked to deserialize a tensor of rank " + std::to_string(D) + " but found a tensor of rank " +  std::to_string(vshape.size()));
        }
    }

    // get the blob data, through old-school c-typing as a T*
    auto data_ref = TensorIsMap ? buf[DataKey] : buf[2];
    auto blob = data_ref.asBlob();
    T* data_typed = data_from_blobview<T>(blob);

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
    if constexpr (std::is_same_v<decltype(blob), std::vector<std::byte>>) {
        // We've gotta copy it...
        return xt::xarray<T>(in_place_xtensor);
    } else { //if constexpr (std::is_same_v<decltype(blob), span<const uint8_t>>) {
        return in_place_xtensor;
    }
}

} // namespace xtensor
} // namespace zerialize
