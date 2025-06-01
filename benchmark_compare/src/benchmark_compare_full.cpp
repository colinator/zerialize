#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <iomanip>
#include <sstream>
#include <array>

// Zerialize includes
#include <zerialize/zerialize.hpp>
#include <zerialize/zerialize_flex.hpp>
#include <zerialize/zerialize_msgpack.hpp>
#include <zerialize/zerialize_json.hpp>
#include <zerialize/zerialize_xtensor.hpp>

// Reflect-cpp includes
#include <rfl/json.hpp>
#include <rfl/flexbuf.hpp>
#include <rfl/msgpack.hpp>
#include <rfl.hpp>

// We're only including xtensor through zerialize_xtensor.hpp
// No direct xtensor includes needed
#include <xtensor/generators/xrandom.hpp>

using namespace zerialize;
using namespace std::chrono;

// Define structs for the test cases
// 1. Small struct (similar to current)
struct SmallStruct {
    int int_value;
    double double_value;
    std::string string_value;
    std::vector<int> array_value;
};

// 2. Small struct with small xtensor
struct SmallTensorStruct {
    int int_value;
    double double_value;
    std::string string_value;
    xt::xarray<float> tensor_value; // Small 3x3x3 tensor
};

// 3. Small struct with large xtensor
struct LargeTensorStruct {
    int int_value;
    double double_value;
    std::string string_value;
    xt::xarray<float> tensor_value; // Large 3x1024x768 tensor
};

// Define corresponding struct for reflect-cpp
struct ReflectSmallStruct {
    int int_value;
    double double_value;
    std::string string_value;
    std::vector<int> array_value;
};

// Note: Reflect-cpp doesn't natively support xtensor, so we won't benchmark tensor structs for it

template<typename T>
struct BenchmarkResult {
    std::string name;
    double serializationTime;
    double deserializationTime;
    double readTime;
    double deserializeAndReadTime;
    double deserializeAndInstantiateTime;
    size_t dataSize;
    int iterations;
    T sample; // Store a sample for verification (optional)
};

// Simple benchmarking function that measures execution time
template<typename Func>
double benchmark(Func&& func, size_t iterations = 100000) {
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; i++) {
        auto r = func();
        (void)r; // Suppress unused variable warning
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    
    return static_cast<double>(duration) / iterations;
}

// Helper function to adjust iterations based on data size
size_t adjust_iterations(size_t dataSize) {
    if (dataSize > 10 * 1024 * 1024) // > 10MB
        return 10;
    else if (dataSize > 1024 * 1024) // > 1MB
        return 100;
    else if (dataSize > 100 * 1024) // > 100KB
        return 1000;
    else if (dataSize > 10 * 1024) // > 10KB
        return 10000;
    else
        return 100000;
}

// Create test data instances
SmallStruct createSmallStruct() {
    return {
        42,
        3.14159,
        "hello world",
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
    };
}

SmallTensorStruct createSmallTensorStruct() {
    auto tensor = xt::random::rand<float>({3, 3, 3});
    return {
        42,
        3.14159,
        "hello world",
        tensor
    };
}

LargeTensorStruct createLargeTensorStruct() {
    auto tensor = xt::random::rand<float>({3, 1024, 768});
    return {
        42,
        3.14159,
        "hello world",
        tensor
    };
}

ReflectSmallStruct createReflectSmallStruct() {
    return {
        42,
        3.14159,
        "hello world",
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
    };
}

// Print benchmark results in a table
template<typename T>
void printResults(const std::vector<BenchmarkResult<T>>& results) {
    std::cout << std::left << std::setw(40) << "Test Name" 
              << std::right << std::setw(15) << "Serialize (µs)" 
              << std::setw(15) << "Deserialize (µs)" 
              << std::setw(15) << "Read (µs)" 
              << std::setw(15) << "Deser+Read (µs)" 
              << std::setw(15) << "Deser+Inst (µs)" 
              << std::setw(15) << "Size (bytes)"
              << std::setw(12) << "(samples)" << std::endl;
    
    std::cout << std::string(142, '-') << std::endl;
    
    for (const auto& result : results) {
        std::cout << std::left << std::setw(40) << result.name 
                  << std::right << std::setw(14) << std::fixed << std::setprecision(3) << result.serializationTime 
                  << std::setw(14) << std::fixed << std::setprecision(3) << result.deserializationTime 
                  << std::setw(14) << std::fixed << std::setprecision(3) << result.readTime 
                  << std::setw(14) << std::fixed << std::setprecision(3) << result.deserializeAndReadTime 
                  << std::setw(14) << std::fixed << std::setprecision(3) << result.deserializeAndInstantiateTime 
                  << std::setw(14) << result.dataSize
                  << std::setw(12) << result.iterations << std::endl;
    }
}

