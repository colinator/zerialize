#include <iostream>
#include <zerialize/zerialize.hpp>
#include <zerialize/zerialize_flex.hpp>
#include <zerialize/zerialize_json.hpp>
#include <zerialize/zerialize_testing_utils.hpp>
#include <zerialize/zerialize_xtensor.hpp>
#include <zerialize/zerialize_eigen.hpp>

using std::string, std::function;
using std::cout, std::endl;

using namespace zerialize;


template <typename SerializerType>
void testem() {

    using ST = SerializerType;

    std::cout << "START testing zerialize: <" << SerializerType::Name << ">" << endl << endl;

    // Passing nothing: {}   
    test_serialization<ST>("nothing",
        [](){ return serialize<ST>(); },
        [](const auto&) {
            return true;
        });

    // Passing a single value: 3
    test_serialization<ST>("3",
        [](){ return serialize<ST>(3); },
        [](const auto& v) {
            return v.asInt32() == 3;
        });

    // Passing a string: "asdf" via char*
    test_serialization<ST>("\"asdf\" (via rvalue char*)",
        [](){ return serialize<ST>("asdf"); },
        [](const auto& v) {
            return v.asString() == "asdf";
        });

    // Passing a string: "asdf" via temp string
    test_serialization<ST>("\"asdf\" (via rvalue temp string)",
        [](){ return serialize<ST>(std::string{"asdf"}); },
        [](const auto& v) {
            return v.asString() == "asdf";
        });

    // Passing a string: "asdf" via lvalue string
    test_serialization<ST>("\"asdf\" (via lvalue string)",
        [](){ std::string s = "asdf"; return serialize<ST>(s); },
        [](const auto& v) {
            return v.asString() == "asdf";
        });
    
    // Passing a string: "asdf" via const lvalue string
    test_serialization<ST>("\"asdf\" (via const lvalue string)",
        [](){ const std::string s = "asdf"; return serialize<ST>(s); },
        [](const auto& v) {
            return v.asString() == "asdf";
        });

    // Passing multiple rvalues via parameter pack: { 3, 5.2, "asdf" }
    test_serialization<SerializerType>("{ 3, 5.2, \"asdf\" } (via parameter pack rvalues)",
        [](){ return zerialize::serialize<SerializerType>(3, 5.2, "asdf"); },
        [](const auto& v) {
            return v[0].asInt32() == 3 &&
                v[1].asDouble() == 5.2 &&
                v[2].asString() == "asdf";
        });

    // Passing an initializer list: { 3, 5.2, "asdf" }
    test_serialization<SerializerType>("3, 5.2, \"asdf\" (via initializer list)",
        [](){ return zerialize::serialize<SerializerType>({ 3, 5.2, "asdf" }); },
        [](const auto& v) {
            return v[0].asInt32() == 3 &&
                v[1].asDouble() == 5.2 &&
                v[2].asString() == "asdf";
        });

    // Passing multiple values including a vector: 3, 5.2, "asdf", [7, 8.2]
    test_serialization<SerializerType>("3, 5.2, \"asdf\", [7, 8.2]",
        [](){ 
            return zerialize::serialize<SerializerType>(
                3, 5.2, "asdf", std::vector<std::any>{7, 8.2}
            ); 
        },
        [](const auto& v) {
            return v[0].asInt32() == 3 &&
                v[1].asDouble() == 5.2 &&
                v[2].asString() == "asdf" &&
                v[3][0].asInt32() == 7 &&
                v[3][1].asDouble() == 8.2;
        });

    // Using an initializer list for a map: {"a": 3, "b": 5.2, "c": "asdf"}
    test_serialization<SerializerType>("{\"a\": 5, \"b\": 5.3, \"c\": \"asdf\"} (via initializer list)",
        [](){ 
            return zerialize::serialize<SerializerType>(
                { {"a", 3}, {"b", 5.2}, {"c", "asdf"} }
            ); 
        },
        [](const auto& v) {
            return v["a"].asInt32() == 3 &&
                v["b"].asDouble() == 5.2 &&
                v["c"].asString() == "asdf";
        });

    // Using a generic rvalue list for a map: {"a": 3, "b": 5.2, "c": "asdf"}
    test_serialization<SerializerType>("Generic rvalue list: {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\"}",
        [](){ 
            return zerialize::serialize<SerializerType, int, double, std::string>(
                {"a", 3}, {"b", 5.2}, {"c", "asdf"}
            ); 
        },
        [](const auto& v) {
            return v["a"].asInt32() == 3 &&
                v["b"].asDouble() == 5.2 &&
                v["c"].asString() == "asdf";
        });

    // Using a generic rvalue list for a map with a nested vector:
    //     {"a": 3, "b": 5.2, "c": "asdf", "d": [7, 8.2]}
    test_serialization<SerializerType>("Generic rvalue list: {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\", \"d\": [7, 8.2]}",
        [](){ 
            return zerialize::serialize<SerializerType, int, double, std::string, std::vector<std::any>>(
                {"a", 3}, {"b", 5.2}, {"c", "asdf"}, {"d", std::vector<std::any>{7, 8.2}}
            ); 
        },
        [](const auto& v) {
            return v["a"].asInt32() == 3 &&
                v["b"].asDouble() == 5.2 &&
                v["c"].asString() == "asdf" &&
                v["d"][0].asInt32() == 7 &&
                v["d"][1].asDouble() == 8.2;
        });

    // Using an initializer list for a map with a nested vector:
    //     {"a": 3, "b": 5.2, "c": "asdf", "d": [7, 8.2]}
    test_serialization<SerializerType>("Initializer list: {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\", \"d\": [7, 8.2]}",
        [](){ 
            return zerialize::serialize<SerializerType>(
                { {"a", 3}, {"b", 5.2}, {"c", "asdf"}, {"d", std::vector<std::any>{7, 8.2}} }
            ); 
        },
        [](const auto& v) {
            return v["a"].asInt32() == 3 &&
                v["b"].asDouble() == 5.2 &&
                v["c"].asString() == "asdf" &&
                v["d"][0].asInt32() == 7 &&
                v["d"][1].asDouble() == 8.2;
        });

    // Using an initializer list for a map with a nested vector:
    //     {"a": 3, "b": 5.2, "c": "asdf", "d": [7, 8.2]}
    test_serialization<SerializerType>("Initializer list: {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\", \"d\": [7, 8.2]}",
        [](){ 
            return zerialize::serialize<SerializerType>(
                { {"a", 3}, {"b", 5.2}, {"c", "asdf"}, {"d", std::vector<std::any>{7, std::map<std::string, std::any>{ {"w", 3.2}, {"y", "yomamma"} }}} }
            ); 
        },
        [](const auto& v) {
            return v["a"].asInt32() == 3 &&
                v["b"].asDouble() == 5.2 &&
                v["c"].asString() == "asdf" &&
                v["d"][0].asInt32() == 7 &&
                v["d"][1]["w"].asDouble() == 3.2 &&
                v["d"][1]["y"].asString() == "yomamma";
        });

    // Using an initializer list for a map with a nested blob:
    //     {"a": Blob{1,2,3,4}, "b": 457835 }
    auto k = std::array<uint8_t, 4>({'a','b','c','z'});
    test_serialization<SerializerType>("Initializer list: {\"a\": std::span<const uint8_t>({'a','b','c','z'}), \"b\": 457835 }",
        [k](){ 
            return zerialize::serialize<SerializerType>(
                { {"a", std::span<const uint8_t>(k)}, { "b", 457835 } }
            ); 
        },
        [k](const auto& v) {
            auto a = v["a"].asBlob();
            return 
                std::equal(a.begin(), a.end(), k.begin()) &&
                v["b"].asInt32() == 457835;
        });

    // Using an initializer list for a map with a nested xtensor:
    //     {"a": xt::xtensor<double, 2>{{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}}, "b": 457835 }
    auto t = xt::xtensor<double, 2>{{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}};
    test_serialization<SerializerType>("Initializer list: {\"a\": xt::xtensor<double, 2>{{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}}, \"b\": 457835 }",
        [&t](){ 
            return zerialize::serialize<SerializerType>(
                { 
                  { "a", zerialize::xtensor::serializer<SerializerType>(t) }, 
                  { "b", 457835 } 
                }
            ); 
        },
        [&t](const auto& v) {
            auto a = zerialize::xtensor::asXTensor<double, 2>(v["a"]);
            //std::cout << " ---- a " << std::endl << a << std::endl;
            //std::cout << " ---- t " << std::endl << t << std::endl;
            return
                a == t &&
                v["b"].asInt32() == 457835;
        });

    // Using an initializer list for a map with a nested xarray:
    //     {"a": xt::xarray<double>{{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}}, "b": 457835 }
    auto t2 = xt::xarray<double>{{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}};
    test_serialization<SerializerType>("Initializer list: {\"a\": xt::xarray<double>{{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}}, \"b\": 457835 }",
        [&t2](){
            return zerialize::serialize<SerializerType>(
                { 
                  { "a", zerialize::xtensor::serializer<SerializerType>(t2) }, 
                  { "b", 457835 } 
                }
            );
        },
        [&t2](const auto& v) {
            auto a = zerialize::xtensor::asXTensor<double>(v["a"]);
            //std::cout << " ---- a " << std::endl << a << std::endl;
            //std::cout << " ---- t2 " << std::endl << t2 << std::endl;
            return
                a == t2 &&
                v["b"].asInt32() == 457835;
        });

    // Using an initializer list for a map with a nested eigen matrix:
    //     {"a": Eigen::Matrix3f, "b": 457835 }
    Eigen::Matrix3f m;
    m << 1, 2, 3,
         4, 5, 6,
         7, 8, 9;
    test_serialization<SerializerType>("Initializer list: {\"a\": Eigen::Matrix3f, \"b\": 457835 }",
        [&m](){
            return zerialize::serialize<SerializerType>(
                { 
                  { "a", zerialize::eigen::serializer<SerializerType>(m) }, 
                  { "b", 457835 } 
                }
            );
        },
        [&m](const auto& v) {
            auto a = zerialize::eigen::asEigenMatrix<float, 3, 3>(v["a"]);
            // std::cout << " ---- a " << std::endl << a << std::endl;
            // std::cout << " ---- m " << std::endl << m << std::endl;
            return
                a == m &&
                v["b"].asInt32() == 457835;
        });


    // Vector of int
    test_serialization<SerializerType>("initializer list of int: {1, 2, 3, 4, 5}",
        [](){ 
            return zerialize::serialize<SerializerType>(
                {1, 2, 3, 4, 5}
            ); 
        },
        [](const auto& v) {
            return v.arraySize() == 5 &&
                v[0].asInt32() == 1 &&
                v[1].asInt32() == 2 &&
                v[2].asInt32() == 3 &&
                v[3].asInt32() == 4 &&
                v[4].asInt32() == 5;
        });

    // Vector of any
    auto va = std::vector<any>{1, 2, 3, 4, 5};
    test_serialization<SerializerType>("Vector of any: [1, 2, 3, 4, 5]",
        [&va](){ 
            return zerialize::serialize<SerializerType>(va); 
        },
        [](const auto& v) {
            return v.arraySize() == 5 &&
                v[0].asInt32() == 1 &&
                v[1].asInt32() == 2 &&
                v[2].asInt32() == 3 &&
                v[3].asInt32() == 4 &&
                v[4].asInt32() == 5;
        });

    // Vector of int
    auto vi = std::vector<int>{1, 2, 3, 4, 5};
    test_serialization<SerializerType>("Vector of int: [1, 2, 3, 4, 5]",
        [&vi](){ 
            return zerialize::serialize<SerializerType>(vi); 
        },
        [](const auto& v) {
            return v.arraySize() == 5 &&
                v[0].asInt32() == 1 &&
                v[1].asInt32() == 2 &&
                v[2].asInt32() == 3 &&
                v[3].asInt32() == 4 &&
                v[4].asInt32() == 5;
        });

    // array of int
    auto ai = std::array<int, 5>{1, 2, 3, 4, 5};
    test_serialization<SerializerType>("array of int: [1, 2, 3, 4, 5]",
        [&ai](){ 
            return zerialize::serialize<SerializerType>(ai); 
        },
        [](const auto& v) {
            return v.arraySize() == 5 &&
                v[0].asInt32() == 1 &&
                v[1].asInt32() == 2 &&
                v[2].asInt32() == 3 &&
                v[3].asInt32() == 4 &&
                v[4].asInt32() == 5;
        });

    // Vector of int
    auto iv = std::vector<int>{1, 2, 3, 4, 5};
    test_serialization<SerializerType>("Vector of int: [1, 2, 3, 4, 5]",
        [&iv](){ 
            //return zerialize::serialize<SerializerType>(iv); 
            return zerialize::serialize<SerializerType>([&iv](SerializerType::Serializer& s){
                s.serializeVector([&iv](SerializerType::Serializer& ser) {
                    for (auto z: iv) {
                        ser.serialize(z);
                    }
                });
            });
        },
        [](const auto& v) {
            return v.arraySize() == 5 &&
                v[0].asInt32() == 1 &&
                v[1].asInt32() == 2 &&
                v[2].asInt32() == 3 &&
                v[3].asInt32() == 4 &&
                v[4].asInt32() == 5;
        });


    // Vector of int
    test_serialization<SerializerType>("Temporary vector of int: [1, 2, 3, 4, 5]",
        [](){ 
            return zerialize::serialize<SerializerType>(
                std::vector<int>{1, 2, 3, 4, 5}
            ); 
        },
        [](const auto& v) {
            return v.arraySize() == 5 &&
                v[0].asInt32() == 1 &&
                v[1].asInt32() == 2 &&
                v[2].asInt32() == 3 &&
                v[3].asInt32() == 4 &&
                v[4].asInt32() == 5;
        });


    // Vector of double
    test_serialization<SerializerType>("Vector of double: [1.1, 2.2, 3.3, 4.4, 5.5]",
        [](){ 
            return zerialize::serialize<SerializerType>(
                std::vector<double>{1.1, 2.2, 3.3, 4.4, 5.5}
            ); 
        },
        [](const auto& v) {
            return v.arraySize() == 5 &&
                v[0].asDouble() == 1.1 &&
                v[1].asDouble() == 2.2 &&
                v[2].asDouble() == 3.3 &&
                v[3].asDouble() == 4.4 &&
                v[4].asDouble() == 5.5;
        });

    // Vector of string
    test_serialization<SerializerType>("Vector of string: [\"one\", \"two\", \"three\"]",
        [](){ 
            return zerialize::serialize<SerializerType>(
                std::vector<std::string>{"one", "two", "three"}
            ); 
        },
        [](const auto& v) {
            return v.arraySize() == 3 &&
                v[0].asString() == "one" &&
                v[1].asString() == "two" &&
                v[2].asString() == "three";
        });

    // Map with string keys and int values
    test_serialization<SerializerType>("Map with string keys and int values: {\"a\": 1, \"b\": 2, \"c\": 3}",
        [](){ 
            return zerialize::serialize<SerializerType>(
                std::map<std::string, int>{{"a", 1}, {"b", 2}, {"c", 3}}
            ); 
        },
        [](const auto& v) {
            return v["a"].asInt32() == 1 &&
                v["b"].asInt32() == 2 &&
                v["c"].asInt32() == 3;
        });


    // Map with string keys and double values
    test_serialization<SerializerType>("Map with string keys and double values: {\"x\": 1.1, \"y\": 2.2, \"z\": 3.3}",
        [](){ 
            return zerialize::serialize<SerializerType>(
                std::map<std::string, double>{{"x", 1.1}, {"y", 2.2}, {"z", 3.3}}
            ); 
        },
        [](const auto& v) {
            return v["x"].asDouble() == 1.1 &&
                v["y"].asDouble() == 2.2 &&
                v["z"].asDouble() == 3.3;
        });

    // Map with string keys and string values
    test_serialization<SerializerType>("Map with string keys and string values: {\"first\": \"one\", \"second\": \"two\", \"third\": \"three\"}",
        [](){ 
            return zerialize::serialize<SerializerType>(
                std::map<std::string, std::string>{{"first", "one"}, {"second", "two"}, {"third", "three"}}
            ); 
        },
        [](const auto& v) {
            return v["first"].asString() == "one" &&
                v["second"].asString() == "two" &&
                v["third"].asString() == "three";
        });

    // Vector of vectors (nested vectors of int)
    test_serialization<SerializerType>("Vector of vectors: [[1, 2], [3, 4], [5, 6]]",
        [](){ 
            return zerialize::serialize<SerializerType>(
                std::vector<std::vector<int>>{{1, 2}, {3, 4}, {5, 6}}
            ); 
        },
        [](const auto& v) {
            return v.arraySize() == 3 &&
                v[0].arraySize() == 2 &&
                v[0][0].asInt32() == 1 &&
                v[0][1].asInt32() == 2 &&
                v[1].arraySize() == 2 &&
                v[1][0].asInt32() == 3 &&
                v[1][1].asInt32() == 4 &&
                v[2].arraySize() == 2 &&
                v[2][0].asInt32() == 5 &&
                v[2][1].asInt32() == 6;
        });

    // Map with string keys and vector values
    test_serialization<SerializerType>("Map with string keys and vector values: {\"nums\": [1, 2, 3], \"decimals\": [4.4, 5.5, 6.6]}",
        [](){ 
            return zerialize::serialize<SerializerType>(
                std::map<std::string, std::vector<double>>{
                    {"nums", {1.0, 2.0, 3.0}}, 
                    {"decimals", {4.4, 5.5, 6.6}}
                }
            ); 
        },
        [](const auto& v) {
            return v["nums"].arraySize() == 3 &&
                v["nums"][0].asDouble() == 1.0 &&
                v["nums"][1].asDouble() == 2.0 &&
                v["nums"][2].asDouble() == 3.0 &&
                v["decimals"].arraySize() == 3 &&
                v["decimals"][0].asDouble() == 4.4 &&
                v["decimals"][1].asDouble() == 5.5 &&
                v["decimals"][2].asDouble() == 6.6;
        });


    // Vector of maps
    test_serialization<SerializerType>("Vector of maps: [{\"a\": 1, \"b\": 2}, {\"c\": 3, \"d\": 4}]",
        [](){ 
            return zerialize::serialize<SerializerType>(
                std::vector<std::map<std::string, int>>{
                    {{"a", 1}, {"b", 2}},
                    {{"c", 3}, {"d", 4}}
                }
            ); 
        },
        [](const auto& v) {
            return v.arraySize() == 2 &&
                v[0]["a"].asInt32() == 1 &&
                v[0]["b"].asInt32() == 2 &&
                v[1]["c"].asInt32() == 3 &&
                v[1]["d"].asInt32() == 4;
        });

    {
        std::cout << std::endl;

        auto v = zerialize::serialize<SerializerType>({{"a", 3}, {"b", 5.2}, {"c", "asdf"}, {"d", std::vector<std::any>{7, 8.2}}});

        std::cout << "--- deserialize ---" << std::endl;
        auto vbuf = v.buf();
        auto vdes = typename SerializerType::BufferType(vbuf);
        std::cout << vdes["a"].asInt32() << " " << vdes["b"].asDouble() << " " << vdes["c"].asString() << " " << vdes["d"][0].asInt32() << " " << vdes["d"][1].asDouble() << std::endl;
        std::cout << std::endl;

        // std::cout << "--- Testing different construction methods ---" << std::endl;
        
        // 1. Using span (zero-copy view)
        std::cout << "1. Construct from span (zero-copy view):" << std::endl;
        auto span_view = std::span<const uint8_t>(v.buf().data(), v.buf().size());
        auto v_span = typename SerializerType::BufferType(span_view);
        std::cout << v_span["a"].asInt32() << " " << v_span["b"].asDouble() << " " << v_span["c"].asString() << " " << v_span["d"][0].asInt32() << " " << v_span["d"][1].asDouble() << std::endl;
        std::cout << std::endl;

        // 2. Move construction (zero-copy transfer)
        std::cout << "2. Move construction (zero-copy transfer):" << std::endl;
        auto buf_to_move = std::vector<uint8_t>(v.buf());  // Make a copy to move from
        auto v_move = typename SerializerType::BufferType(std::move(buf_to_move));
        std::cout << v_move["a"].asInt32() << " " << v_move["b"].asDouble() << " " << v_move["c"].asString() << " " << v_move["d"][0].asInt32() << " " << v_move["d"][1].asDouble() << std::endl;
        std::cout << "Original vector size after move: " << buf_to_move.size() << std::endl;  // Should be 0
        std::cout << std::endl;

        // 3. Copy construction
        std::cout << "3. Copy construction:" << std::endl;
        auto v_copy = typename SerializerType::BufferType(v.buf());
        std::cout << v_copy["a"].asInt32() << " " << v_copy["b"].asDouble() << " " << v_copy["c"].asString() << " " << v_copy["d"][0].asInt32() << " " << v_copy["d"][1].asDouble() << std::endl;
        std::cout << std::endl;
        auto mapkeys = v_copy.mapKeys();
        std::cout << "MAP KEYS: ";
        for (auto k : mapkeys) {
            std::cout << k << " ";
        }
        std::cout << std::endl;
    }

     std::cout << "..END testing zerialize: <" << SerializerType::Name << ">" << endl << endl;
}


