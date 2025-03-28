#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <iomanip>

// Zerialize includes
#include <zerialize/zerialize.hpp>
#include <zerialize/zerialize_flex.hpp>
#include <zerialize/zerialize_json.hpp>
#include <zerialize/zerialize_msgpack.hpp>

// Reflect-cpp includes
#include <rfl/json.hpp>
#include <rfl/flexbuf.hpp>
#include <rfl/msgpack.hpp>
#include <rfl.hpp>

using namespace zerialize;
using namespace std::chrono;

// Define a struct for reflect-cpp that matches our test data
struct TestData {
    int int_value;
    double double_value;
    std::string string_value;
    std::vector<int> array_value;
};

    
// Create test data
TestData testData{
    42,
    3.14159,
    "hello world",
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
};

// Simple benchmarking function that measures execution time
template<typename Func>
double benchmark(Func&& func, size_t iterations = 10000000) {
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; i++) {
        func();
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    
    return static_cast<double>(duration) / iterations;
}

// Benchmark results structure
struct BenchmarkResult {
    std::string name;
    double serializationTime;
    double deserializationTime;
    double readTime;
    size_t dataSize;
    int iterations;
};

// Print benchmark results in a table
void printResults(const std::vector<BenchmarkResult>& results) {
    std::cout << std::left << std::setw(35) << "Test Name" 
              << std::right << std::setw(18) << "Serialize (µs)" 
              << std::setw(18) << "Deserialize (µs)" 
              << std::setw(18) << "Read (µs)" 
              << std::setw(18) << "Data Size (bytes)"
              << std::setw(14) << "(samples)" << std::endl;
    
    std::cout << std::string(118, '-') << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::left << std::setw(35) << result.name 
                  << std::right << std::setw(17) << std::fixed << std::setprecision(3) << result.serializationTime 
                  << std::setw(17) << std::fixed << std::setprecision(3) << result.deserializationTime 
                  << std::setw(17) << std::fixed << std::setprecision(3) << result.readTime 
                  << std::setw(17) << result.dataSize
                  << std::setw(15) << result.iterations << std::endl;
    }
}

constexpr bool ZerializeAsVector = false;

