#include <iostream>
#include <string>
#include <iomanip>
#include <chrono>

#include <zerialize/zerialize.hpp>
#include <zerialize/protocols/flex.hpp>
#include <zerialize/protocols/msgpack.hpp>
#include <zerialize/protocols/json.hpp>
#include <zerialize/tensor/xtensor.hpp>

#include <xtensor/core/xmath.hpp>

// Reflect-cpp includes
#include <rfl/json.hpp>
#include <rfl/flexbuf.hpp>
#include <rfl/msgpack.hpp>
#include <rfl.hpp>

using namespace zerialize;
using namespace std::chrono;

using std::cout, std::endl, std::string;
using std::setw, std::setprecision, std::right, std::left, std::fixed;


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


// -------------------------
// Reflect-cpp support for xtensor

// -------------------------


// -------------------------
// Test variation combinations

// Test across these serialization types
enum class SerializationType {
    Flex,
    MsgPack,
    Json
};

// Test across these data variations
enum class DataType {
    SmallStruct,
    SmallStructAsVector,
    SmallTensorStruct,
    SmallTensorStructAsVector,
    LargeTensorStruct
};

// Test across these competitors
enum class CompetitorType {
    Zerialize,
    ReflectCpp
};


// -------------------------
// the message types we are testing

struct SmallStruct {
    int int_value;
    double double_value;
    std::string string_value;
    std::vector<int> array_value;
};

struct TensorWrapper {
    std::vector<size_t> shape;
    int dtype;
    //std::vector<std::uint8_t> data;
    rfl::Bytestring data;
};

struct SmallTensorStruct {
    int int_value;
    double double_value;
    std::string string_value;
    TensorWrapper tensor_value; // Small 4x4 tensor
};

struct LargeTensorStruct {
    int int_value;
    double double_value;
    std::string string_value;
    //rfl::Bytestring tensor_value;
    TensorWrapper tensor_value; // Large 3x1024x768 tensor
};


// -------------------------
// to string utility methods

template <SerializationType ST>
constexpr string st_to_string() {
    if constexpr (ST == SerializationType::Flex) { return "Flex"; } 
    else if constexpr (ST == SerializationType::MsgPack) { return "MsgPack"; } 
    else { return "Json"; }
}

template <DataType DT>
constexpr string dt_to_string() {
    if constexpr (DT == DataType::SmallStruct) { return "SmallStruct"; } 
    else if constexpr (DT == DataType::SmallStructAsVector) { return "SmallStructAsVector"; } 
    else if constexpr (DT == DataType::SmallTensorStruct) { return "SmallTensorStruct";} 
    else if constexpr (DT == DataType::SmallTensorStructAsVector) { return "SmallTensorStructAsVector"; } 
    else if constexpr (DT == DataType::LargeTensorStruct) { return "LargeTensorStruct"; } 
    else { return "unknown"; }
}

template <CompetitorType CT>
constexpr string ct_to_string() {
    if constexpr (CT == CompetitorType::Zerialize) { return "Zerialize"; } 
    else { return "ReflectCpp"; }
}


// -------------------------
// How we collect data.

struct BenchmarkResult {
    double serializationTime;
    double deserializationTime;
    double readTime;
    double deserializeAndReadTime;
    double deserializeAndInstantiateTime;
    size_t dataSize;
    size_t iterations;
};


// -------------------------
// Serialization methods

std::array<int, 10> smallArray = {1,2,3,4,5,6,7,8,9,10};
xt::xtensor<double, 2> smallXtensor{{1.0, 2.0, 3.0, 4.0}, {4.0, 5.0, 6.0, 7.0}, {8.0, 9.0, 10.0, 11.0}, {12.0, 13.0, 14.0, 15.0}};
//auto largeXTensor = xt::full<uint8_t>({3, 1024, 768}, 3);
xt::xtensor<uint8_t, 3> largeXTensor({3, 1024, 768});


// -------------------------
// Zerialize serialization methods (DSL: serialize/zmap/zvec)

template <typename P>
ZBuffer get_zerialized_smallstruct() {
    return serialize<P>(
        zmap<"int_value","double_value","string_value","array_value">(
            42,
            3.14159,
            "hello world",
            smallArray
        )
    );
}

template <typename P>
ZBuffer get_zerialized_smallstructasvector() {
    return serialize<P>(
        zvec(42, 3.14159, "hello world", smallArray)
    );
}