template<typename SrcSerializerType, typename DestSerializerType>
void test_conversion() {
    std::cout << "Testing conversion from " << SrcSerializerType::Name << " to " << DestSerializerType::Name << std::endl;
    auto m = xt::xarray<double>{{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}};
    auto v1 = zerialize::serialize<SrcSerializerType>({{"a", 3}, {"b", 5.2}, {"k", 1028}, {"c", "asdf"}, {"d", std::vector<std::any>{7, 8.2, std::map<std::string, std::any>{{"pi", 3.14159}, {"e", 2.613}, {"m", zerialize::xtensor::serializer<SrcSerializerType>(m)}}}}});
    auto v2 = zerialize::convert<SrcSerializerType, DestSerializerType>(v1);
    auto v3 = zerialize::convert<DestSerializerType, SrcSerializerType>(v2);
    auto v4 = zerialize::convert<SrcSerializerType, DestSerializerType>(v3);
    auto v5 = zerialize::convert<DestSerializerType, SrcSerializerType>(v2);
    auto v6 = zerialize::convert<SrcSerializerType, DestSerializerType>(v3);
    std::cout << "1: " << v1.to_string() << std::endl;
    std::cout << "2: " << v2.to_string() << std::endl;
    std::cout << "3: " << v3.to_string() << std::endl;
    std::cout << "4: " << v4.to_string() << std::endl;
    std::cout << "5: " << v5.to_string() << std::endl;
    std::cout << "6: " << v6.to_string() << std::endl;
    std::cout << std::endl;
}

int main() {
    testem<zerialize::Flex>();
    testem<zerialize::Json>();
    test_conversion<zerialize::Flex, zerialize::Json>();
    test_conversion<zerialize::Json, zerialize::Flex>();
    std::cout << "test zerialize done, ALL SUCCEEDED" << std::endl;
    return 0;
}
