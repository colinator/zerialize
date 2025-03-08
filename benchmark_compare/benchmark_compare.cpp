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
//#include <zerialize/zerialize_msgpack.hpp>

// Reflect-cpp includes
#include <rfl/json.hpp>
#include <rfl/flexbuf.hpp>
#include <rfl.hpp>

using namespace zerialize;
using namespace std::chrono;

// Define a struct for reflect-cpp that matches our test data
struct TestData {
    int int_value;
    double double_value;
    std::string string_value;
    std::vector<int> array_value;

    // // Reflect-cpp reflection definition
    // REFLECT(TestData, int_value, double_value, string_value, array_value)
};

// Simple benchmarking function that measures execution time
template<typename Func>
double benchmark(Func&& func, int iterations = 1000000) {
    auto start = high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
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
    
    // Medium data: Map nested values
    {
        // Benchmark serialization
        double serTime = ZerializeAsVector ?
            benchmark([&]() {
                auto serialized = serialize<SerializerType>({
                    42,
                    3.14159,
                    "hello world",
                    std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
                });
                return serialized;
            }) :
            benchmark([&]() {
                auto serialized = serialize<SerializerType>({
                    {"int_value", 42},
                    {"double_value", 3.14159},
                    {"string_value", "hello world"},
                    {"array_value", std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}}
                });
                return serialized;
            });

        
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

        std::vector<uint8_t> bufferCopy(serialized.buf());
        span<const uint8_t> newBuf(bufferCopy);

        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;
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
    
    // Create test data
    const TestData testData{
        42,
        3.14159,
        "hello world",
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
    };
    
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
    
    // Create test data
    TestData testData{
        42,
        3.14159,
        "hello world",
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
    };
    
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

// // Run Reflect-cpp MessagePack benchmarks
// std::vector<BenchmarkResult> runReflectCppMsgPackBenchmarks() {
//     std::vector<BenchmarkResult> results;
    
//     // Create test data
//     TestData testData{
//         42,
//         3.14159,
//         "hello world",
//         {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
//     };
    
//     // Measure serialization time
//     double serTime = benchmark([&]() {
//         auto serialized = rfl::flexbuf::write(testData);
//         return serialized;
//     });
    
//     // Create serialized data once for deserialization tests
//     auto serialized = reflect::msgpack::to_vector(testData);
    
//     // Measure deserialization time
//     double deserTime = benchmark([&]() {
//         auto deserialized = reflect::msgpack::from_vector<TestData>(serialized);
//         return deserialized;
//     });
    
//     results.push_back({
//         "Reflect-cpp: MessagePack", 
//         serTime, 
//         deserTime,
//         serialized.size(),
//         1000000
//     });
    
//     return results;
// }

int main() {
    std::cout << "Benchmarking Zerialize vs Reflect-cpp" << std::endl;
    std::cout << "====================================" << std::endl << std::endl;
    
    // Run Zerialize benchmarks

    std::cout << "Zerialize JSON Serializer:" << std::endl;
    auto zerializeJsonResults = runZerializeBenchmarks<Json>();
    printResults(zerializeJsonResults);
    std::cout << std::endl;

    std::cout << "Zerialize Flex Serializer:" << std::endl;
    auto zerializeFlexResults = runZerializeBenchmarks<Flex>();
    printResults(zerializeFlexResults);
    std::cout << std::endl;
    
    // std::cout << "Zerialize MsgPack Serializer:" << std::endl;
    // auto zerializeMsgPackResults = runZerializeBenchmarks<MsgPack>();
    // printResults(zerializeMsgPackResults);
    // std::cout << std::endl;
    
    // Run Reflect-cpp benchmarks
    std::cout << "Reflect-cpp JSON:" << std::endl;
    auto reflectCppJsonResults = runReflectCppJsonBenchmarks();
    printResults(reflectCppJsonResults);
    std::cout << std::endl;
    
    std::cout << "Reflect-cpp FlexBuffers:" << std::endl;
    auto reflectCppFlexBuffersResults = runReflectCppFlexBuffersBenchmarks();
    printResults(reflectCppFlexBuffersResults);
    std::cout << std::endl;
    
    // std::cout << "Reflect-cpp MessagePack:" << std::endl;
    // auto reflectCppMsgPackResults = runReflectCppMsgPackBenchmarks();
    // printResults(reflectCppMsgPackResults);
    // std::cout << std::endl;
    
    std::cout << "Benchmark complete!" << std::endl;
    return 0;
}