// Run Zerialize benchmarks for SmallStruct
template<typename SerializerType>
BenchmarkResult<SmallStruct> runZerializeSmallStructBenchmark(const std::string& testName) {
    auto testData = createSmallStruct();
    
    // Benchmark serialization
    double serTime = benchmark([&]() {
        auto serialized = serialize<SerializerType>({
            {"int_value", testData.int_value},
            {"double_value", testData.double_value},
            {"string_value", testData.string_value},
            {"array_value", testData.array_value}
        });
        return serialized;
    });
    
    // Create serialized data once for deserialization tests
    auto buffer = serialize<SerializerType>({
        {"int_value", testData.int_value},
        {"double_value", testData.double_value},
        {"string_value", testData.string_value},
        {"array_value", testData.array_value}
    });
    
    auto bufferCopy = buffer.to_vector_copy();
    span<const uint8_t> bufferSpan(bufferCopy.data(), bufferCopy.size());
    
    // Measure deserialization time (constructing the buffer)
    double deserTime = benchmark([&]() {
        typename SerializerType::BufferType deserialized(bufferSpan);
        return deserialized;
    });
    
    // Create a buffer for read tests
    auto readBuffer = typename SerializerType::BufferType(bufferCopy);
    
    // Measure read time (accessing values from buffer)
    double readTime = benchmark([&]() {
        int i = readBuffer["int_value"].template as<int>();
        double d = readBuffer["double_value"].template as<double>();
        std::string s = readBuffer["string_value"].template as<std::string>();
        auto arr = readBuffer["array_value"];
        int sum = 0;
        for (size_t idx = 0; idx < arr.arraySize(); idx++) {
            sum += arr[idx].asInt32();
        }
        return i + d + s.size() + sum;
    });
    
    // Measure deserialize and read time (combined)
    double deserAndReadTime = benchmark([&]() {
        typename SerializerType::BufferType deserialized(bufferSpan);
        int i = deserialized["int_value"].template as<int>();
        double d = deserialized["double_value"].template as<double>();
        std::string s = deserialized["string_value"].template as<std::string>();
        auto arr = deserialized["array_value"];
        int sum = 0;
        for (size_t idx = 0; idx < arr.arraySize(); idx++) {
            sum += arr[idx].asInt32();
        }
        return i + d + s.size() + sum;
    });
    
    // Measure deserialize and instantiate struct time
    double deserAndInstantiateTime = benchmark([&]() {
        typename SerializerType::BufferType deserialized(bufferSpan);
        SmallStruct result;
        result.int_value = deserialized["int_value"].template as<int>();
        result.double_value = deserialized["double_value"].template as<double>();
        result.string_value = deserialized["string_value"].template as<std::string>();
        
        auto arr = deserialized["array_value"];
        result.array_value.clear();
        result.array_value.reserve(arr.arraySize());
        for (size_t idx = 0; idx < arr.arraySize(); idx++) {
            result.array_value.push_back(arr[idx].asInt32());
        }
        return result;
    });
    
    // Calculate iterations for future runs based on data size
    size_t iterations = adjust_iterations(bufferCopy.size());
    
    return {
        "Zerialize " + SerializerType::Name + ": " + testName,
        serTime,
        deserTime,
        readTime,
        deserAndReadTime,
        deserAndInstantiateTime,
        bufferCopy.size(),
        static_cast<int>(iterations),
        testData
    };
}

