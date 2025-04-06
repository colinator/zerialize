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
#include <zerialize/zerialize_msgpack.hpp>
#include <zerialize/zerialize_yyjson.hpp>

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
double benchmark(Func&& func, size_t iterations = 1000000) {
    auto start = high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; i++) {
        auto r = func();
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
       
        // // Create serialized data once for deserialization tests
        // auto serialized = ZerializeAsVector ?
        //    serialize<SerializerType>({
        //         42,
        //         3.14159,
        //         "hello world",
        //         std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
        //     }) :
        //     serialize<SerializerType>({
        //         {"int_value", 42},
        //         {"double_value", 3.14159},
        //         {"string_value", "hello world"},
        //         {"array_value", std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}}
        //     });
        // Create serialized data once for deserialization tests

        auto buffer = ZerializeAsVector ?
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


        //std::vector<uint8_t> bufferCopy(serialized.buf().begin(), serialized.buf().end());
        auto bufCopy = buffer.to_vector_copy();
        span<const uint8_t> newBuf(bufCopy.begin(), bufCopy.end());

        // for (int i=0; i<bufCopy.size(); i++) {
        //     std::cout << "" << (char)bufCopy[i];
        // }

        // Measure deserialization time (instantiating Buffer from copy)
        double deserTime = benchmark([&]() {
            typename SerializerType::BufferType deserialized(newBuf);
            return deserialized;

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

        auto serialized = typename SerializerType::BufferType(buffer.to_vector_copy());

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

double get_pure_serialization_time_msgpack(const std::array<int, 10>& sharedVec) {

    return benchmark([&]() {
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

        size_t size = sbuf.size;
        char* data = sbuf.data;
        //auto owned = std::unique_ptr<uint8_t[]>(reinterpret_cast<uint8_t*>(sbuf.data));

        // Prevent sbuffer from double-freeing
        sbuf.data = nullptr;
        sbuf.size = 0;
        sbuf.alloc = 0;

        //return MsgPackBuffer(std::move(owned), size);
        return ZBuffer(data, size, ZBuffer::Deleters::Free);
    });
}

double get_pure_serialization_time_yyjson(const std::array<int, 10>& sharedVec) {

    // Pre-define keys outside the lambda to avoid repeated construction
    // if the benchmark runs the lambda multiple times.
    const std::string k1 = "int_value";
    const std::string k2 = "double_value";
    const std::string k3 = "string_value";
    const std::string k4 = "array_value";
    const std::string v_str = "string_value"; // Renamed 'v' for clarity

    return benchmark([&]() { // Capture strings by reference

        // 1. Initialize yyjson mutable document
        yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr); // Use default allocator
        if (!doc) {
            // In a real benchmark, might abort or return specific error code
            // instead of throwing, as throwing can add overhead.
            throw std::runtime_error("yyjson_mut_doc_new failed");
        }

        // 2. Create the root object
        yyjson_mut_val *root_obj = yyjson_mut_obj(doc);
        if (!root_obj) {
            yyjson_mut_doc_free(doc); // Clean up doc
            throw std::runtime_error("yyjson_mut_obj failed");
        }

        // --- Use a try-catch block to ensure doc is freed on error ---
        std::vector<uint8_t> result_buffer; // Declare outside try for return
        char* json_str = nullptr;           // Pointer for allocated string

        try {
            // 3. Add key-value pairs directly
            // Note: yyjson_mut_obj_add takes ownership of key and val pointers
            // after successful addition. No need to free them separately.

            // Pair 1: int_value: 42
            yyjson_mut_val* key1 = yyjson_mut_strncpy(doc, k1.data(), k1.size());
            yyjson_mut_val* val1 = yyjson_mut_sint(doc, 42);
            if (!key1 || !val1 || !yyjson_mut_obj_add(root_obj, key1, val1)) {
                throw std::runtime_error("Failed to add int_value");
            }

            // Pair 2: double_value: 3.14159
            yyjson_mut_val* key2 = yyjson_mut_strncpy(doc, k2.data(), k2.size());
            yyjson_mut_val* val2 = yyjson_mut_real(doc, 3.14159);
            if (!key2 || !val2 || !yyjson_mut_obj_add(root_obj, key2, val2)) {
                throw std::runtime_error("Failed to add double_value");
            }

            // Pair 3: string_value: "string_value"
            yyjson_mut_val* key3 = yyjson_mut_strncpy(doc, k3.data(), k3.size());
            yyjson_mut_val* val3 = yyjson_mut_strncpy(doc, v_str.data(), v_str.size());
            if (!key3 || !val3 || !yyjson_mut_obj_add(root_obj, key3, val3)) {
                throw std::runtime_error("Failed to add string_value");
            }

            // Pair 4: array_value: [...]
            yyjson_mut_val* key4 = yyjson_mut_strncpy(doc, k4.data(), k4.size());
            yyjson_mut_val* arr = yyjson_mut_arr(doc);
            if (!key4 || !arr) {
                throw std::runtime_error("Failed to create key or array for array_value");
            }
            // Add elements to the array
            for (int item : sharedVec) {
                yyjson_mut_val* arr_item = yyjson_mut_sint(doc, item);
                // yyjson_mut_arr_append takes ownership of arr_item on success
                if (!arr_item || !yyjson_mut_arr_append(arr, arr_item)) {
                    throw std::runtime_error("Failed to append item to array");
                }
            }
            // Add the completed array to the root object
            if (!yyjson_mut_obj_add(root_obj, key4, arr)) {
                 throw std::runtime_error("Failed to add array_value");
            }

            // 4. Set the root of the document
            yyjson_mut_doc_set_root(doc, root_obj);

            // 5. Serialize the document to a string
            size_t len = 0;
            // Use YYJSON_WRITE_NOFLAG for potentially fastest output (no pretty print)
            json_str = yyjson_mut_write(doc, YYJSON_WRITE_NOFLAG, &len);
            if (!json_str) {
                throw std::runtime_error("yyjson_mut_write failed");
            }

            // 6. Copy the result into the desired buffer format (vector<uint8_t>)
            //    Do this *before* freeing the doc, as json_str points into doc memory?
            //    Correction: yyjson_mut_write allocates separately, safe to copy then free doc.
            result_buffer.assign(json_str, json_str + len);

        } catch (...) {
             // Clean up resources on exception
             if(json_str) free(json_str); // Free allocated string if write succeeded partially
             if(doc) yyjson_mut_doc_free(doc); // Free the document structure
             throw; // Re-throw
        }

        // 7. Clean up yyjson resources (on success path)
        free(json_str); // Free yyjson's allocated string
        yyjson_mut_doc_free(doc); // Free the document structure and its pooled memory

        // 8. Return the result buffer from the lambda (for benchmark wrapper)
        return result_buffer;

    }); // End benchmark lambda
}

int main() {
    std::cout << "Benchmarking Zerialize vs Reflect-cpp" << std::endl;
    std::cout << "====================================" << std::endl << std::endl;
    
    
    // Run Zerialize benchmarks

    // std::cout << "Zerialize JSON Serializer:" << std::endl;
    // auto zerializeJsonResults = runZerializeBenchmarks<Json>();
    // printResults(zerializeJsonResults);
    // std::cout << std::endl;

    std::cout << "Zerialize ---" << std::endl << std::endl;

    std::cout << "YYJson Serializer:" << std::endl;
    auto zerializeYYJsonPackResults = runZerializeBenchmarks<Yyjson>();
    printResults(zerializeYYJsonPackResults);
    std::cout << std::endl;

    std::cout << "Flex Serializer:" << std::endl;
    auto zerializeFlexResults = runZerializeBenchmarks<Flex>();
    printResults(zerializeFlexResults);
    std::cout << std::endl;
    
    std::cout << "MsgPack Serializer:" << std::endl;
    auto zerializeMsgPackResults = runZerializeBenchmarks<MsgPack>();
    printResults(zerializeMsgPackResults);
    std::cout << std::endl;

    
    // Run Reflect-cpp benchmarks

    std::cout << "Reflect-cpp ---" << std::endl << std::endl;

    std::cout << "JSON:" << std::endl;
    auto reflectCppJsonResults = runReflectCppJsonBenchmarks();
    printResults(reflectCppJsonResults);
    std::cout << std::endl;
    
    std::cout << "FlexBuffers:" << std::endl;
    auto reflectCppFlexBuffersResults = runReflectCppFlexBuffersBenchmarks();
    printResults(reflectCppFlexBuffersResults);
    std::cout << std::endl;
    
    std::cout << "MessagePack:" << std::endl;
    auto reflectCppMsgPackResults = runReflectCppMsgPackBenchmarks();
    printResults(reflectCppMsgPackResults);
    std::cout << std::endl;


    std::array<int, 10> sharedVec = {1,2,3,4,5,6,7,8,9,10};
    std::cout << "Pure serialization time, msgpack: " << get_pure_serialization_time_msgpack(sharedVec) << "µs" << std::endl;
    std::cout << "Pure serialization time, yyjson:  " << get_pure_serialization_time_yyjson(sharedVec)  << "µs"<< std::endl;
    
    std::cout << "Benchmark complete!" << std::endl;
    return 0;
}
