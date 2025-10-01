module;

#ifdef ZERIALIZE_HAS_CBOR
#include <zerialize/protocols/cbor.hpp>
#endif

export module zerialize:cbor;

export namespace zerialize {
    #ifdef ZERIALIZE_HAS_CBOR
    using zerialize::CBOR;
    #endif
    namespace cborjc {
        #ifdef ZERIALIZE_HAS_CBOR
        using zerialize::cborjc::RootSerializer;
        using zerialize::cborjc::Serializer;
        using zerialize::cborjc::CborDeserializer;
        using zerialize::cborjc::operator==;
        #endif
    }
}
