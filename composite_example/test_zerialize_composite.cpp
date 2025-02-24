#include <iostream>
#include "zerialize_composite.hpp"
#include "zerialize/test_zerialize.hpp"
#include "zerialize/zerialize_flex.hpp"
#include "zerialize/zerialize_json.hpp"

using namespace zerialize;
using namespace zerialize::composite;

template<typename SerializerType>
void test_composite() {

    MyComposite<double, 3> c(3.14159, "yodude");

    testit<SerializerType>("Initializer list: MyComposite<float, 3>{3.14159,\"yodude\"}",
        //[c](){ return zerialize::composite::serialize<SerializerType>(c); },
        [c](){ return zerialize::serialize<SerializerType>(serialize(c)); },
        [c](const auto& v) {
            auto l = asMyComposite<double, 3>(v);
            std::cout << "Deserialized MyComposite: " << l.to_string() << std::endl;
            return l == c &&
                v["s"].asUInt64() == 3 &&
                v["a"].asDouble() == 3.14159 &&
                v["b"].asString() == "yodude";
        });

    testit<SerializerType>("Initializer with serialized composite",
        [c](){ 
            return zerialize::serialize<SerializerType>(
                { {"a", serialize(c)}, { "b", 457835 } }
            ); 
        },
        [c](const auto& v) {
            auto l = asMyComposite<double, 3>(v["a"]);
            return l == c && v["b"].asInt32() == 457835;
        });

    testit<SerializerType>("Initializer with serializer function",
        [c](){ 
            return zerialize::serialize<SerializerType>(
                { {"a", serializer<SerializerType>(c)}, { "b", 457835 } }
            ); 
        },
        [c](const auto& v) {
            auto l = asMyComposite<double, 3>(v["a"]);
            return l == c && v["b"].asInt32() == 457835;
        });
}

int main() {
    test_composite<zerialize::Flex>();
    std::cout << "test zerialize composite done, ALL SUCCEEDED" << std::endl;
    return 0;
}
