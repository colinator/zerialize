#include <iostream>
#include <vector>
#include <array>
#include <map>
#include <zerialize/zerialize.hpp>
#include <zerialize/protocols/json.hpp>
#include <zerialize/protocols/flex.hpp>
#include <zerialize/protocols/msgpack.hpp>
#include <zerialize/tensor/eigen.hpp>
#include <zerialize/translate.hpp>

namespace z = zerialize;

using z::ZBuffer, z::serialize, z::zvec, z::zmap;
using z::JSON, z::Flex, z::MsgPack;
using std::cout, std::endl;

int main() {

    // Empty
    ZBuffer b0 = serialize<JSON>();
    auto d0 = JSON::Deserializer(b0.buf());
    cout << d0.to_string() << endl;

    // Empty map
    ZBuffer b01 = serialize<JSON>(zmap());
    auto d01 = JSON::Deserializer(b01.buf());
    cout << d01.to_string() << endl;

    // Empty array
    ZBuffer b02 = serialize<JSON>(zvec());
    auto d02 = JSON::Deserializer(b02.buf());
    cout << d02.to_string() << endl;

    // Empty string
    ZBuffer b03 = serialize<JSON>("");
    auto d03 = JSON::Deserializer(b03.buf());
    cout << d03.to_string() << endl;

    // Single int value
    ZBuffer b1 = serialize<JSON>(1);
    auto d1 = JSON::Deserializer(b1.buf());
    cout << d1.asInt32() << endl;

    // Single string value
    ZBuffer b2 = serialize<Flex>("hello world");
    auto d2 = Flex::Deserializer(b2.buf());
    cout << d2.asString() << endl;

    // Vector of heterogeneous values
    ZBuffer b3 = serialize<MsgPack>(zvec(3.14159, "hello world"));
    auto d3 = MsgPack::Deserializer(b3.buf());
    cout << d3[0].asDouble() << " " << d3[1].asString() << endl;
    
    // Map of string keys to heterogeneous values
    ZBuffer b4 = serialize<JSON>(zmap<"value", "description">(2.71828, "eulers"));
    auto d4 = JSON::Deserializer(b4.buf());
    cout << d4["value"].asDouble() << " " << d4["description"].asString() << endl;

    // Nesting
    ZBuffer b5 = serialize<Flex>(
        zmap<"users", "metadata">(
            zvec(
                zmap<"id", "name">(1, "Alice"),
                zmap<"id", "name">(2, "Bob")
            ),
            zmap<"version", "timestamp">(
                "1.0", 
                1234567890
            )
        )
    );
    auto d5 = Flex::Deserializer(b5.buf());
    cout << d5["users"][0]["name"].asString() << " " << d5["users"][1]["name"].asString() << endl;

    // Eigen matrices (and xtensors) are zero-copy deserializeable if
    // the protocol allows (flex, msgpack so far).
    auto eigen_mat = Eigen::Matrix<double, 3, 2>();
    eigen_mat << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0;
    ZBuffer b6 = serialize<Flex>(zmap<"tensor", "description">(eigen_mat, "counts"));
    auto d6 = Flex::Deserializer(b6.buf());
    cout << d6["description"].asString() << endl 
         << zerialize::eigen::asEigenMatrix<double, 3, 2>(d6["tensor"]) << endl;
   
    // std::vector (can also be nested in zmap/zvec)
    std::vector<int> vec {1,2,3};
    ZBuffer b7 = serialize<MsgPack>(vec);
    auto d7 = MsgPack::Deserializer(b7.buf());
    cout << d7[0].asInt32() << " " << d7[1].asInt32() << " " << d7[2].asInt32() << endl;

    // std::map (can also be nested in zmap/zvec)
    std::map<std::string, int> mappy {{"a", 1}, {"b", 2}};
    ZBuffer b8 = serialize<JSON>(mappy);
    auto d8 = JSON::Deserializer(b8.buf());
    cout << d8["a"].asInt32() << " " << d8["b"].asInt32() << endl;

    // Outputs:JSON
    //
    // null
    // {}
    // []
    // ""
    // 1
    // hello world
    // 3.14159 hello world
    // 2.71828 eulers
    // Alice Bob
    // counts
    // 1 2
    // 3 4
    // 5 6
    // 1 2 3
    // 1 2
}

