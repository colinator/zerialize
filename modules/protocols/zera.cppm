module;

#include <zerialize/protocols/zer.hpp>

export module zerialize:zera;

export namespace zerialize {
    // Built-in, dependency-free protocol.
    using zerialize::Zer;
    using Zera = zerialize::Zer;

    namespace zer {
        using zerialize::zer::RootSerializer;
        using zerialize::zer::Serializer;
        using zerialize::zer::ZerViewBase;
        using zerialize::zer::ZerValue;
        using zerialize::zer::ZerDeserializer;
        using zerialize::zer::operator==;
    }
}

