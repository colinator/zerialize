module;

#ifdef ZERIALIZE_ENABLE_XTENSOR
#include <zerialize/tensor/utils.hpp>
#endif

export module zerialize:utils;

export namespace zerialize {
    #ifdef ZERIALIZE_ENABLE_XTENSOR
    using zerialize::complex;
    using zerialize::tensor_dtype_index;
    using zerialize::tensor_dtype_name;
    using zerialize::type_name_from_code;
    using zerialize::ShapeKey;
    using zerialize::DTypeKey;
    using zerialize::DataKey;
    using zerialize::TensorShapeElement;
    using zerialize::TensorShape;
    using zerialize::shape_of_sizet;
    using zerialize::tensor_shape;
    using zerialize::HasDataAndSize;
    using zerialize::span_from_data_of;
    using zerialize::data_from_blobview;
    using zerialize::isTensor;
    #endif
}