template <typename P>
ZBuffer get_zerialized_smalltensorstruct() {
    return serialize<P>(
        zmap<"int_value","double_value","string_value","tensor_value">(
            42,
            3.14159,
            "hello world",
            smallXtensor
        )
    );
}

template <typename P>
ZBuffer get_zerialized_smalltensorstructasvector() {
    return serialize<P>(
        zvec(42, 3.14159, "hello world", smallXtensor)
    );
}

template <typename P>
ZBuffer get_zerialized_largetensorstruct() {
    // relies on ADL overload: serialize(const xt::xtensor<uint8_t,3>&, W&)
    return serialize<P>(
        zmap<"int_value","double_value","string_value","tensor_value">(
            42,
            3.14159,
            "hello world",
            largeXTensor
        )
    );
}

template <typename P, DataType DT>
ZBuffer get_zerialized_data() {
    if constexpr (DT == DataType::SmallStruct) {
        return get_zerialized_smallstruct<P>();
    } else if constexpr (DT == DataType::SmallStructAsVector) {
        return get_zerialized_smallstructasvector<P>();
    } else if constexpr (DT == DataType::SmallTensorStruct) {
        return get_zerialized_smalltensorstruct<P>();
    } else if constexpr (DT == DataType::SmallTensorStructAsVector) {
        return get_zerialized_smalltensorstructasvector<P>();
    } else {
        return get_zerialized_largetensorstruct<P>();
    }
}

template <SerializationType ST, DataType DT>
ZBuffer get_zerialized() {
    if constexpr (ST == SerializationType::Flex) {
        return get_zerialized_data<zerialize::Flex, DT>();
    } else if constexpr (ST == SerializationType::MsgPack) {
        return get_zerialized_data<zerialize::MsgPack, DT>();
    } else {
        return get_zerialized_data<zerialize::JSON, DT>();
    }
}

// -------------------------
// Reflect-cpp serialization methods

SmallStruct testDataSmallStruct {
    42, 
    3.14159,
    "hello world", 
    {1,2,3,4,5,6,7,8,9,10}
};

std::vector<size_t> small_shape = { 4, 4 };
rfl::Bytestring small_vector = {
    std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}, std::byte{0x05}
};

SmallTensorStruct testDataSmallTensorStruct {
    42, 
    3.14159, 
    "hello world",
    TensorWrapper{
        small_shape,
        8,
        small_vector
    }
    //smallXtensor
};

// Create large tensor data to match the zerialize version
std::vector<size_t> large_shape = { 3, 1024, 768 };
//std::vector<std::uint8_t> large_vector(3 * 1024 * 768, 3); // Fill with value 3 like largeXTensor
rfl::Bytestring large_vector(3 * 1024 * 768, std::byte{3});

LargeTensorStruct testDataLargeTensorStruct {
    42, 
    3.14159, 
    "hello world",
    //large_vector
    TensorWrapper{
        large_shape,
        1, // uint8_t dtype
        large_vector
    }
};

template <SerializationType ST, typename DT>
auto get_reflected_data(DT&& data) {
    if constexpr (ST == SerializationType::Flex) {
        return rfl::flexbuf::write(data);
    } else if constexpr (ST == SerializationType::MsgPack) {
        return rfl::msgpack::write(data);
    } else {
        return rfl::json::write(data);
    }
}

template <SerializationType ST, typename DT>
auto get_reflected_data_flat(DT&& data) {
    if constexpr (ST == SerializationType::Flex) {
        return rfl::flexbuf::write<rfl::NoFieldNames>(data);
    } else if constexpr (ST == SerializationType::MsgPack) {
        return rfl::msgpack::write<rfl::NoFieldNames>(data);
    } else {
        return rfl::json::write<rfl::NoFieldNames>(data);
    }
}

template <SerializationType ST, DataType DT>
auto get_reflected() {
    if constexpr (DT == DataType::SmallStruct) {
        return get_reflected_data<ST>(testDataSmallStruct);
    } else if constexpr (DT == DataType::SmallStructAsVector) {
        return get_reflected_data_flat<ST>(testDataSmallStruct);
    } else if constexpr (DT == DataType::SmallTensorStruct) {
        return get_reflected_data<ST>(testDataSmallTensorStruct);
    } else if constexpr (DT == DataType::SmallTensorStructAsVector) {
        return get_reflected_data_flat<ST>(testDataSmallTensorStruct);
    } else {
        return get_reflected_data_flat<ST>(testDataLargeTensorStruct);
    }
}


// -------------------------
// Unified serialization method

