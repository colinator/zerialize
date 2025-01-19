#include <iostream>
#include <zerialize/zerialize.hpp>

int main() {
    std::cout << "testing zerialize:" << std::endl << std::endl;

    // std::cout << "--- nothing ---" << std::endl;
    // auto s0 = zerialize::serialize<zerialize::Flex>();
    // std::cout << s0.to_string() << std::endl;
    // std::cout << std::endl;

    // std::cout << "--- 3 ---" << std::endl;
    // auto v1 = zerialize::serialize<zerialize::Flex>(3);
    // std::cout << v1.to_string() << std::endl;
    // std::cout << v1.asInt32() << std::endl;
    // std::cout << std::endl;

    {
        std::cout << "--- asdf (via rvalue char*) ---" << std::endl;
        auto v = zerialize::serialize<zerialize::Flex>("asdf");
        std::cout << v.to_string() << std::endl;
        std::cout << v.asString() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- asdf (via rvalue temp string) ---" << std::endl;
        auto v = zerialize::serialize<zerialize::Flex>(std::string{"asdf"});
        std::cout << v.to_string() << std::endl;
        std::cout << v.asString() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- asdf (via lvalue string) ---" << std::endl;
        std::string s = "asdf";
        auto v = zerialize::serialize<zerialize::Flex>(s);
        std::cout << v.to_string() << std::endl;
        std::cout << v.asString() << std::endl;
        std::cout << std::endl;
    }

    // {
    //     std::cout << "--- { 3, 5.2, \"asdf\" } ---" << std::endl;
    //     auto v = zerialize::serialize<zerialize::Flex>({ 3, 5.2, "asdf"});
    //     std::cout << v.to_string() << std::endl;
    //     std::cout << v[0].asInt32() << " " << v[1].asDouble() << " " << v[2].asString() << std::endl;
    //     std::cout << std::endl;
    // }

    // {
    //     std::cout << "--- 3, 5.2, asdf ---" << std::endl;
    //     auto v = zerialize::serialize<zerialize::Flex>(3, 5.2, "asdf");
    //     std::cout << v[0].asInt32() << " " << v[1].asDouble() << " " << v[2].asString() << std::endl;
    //     std::cout << v.to_string() << std::endl;
    //     std::cout << std::endl;
    // }

    // std::cout << "--- 3, 5.2, asdf, [7, 8.2] ---" << std::endl;
    // auto v3 = zerialize::serialize<zerialize::Flex>({ 3, 5.2, "asdf", std::vector<std::any>{7, 8.2}});
    // std::cout << v3[0].asInt32() << " " << v3[1].asDouble() << " " << v3[2].asString() << " " << v3[3][0].asInt32() << " " << v3[3][1].asDouble() << std::endl;
    // std::cout << std::endl;

    // std::cout << "--- {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\"} ---" << std::endl;
    // auto v4 = zerialize::serialize<zerialize::Flex>({ {"a", 3}, {"b", 5.2}, {"c", "asdf"}});
    // std::cout << v4["a"].asInt32() << " " << v4["b"].asDouble() << " " << v4["c"].asString() << std::endl;
    // std::cout << std::endl;

    // std::cout << "--- deserialize ---" << std::endl;
    // auto v4buf = v4.buf();
    // auto v4d = zerialize::Flex::BufferType(v4buf);
    // std::cout << v4d["a"].asInt32() << " " << v4d["b"].asDouble() << " " << v4d["c"].asString() << std::endl;
    // std::cout << std::endl;

    // std::cout << "--- Creating test data ---" << std::endl;
    // auto v4_test = zerialize::serialize<zerialize::Flex>({ {"a", 3}, {"b", 5.2}, {"c", "asdf"}});
    // std::cout << v4_test["a"].asInt32() << " " << v4_test["b"].asDouble() << " " << v4_test["c"].asString() << std::endl;
    // std::cout << std::endl;

    // std::cout << "--- Testing different construction methods ---" << std::endl;
    
    // // 1. Using span (zero-copy view)
    // std::cout << "1. Construct from span (zero-copy view):" << std::endl;
    // auto span_view = flatbuffers::span<const uint8_t>(v4_test.buf().data(), v4_test.buf().size());
    // auto v4_span = zerialize::Flex::BufferType(span_view);
    // std::cout << v4_span["a"].asInt32() << " " << v4_span["b"].asDouble() << " " << v4_span["c"].asString() << std::endl;
    // std::cout << std::endl;

    // // 2. Move construction (zero-copy transfer)
    // std::cout << "2. Move construction (zero-copy transfer):" << std::endl;
    // auto buf_to_move = std::vector<uint8_t>(v4_test.buf());  // Make a copy to move from
    // auto v4_move = zerialize::Flex::BufferType(std::move(buf_to_move));
    // std::cout << v4_move["a"].asInt32() << " " << v4_move["b"].asDouble() << " " << v4_move["c"].asString() << std::endl;
    // std::cout << "Original vector size after move: " << buf_to_move.size() << std::endl;  // Should be 0
    // std::cout << std::endl;

    // // 3. Copy construction
    // std::cout << "3. Copy construction:" << std::endl;
    // auto v4_copy = zerialize::Flex::BufferType(v4_test.buf());
    // std::cout << v4_copy["a"].asInt32() << " " << v4_copy["b"].asDouble() << " " << v4_copy["c"].asString() << std::endl;
    // std::cout << std::endl;

    std::cout << "done" << std::endl;

    return 0;
}