#pragma once

#include <iostream>
#include <zerialize/zerialize.hpp>

using std::string, std::function;
using std::cout, std::endl;

namespace zerialize {

template <typename SerializerType>
bool test_serialization(const string& name,
    const function<typename SerializerType::BufferType()>& serializer,
    const function<bool(const typename SerializerType::BufferType&)>& test)
{
    string str = string("TEST <") + SerializerType::Name + "> --- " + name + " ---";
    cout << "START " << str << std::endl;
    auto buffer = serializer();
    cout << "serialized buffer: \"" << buffer.to_string() << "\" size: " << buffer.buf().size() << endl;
    bool res = test(buffer);
    cout << (res ? "   OK " : " FAIL ") << str << endl << endl;
    if (!res) {
        throw std::runtime_error(string("test failed!!!") + str);
    }

    // Copy the entire underlying vector store
    const vector<uint8_t> bufferCopy = buffer.buf();
    
    // Create a new buffer from the copy
    typename SerializerType::BufferType newBuffer = typename SerializerType::BufferType(bufferCopy);
    
    // Apply test to new buffer
    bool newRes = test(newBuffer);
    if (!newRes) {
        throw std::runtime_error(string("test failed after buffer copy!!!") + str);
    }    
    
    // Get and compare addresses
    const uint8_t* firstBufferAddr = buffer.buf().data();
    const uint8_t* secondBufferAddr = newBuffer.buf().data();
    
    //cout << "First buffer address: " << static_cast<const void*>(firstBufferAddr) << endl;
    //cout << "Second buffer address: " << static_cast<const void*>(secondBufferAddr) << endl;
    
    if (firstBufferAddr == secondBufferAddr && firstBufferAddr != 0) {
        throw std::runtime_error(string("Buffer addresses match! This indicates a potential memory issue. ") + str);
    }
    
    return res;
}

} // namespace zerialize