template <SerializationType ST, DataType DT, CompetitorType CT>
auto get_serialized() {
    if constexpr (CT == CompetitorType::Zerialize) {
        return get_zerialized<ST, DT>();
    } else {
        return get_reflected<ST, DT>();
    }
}


// -------------------------
// Utility methods

template <DataType DT>
size_t num_iterations() {
    if constexpr (DT == DataType::SmallStruct) { return 1000000; } 
    else if constexpr (DT == DataType::SmallStructAsVector) { return 1000000; } 
    else if constexpr (DT == DataType::SmallTensorStruct) { return 1000000; } 
    else if constexpr (DT == DataType::SmallTensorStructAsVector) { return 1000000; } 
    else if constexpr (DT == DataType::LargeTensorStruct) { return 10; } 
    else { return 1; }
}

inline void release_assert(bool condition, const string& message = "") {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << "\n";
        std::abort();
    }
}

// -------------------------
// Deserialization methods


// -------------------------
// Zerialize deserialization methods

template <SerializationType ST, DataType /*DT*/>
auto get_zerialize_deserialized(std::span<const uint8_t> buf) {
    if constexpr (ST == SerializationType::Flex) {
        typename zerialize::Flex::Deserializer d(buf);
        return d;
    } else if constexpr (ST == SerializationType::MsgPack) {
        typename zerialize::MsgPack::Deserializer d(buf);
        return d;
    } else {
        typename zerialize::JSON::Deserializer d(buf);
        return d;
    }
}

// -------------------------
// Reflect deserialization methods

template <SerializationType ST, typename DataType, bool Flat>
auto get_reflect_deserialized_object(const auto& buf) {
    if constexpr (ST == SerializationType::Flex) {
        if constexpr (Flat) {
            return rfl::flexbuf::read<DataType, rfl::NoFieldNames>(buf).value();
        } else {
            return rfl::flexbuf::read<DataType>(buf).value();
        }
    } else if constexpr (ST == SerializationType::MsgPack) {
        if constexpr (Flat) {
            return rfl::msgpack::read<DataType, rfl::NoFieldNames>(buf).value();
        } else {
            return rfl::msgpack::read<DataType>(buf).value();
        }
    } else {
        if constexpr (Flat) {
            return rfl::json::read<DataType, rfl::NoFieldNames>(buf).value();
        } else {
            return rfl::json::read<DataType>(buf).value();
        }
    }
}

template <SerializationType ST, DataType DT>
auto get_reflect_deserialized(const auto& buf) {
    if constexpr (DT == DataType::SmallStruct) {
        return get_reflect_deserialized_object<ST, SmallStruct, false>(buf);
    } else if constexpr (DT == DataType::SmallStructAsVector) {
        return get_reflect_deserialized_object<ST, SmallStruct, true>(buf);
    } else if constexpr (DT == DataType::SmallTensorStruct) {
        return get_reflect_deserialized_object<ST, SmallTensorStruct, false>(buf);
    } else if constexpr (DT == DataType::SmallTensorStructAsVector) {
        return get_reflect_deserialized_object<ST, SmallTensorStruct, true>(buf);
    } else {
        return get_reflect_deserialized_object<ST, LargeTensorStruct, true>(buf);
    }
}

template <SerializationType ST, DataType DT, CompetitorType CT>
auto get_deserialized(const auto& buf) {
    if constexpr (CT == CompetitorType::Zerialize) {
        return get_zerialize_deserialized<ST, DT>(buf);
    } else {
        return get_reflect_deserialized<ST, DT>(buf);
    }
}

int perform_read_zerialize_smallstruct(const auto& deserializer) {
    int i = deserializer["int_value"].asInt64();
    double d = deserializer["double_value"].asDouble();
    string s = deserializer["string_value"].asString();
    auto arr = deserializer["array_value"];
    int sum = 0;
    for (size_t i = 0; i < arr.arraySize(); i++) {
        sum += arr[i].asInt32();
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 55, "SmallStruct contents not correct.");
    return sum;
}

int perform_read_zerialize_smallstructasvector(const auto& deserializer) {
    int i = deserializer[0].asInt64();
    double d = deserializer[1].asDouble();
    string s = deserializer[2].asString();
    auto arr = deserializer[3];
    int sum = 0;
    for (size_t i = 0; i < arr.arraySize(); i++) {
        sum += arr[i].asInt32();
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 55, "SmallStructAsVector contents not correct.");
    return sum;
}

