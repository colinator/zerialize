#pragma once

#include <array>
#include <cstring>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
//#include <zerialize/zerialize.hpp>
#include <Eigen/Dense>
#include <zerialize/zerialize.hpp>
#include <zerialize/tensor/utils.hpp>
#include <zerialize/zbuilders.hpp>

namespace Eigen {

template <typename T, int R, int C, int Options, zerialize::Writer W>
void serialize(const Eigen::Matrix<T, R, C, Options>& m, W& w) {
    const std::array<std::size_t, 2> shape{
        static_cast<std::size_t>(m.rows()),
        static_cast<std::size_t>(m.cols())
    };

    zerialize::zvec(
        zerialize::tensor_dtype_index<T>,
        shape,
        zerialize::span_from_data_of(m)
    )(w);
}

} // namespace Eigen

namespace zerialize {
namespace eigen {

// A small "owned-or-view" wrapper for an Eigen matrix deserialized from a tensor.
//
// Motivation:
// - Some protocols expose blobs as a non-owning `std::span<const std::byte>` (true zero-copy possible).
// - Other protocols materialize blobs as an owning `std::vector<std::byte>` (e.g. JSON base64 decode),
//   so returning an Eigen view would dangle unless we take ownership.
//
// `EigenMatrixView` type-erases only the *ownership* of the underlying bytes:
// - If a zero-copy view is safe, it stores a span into the reader's backing storage.
// - Otherwise it stores an owning `Eigen::Matrix` copy.
//
// Lifetime note: if `owned_` is empty, the returned object references the underlying reader storage;
// callers must keep the reader/buffer alive as long as they use `.map()`.
template <typename T, int NRows = Eigen::Dynamic, int NCols = Eigen::Dynamic, int Options = Eigen::ColMajor>
class EigenMatrixView {
public:
    using MatrixType = Eigen::Matrix<T, NRows, NCols, Options | Eigen::DontAlign>;
    using MapType = Eigen::Map<const MatrixType, Eigen::Unaligned>;

    // Construct a non-owning view over raw bytes. The caller must ensure the bytes outlive this object.
    explicit EigenMatrixView(std::span<const std::byte> bytes, std::size_t rows, std::size_t cols)
        : bytes_(bytes), rows_(rows), cols_(cols) {}

    // Construct an owning view by taking ownership of a fully materialized matrix.
    explicit EigenMatrixView(MatrixType owned, std::size_t rows, std::size_t cols)
        : owned_(std::move(owned)), rows_(rows), cols_(cols) {}

    std::size_t rows() const { return rows_; }
    std::size_t cols() const { return cols_; }

    // Returns an `Eigen::Map` that views the underlying storage.
    // This is always safe to call: if the view is not zero-copy, it maps the owned matrix storage.
    MapType map() const {
        if constexpr (NRows == Eigen::Dynamic || NCols == Eigen::Dynamic) {
            return MapType(data(), static_cast<Eigen::Index>(rows_), static_cast<Eigen::Index>(cols_));
        } else {
            return MapType(data());
        }
    }

    // Returns an owning `Eigen::Matrix` (copying if the view is backed by a span).
    MatrixType matrix() const {
        if (owned_) return *owned_;
        if constexpr (NRows == Eigen::Dynamic || NCols == Eigen::Dynamic) {
            MatrixType out(static_cast<Eigen::Index>(rows_), static_cast<Eigen::Index>(cols_));
            std::memcpy(out.data(), bytes_.data(), bytes_.size());
            return out;
        } else {
            MatrixType out;
            std::memcpy(out.data(), bytes_.data(), bytes_.size());
            return out;
        }
    }

private:
    // Pointer to the first scalar element, regardless of whether storage is owned or borrowed.
    const T* data() const {
        if (owned_) return owned_->data();
        return reinterpret_cast<const T*>(bytes_.data());
    }