// Run Zerialize benchmarks for SmallTensorStruct
template<typename SerializerType>
BenchmarkResult<SmallTensorStruct> runZerializeSmallTensorStructBenchmark(const std::string& testName) {
    auto testData = createSmallTensorStruct();
    
    // Benchmark serialization
    double serTime = benchmark([&]() {
        auto serialized = serialize<SerializerType>({
            {"int_value", testData.int_value},
            {"double_value", testData.double_value},
            {"string_value", testData.string_value},
            {"tensor_value", testData.tensor_value}
        });
        return serialized;
    });
    
    // Create serialized data once for deserialization tests
    auto buffer = serialize<SerializerType>({
        {"int_value", testData.int_value},
        {"double_value", testData.double_value},
        {"string_value", testData.string_value},
        {"tensor_value", testData.tensor_value}
    });
    
    auto bufferCopy = buffer.to_vector_copy();
    span<const uint8_t> bufferSpan(bufferCopy.data(), bufferCopy.size());
    
    // Measure deserialization time (constructing the buffer)
    double deserTime = benchmark([&]() {
        typename SerializerType::BufferType deserialized(bufferSpan);
        return deserialized;
    });
    
    // Create a buffer for read tests
    auto readBuffer = typename SerializerType::BufferType(bufferCopy);
    
    // Measure read time (accessing values from buffer)
    double readTime = benchmark([&]() {
        int i = readBuffer["int_value"].template as<int>();
        double d = readBuffer["double_value"].template as<double>();
        std::string s = readBuffer["string_value"].template as<std::string>();
        // For tensors, we'll just access a few elements
        auto tensor = readBuffer["tensor_value"].template as<xt::xarray<float>>();
        float sum = tensor(0, 0, 0) + tensor(1, 1, 1) + tensor(2, 2, 2);
        return i + d + s.size() + sum;
    }, 10000); // Reduce iterations for tensor operations
    
    // Measure deserialize and read time (combined)
    double deserAndReadTime = benchmark([&]() {
        typename SerializerType::BufferType deserialized(bufferSpan);
        int i = deserialized["int_value"].template as<int>();
        double d = deserialized["double_value"].template as<double>();
        std::string s = deserialized["string_value"].template as<std::string>();
        auto tensor = deserialized["tensor_value"].template as<xt::xarray<float>>();
        float sum = tensor(0, 0, 0) + tensor(1, 1, 1) + tensor(2, 2, 2);
        return i + d + s.size() + sum;
    }, 10000);
    
    // Measure deserialize and instantiate struct time
    double deserAndInstantiateTime = benchmark([&]() {
        typename SerializerType::BufferType deserialized(bufferSpan);
        SmallTensorStruct result;
        result.int_value = deserialized["int_value"].template as<int>();
        result.double_value = deserialized["double_value"].template as<double>();
        result.string_value = deserialized["string_value"].template as<std::string>();
        result.tensor_value = deserialized["tensor_value"].template as<xt::xarray<float>>();
        return result;
    }, 10000);
    
    // Calculate iterations for future runs based on data size
    size_t iterations = adjust_iterations(bufferCopy.size());
    
    return {
        "Zerialize " + SerializerType::Name + ": " + testName,
        serTime,
        deserTime,
        readTime,
        deserAndReadTime,
        deserAndInstantiateTime,
        bufferCopy.size(),
        static_cast<int>(iterations),
        testData
    };
}