// Run Zerialize benchmarks for a specific serializer type
template<typename SerializerType>
std::vector<BenchmarkResult> runZerializeBenchmarks() {
    std::vector<BenchmarkResult> results;

    std::array<int, 10> sharedVec = {1,2,3,4,5,6,7,8,9,10};
    std::string sharedStr = "hello world";
    std::string key1 = "hint_value";
    std::string key2 = "double_value";
    std::string key3 = "string_value";
    std::string key4 = "array_value";

    std::map<std::string, std::any> m = { 
        { "int_value", 42 },
        { "double_value", 3.14159 },
        { "string_value", "hello world"},
        { "array_value", std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10} }
    };

    // Medium data: Map nested values
    {
        // Benchmark serialization
        double serTime = ZerializeAsVector ?
            benchmark([&]() {
                std::cout << "YOYOYO" << std::endl;
                auto serialized = serialize<SerializerType>({
                    42,
                    3.14159,
                    "hello world",
                    std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
                });
                return serialized;
            }) :
            benchmark([&]() {

                auto serialized = serialize<SerializerType>(
                    // {
                    // {"int_value", 42},
                    // {"double_value", 3.14159},
                    // {"string_value", "hello world"},
                    // {"array_value", std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }}
                    // }

                    // {"int_value", 42},
                    // {"double_value", 3.14159},
                    // {"string_value", sharedStr},
                    // {"array_value", sharedVec}
                    //zmap(
                        zkv("int_value", 42),
                        zkv("double_value", 3.14159),
                        zkv("string_value", sharedStr),
                        zkv("array_value", sharedVec)
                    //)

                    // kv( "int_value", 42 ),
                    // kv( "double_value", 3.14159 ),
                    // kv( "string_value", "hello world" ),
                    // kv( "array_value", std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } )

                    // kv( key1, 42 ),
                    // kv( key2, 3.14159 ),
                    // kv( key3, sharedStr ),
                    // kv( key4, sharedVec )

                    // kv( "string_value", "hello world" ),
                    // kv( "array_value", std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } )
                );
                return serialized;
            });
            // benchmark([&]() {
            //     auto serialized = serialize<SerializerType>(m);
            //     return serialized;
            // }) :
            // benchmark([&]() {
            //     auto serialized = serialize<SerializerType>(m);
            //     return serialized;
            // });

       
        // Create serialized data once for deserialization tests
        auto serialized = ZerializeAsVector ?
           serialize<SerializerType>({
                42,
                3.14159,
                "hello world",
                std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
            }) :
            serialize<SerializerType>({
                {"int_value", 42},
                {"double_value", 3.14159},
                {"string_value", "hello world"},
                {"array_value", std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}}
            });   

        std::vector<uint8_t> bufferCopy(serialized.buf().begin(), serialized.buf().end());
        span<const uint8_t> newBuf(bufferCopy);

        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;

            // int i = buffer["int_value"].template as<int>();
            // double d = buffer["double_value"].template as<double>();
            // std::string s = buffer["string_value"].template as<std::string>();
            // auto arr = buffer["array_value"];
            // int sum = 0;
            // for (size_t i = 0; i < arr.arraySize(); i++) {
            //     sum += arr[i].asInt32();
            // }

            // std::vector<int> vec;
            // vec.reserve(arr.arraySize());
            // for (size_t i = 0; i < arr.arraySize(); i++) {
            //     vec.emplace_back(arr[i].asInt32());
            // }
            // TestData td{
            //     .int_value = i,
            //     .double_value = d,
            //     .string_value = s,
            //     .array_value = vec
            // };

            //return td;
        });
        
        // Measure read time
        double readTime = ZerializeAsVector ?
            // Just to use the values
            benchmark([&]() {
                int i = serialized[0].template as<int>();
                double d = serialized[1].template as<double>();
                std::string s = serialized[2].template as<std::string>();
                auto arr = serialized[3];
                int sum = 0;
                for (size_t i = 0; i < arr.arraySize(); i++) {
                    sum += arr[i].asInt32();
                }

                std::vector<int> vec;
                vec.reserve(arr.arraySize());
                for (size_t i = 0; i < arr.arraySize(); i++) {
                    vec.push_back(arr[i].asInt32());
                }
                TestData td{
                    .int_value = serialized[0].template as<int>(),
                    .double_value = serialized[1].template as<double>(),
                    .string_value = serialized[2].template as<std::string>(),
                    .array_value = vec
                };

                return i + d + s.size() + sum; 
            }) :
            benchmark([&]() {
                int i = serialized["int_value"].template as<int>();
                double d = serialized["double_value"].template as<double>();
                std::string s = serialized["string_value"].template as<std::string>();
                auto arr = serialized["array_value"];
                int sum = 0;
                for (size_t i = 0; i < arr.arraySize(); i++) {
                    sum += arr[i].asInt32();
                }

                // std::vector<int> vec;
                // vec.reserve(arr.arraySize());
                // for (size_t i = 0; i < arr.arraySize(); i++) {
                //     vec.emplace_back(arr[i].asInt32());
                // }
                // TestData td{
                //     .int_value = i,
                //     .double_value = d,
                //     .string_value = s,
                //     .array_value = vec
                // };
                return i + d + s.size() + sum;
            });

        results.push_back({
            "Zerialize: Map Nested Values", 
            serTime, 
            deserTime,
            readTime,
            serialized.size(),
            10000000
        });
    }
    
    return results;
}

