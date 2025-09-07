#include <iostream>
#include <zerialize/zerialize.hpp>
#include <zerialize/protocols/json.hpp>
#include <zerialize/protocols/flex.hpp>
#include <zerialize/translate.hpp>

namespace z = zerialize;

int main() {

    // Serialize and deserialize a map in Json format. 
    // Can also be z::Flex or z::MsgPack, more to come.
    // to_string() is only for debugging purposes.

    // Serialize into a data buffer.
    z::ZBuffer databuf = z::serialize<z::JSON>(
        z::zmap<"name", "age">("James Bond", 37)
    );
    
    std::cout << databuf.to_string() << std::endl;
    // outputs:
    // <ZBuffer 30 bytes, owned=true>

    // Deserialize from a span or vector of bytes.
    auto d = z::JSON::Deserializer(databuf.buf());
    
    std::cout << d.to_string() << std::endl;
    // outputs:
    // {
    //     "name": "James Bond",
    //     "age": 37
    // }

    // Translate from one format to another.
    z::Flex::Deserializer f = z::translate<z::Flex>(d);

    std::cout << f.to_string() << std::endl;
    // outputs:
    // map {
    //   "age": int|37,
    //   "name": str|"James Bond"
    // }
}