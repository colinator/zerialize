module;

#ifdef ZERIALIZE_ENABLE_EIGEN
#include <zerialize/tensor/eigen.hpp>
#endif

export module zerialize:eigen;

export namespace Eigen {
    #ifdef ZERIALIZE_ENABLE_EIGEN
    using Eigen::serialize;
    #endif
}

export namespace zerialize::eigen {
    #ifdef ZERIALIZE_ENABLE_EIGEN
    using zerialize::eigen::asEigenMatrix;
    #endif
}