// Run Reflect-cpp JSON benchmarks
std::vector<BenchmarkResult> runReflectCppJsonBenchmarks() {
    std::vector<BenchmarkResult> results;
    
    // Measure serialization time
    double serTime = benchmark([&]() {
        auto serialized = rfl::json::write(testData);
        return serialized;
    });
    
    // Create serialized data once for deserialization tests
    auto serialized = rfl::json::write(testData);
    
    // Measure deserialization time
    double deserTime = benchmark([&]() {
        // auto deserialized = reflect::json::from_string<TestData>(serialized);
        auto deserialized = rfl::json::read<TestData>(serialized).value();
        return deserialized;
    });

    auto deserialized = rfl::json::read<TestData>(serialized).value();

    // Measure read time
    double readTime = benchmark([&]() {
        int i = deserialized.int_value;
        double d = deserialized.double_value;
        std::string s = deserialized.string_value;
        auto arr = deserialized.array_value;
        int sum = 0;
        for (size_t i = 0; i < arr.size(); i++) {
            sum += arr[i];
        }
        return i + d + s.size() + sum;  // Just to use the values
    });
    
    results.push_back({
        "Reflect-cpp: JSON", 
        serTime, 
        deserTime,
        readTime,
        serialized.size(),
        1000000
    });
    
    return results;
}

// Run Reflect-cpp FlexBuffers benchmarks
std::vector<BenchmarkResult> runReflectCppFlexBuffersBenchmarks() {
    std::vector<BenchmarkResult> results;
    
    // Measure serialization time
    double serTime = benchmark([&]() {
        const auto serialized = rfl::flexbuf::write(testData);
        return serialized;
    });
    
    // Create serialized data once for deserialization tests
    const auto serialized = rfl::flexbuf::write(testData);
    
    // Measure deserialization time
    double deserTime = benchmark([&]() {
        const auto deserialized = rfl::flexbuf::read<TestData>(serialized);
        return deserialized;
    });
    
    auto deserialized = rfl::flexbuf::read<TestData>(serialized).value();

    // Measure read time
    double readTime = benchmark([&]() {
        int i = deserialized.int_value;
        double d = deserialized.double_value;
        std::string s = deserialized.string_value;
        auto arr = deserialized.array_value;
        int sum = 0;
        for (size_t i = 0; i < arr.size(); i++) {
            sum += arr[i];
        }
        return i + d + s.size() + sum;  // Just to use the values
    });

    results.push_back({
        "Reflect-cpp: FlexBuffers", 
        serTime, 
        deserTime,
        readTime,
        serialized.size(),
        1000000
    });
    
    return results;
}

// Run Reflect-cpp MessagePack benchmarks
std::vector<BenchmarkResult> runReflectCppMsgPackBenchmarks() {
    std::vector<BenchmarkResult> results;
    
    // Measure serialization time
    double serTime = benchmark([&]() {
        auto serialized = rfl::msgpack::write(testData);
        return serialized;
    });
    
    // Create serialized data once for deserialization tests
    auto serialized = rfl::msgpack::write(testData);
    
    // Measure deserialization time
    double deserTime = benchmark([&]() {
        auto deserialized = rfl::msgpack::read<TestData>(serialized);
        return deserialized;
    });
    
    auto deserialized = rfl::msgpack::read<TestData>(serialized).value();

    // Measure read time
    double readTime = benchmark([&]() {
        int i = deserialized.int_value;
        double d = deserialized.double_value;
        std::string s = deserialized.string_value;
        auto arr = deserialized.array_value;
        int sum = 0;
        for (size_t i = 0; i < arr.size(); i++) {
            sum += arr[i];
        }
        return i + d + s.size() + sum;  // Just to use the values
    });

    results.push_back({
        "Reflect-cpp: MessagePack", 
        serTime, 
        deserTime,
        readTime,
        serialized.size(),
        10000000
    });
    
    return results;
}

