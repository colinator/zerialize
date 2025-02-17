#pragma once

#include <iostream>
#include <zerialize/zerialize.hpp>
// #include <zerialize/zerialize_flex.hpp>
// #include <zerialize/zerialize_json.hpp>

using std::string, std::function;
using std::cout, std::endl;

namespace zerialize {

template <typename SerializerType>
bool testit(const string& name,
    const function<typename SerializerType::BufferType()>& serializer,
    const function<bool(const typename SerializerType::BufferType&)>& test)
{
    string str = string("TEST <") + SerializerName<SerializerType>::value + "> --- " + name + " ---";
    cout << "START " << str << std::endl;
    auto buffer = serializer();
    cout << "serialized buffer: \"" << buffer.to_string() << "\" size: " << buffer.buf().size() << endl;
    bool res = test(buffer);
    cout << (res ? "   OK " : " FAIL ") << str << endl << endl;
    if (!res) {
        throw std::runtime_error(string("test failed!!!") + str);
    }
    return res;
}

} // namespace zerialize
