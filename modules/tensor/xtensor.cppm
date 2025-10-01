module;

#ifdef ZERIALIZE_ENABLE_XTENSOR
#include <zerialize/tensor/xtensor.hpp>
#endif

export module zerialize:xtensor;

export namespace xt {
    #ifdef ZERIALIZE_ENABLE_XTENSOR
    using xt::serialize;
    #endif
}

export namespace zerialize::xtensor {
    #ifdef ZERIALIZE_ENABLE_XTENSOR
    using zerialize::xtensor::flextensor_adaptor;
    using zerialize::xtensor::asXTensor;
    #endif
}

