#include <iostream>
#include <zerialize/zerialize.hpp>
#include <zerialize/protocols/json.hpp>
#include <zerialize/protocols/flex.hpp>
#include <zerialize/translate.hpp>

namespace z = zerialize;

int main() {

    // ----------------------------------------------
    // Serialize and deserialize a map in Json format. 
    // Can also be z::Flex or z::MsgPack, more to come.
    // to_string() is only for debugging purposes.

    // Serialize into a data buffer.
    z::ZBuffer databuf = z::serialize<z::JSON>(
        z::zmap<"name", "age">("James Bond", 37)
    );
    std::cout << "BYTES: " << databuf.to_string() << std::endl;

    // Deserialize from a span or vector of bytes.
    auto d = z::JSON::Deserializer(databuf.buf());
    std::cout << "JSON: " << d.to_string() << std::endl;

    // Read attributes dynamically and lazily. You provide the type.
    std::cout << "JSON AGENT " 
              << "agent name: " << d["name"].asString()
              <<       " age: " << d["age"].asUInt16() << std::endl;

    // Translate from one format to another.
    z::Flex::Deserializer f = z::translate<z::Flex>(d);
    std::cout << "FLEX: " << f.to_string() << std::endl;

    // For protocols that support it (flex, msgpack), reads
    // are zero-copy to the extent possible.
    std::cout << "FLEX AGENT " 
              << "agent name: " << d["name"].asString()
              <<       " age: " << d["age"].asUInt16() << std::endl;

    // OUTPUTS:
    //
    // BYTES: <ZBuffer 30 bytes, owned=true>
    // JSON: {
    //     "name": "James Bond",
    //     "age": 37
    // }
    // JSON AGENT agent name: James Bond age: 37
    // FLEX: map {
    //   "age": int|37,
    //   "name": str|"James Bond"
    // }
    // FLEX AGENT agent name: James Bond age: 37
}