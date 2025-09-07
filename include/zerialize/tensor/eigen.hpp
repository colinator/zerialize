#pragma once

#include <array>
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
auto asEigenMatrix(const Reader auto& buf) {
    using MatrixType = Eigen::Matrix<T, NRows, NCols, Options>; // | Eigen::DontAlign>;

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
    T* data_typed = data_from_blobview<T>(blob);

    // Create a Map object. If all is well, then this
    // should perform no copies - it should basically
    // be a 'view' over the blob's data.
    // NOTE! Reading in this way from arbitrary data cannot
    // guarantee any sort of alignment. This may or may not
    // slow down SIMD instructions (which Eigen uses). 
    auto in_place_matrix = Eigen::Map<const MatrixType>( //, Eigen::Unaligned>(
        data_typed,
        NRows,
        NCols);
    
    // Depending on the blob type (is it owned or not), we might
    // have to instantiate (copy) the matrix. This will happen for 
    // serializable types that do not support 0-copy blobs (such as Json).
    if constexpr (std::is_same_v<decltype(blob), std::vector<std::byte>>) {
        // We've gotta copy it...
        return MatrixType(in_place_matrix);
    } else { //if constexpr (std::is_same_v<decltype(blob), span<const uint8_t>>) {
        return in_place_matrix;
    }
}

} // eigen
} // zerialize