// Run Zerialize benchmarks for LargeTensorStruct
template<typename SerializerType>
BenchmarkResult<LargeTensorStruct> runZerializeLargeTensorStructBenchmark(const std::string& testName) {
    auto testData = createLargeTensorStruct();
    
    // Benchmark serialization
    double serTime = benchmark([&]() {
        auto serialized = serialize<SerializerType>({
            {"int_value", testData.int_value},
            {"double_value", testData.double_value},
            {"string_value", testData.string_value},
            {"tensor_value", testData.tensor_value}
        });
        return serialized;
    }, 100); // Reduce iterations for large tensors
    
    // Create serialized data once for deserialization tests
    auto buffer = serialize<SerializerType>({
        {"int_value", testData.int_value},
        {"double_value", testData.double_value},
        {"string_value", testData.string_value},
        {"tensor_value", testData.tensor_value}
    });
    
    auto bufferCopy = buffer.to_vector_copy();
    span<const uint8_t> bufferSpan(bufferCopy.data(), bufferCopy.size());
    
    // Measure deserialization time (constructing the buffer)
    double deserTime = benchmark([&]() {
        typename SerializerType::BufferType deserialized(bufferSpan);
        return deserialized;
    }, 100);
    
    // Create a buffer for read tests
    auto readBuffer = typename SerializerType::BufferType(bufferCopy);
    
    // Measure read time (accessing values from buffer)
    double readTime = benchmark([&]() {
        int i = readBuffer["int_value"].template as<int>();
        double d = readBuffer["double_value"].template as<double>();
        std::string s = readBuffer["string_value"].template as<std::string>();
        // For tensors, we'll just access a few elements
        auto tensor = readBuffer["tensor_value"].template as<xt::xarray<float>>();
        float sum = tensor(0, 0, 0) + tensor(1, 100, 100) + tensor(2, 500, 500);
        return i + d + s.size() + sum;
    }, 100);
    
    // Measure deserialize and read time (combined)
    double deserAndReadTime = benchmark([&]() {
        typename SerializerType::BufferType deserialized(bufferSpan);
        int i = deserialized["int_value"].template as<int>();
        double d = deserialized["double_value"].template as<double>();
        std::string s = deserialized["string_value"].template as<std::string>();
        auto tensor = deserialized["tensor_value"].template as<xt::xarray<float>>();
        float sum = tensor(0, 0, 0) + tensor(1, 100, 100) + tensor(2, 500, 500);
        return i + d + s.size() + sum;
    }, 100);
    
    // Measure deserialize and instantiate struct time
    double deserAndInstantiateTime = benchmark([&]() {
        typename SerializerType::BufferType deserialized(bufferSpan);
        LargeTensorStruct result;
        result.int_value = deserialized["int_value"].template as<int>();
        result.double_value = deserialized["double_value"].template as<double>();
        result.string_value = deserialized["string_value"].template as<std::string>();
        result.tensor_value = deserialized["tensor_value"].template as<xt::xarray<float>>();
        return result;
    }, 100);
    
    // Calculate iterations for future runs based on data size
    size_t iterations = adjust_iterations(bufferCopy.size());
    
    return {
        "Zerialize " + SerializerType::Name + ": " + testName,
        serTime,
        deserTime,
        readTime,
        deserAndReadTime,
        deserAndInstantiateTime,
        bufferCopy.size(),
        static_cast<int>(iterations),
        testData
    };
}

// Run Reflect-cpp benchmarks for SmallStruct (JSON)
BenchmarkResult<ReflectSmallStruct> runReflectCppJsonBenchmark(const std::string& testName) {
    auto testData = createReflectSmallStruct();
    
    // Benchmark serialization
    double serTime = benchmark([&]() {
        auto serialized = rfl::json::write(testData);
        return serialized;
    });
    
    // Create serialized data once for deserialization tests
    auto serialized = rfl::json::write(testData);
    
    // Measure deserialization time
    double deserTime = benchmark([&]() {
        auto deserialized = rfl::json::read<ReflectSmallStruct>(serialized).value();
        return deserialized;
    });
    
    // Create a deserialized object for read tests
    auto deserialized = rfl::json::read<ReflectSmallStruct>(serialized).value();
    
    // Measure read time
    double readTime = benchmark([&]() {
        int i = deserialized.int_value;
        double d = deserialized.double_value;
        std::string s = deserialized.string_value;
        auto& arr = deserialized.array_value;
        int sum = 0;
        for (size_t idx = 0; idx < arr.size(); idx++) {
            sum += arr[idx];
        }
        return i + d + s.size() + sum;
    });
    
    // Measure deserialize and read time (combined)
    double deserAndReadTime = benchmark([&]() {
        auto deser = rfl::json::read<ReflectSmallStruct>(serialized).value();
        int i = deser.int_value;
        double d = deser.double_value;
        std::string s = deser.string_value;
        auto& arr = deser.array_value;
        int sum = 0;
        for (size_t idx = 0; idx < arr.size(); idx++) {
            sum += arr[idx];
        }
        return i + d + s.size() + sum;
    });
    
    // For reflect-cpp, deserialize is the same as instantiate
    double deserAndInstantiateTime = deserTime;
    
    return {
        "Reflect-cpp JSON: " + testName,
        serTime,
        deserTime,
        readTime,
        deserAndReadTime,
        deserAndInstantiateTime,
        serialized.size(),
        100000,
        testData
    };
}

