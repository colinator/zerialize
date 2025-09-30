module;

#ifdef ZERIALIZE_ENABLE_MSGPACK
#include <zerialize/protocols/msgpack.hpp>
#endif

export module zerialize:msgpack;

export namespace zerialize {
    #ifdef ZERIALIZE_ENABLE_MSGPACK
    using zerialize::MsgPackDeserializer;
    using zerialize::MsgPackRootSerializer;
    using zerialize::MsgPackSerializer;
    using zerialize::MsgPack;
    #endif
}
