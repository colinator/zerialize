#pragma once

#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>
#include <cstdint>
#include <xtensor/containers/xtensor.hpp>
#include <xtensor/containers/xadapt.hpp>
#include <xtensor/containers/xarray.hpp>
#include <xtensor/io/xio.hpp>
#include <xtensor/core/xexpression.hpp>
#include <zerialize/zerialize.hpp>
#include <zerialize/tensor/utils.hpp>
#include <zerialize/tensor/view_info.hpp>
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

// An "owned-or-view" wrapper for an xtensor deserialized from a tensor.
//
// Like Eigen, zero-copy is only possible when:
// - the blob is non-owning (e.g. `std::span<const std::byte>` from flexbuffers/cbor), and
// - the blob pointer is aligned for `T` (address % alignof(T) == 0), and
// - the blob size matches prod(shape) * sizeof(T).
//
// If those conditions are not met (including protocols that materialize blobs as owning
// `std::vector<std::byte>`, such as JSON), this stores an owning `xt::xarray<T>` copy.
template <typename T>
class XTensorView {
public:
    using ArrayType = xt::xarray<T>;

    explicit XTensorView(std::span<const std::byte> bytes, TensorShape shape, std::size_t element_count, tensor::TensorViewInfo info)
        : info_(info), bytes_(bytes), shape_(std::move(shape)), element_count_(element_count) {}

    explicit XTensorView(ArrayType owned, TensorShape shape, std::size_t element_count, tensor::TensorViewInfo info)
        : info_(info), owned_(std::move(owned)), shape_(std::move(shape)), element_count_(element_count) {}

    std::size_t rank() const { return shape_.size(); }
    const TensorShape& shape() const { return shape_; }
    const tensor::TensorViewInfo& viewInfo() const { return info_; }

    // Returns an xtensor adaptor that views the underlying storage.
    // If the view is not zero-copy, it adapts the owned array storage.
    auto tensor() const {
        T* ptr = owned_ ? const_cast<T*>(owned_->data()) : const_cast<T*>(reinterpret_cast<const T*>(bytes_.data()));
        return xt::adapt(ptr, element_count_, xt::no_ownership(), shape_);
    }

    // Returns an owning xarray (copying if this is backed by a span).
    ArrayType array() const {
        if (owned_) return *owned_;
        auto out = ArrayType::from_shape(shape_as_sizet());
        std::memcpy(out.data(), bytes_.data(), bytes_.size());
        return out;
    }

private:
    std::vector<std::size_t> shape_as_sizet() const {
        std::vector<std::size_t> out;
        out.reserve(shape_.size());
        for (auto d : shape_) out.push_back(static_cast<std::size_t>(d));
        return out;
    }

    tensor::TensorViewInfo info_{};
    std::optional<ArrayType> owned_;
    std::span<const std::byte> bytes_{};
    TensorShape shape_{};
    std::size_t element_count_ = 0;
};

// Multiply all dimensions (with overflow checking) to get total element count.
// This is used for validating that the blob size matches prod(shape) * sizeof(T).
inline std::size_t checked_element_count(const TensorShape& shape) {
    std::size_t count = 1;
    for (auto d : shape) {
        const std::size_t dim = static_cast<std::size_t>(d);
        if (dim == 0) return 0;
        if (count > (std::numeric_limits<std::size_t>::max() / dim)) {
            throw DeserializationError("tensor element count overflow");
        }
        count *= dim;
    }
    return count;
}

// Normalize the protocol-specific blob representation into a `std::span<const std::byte>`.
// Some protocols return non-owning spans (zero-copy possible); others return owning vectors.
template <typename Blob>
inline std::span<const std::byte> blob_to_span(const Blob& blob) {
    if constexpr (std::is_same_v<Blob, std::span<const std::byte>>) {
        return blob;
    } else {
        return std::span<const std::byte>(std::data(blob), std::size(blob));
    }
}

