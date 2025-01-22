#include <iostream>
#include <zerialize/zerialize.hpp>
#include <zerialize/zerialize_flex.hpp>
#include <zerialize/zerialize_json.hpp>

template <typename SerializerType>
void testem() {
    std::cout << "testing zerialize:" << std::endl << std::endl;

    {
        std::cout << "--- nothing ---" << std::endl;
        auto v = zerialize::serialize<SerializerType>();
        std::cout << v.to_string() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- 3 ---" << std::endl;
        auto v = zerialize::serialize<SerializerType>(3);
        std::cout << v.to_string() << std::endl;
        std::cout << v.asInt32() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- asdf (via rvalue char*) ---" << std::endl;
        auto v = zerialize::serialize<SerializerType>("asdf");
        std::cout << v.to_string() << std::endl;
        std::cout << v.asString() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- asdf (via rvalue temp string) ---" << std::endl;
        auto v = zerialize::serialize<SerializerType>(std::string{"asdf"});
        std::cout << v.to_string() << std::endl;
        std::cout << v.asString() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- asdf (via lvalue string) ---" << std::endl;
        std::string s = "asdf";
        auto v = zerialize::serialize<SerializerType>(s);
        std::cout << v.to_string() << std::endl;
        std::cout << v.asString() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- asdf (via const lvalue string) ---" << std::endl;
        const std::string s = "asdf";
        auto v = zerialize::serialize<SerializerType>(s);
        std::cout << v.to_string() << std::endl;
        std::cout << v.asString() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- { 3, 5.2, \"asdf\" } via rvalues ---" << std::endl;
        auto v = zerialize::serialize<SerializerType>(3, 5.2, "asdf");
        std::cout << v.to_string() << std::endl;
        std::cout << v[0].asInt32() << " " << v[1].asDouble() << " " << v[2].asString() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- { 3, 5.2, asdf } via initializer list ---" << std::endl;
        auto v = zerialize::serialize<SerializerType>({ 3, 5.2, "asdf"});
        std::cout << v.to_string() << std::endl;
        std::cout << v[0].asInt32() << " " << v[1].asDouble() << " " << v[2].asString() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- 3, 5.2, asdf, [7, 8.2] ---" << std::endl;
        auto v = zerialize::serialize<SerializerType>(3, 5.2, "asdf", std::vector<std::any>{7, 8.2});
        std::cout << v.to_string() << std::endl;
        std::cout << v[0].asInt32() << " " << v[1].asDouble() << " " << v[2].asString() << " " << v[3][0].asInt32() << " " << v[3][1].asDouble() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\"} via initializer list ---" << std::endl;
        auto v = zerialize::serialize<SerializerType>({ {"a", 3}, {"b", 5.2}, {"c", "asdf"}});
        std::cout << v.to_string() << std::endl;
        std::cout << v["a"].asInt32() << " " << v["b"].asDouble() << " " << v["c"].asString() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\"} via generic rvalue list ---" << std::endl;
        auto v = zerialize::serialize<SerializerType, int, double, std::string>({"a", 3}, {"b", 5.2}, {"c", "asdf"});
        std::cout << v.to_string() << std::endl;
        std::cout << v["a"].asInt32() << " " << v["b"].asDouble() << " " << v["c"].asString() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\", \"d\": [7, 8.2]} via generic rvalue list ---" << std::endl;
        auto v = zerialize::serialize<SerializerType, int, double, std::string, std::vector<std::any>>({"a", 3}, {"b", 5.2}, {"c", "asdf"}, {"d", std::vector<std::any>{7, 8.2}});
        std::cout << v.to_string() << std::endl;
        std::cout << v["a"].asInt32() << " " << v["b"].asDouble() << " " << v["c"].asString() << " " << v["d"][0].asInt32() << " " << v["d"][1].asDouble() << std::endl;
        std::cout << std::endl;
    }

    {
        std::cout << "--- {\"a\": 5, \"b\": 5.3, \"c\": \"asdf\", \"d\": [7, 8.2]} via initializer list ---" << std::endl;
        auto v = zerialize::serialize<SerializerType>({{"a", 3}, {"b", 5.2}, {"c", "asdf"}, {"d", std::vector<std::any>{7, 8.2}}});
        std::cout << v.to_string() << std::endl;
        std::cout << v["a"].asInt32() << " " << v["b"].asDouble() << " " << v["c"].asString() << " " << v["d"][0].asInt32() << " " << v["d"][1].asDouble() << std::endl;
        std::cout << std::endl;
    }

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
    }

    std::cout << "done" << std::endl;
}

int main() {
    std::cout << "zerialize::Flex" << std::endl;
    testem<zerialize::Flex>();
    std::cout << std::endl << std::endl;

    std::cout << "zerialize::Json" << std::endl;
    testem<zerialize::Json>();
    std::cout << std::endl << std::endl;

    return 0;
}