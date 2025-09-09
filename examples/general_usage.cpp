#include <iostream>
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

    // Eigen matrices (and xtensors) are zero-copy deserializeable if
    // the protocol allows (flex, msgpack so far).
    auto eigen_mat = Eigen::Matrix<double, 3, 2>();
    eigen_mat << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0;
    ZBuffer b5 = serialize<Flex>(zmap<"tensor", "description">(eigen_mat, "counts"));
    auto d5 = Flex::Deserializer(b5.buf());
    cout << d5["description"].asString() << endl 
         << zerialize::eigen::asEigenMatrix<double, 3, 2>(d5["tensor"]) << endl;
   

    // Outputs:
    //
    // null
    // 1
    // hello world
    // 3.14159 hello world
    // 2.71828 eulers
    // counts
    // 1 2
    // 3 4
    // 5 6
}