// Deserialize as a view-wrapper that can be zero-copy when safe, and otherwise owns a copy.
template <typename T, int D = -1, bool TensorIsMap = false>
XTensorView<T> asXTensorView(const Reader auto& buf) {
    if (!isTensor<T, TensorIsMap>(buf)) { throw DeserializationError("not a tensor"); }

    auto dtype_ref = TensorIsMap ? buf[DTypeKey] : buf[0];
    auto dtype = dtype_ref.asInt32();
    if (dtype != tensor_dtype_index<T>) {
        throw DeserializationError(
            std::string("asXTensorView asked to deserialize a tensor of type ") +
            std::string(tensor_dtype_name<T>) + " but found a tensor of type " + std::string(type_name_from_code(dtype))
        );
    }

    auto shape_ref = TensorIsMap ? buf[ShapeKey] : buf[1];
    TensorShape vshape = tensor_shape(shape_ref);

    if constexpr (D >= 0) {
        if (vshape.size() != static_cast<std::size_t>(D)) {
            throw DeserializationError(
                "asXTensorView asked to deserialize a tensor of rank " + std::to_string(D) +
                " but found a tensor of rank " + std::to_string(vshape.size())
            );
        }
    }

    auto data_ref = TensorIsMap ? buf[DataKey] : buf[2];
    auto blob = data_ref.asBlob();
    auto bytes = blob_to_span(blob);

    // Validate payload size to avoid reading off the end, truncating, or leaving elements uninitialized.
    const std::size_t element_count = checked_element_count(vshape);
    const std::size_t expected_bytes = element_count * sizeof(T);
    if (bytes.size() != expected_bytes) {
        throw DeserializationError(
            "asXTensorView expected " + std::to_string(expected_bytes) + " bytes, but found " + std::to_string(bytes.size())
        );
    }

    // Copy into an owning xarray (which provides proper `T`-aligned storage).
    auto make_copy = [&] {
        xt::xarray<T> out = xt::xarray<T>::from_shape([&] {
            std::vector<std::size_t> s;
            s.reserve(vshape.size());
            for (auto d : vshape) s.push_back(static_cast<std::size_t>(d));
            return s;
        }());
        std::memcpy(out.data(), bytes.data(), bytes.size());
        tensor::TensorViewInfo info{};
        info.zero_copy = false;
        info.reason = tensor::TensorViewReason::NotSpanBacked;
        info.required_alignment = alignof(T);
        info.address = reinterpret_cast<std::uintptr_t>(bytes.data());
        info.byte_size = bytes.size();
        return XTensorView<T>(std::move(out), std::move(vshape), element_count, info);
    };

    if constexpr (!std::is_same_v<decltype(blob), std::span<const std::byte>>) {
        return make_copy();
    } else {
        // Special note about alignment:
        // Even if the serialized bytes "contain floats/doubles", a `std::byte*` can point to any address.
        // Turning those bytes into a `T*` view (which xt::adapt eventually does) is only well-defined if
        // `bytes.data()` is aligned to `alignof(T)`. If it's not, element access becomes undefined behavior
        // on many platforms/compilers. So: view when aligned, copy when not.
        tensor::TensorViewInfo info{};
        info.required_alignment = alignof(T);
        info.address = reinterpret_cast<std::uintptr_t>(bytes.data());
        info.byte_size = bytes.size();
        if ((info.address % alignof(T)) != 0) {
            info.zero_copy = false;
            info.reason = tensor::TensorViewReason::Misaligned;
            xt::xarray<T> out = xt::xarray<T>::from_shape([&] {
                std::vector<std::size_t> s;
                s.reserve(vshape.size());
                for (auto d : vshape) s.push_back(static_cast<std::size_t>(d));
                return s;
            }());
            std::memcpy(out.data(), bytes.data(), bytes.size());
            return XTensorView<T>(std::move(out), std::move(vshape), element_count, info);
        }
        info.zero_copy = true;
        info.reason = tensor::TensorViewReason::Ok;
        return XTensorView<T>(bytes, std::move(vshape), element_count, info);
    }
}

// Deserialize an xtensor/x-adapter
template <typename T, int D=-1, bool TensorIsMap=false>
xt::xarray<T> asXTensor(const Reader auto& buf) {
    return asXTensorView<T, D, TensorIsMap>(buf).array();
}

} // namespace xtensor
} // namespace zerialize
