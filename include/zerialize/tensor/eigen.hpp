#pragma once

#include <array>
#include <cstring>
#include <cstdint>
#include <zerialize/zerialize.hpp>
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

// Deserialize an eigen matrix/map
template <typename T, int NRows, int NCols, bool TensorIsMap=false, int Options=Eigen::ColMajor>
Eigen::Matrix<T, NRows, NCols, Options | Eigen::DontAlign> asEigenMatrix(const Reader auto& buf) {
    using MatrixType = Eigen::Matrix<T, NRows, NCols, Options | Eigen::DontAlign>;

    if (!isTensor<T>(buf)) { throw DeserializationError("not a tensor"); }

    // Check that the serialized dtype matches T
    auto dtype_ref = TensorIsMap ? buf[DTypeKey] : buf[0];
    auto dtype = dtype_ref.asInt32();
    if (dtype != tensor_dtype_index<T>) {
        throw DeserializationError(std::string("asEigenMatrix asked to deserialize a matrix of type ") + 
        std::string(tensor_dtype_name<T>) + " but found a matrix of type " + std::string(type_name_from_code(dtype)));
    }

    // get the shape
    auto shape_ref = TensorIsMap ? buf[ShapeKey] : buf[1];
    TensorShape vshape = tensor_shape(shape_ref);

    // perform dimension check
    if (vshape.size() != 2) {
        throw DeserializationError("asEigenMatrix asked to deserialize a matrix of rank 2 but found a matrix of rank " +  std::to_string(vshape.size()));
    }

    // another dimension check: check the actual dimensions
    if (NRows != Eigen::Dynamic) {
        if (vshape[0] != NRows) {
            throw DeserializationError(
                "asEigenMatrix expected " + std::to_string(NRows) +
                " rows, but found " + std::to_string(vshape[0]) + ".");
        }
    }
    if (NCols != Eigen::Dynamic) {
        if (vshape[1] != NCols) {
            throw DeserializationError(
                "asEigenMatrix expected " + std::to_string(NCols) +
                " cols, but found " + std::to_string(vshape[1]) + ".");
        }
    }

    // read actual data
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

    // If the blob is owning (e.g., JSON), copy. If it's a non-owning span, we can map unaligned.
    if constexpr (!std::is_same_v<decltype(blob), std::span<const std::byte>>) {
        MatrixType copy(NRows, NCols);
        std::memcpy(copy.data(), bytes.data(), std::min(bytes.size(), copy.size() * sizeof(T)));
        return copy;
    } else {
        // Eigen::Map with Unaligned still expects at least alignof(T).
        std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(bytes.data());
        bool scalar_aligned = (addr % alignof(T)) == 0;
        if (!scalar_aligned) {
            MatrixType copy(NRows, NCols);
            std::memcpy(copy.data(), bytes.data(), std::min(bytes.size(), copy.size() * sizeof(T)));
            return copy;
        }
        return Eigen::Map<const MatrixType, Eigen::Unaligned>(
            reinterpret_cast<const T*>(bytes.data()),
            NRows,
            NCols);
    }
}

} // eigen
} // zerialize