    std::optional<MatrixType> owned_;
    std::span<const std::byte> bytes_{};
    std::size_t rows_ = 0;
    std::size_t cols_ = 0;
};

template <typename T, int NRows, int NCols, bool TensorIsMap = false, int Options = Eigen::ColMajor>
EigenMatrixView<T, NRows, NCols, Options> asEigenMatrixView(const Reader auto& buf) {
    using ViewType = EigenMatrixView<T, NRows, NCols, Options>;
    using MatrixType = typename ViewType::MatrixType;

    // Note: `isTensor` does dtype/shape/blob presence checks but does not validate payload size.
    if (!isTensor<T, TensorIsMap>(buf)) { throw DeserializationError("not a tensor"); }

    auto dtype_ref = TensorIsMap ? buf[DTypeKey] : buf[0];
    auto dtype = dtype_ref.asInt32();
    if (dtype != tensor_dtype_index<T>) {
        throw DeserializationError(
            std::string("asEigenMatrixView asked to deserialize a matrix of type ") +
            std::string(tensor_dtype_name<T>) + " but found a matrix of type " + std::string(type_name_from_code(dtype))
        );
    }

    auto shape_ref = TensorIsMap ? buf[ShapeKey] : buf[1];
    TensorShape vshape = tensor_shape(shape_ref);
    if (vshape.size() != 2) {
        throw DeserializationError(
            "asEigenMatrixView asked to deserialize a matrix of rank 2 but found a matrix of rank " + std::to_string(vshape.size())
        );
    }

    const std::size_t rows = static_cast<std::size_t>(vshape[0]);
    const std::size_t cols = static_cast<std::size_t>(vshape[1]);

    if constexpr (NRows != Eigen::Dynamic) {
        if (rows != static_cast<std::size_t>(NRows)) {
            throw DeserializationError("asEigenMatrixView expected " + std::to_string(NRows) + " rows, but found " + std::to_string(rows) + ".");
        }
    }
    if constexpr (NCols != Eigen::Dynamic) {
        if (cols != static_cast<std::size_t>(NCols)) {
            throw DeserializationError("asEigenMatrixView expected " + std::to_string(NCols) + " cols, but found " + std::to_string(cols) + ".");
        }
    }

    auto data_ref = TensorIsMap ? buf[DataKey] : buf[2];
    auto blob = data_ref.asBlob();

    auto to_span = [](auto&& b) {
        using B = std::remove_cvref_t<decltype(b)>;
        if constexpr (std::is_same_v<B, std::span<const std::byte>>) {
            return b;
        } else {
            return std::span<const std::byte>(std::data(b), std::size(b));
        }
    };
    auto bytes = to_span(blob);

    const std::size_t expected = rows * cols * sizeof(T);
    if (bytes.size() != expected) {
        throw DeserializationError(
            "asEigenMatrixView expected " + std::to_string(expected) + " bytes, but found " + std::to_string(bytes.size())
        );
    }

    auto make_copy = [&] {
        if constexpr (NRows == Eigen::Dynamic || NCols == Eigen::Dynamic) {
            MatrixType copy(static_cast<Eigen::Index>(rows), static_cast<Eigen::Index>(cols));
            std::memcpy(copy.data(), bytes.data(), bytes.size());
            return ViewType(std::move(copy), rows, cols);
        } else {
            MatrixType copy;
            std::memcpy(copy.data(), bytes.data(), bytes.size());
            return ViewType(std::move(copy), rows, cols);
        }
    };

    // If the blob is owning (e.g. JSON), we must copy to keep storage alive.
    if constexpr (!std::is_same_v<decltype(blob), std::span<const std::byte>>) {
        return make_copy();
    } else {
        // For non-owning blobs, only take the view if the data meets Eigen's scalar alignment needs.
        std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(bytes.data());
        if ((addr % alignof(T)) != 0) return make_copy();
        return ViewType(bytes, rows, cols);
    }
}

// Deserialize an eigen matrix/map
template <typename T, int NRows, int NCols, bool TensorIsMap=false, int Options=Eigen::ColMajor>
Eigen::Matrix<T, NRows, NCols, Options | Eigen::DontAlign> 
//auto 
asEigenMatrix(const Reader auto& buf) {
    return asEigenMatrixView<T, NRows, NCols, TensorIsMap, Options>(buf).matrix();
}

} // eigen
} // zerialize