// Run Reflect-cpp benchmarks for SmallStruct (FlexBuffers)
BenchmarkResult<ReflectSmallStruct> runReflectCppFlexBenchmark(const std::string& testName) {
    auto testData = createReflectSmallStruct();
    
    // Benchmark serialization
    double serTime = benchmark([&]() {
        auto serialized = rfl::flexbuf::write(testData);
        return serialized;
    });
    
    // Create serialized data once for deserialization tests
    auto serialized = rfl::flexbuf::write(testData);
    
    // Measure deserialization time
    double deserTime = benchmark([&]() {
        auto deserialized = rfl::flexbuf::read<ReflectSmallStruct>(serialized).value();
        return deserialized;
    });
    
    // Create a deserialized object for read tests
    auto deserialized = rfl::flexbuf::read<ReflectSmallStruct>(serialized).value();
    
    // Measure read time
    double readTime = benchmark([&]() {
        int i = deserialized.int_value;
        double d = deserialized.double_value;
        std::string s = deserialized.string_value;
        auto& arr = deserialized.array_value;
        int sum = 0;
        for (size_t idx = 0; idx < arr.size(); idx++) {
            sum += arr[idx];
        }
        return i + d + s.size() + sum;
    });
    
    // Measure deserialize and read time (combined)
    double deserAndReadTime = benchmark([&]() {
        auto deser = rfl::flexbuf::read<ReflectSmallStruct>(serialized).value();
        int i = deser.int_value;
        double d = deser.double_value;
        std::string s = deser.string_value;
        auto& arr = deser.array_value;
        int sum = 0;
        for (size_t idx = 0; idx < arr.size(); idx++) {
            sum += arr[idx];
        }
        return i + d + s.size() + sum;
    });
    
    // For reflect-cpp, deserialize is the same as instantiate
    double deserAndInstantiateTime = deserTime;
    
    return {
        "Reflect-cpp FlexBuffers: " + testName,
        serTime,
        deserTime,
        readTime,
        deserAndReadTime,
        deserAndInstantiateTime,
        serialized.size(),
        100000,
        testData
    };
}

// Run Reflect-cpp benchmarks for SmallStruct (MsgPack)
BenchmarkResult<ReflectSmallStruct> runReflectCppMsgPackBenchmark(const std::string& testName) {
    auto testData = createReflectSmallStruct();
    
    // Benchmark serialization
    double serTime = benchmark([&]() {
        auto serialized = rfl::msgpack::write(testData);
        return serialized;
    });
    
    // Create serialized data once for deserialization tests
    auto serialized = rfl::msgpack::write(testData);
    
    // Measure deserialization time
    double deserTime = benchmark([&]() {
        auto deserialized = rfl::msgpack::read<ReflectSmallStruct>(serialized).value();
        return deserialized;
    });
    
    // Create a deserialized object for read tests
    auto deserialized = rfl::msgpack::read<ReflectSmallStruct>(serialized).value();
    
    // Measure read time
    double readTime = benchmark([&]() {
        int i = deserialized.int_value;
        double d = deserialized.double_value;
        std::string s = deserialized.string_value;
        auto& arr = deserialized.array_value;
        int sum = 0;
        for (size_t idx = 0; idx < arr.size(); idx++) {
            sum += arr[idx];
        }
        return i + d + s.size() + sum;
    });
    
    // Measure deserialize and read time (combined)
    double deserAndReadTime = benchmark([&]() {
        auto deser = rfl::msgpack::read<ReflectSmallStruct>(serialized).value();
        int i = deser.int_value;
        double d = deser.double_value;
        std::string s = deser.string_value;
        auto& arr = deser.array_value;
        int sum = 0;
        for (size_t idx = 0; idx < arr.size(); idx++) {
            sum += arr[idx];
        }
        return i + d + s.size() + sum;
    });
    
    // For reflect-cpp, deserialize is the same as instantiate
    double deserAndInstantiateTime = deserTime;
    
    return {
        "Reflect-cpp MsgPack: " + testName,
        serTime,
        deserTime,
        readTime,
        deserAndReadTime,
        deserAndInstantiateTime,
        serialized.size(),
        100000,
        testData
    };
}

