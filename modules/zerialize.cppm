module;

#include <zerialize/zerialize.hpp>

export module zerialize;

export import :cbor;
export import :flex;
export import :json;
export import :msgpack;
export import :eigen;
export import :utils;
export import :xtensor;

export namespace zerialize {
    using zerialize::BlobView;
    using zerialize::StringViewRange;
    using zerialize::ValueView;
    using zerialize::Reader;
    using zerialize::Writer;
    using zerialize::Builder;
    using zerialize::RootSerializer;
    using zerialize::SerializerFor;
    using zerialize::Protocol;
    using zerialize::SerializationError;
    using zerialize::DeserializationError;
    using zerialize::serialize;
    using zerialize::write_value;
    using zerialize::translate;
    using zerialize::translate_bytes;
    using zerialize::ZBuffer;
    using zerialize::BuilderWrapper;
    using zerialize::zvec;
    using zerialize::zmap;
}