int main() {
    std::cout << "Benchmarking Zerialize vs Reflect-cpp" << std::endl;
    std::cout << "====================================" << std::endl << std::endl;
    
    
    // Run Zerialize benchmarks

    // std::cout << "Zerialize JSON Serializer:" << std::endl;
    // auto zerializeJsonResults = runZerializeBenchmarks<Json>();
    // printResults(zerializeJsonResults);
    // std::cout << std::endl;

    std::cout << "Zerialize Flex Serializer:" << std::endl;
    auto zerializeFlexResults = runZerializeBenchmarks<Flex>();
    printResults(zerializeFlexResults);
    std::cout << std::endl;
    
    std::cout << "Zerialize MsgPack Serializer:" << std::endl;
    auto zerializeMsgPackResults = runZerializeBenchmarks<MsgPack>();
    printResults(zerializeMsgPackResults);
    std::cout << std::endl;
    
    
    // Run Reflect-cpp benchmarks

    std::cout << "Reflect-cpp JSON:" << std::endl;
    auto reflectCppJsonResults = runReflectCppJsonBenchmarks();
    printResults(reflectCppJsonResults);
    std::cout << std::endl;
    
    std::cout << "Reflect-cpp FlexBuffers:" << std::endl;
    auto reflectCppFlexBuffersResults = runReflectCppFlexBuffersBenchmarks();
    printResults(reflectCppFlexBuffersResults);
    std::cout << std::endl;
    
    std::cout << "Reflect-cpp MessagePack:" << std::endl;
    auto reflectCppMsgPackResults = runReflectCppMsgPackBenchmarks();
    printResults(reflectCppMsgPackResults);
    std::cout << std::endl;


    unsigned long long ct = 0;

    std::array<int, 10> sharedVec = {1,2,3,4,5,6,7,8,9,10};

    double pureSerTime = benchmark([&]() {
        msgpack_sbuffer sbuf;
        msgpack_packer pk;

        msgpack_sbuffer_init(&sbuf);
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

        msgpack_pack_map(&pk, 4);

        const std::string k1 = "int_value";
        const std::string k2 = "double_value";
        const std::string k3 = "string_value";
        const std::string k4 = "array_value";
        const std::string v = "string_value";

        msgpack_pack_str(&pk, k1.size()); 
        msgpack_pack_str_body(&pk, k1.data(), k1.size());
        msgpack_pack_int32(&pk, 42);

        msgpack_pack_str(&pk, k2.size()); 
        msgpack_pack_str_body(&pk, k2.data(), k2.size());
        msgpack_pack_double(&pk, 3.14159);

        msgpack_pack_str(&pk, k3.size()); 
        msgpack_pack_str_body(&pk, k3.data(), k3.size());
        msgpack_pack_str(&pk, v.size()); 
        msgpack_pack_str_body(&pk, v.data(), v.size());

        msgpack_pack_str(&pk, k4.size()); 
        msgpack_pack_str_body(&pk, k4.data(), k4.size());

        msgpack_pack_array(&pk, sharedVec.size());
        for (auto & k: sharedVec) {
            msgpack_pack_int32(&pk, k);
        }
        // for (int i = 1; i <= 10; ++i) {
        //     msgpack_pack_int32(&pk, i);
        // }

        // auto sz = sbuf.size;
        // auto d = sbuf.data;

            size_t size = sbuf.size;
            auto owned = std::unique_ptr<uint8_t[]>(reinterpret_cast<uint8_t*>(sbuf.data));

            // Prevent sbuffer from double-freeing
            sbuf.data = nullptr;
            sbuf.size = 0;
            sbuf.alloc = 0;

            return MsgPackBuffer(std::move(owned), size);

        // msgpack_sbuffer_destroy(&sbuf);

        // ct += sz;

        // return sz;
    });

    std::cout << "PURE SER TIME: " << pureSerTime << " FROM CT:" << ct << std::endl;
    
    std::cout << "Benchmark complete!" << std::endl;
    return 0;
}
