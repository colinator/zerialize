module;

#ifdef ZERIALIZE_ENABLE_JSON
#include <zerialize/protocols/json.hpp>
#endif

export module zerialize:json;

export namespace zerialize {
    #ifdef ZERIALIZE_ENABLE_JSON
    using zerialize::JSON;
    #endif
    namespace json {
        #ifdef ZERIALIZE_ENABLE_JSON
        using zerialize::json::blobTag;
        using zerialize::json::blobEncoding;
        using zerialize::json::KeysView;
        using zerialize::json::JsonDeserializer;
        using zerialize::json::RootSerializer;
        using zerialize::json::Serializer;
        using zerialize::json::operator==;
        using zerialize::json::operator!=;
        #endif
    }
}
