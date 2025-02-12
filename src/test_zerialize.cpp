#include <iostream>
#include <zerialize/zerialize.hpp>
#include <zerialize/zerialize_flex.hpp>
#include <zerialize/zerialize_json.hpp>

using std::string, std::function;
using std::cout, std::endl;

using namespace zerialize;

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

template <typename SerializerType>
void testem() {

    using ST = SerializerType;

    std::cout << "START testing zerialize: <" << SerializerName<ST>::value << ">" << endl << endl;

    // Passing nothing: {}   
    testit<ST>("nothing",
        [](){ return serialize<ST>(); },
        [](const auto&) {
            return true;
        });

    // Passing a single value: 3
    testit<ST>("3",
        [](){ return serialize<ST>(3); },
        [](const auto& v) {
            return v.asInt32() == 3;
        });

    // Passing a string: "asdf" via char*
    testit<ST>("\"asdf\" (via rvalue char*)",
        [](){ return serialize<ST>("asdf"); },
        [](const auto& v) {
            return v.asString() == "asdf";
        });

    // Passing a string: "asdf" via temp string
    testit<ST>("\"asdf\" (via rvalue temp string)",
        [](){ return serialize<ST>(std::string{"asdf"}); },
        [](const auto& v) {
            return v.asString() == "asdf";
        });

    // Passing a string: "asdf" via lvalue string
    testit<ST>("\"asdf\" (via lvalue string)",
        [](){ std::string s = "asdf"; return serialize<ST>(s); },
        [](const auto& v) {
            return v.asString() == "asdf";
        });
    
    // Passing a string: "asdf" via const lvalue string
    testit<ST>("\"asdf\" (via const lvalue string)",
        [](){ const std::string s = "asdf"; return serialize<ST>(s); },
        [](const auto& v) {
            return v.asString() == "asdf";
        });

    // Passing multiple rvalues: { 3, 5.2, "asdf" }
    testit<SerializerType>("{ 3, 5.2, \"asdf\" } (via rvalues)",
        [](){ return zerialize::serialize<SerializerType>(3, 5.2, "asdf"); },
        [](const auto& v) {
            return v[0].asInt32() == 3 &&
                v[1].asDouble() == 5.2 &&
                v[2].asString() == "asdf";
        });

    // Passing an initializer list: { 3, 5.2, "asdf" }
    testit<SerializerType>("3, 5.2, \"asdf\" (via initializer list)",
        [](){ return zerialize::serialize<SerializerType>({ 3, 5.2, "asdf" }); },
        [](const auto& v) {
            return v[0].asInt32() == 3 &&
                v[1].asDouble() == 5.2 &&
                v[2].asString() == "asdf";
        });

    // Passing multiple values including a vector: 3, 5.2, "asdf", [7, 8.2]
    testit<SerializerType>("3, 5.2, \"asdf\", [7, 8.2]",
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
    testit<SerializerType>("{\"a\": 5, \"b\": 5.3, \"c\": \"asdf\"} (via initializer list)",
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
    testit<SerializerType>("Generic rvalue list: {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\"}",
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
    testit<SerializerType>("Generic rvalue list: {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\", \"d\": [7, 8.2]}",
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
    testit<SerializerType>("Initializer list: {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\", \"d\": [7, 8.2]}",
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


    //return;


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
        auto span_view = flatbuffers::span<const uint8_t>(v.buf().data(), v.buf().size());
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

     std::cout << "..END testing zerialize: <" << zerialize::SerializerName<SerializerType>::value << ">" << endl << endl;
}


template<typename SrcSerializerType, typename DestSerializerType>
void test_conversion() {
    auto v1 = zerialize::serialize<SrcSerializerType>({{"a", 3}, {"b", 5.2}, {"c", "asdf"}, {"d", std::vector<std::any>{7, 8.2, std::map<std::string, std::any>{{"pi", 3.14159}, {"e", 2.613}}}}});
    //auto v1 = zerialize::serialize<SrcSerializerType>({ {"a", 3}, {"b", 5.2}, {"c", "asdf"}, {"d", std::vector<std::any>{7, 8.2}} });
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
}

int main() {
    testem<zerialize::Flex>();
    testem<zerialize::Json>();
    test_conversion<zerialize::Flex, zerialize::Json>();
    test_conversion<zerialize::Json, zerialize::Flex>();
    return 0;
}