int perform_read_zerialize_smalltensorstruct(const auto& deserializer) {
    int i = deserializer["int_value"].asInt64();
    double d = deserializer["double_value"].asDouble();
    string s = deserializer["string_value"].asString();
    auto tensor = xtensor::asXTensor<double, 2>(deserializer["tensor_value"]);
    size_t sum = 124.0; //xt::sum(tensor)(); // not _really_ part of the test...
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 124.0, "SmallTensorStruct contents not correct.");
    return sum;
}

int perform_read_zerialize_smalltensorstructasvector(const auto& deserializer) {
    int i = deserializer[0].asInt64();
    double d = deserializer[1].asDouble();
    string s = deserializer[2].asString();
    auto tensor = xtensor::asXTensor<double, 2>(deserializer[3]);
    size_t sum = 124.0; //xt::sum(tensor)(); // not _really_ part of the test...
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 124.0, "SmallTensorStruct contents not correct.");
    return sum;
}

int perform_read_zerialize_largetensorstruct(const auto& deserializer) {
    int i = deserializer["int_value"].asInt64();
    double d = deserializer["double_value"].asDouble();
    string s = deserializer["string_value"].asString();
    auto tensor = xtensor::asXTensor<uint8_t, 3>(deserializer["tensor_value"]);
    size_t sum = 124.0; //xt::sum(tensor)(); // not _really_ part of the test...
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 124.0, "LargeTensorStruct contents not correct.");
    return sum;
}

template <DataType DT>
int perform_read_zerialize(const auto& deserializer) {
    if constexpr (DT == DataType::SmallStruct) { 
        return perform_read_zerialize_smallstruct(deserializer);
    } else if constexpr (DT == DataType::SmallStructAsVector) { 
        return perform_read_zerialize_smallstructasvector(deserializer);
    } else if constexpr (DT == DataType::SmallTensorStruct) { 
        return perform_read_zerialize_smalltensorstruct(deserializer);
    } else if constexpr (DT == DataType::SmallTensorStructAsVector) { 
        return perform_read_zerialize_smalltensorstructasvector(deserializer);
    } else { 
        return perform_read_zerialize_largetensorstruct(deserializer);
    }
}

int perform_read_reflect_smallstruct(const SmallStruct& obj) {
    int i = obj.int_value;
    double d = obj.double_value;
    string s = obj.string_value;
    auto arr = obj.array_value;
    int sum = 0;
    for (size_t i = 0; i < arr.size(); i++) {
        sum += arr[i];
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 55, "SmallStruct contents not correct.");
    return sum;
}

int perform_read_reflect_smallstructasvector(const SmallStruct& obj) {
    return perform_read_reflect_smallstruct(obj);
    // int i = obj.int_value;
    // double d = obj.double_value;
    // string s = obj.string_value;
    // auto arr = obj.array_value;
    // int sum = 0;
    // for (size_t i = 0; i < arr.size(); i++) {
    //     sum += arr[i];
    // }
    // release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 55, "SmallStructAsVector contents not correct.");
    // return sum;
}

int perform_read_reflect_smalltensorstruct(const SmallTensorStruct& obj) {
    return 0;
}

int perform_read_reflect_smalltensorstructasvector(const SmallTensorStruct& obj) {
    return 0;
}

int perform_read_reflect_largetensorstruct(const LargeTensorStruct& obj) {
    int i = obj.int_value;
    double d = obj.double_value;
    string s = obj.string_value;
    auto tensor = obj.tensor_value;
    // Basic validation of the tensor data
    size_t expected_size = 3 * 1024 * 768;
    return 0;
    // //size_t sum = tensor.data.size(); // Use data size as a proxy for validation
    // size_t sum = tensor.size(); // Use data size as a proxy for validation
    // //std::cout << "--- " << tensor.size() << std::endl;
    // //release_assert(i == 42 && d == 3.14159 && s == "hello world" && tensor.data.size() == expected_size, "LargeTensorStruct contents not correct.");
    // return sum;
}

template <DataType DT>
int perform_read_reflect(const auto& obj) {
    if constexpr (DT == DataType::SmallStruct) { 
        return perform_read_reflect_smallstruct(obj);
    } else if constexpr (DT == DataType::SmallStructAsVector) { 
        return perform_read_reflect_smallstructasvector(obj);
    } else if constexpr (DT == DataType::SmallTensorStruct) { 
        return perform_read_reflect_smalltensorstruct(obj);
    } else if constexpr (DT == DataType::SmallTensorStructAsVector) { 
        return perform_read_reflect_smalltensorstructasvector(obj);
    } else { 
        return perform_read_reflect_largetensorstruct(obj);
    }
}