// Helper function to print section header
void printSectionHeader(const std::string& title) {
    std::cout << "\n" << std::string(40, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(40, '=') << "\n\n";
}

int main() {
    std::cout << "Zerialize vs Reflect-cpp Comprehensive Benchmark\n";
    std::cout << "=============================================\n\n";
    
    // Group 1: JSON Serializer benchmarks
    printSectionHeader("JSON SERIALIZER");
    
    // JSON - Small Struct
    auto zerializeJsonSmall = runZerializeSmallStructBenchmark<zerialize::Json>("Small Struct");
    auto reflectJsonSmall = runReflectCppJsonBenchmark("Small Struct");
    
    std::vector<BenchmarkResult<SmallStruct>> jsonSmallResults;
    jsonSmallResults.push_back(zerializeJsonSmall);
    // Convert reflectJsonSmall to SmallStruct result for consistent printing
    BenchmarkResult<SmallStruct> convertedJsonSmall{
        reflectJsonSmall.name,
        reflectJsonSmall.serializationTime,
        reflectJsonSmall.deserializationTime,
        reflectJsonSmall.readTime,
        reflectJsonSmall.deserializeAndReadTime,
        reflectJsonSmall.deserializeAndInstantiateTime,
        reflectJsonSmall.dataSize,
        reflectJsonSmall.iterations,
        createSmallStruct() // Just a placeholder
    };
    jsonSmallResults.push_back(convertedJsonSmall);
    printResults(jsonSmallResults);
    
    // JSON - Small Tensor Struct
    auto zerializeJsonSmallTensor = runZerializeSmallTensorStructBenchmark<zerialize::Json>("Small Tensor Struct");
    std::vector<BenchmarkResult<SmallTensorStruct>> jsonSmallTensorResults;
    jsonSmallTensorResults.push_back(zerializeJsonSmallTensor);
    printResults(jsonSmallTensorResults);
    
    // JSON - Large Tensor Struct
    auto zerializeJsonLargeTensor = runZerializeLargeTensorStructBenchmark<zerialize::Json>("Large Tensor Struct");
    std::vector<BenchmarkResult<LargeTensorStruct>> jsonLargeTensorResults;
    jsonLargeTensorResults.push_back(zerializeJsonLargeTensor);
    printResults(jsonLargeTensorResults);
    
    // Group 2: FlexBuffer Serializer benchmarks
    printSectionHeader("FLEXBUFFER SERIALIZER");
    
    // Flex - Small Struct
    auto zerializeFlexSmall = runZerializeSmallStructBenchmark<zerialize::Flex>("Small Struct");
    auto reflectFlexSmall = runReflectCppFlexBenchmark("Small Struct");
    
    std::vector<BenchmarkResult<SmallStruct>> flexSmallResults;
    flexSmallResults.push_back(zerializeFlexSmall);
    // Convert reflectFlexSmall to SmallStruct result for consistent printing
    BenchmarkResult<SmallStruct> convertedFlexSmall{
        reflectFlexSmall.name,
        reflectFlexSmall.serializationTime,
        reflectFlexSmall.deserializationTime,
        reflectFlexSmall.readTime,
        reflectFlexSmall.deserializeAndReadTime,
        reflectFlexSmall.deserializeAndInstantiateTime,
        reflectFlexSmall.dataSize,
        reflectFlexSmall.iterations,
        createSmallStruct() // Just a placeholder
    };
    flexSmallResults.push_back(convertedFlexSmall);
    printResults(flexSmallResults);
    
    // Flex - Small Tensor Struct
    auto zerializeFlexSmallTensor = runZerializeSmallTensorStructBenchmark<zerialize::Flex>("Small Tensor Struct");
    std::vector<BenchmarkResult<SmallTensorStruct>> flexSmallTensorResults;
    flexSmallTensorResults.push_back(zerializeFlexSmallTensor);
    printResults(flexSmallTensorResults);
    
    // Flex - Large Tensor Struct
    auto zerializeFlexLargeTensor = runZerializeLargeTensorStructBenchmark<zerialize::Flex>("Large Tensor Struct");
    std::vector<BenchmarkResult<LargeTensorStruct>> flexLargeTensorResults;
    flexLargeTensorResults.push_back(zerializeFlexLargeTensor);
    printResults(flexLargeTensorResults);
    
    // Group 3: MsgPack Serializer benchmarks
    printSectionHeader("MSGPACK SERIALIZER");
    
    // MsgPack - Small Struct
    auto zerializeMsgPackSmall = runZerializeSmallStructBenchmark<zerialize::MsgPack>("Small Struct");
    auto reflectMsgPackSmall = runReflectCppMsgPackBenchmark("Small Struct");
    
    std::vector<BenchmarkResult<SmallStruct>> msgPackSmallResults;
    msgPackSmallResults.push_back(zerializeMsgPackSmall);
    // Convert reflectMsgPackSmall to SmallStruct result for consistent printing
    BenchmarkResult<SmallStruct> convertedMsgPackSmall{
        reflectMsgPackSmall.name,
        reflectMsgPackSmall.serializationTime,
        reflectMsgPackSmall.deserializationTime,
        reflectMsgPackSmall.readTime,
        reflectMsgPackSmall.deserializeAndReadTime,
        reflectMsgPackSmall.deserializeAndInstantiateTime,
        reflectMsgPackSmall.dataSize,
        reflectMsgPackSmall.iterations,
        createSmallStruct() // Just a placeholder
    };
    msgPackSmallResults.push_back(convertedMsgPackSmall);
    printResults(msgPackSmallResults);
    
    // MsgPack - Small Tensor Struct
    auto zerializeMsgPackSmallTensor = runZerializeSmallTensorStructBenchmark<zerialize::MsgPack>("Small Tensor Struct");
    std::vector<BenchmarkResult<SmallTensorStruct>> msgPackSmallTensorResults;
    msgPackSmallTensorResults.push_back(zerializeMsgPackSmallTensor);
    printResults(msgPackSmallTensorResults);
    
    // MsgPack - Large Tensor Struct
    auto zerializeMsgPackLargeTensor = runZerializeLargeTensorStructBenchmark<zerialize::MsgPack>("Large Tensor Struct");
    std::vector<BenchmarkResult<LargeTensorStruct>> msgPackLargeTensorResults;
    msgPackLargeTensorResults.push_back(zerializeMsgPackLargeTensor);
    printResults(msgPackLargeTensorResults);
    
    // Print summary table by test variant
    printSectionHeader("SUMMARY BY TEST VARIANT");
    
    // Small Struct Summary
    std::cout << "Small Struct Results:\n";
    std::vector<BenchmarkResult<SmallStruct>> smallStructSummary;
    smallStructSummary.push_back(zerializeJsonSmall);
    smallStructSummary.push_back(convertedJsonSmall);
    smallStructSummary.push_back(zerializeFlexSmall);
    smallStructSummary.push_back(convertedFlexSmall);
    smallStructSummary.push_back(zerializeMsgPackSmall);
    smallStructSummary.push_back(convertedMsgPackSmall);
    printResults(smallStructSummary);
    
    // Small Tensor Struct Summary
    std::cout << "\nSmall Tensor Struct Results:\n";
    std::vector<BenchmarkResult<SmallTensorStruct>> smallTensorStructSummary;
    smallTensorStructSummary.push_back(zerializeJsonSmallTensor);
    smallTensorStructSummary.push_back(zerializeFlexSmallTensor);
    smallTensorStructSummary.push_back(zerializeMsgPackSmallTensor);
    printResults(smallTensorStructSummary);
    
    // Large Tensor Struct Summary
    std::cout << "\nLarge Tensor Struct Results:\n";
    std::vector<BenchmarkResult<LargeTensorStruct>> largeTensorStructSummary;
    largeTensorStructSummary.push_back(zerializeJsonLargeTensor);
    largeTensorStructSummary.push_back(zerializeFlexLargeTensor);
    largeTensorStructSummary.push_back(zerializeMsgPackLargeTensor);
    printResults(largeTensorStructSummary);
    
    std::cout << "\nBenchmark complete!" << std::endl;
    return 0;
}
