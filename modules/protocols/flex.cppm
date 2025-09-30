module;

#ifdef ZERIALIZE_ENABLE_FLEXBUFFERS
#include <zerialize/protocols/flex.hpp>
#endif

export module zerialize:flex;

export namespace zerialize {
    #ifdef ZERIALIZE_ENABLE_FLEXBUFFERS
    using zerialize::Flex;
    #endif
    namespace flex {
        #ifdef ZERIALIZE_ENABLE_FLEXBUFFERS
        using zerialize::flex::RootSerializer;
        using zerialize::flex::Serializer;
        using zerialize::flex::FlexViewBase;
        using zerialize::flex::FlexValue;
        using zerialize::flex::FlexDeserializer;
        using zerialize::flex::operator==;
        #endif
    }
}