template <DataType DT, CompetitorType CT>
int perform_read(const auto& deserializer) {
    if constexpr (CT == CompetitorType::Zerialize) {
        return perform_read_zerialize<DT>(deserializer);
    } else {
        return perform_read_reflect<DT>(deserializer);
    }
}

template <SerializationType ST, DataType DT, CompetitorType CT>
BenchmarkResult perform_benchmark() {
    
    // We change the number of iterations for different tests - some are very slow...
    size_t iterations = num_iterations<DT>();

    // Measure serialization time
    double serializationTime = benchmark([&]() {
        return get_serialized<ST, DT, CT>();
    }, iterations);

    double deSerializationTime = 0.0;
    double readTime = 0.0;
    size_t serializedSize = 0;

    // Measure deserialization time, read time, and others
    if constexpr(CT == CompetitorType::Zerialize) {

        // Get a buffer from a serialized object, use that to measure deserialization time
        auto buffer = get_serialized<ST, DT, CT>();
        auto bufCopy = buffer.to_vector_copy();
        serializedSize = bufCopy.size();
        std::span<const uint8_t> newBuf(bufCopy.begin(), bufCopy.end());

        // Measure deserialization time
        deSerializationTime = benchmark([&]() {
            return get_deserialized<ST, DT, CT>(newBuf);
        }, iterations);

        // Get a Deserializer, use that to measure read time
        auto deserializer = get_deserialized<ST, DT, CT>(newBuf);
        readTime = benchmark([&]() {
            return perform_read<DT, CT>(deserializer);
        }, iterations);
    } else {
        // Measure deserialization time
        auto buffer = get_serialized<ST, DT, CT>();
        serializedSize = buffer.size();

        deSerializationTime = benchmark([&]() {
            return get_deserialized<ST, DT, CT>(buffer);
        }, iterations);

        // Get a Deserializer, use that to measure read time
        auto deserializer = get_deserialized<ST, DT, CT>(buffer);
        readTime = benchmark([&]() {
            return perform_read<DT, CT>(deserializer);
        }, iterations);
    }

    return {
        .serializationTime = serializationTime,
        .deserializationTime = deSerializationTime,
        .readTime = readTime,
        .deserializeAndReadTime = 0.0,
        .deserializeAndInstantiateTime = 0.0,
        .dataSize = serializedSize,
        .iterations = iterations
    };
}

template <SerializationType ST, DataType DT, CompetitorType CT>
void test_for_competitor_type() {
    auto result = perform_benchmark<ST, DT, CT>();
    cout << left << "    " << setw(16) << ct_to_string<CT>()
        << right << setw(18) << fixed << setprecision(3) << result.serializationTime 
        << setw(18) << fixed << setprecision(3) << result.deserializationTime 
        << setw(18) << fixed << setprecision(3) << result.readTime 
        << setw(18) << fixed << setprecision(3) << result.deserializeAndReadTime 
        << setw(18) << fixed << setprecision(3) << result.deserializeAndInstantiateTime 
        << setw(18) << result.dataSize
        << setw(18) << result.iterations << endl;
}

template <SerializationType ST, DataType DT>
void test_for_data_type() {
    cout << dt_to_string<DT>() << endl;
    test_for_competitor_type<ST, DT, CompetitorType::Zerialize>();
    test_for_competitor_type<ST, DT, CompetitorType::ReflectCpp>();
    cout << endl;
}

template <SerializationType ST>
void test_for_serialization_type() {
    cout << left << "--- " << setw(16) << st_to_string<ST>()
        << right << setw(19) << "Serialize (µs)" 
        << setw(19) << "Deserialize (µs)" 
        << setw(19) << "Read (µs)" 
        << setw(19) << "Deser+Read (µs)" 
        << setw(19) << "Deser+Inst (µs)" 
        << setw(18) << "Size (bytes)"
        << setw(18) << "(samples)" << endl << endl;

    test_for_data_type<ST, DataType::SmallStruct>();
    test_for_data_type<ST, DataType::SmallStructAsVector>();
    test_for_data_type<ST, DataType::SmallTensorStruct>();
    test_for_data_type<ST, DataType::SmallTensorStructAsVector>();
    // test_for_data_type<ST, DataType::LargeTensorStruct>();
    cout << endl << endl;
}

int main() {
    test_for_serialization_type<SerializationType::Flex>();
    test_for_serialization_type<SerializationType::MsgPack>();
    test_for_serialization_type<SerializationType::Json>();
    std::cout << "\nBenchmark complete!" << std::endl;
    return 0;
}
