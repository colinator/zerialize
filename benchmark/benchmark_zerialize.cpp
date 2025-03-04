#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <iomanip>
#include <zerialize/zerialize.hpp>
#include <zerialize/zerialize_flex.hpp>
#include <zerialize/zerialize_json.hpp>
#include <zerialize/zerialize_msgpack.hpp>
#include <zerialize/zerialize_xtensor.hpp>
#include <zerialize/zerialize_eigen.hpp>

using namespace zerialize;
using namespace std::chrono;

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

// Template function to run benchmarks for a specific serializer type
template<typename SerializerType>
std::vector<BenchmarkResult> runBenchmarks() {
    std::vector<BenchmarkResult> results;
    
    // Nothing - reference for function calling overhead
    {
        // Measure serialization time
        size_t counter = 0;
        double serTime = benchmark([&]() {
            // It's gotta do something...
            counter += 1;
        });

        // Measure deserialization time
        double deserTime = benchmark([&]() {
            counter += 1;
        });
        
        // Measure read time
        double readTime = benchmark([&]() {
            counter += 1;
            return 0;
        });
        
        results.push_back({
            "Function call only baseline", 
            serTime, 
            deserTime,
            readTime,
            counter,
            1000000
        });
    }

    // Null
    {
        // Measure serialization time
        double serTime = benchmark([&]() {
            auto serialized = serialize<SerializerType>();
            return serialized;
        });
        
        // Create serialized data once for deserialization tests
        auto serialized = serialize<SerializerType>();

        std::vector<uint8_t> bufferCopy(serialized.buf());
        span<const uint8_t> newBuf(bufferCopy);
        
        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            // Instantiate Buffer from copy
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;
        });
        
        // Measure read time
        double readTime = benchmark([&]() {
            return 0;
        });
        
        results.push_back({
            "Null", 
            serTime, 
            deserTime,
            readTime,
            serialized.size(),
            1000000
        });
    }


    // Small data: single int
    {
        int value = 42;
        
        // Measure serialization time
        double serTime = benchmark([&]() {
            auto serialized = serialize<SerializerType>(value);
            return serialized;
        });
        
        // Create serialized data once for deserialization tests
        auto serialized = serialize<SerializerType>(value);

        std::vector<uint8_t> bufferCopy(serialized.buf());
        span<const uint8_t> newBuf(bufferCopy);
        
        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            // Instantiate Buffer from copy
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;
        });
        
        // Measure read time
        double readTime = benchmark([&]() {
            auto deserialized = serialized.template as<int>();
            return deserialized;
        });
        
        results.push_back({
            "Small: Single Int", 
            serTime, 
            deserTime,
            readTime,
            serialized.size(),
            1000000
        });
    }
    
    // Small data: few values
    {
        // Measure serialization time
        double serTime = benchmark([&]() {
            auto serialized = serialize<SerializerType>(42, 3.14, "hello");
            return serialized;
        });
        
        // Create serialized data once for deserialization tests
        auto serialized = serialize<SerializerType>(42, 3.14, "hello");
        std::vector<uint8_t> bufferCopy(serialized.buf());
        span<const uint8_t> newBuf(bufferCopy);

        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;
        });
        
        // Measure read time
        double readTime = benchmark([&]() {
            int i = serialized[0].template as<int>();
            double d = serialized[1].template as<double>();
            std::string s = serialized[2].template as<std::string>();
            return i + d + s.size();  // Just to use the values
        });
        
        results.push_back({
            "Small: Int, Double, String", 
            serTime, 
            deserTime,
            readTime,
            serialized.size(),
            1000000
        });
    }

    // Medium data: map with nested values
    {
        // Measure serialization time
        double serTime = benchmark([&]() {
            auto serialized = serialize<SerializerType>({
                {"int", 42},
                {"double", 3.14159},
                {"string", "hello world"},
                {"array", std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}}
            });
            return serialized;
        });
        
        // Create serialized data once for deserialization tests
        auto serialized = serialize<SerializerType>({
            {"int", 42},
            {"double", 3.14159},
            {"string", "hello world"},
            {"array", std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}}
        });
        
        std::vector<uint8_t> bufferCopy(serialized.buf());
        span<const uint8_t> newBuf(bufferCopy);

        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;
        });
        
        // Measure read time
        double readTime = benchmark([&]() {
            int i = serialized["int"].template as<int>();
            double d = serialized["double"].template as<double>();
            std::string s = serialized["string"].template as<std::string>();
            auto arr = serialized["array"];
            int sum = 0;
            for (size_t i = 0; i < arr.arraySize(); i++) {
                sum += arr[i].asInt32();
            }
            return i + d + s.size() + sum;  // Just to use the values
        });
        
        results.push_back({
            "Medium: Map with Nested Values", 
            serTime, 
            deserTime,
            readTime,
            serialized.size(),
            1000000
        });
    }

    // Medium data: small xtensor
    {
        auto tensor = xt::xtensor<double, 2>{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
        
        // Measure serialization time
        double serTime = benchmark([&]() {
            auto serialized = serialize<SerializerType>({
                {"tensor", xtensor::serializer<SerializerType>(tensor)},
                {"name", "small tensor"}
            });
            return serialized;
        });
        
        // Create serialized data once for deserialization tests
        auto serialized = serialize<SerializerType>({
            {"tensor", xtensor::serializer<SerializerType>(tensor)},
            {"name", "small tensor"}
        });
        
        std::vector<uint8_t> bufferCopy(serialized.buf());
        span<const uint8_t> newBuf(bufferCopy);

        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;
        });
        
        // Measure read time
        double readTime = benchmark([&]() {
            auto t = xtensor::asXTensor<double, 2>(serialized["tensor"]);
            std::string s = serialized["name"].template as<std::string>();
            return t(0, 0) + s.size();  // Just to use the values
        });
        
        results.push_back({
            "Medium: Small XTensor (2x3)", 
            serTime, 
            deserTime,
            readTime,
            serialized.size(),
            1000000
        });
    }
    
    // Medium data: small Eigen matrix
    {
        Eigen::Matrix3d matrix;
        matrix << 1, 2, 3, 4, 5, 6, 7, 8, 9;
        
        // Measure serialization time
        double serTime = benchmark([&]() {
            auto serialized = serialize<SerializerType>({
                {"matrix", eigen::serializer<SerializerType>(matrix)},
                {"name", "small matrix"}
            });
            return serialized;
        });
        
        // Create serialized data once for deserialization tests
        auto serialized = serialize<SerializerType>({
            {"matrix", eigen::serializer<SerializerType>(matrix)},
            {"name", "small matrix"}
        });
        
        std::vector<uint8_t> bufferCopy(serialized.buf());
        span<const uint8_t> newBuf(bufferCopy);

        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;
        });
        
        // Measure read time
        double readTime = benchmark([&]() {
            auto m = eigen::asEigenMatrix<double, 3, 3>(serialized["matrix"]);
            std::string s = serialized["name"].template as<std::string>();
            return m(0, 0) + s.size();  // Just to use the values
        });
        
        results.push_back({
            "Medium: Small Eigen Matrix (3x3)", 
            serTime, 
            deserTime,
            readTime,
            serialized.size(),
            1000000
        });
    }
    
    // Large data: larger xtensor
    {
        auto tensor = xt::xtensor<double, 2>::from_shape({20, 20});
        for (size_t i = 0; i < 20; ++i) {
            for (size_t j = 0; j < 20; ++j) {
                tensor(i, j) = i * 20 + j;
            }
        }
        
        // Measure serialization time
        double serTime = benchmark([&]() {
            auto serialized = serialize<SerializerType>({
                {"tensor", xtensor::serializer<SerializerType>(tensor)},
                {"name", "large tensor"}
            });
            return serialized;
        }, 1000);  // Fewer iterations for large data
        
        // Create serialized data once for deserialization tests
        auto serialized = serialize<SerializerType>({
            {"tensor", xtensor::serializer<SerializerType>(tensor)},
            {"name", "large tensor"}
        });
        
        std::vector<uint8_t> bufferCopy(serialized.buf());
        span<const uint8_t> newBuf(bufferCopy);

        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;
        }, 1000);  // Fewer iterations for large data
        
        // Measure read time
        double readTime = benchmark([&]() {
            auto t = xtensor::asXTensor<double, 2>(serialized["tensor"]);
            std::string s = serialized["name"].template as<std::string>();
            return t(0, 0) + s.size();  // Just to use the values
        }, 100);  // Fewer iterations for large data
        
        results.push_back({
            "Large: XTensor (20x20)", 
            serTime, 
            deserTime,
            readTime,
            serialized.size(),
            1000
        });
    }
    
    // Large data: larger Eigen matrix
    {
        Eigen::MatrixXd matrix(20, 20);
        for (int i = 0; i < 20; ++i) {
            for (int j = 0; j < 20; ++j) {
                matrix(i, j) = i * 20 + j;
            }
        }
        
        // Measure serialization time
        double serTime = benchmark([&]() {
            auto serialized = serialize<SerializerType>({
                {"matrix", eigen::serializer<SerializerType>(matrix)},
                {"name", "large matrix"}
            });
            return serialized;
        }, 100);  // Fewer iterations for large data
        
        // Create serialized data once for deserialization tests
        auto serialized = serialize<SerializerType>({
            {"matrix", eigen::serializer<SerializerType>(matrix)},
            {"name", "large matrix"}
        });
        
        std::vector<uint8_t> bufferCopy(serialized.buf());
        span<const uint8_t> newBuf(bufferCopy);

        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;
        }, 1000);  // Fewer iterations for large data
        
        // Measure read time
        double readTime = benchmark([&]() {
            auto m = eigen::asEigenMatrix<double, 20, 20>(serialized["matrix"]);
            std::string s = serialized["name"].template as<std::string>();
            return m(0, 0) + s.size();  // Just to use the values
        }, 1000);  // Fewer iterations for large data
        
        results.push_back({
            "Large: Eigen Matrix (20x20)", 
            serTime, 
            deserTime,
            readTime,
            serialized.size(),
            1000
        });
    }

       // Very large data: very large xtensor
    {
        auto tensor = xt::xtensor<uint8_t, 3>::from_shape({3, 640, 480});
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 640; ++j) {
                for (size_t k = 0; k < 480; ++k) {
                    tensor(i, j, k) = i * 3 + j + k;
                }
            }
        }
        
        // Measure serialization time
        double serTime = benchmark([&]() {
            auto serialized = serialize<SerializerType>({
                {"tensor", xtensor::serializer<SerializerType>(tensor)},
                {"name", "large tensor"}
            });
            return serialized;
        }, 1000);  // Fewer iterations for large data
        
        // Create serialized data once for deserialization tests
        auto serialized = serialize<SerializerType>({
            {"tensor", xtensor::serializer<SerializerType>(tensor)},
            {"name", "large tensor"}
        });

        std::vector<uint8_t> bufferCopy(serialized.buf());
        span<const uint8_t> newBuf(bufferCopy);

        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            typename SerializerType::BufferType buffer(newBuf);
            return buffer;
        }, 1000);  // Fewer iterations for large data
        
        // Measure read time
        double readTime = benchmark([&]() {
            auto t = xtensor::asXTensor<uint8_t, 3>(serialized["tensor"]);
            std::string s = serialized["name"].template as<std::string>();
            //return xt::sum(t)() + s.size();  // Just to use the values
            return t(0, 0, 0) + s.size();  // Just to use the values
        }, 1000);  // Fewer iterations for large data
        
        results.push_back({
            "Very large: XTensor (3x640x480)", 
            serTime, 
            deserTime,
            readTime,
            serialized.size(),
            1000
        });
    }
    
    return results;
}

int main() {
    std::cout << "Benchmarking Zerialize Library" << std::endl;
    std::cout << "=============================" << std::endl << std::endl;
    
    // Run benchmarks for each serializer type
    std::cout << "Flex Serializer:" << std::endl;
    auto flexResults = runBenchmarks<Flex>();
    printResults(flexResults);
    std::cout << std::endl;
    
    std::cout << "JSON Serializer:" << std::endl;
    auto jsonResults = runBenchmarks<Json>();
    printResults(jsonResults);
    std::cout << std::endl;
    
    std::cout << "MsgPack Serializer:" << std::endl;
    auto msgpackResults = runBenchmarks<MsgPack>();
    printResults(msgpackResults);
    std::cout << std::endl;
    
    std::cout << "Benchmark complete!" << std::endl;
    return 0;
}
