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

    // invoke the serializer function: that will give us a serialized buffer
    auto buffer = serializer();
   cout << "serialized buffer: \"" << buffer.to_string() << "\" size: " << buffer.buf().size() << endl;

    // invoke the test predicate to determine whether that matched what we want
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
    if (!test(newBuffer)) {
        throw std::runtime_error(string("test failed after buffer copy!!! ") + str);
    }    
    
    // Get and compare addresses
    const uint8_t* firstBufferAddr = buffer.buf().data();
    const uint8_t* secondBufferAddr = newBuffer.buf().data();
    
    //cout << "First buffer address: " << static_cast<const void*>(firstBufferAddr) << endl;
    //cout << "Second buffer address: " << static_cast<const void*>(secondBufferAddr) << endl;
    
    if (firstBufferAddr == secondBufferAddr && firstBufferAddr != 0) {
        throw std::runtime_error(string("Buffer addresses match! This indicates a potential memory issue. ") + str);
    }

    // Deserialize from a span
    auto span_view = std::span<const uint8_t>(newBuffer.buf().data(), newBuffer.buf().size());
    auto spanBuffer = typename SerializerType::BufferType(span_view);
    if (!test(spanBuffer)) {
        throw std::runtime_error(string("test failed after span init!!! ") + str);
    } 

    // Deserialize from a moved vector
    auto buf_to_move = std::vector<uint8_t>(newBuffer.buf());  // Make a copy to move from
    auto moveBuffer = typename SerializerType::BufferType(std::move(buf_to_move));
    if (!test(moveBuffer)) {
        throw std::runtime_error(string("test failed after move init!!! ") + str);
    } 

    // Deserialize copy constructor
    auto copiedBuffer = typename SerializerType::BufferType(newBuffer.buf());
    if (!test(copiedBuffer)) {
        throw std::runtime_error(string("test failed after copy init!!! ") + str);
    } 
    
    return res;
}

} // namespace zerialize
