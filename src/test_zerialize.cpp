#include <iostream>
#include <zerialize/zerialize.hpp>
#include <zerialize/zerialize_flex.hpp>
#include <zerialize/zerialize_json.hpp>
#include <zerialize/zerialize_msgpack.hpp>
#include <zerialize/zerialize_testing_utils.hpp>
#include <zerialize/zerialize_xtensor.hpp>
#include <zerialize/zerialize_eigen.hpp>

using std::string, std::function;
using std::cout, std::endl;

using namespace zerialize;


template <typename SerializerType>
void test_much_serialization() {

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
            return zerialize::serialize<SerializerType>([&iv](SerializingConcept auto& s){
                s.serializeVector([&iv](SerializingConcept auto& ser) {
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


    // Boolean values
    test_serialization<SerializerType>("Boolean values: true and false",
        [](){ 
            return zerialize::serialize<SerializerType>(
                { {"true_val", true}, {"false_val", false} }
            ); 
        },
        [](const auto& v) {
            return v["true_val"].asBool() == true &&
                v["false_val"].asBool() == false;
        });

    // Different integer types
    test_serialization<SerializerType>("Different integer types",
        [](){ 
            int8_t i8 = -42;
            uint8_t ui8 = 200;
            int16_t i16 = -12345;
            uint16_t ui16 = 54321;
            int32_t i32 = -12345789;
            uint32_t ui32 = 54321234;
            int64_t i64 = -9223372036854775807LL;
            uint64_t ui64 = 18446744073709551615ULL;
            return zerialize::serialize<SerializerType>(
                { 
                    {"int8", i8}, 
                    {"uint8", ui8}, 
                    {"int16", i16}, 
                    {"uint16", ui16}, 
                    {"int32", i32}, 
                    {"uint32", ui32}, 
                    {"int64", i64}, 
                    {"uint64", ui64} 
                }
            ); 
        },
        [](const auto& v) {
            return v["int8"].asInt32() == -42 &&
                v["uint8"].asUInt32() == 200 &&
                v["int16"].asInt32() == -12345 &&
                v["uint16"].asUInt32() == 54321 &&
                v["int32"].asInt32() == -12345789 &&
                v["uint32"].asUInt32() == 54321234 &&
                v["int64"].asInt64() == -9223372036854775807LL &&
                v["uint64"].asUInt64() == 18446744073709551615ULL;
        });

    // Float type (already testing double in other tests)
    test_serialization<SerializerType>("Float values",
        [](){ 
            float f1 = 3.14159f;
            float f2 = -2.71828f;
            
            return zerialize::serialize<SerializerType>(
                { {"pi", f1}, {"neg_e", f2} }
            ); 
        },
        [](const auto& v) {
            return std::abs(v["pi"].asFloat() - 3.14159f) < 0.0001f &&
                std::abs(v["neg_e"].asFloat() - (-2.71828f)) < 0.0001f;
        });
        
/*
    // Null values
    test_serialization<SerializerType>("Null values",
        [](){ 
            return zerialize::serialize<SerializerType>(
                { {"null_val", nullptr} }
            ); 
        },
        [](const auto& v) {
            return v["null_val"].isNull();
        });

    // std::optional
    test_serialization<SerializerType>("std::optional values",
        [](){ 
            std::optional<int> opt_with_value = 42;
            std::optional<std::string> opt_without_value;
            
            return zerialize::serialize<SerializerType>(
                { {"with_value", opt_with_value}, {"without_value", opt_without_value} }
            ); 
        },
        [](const auto& v) {
            return v["with_value"].asInt32() == 42 &&
                v["without_value"].isNull();
        });

    // std::variant
    test_serialization<SerializerType>("std::variant values",
        [](){ 
            std::variant<int, double, std::string> var1 = 42;
            std::variant<int, double, std::string> var2 = 3.14159;
            std::variant<int, double, std::string> var3 = std::string("hello");
            
            return zerialize::serialize<SerializerType>(
                { {"int_variant", var1}, {"double_variant", var2}, {"string_variant", var3} }
            ); 
        },
        [](const auto& v) {
            return v["int_variant"].asInt32() == 42 &&
                std::abs(v["double_variant"].asDouble() - 3.14159) < 0.0001 &&
                v["string_variant"].asString() == "hello";
        });

    // std::tuple
    test_serialization<SerializerType>("std::tuple values",
        [](){ 
            auto tuple1 = std::make_tuple(1, 2.5, "three");
            auto tuple2 = std::make_tuple(true, 42);
            
            return zerialize::serialize<SerializerType>(
                { {"tuple1", tuple1}, {"tuple2", tuple2} }
            ); 
        },
        [](const auto& v) {
            return v["tuple1"].arraySize() == 3 &&
                v["tuple1"][0].asInt32() == 1 &&
                v["tuple1"][1].asDouble() == 2.5 &&
                v["tuple1"][2].asString() == "three" &&
                v["tuple2"].arraySize() == 2 &&
                v["tuple2"][0].asBool() == true &&
                v["tuple2"][1].asInt32() == 42;
        });

    // std::pair
    test_serialization<SerializerType>("std::pair values",
        [](){ 
            auto pair1 = std::make_pair("key1", 100);
            auto pair2 = std::make_pair(200, "value2");
            
            return zerialize::serialize<SerializerType>(
                { {"pair1", pair1}, {"pair2", pair2} }
            ); 
        },
        [](const auto& v) {
            return v["pair1"].arraySize() == 2 &&
                v["pair1"][0].asString() == "key1" &&
                v["pair1"][1].asInt32() == 100 &&
                v["pair2"].arraySize() == 2 &&
                v["pair2"][0].asInt32() == 200 &&
                v["pair2"][1].asString() == "value2";
        });

    // std::set and std::unordered_set
    test_serialization<SerializerType>("std::set and std::unordered_set",
        [](){ 
            std::set<int> set1 = {1, 2, 3, 4, 5};
            std::unordered_set<std::string> set2 = {"apple", "banana", "cherry"};
            
            return zerialize::serialize<SerializerType>(
                { {"set", set1}, {"unordered_set", set2} }
            ); 
        },
        [](const auto& v) {
            // Sets may not preserve order, so we check if all elements are present
            auto set1 = v["set"];
            auto set2 = v["unordered_set"];
            
            bool set1_valid = set1.arraySize() == 5 &&
                (set1[0].asInt32() == 1 || set1[1].asInt32() == 1 || set1[2].asInt32() == 1 || 
                 set1[3].asInt32() == 1 || set1[4].asInt32() == 1) &&
                (set1[0].asInt32() == 2 || set1[1].asInt32() == 2 || set1[2].asInt32() == 2 || 
                 set1[3].asInt32() == 2 || set1[4].asInt32() == 2) &&
                (set1[0].asInt32() == 3 || set1[1].asInt32() == 3 || set1[2].asInt32() == 3 || 
                 set1[3].asInt32() == 3 || set1[4].asInt32() == 3) &&
                (set1[0].asInt32() == 4 || set1[1].asInt32() == 4 || set1[2].asInt32() == 4 || 
                 set1[3].asInt32() == 4 || set1[4].asInt32() == 4) &&
                (set1[0].asInt32() == 5 || set1[1].asInt32() == 5 || set1[2].asInt32() == 5 || 
                 set1[3].asInt32() == 5 || set1[4].asInt32() == 5);
                
            bool set2_valid = set2.arraySize() == 3 &&
                (set2[0].asString() == "apple" || set2[1].asString() == "apple" || set2[2].asString() == "apple") &&
                (set2[0].asString() == "banana" || set2[1].asString() == "banana" || set2[2].asString() == "banana") &&
                (set2[0].asString() == "cherry" || set2[1].asString() == "cherry" || set2[2].asString() == "cherry");
                
            return set1_valid && set2_valid;
        });

    // std::unordered_map
    test_serialization<SerializerType>("std::unordered_map",
        [](){ 
            std::unordered_map<std::string, int> map1 = {{"one", 1}, {"two", 2}, {"three", 3}};
            
            return zerialize::serialize<SerializerType>(map1); 
        },
        [](const auto& v) {
            return v["one"].asInt32() == 1 &&
                v["two"].asInt32() == 2 &&
                v["three"].asInt32() == 3;
        });

    // Empty containers
    test_serialization<SerializerType>("Empty containers",
        [](){ 
            std::vector<int> empty_vec;
            std::map<std::string, int> empty_map;
            std::set<double> empty_set;
            
            return zerialize::serialize<SerializerType>(
                { {"empty_vector", empty_vec}, {"empty_map", empty_map}, {"empty_set", empty_set} }
            ); 
        },
        [](const auto& v) {
            return v["empty_vector"].arraySize() == 0 &&
                v["empty_map"].objectSize() == 0 &&
                v["empty_set"].arraySize() == 0;
        });

    // Enum types
    enum class TestEnum { Value1, Value2, Value3 };
    test_serialization<SerializerType>("Enum types",
        [](){ 
            TestEnum e1 = TestEnum::Value1;
            TestEnum e2 = TestEnum::Value2;
            TestEnum e3 = TestEnum::Value3;
            
            return zerialize::serialize<SerializerType>(
                { 
                    {"enum1", static_cast<int>(e1)}, 
                    {"enum2", static_cast<int>(e2)}, 
                    {"enum3", static_cast<int>(e3)} 
                }
            ); 
        },
        [](const auto& v) {
            return v["enum1"].asInt32() == 0 &&
                v["enum2"].asInt32() == 1 &&
                v["enum3"].asInt32() == 2;
        });

    // Nested complex structure
    test_serialization<SerializerType>("Nested complex structure",
        [](){ 
            std::map<std::string, std::any> nested_map = {
                {"int_value", 42},
                {"string_value", std::string("hello")},
                {"vector", std::vector<int>{1, 2, 3}},
                {"map", std::map<std::string, double>{{"a", 1.1}, {"b", 2.2}}},
                {"optional", std::optional<int>(123)},
                {"null_optional", std::optional<int>()}
            };
            
            return zerialize::serialize<SerializerType>(nested_map); 
        },
        [](const auto& v) {
            return v["int_value"].asInt32() == 42 &&
                v["string_value"].asString() == "hello" &&
                v["vector"].arraySize() == 3 &&
                v["vector"][0].asInt32() == 1 &&
                v["vector"][1].asInt32() == 2 &&
                v["vector"][2].asInt32() == 3 &&
                v["map"]["a"].asDouble() == 1.1 &&
                v["map"]["b"].asDouble() == 2.2 &&
                v["optional"].asInt32() == 123 &&
                v["null_optional"].isNull();
        });
*/

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
    test_much_serialization<zerialize::Flex>();
    test_much_serialization<zerialize::Json>();
    test_much_serialization<zerialize::MsgPack>();
    test_conversion<zerialize::Flex, zerialize::Json>();
    test_conversion<zerialize::Flex, zerialize::MsgPack>();
    test_conversion<zerialize::Json, zerialize::Flex>();
    test_conversion<zerialize::Json, zerialize::MsgPack>();
    test_conversion<zerialize::MsgPack, zerialize::Json>();
    test_conversion<zerialize::MsgPack, zerialize::Flex>();
    std::cout << "test zerialize done, ALL SUCCEEDED" << std::endl;
    return 0;
}
