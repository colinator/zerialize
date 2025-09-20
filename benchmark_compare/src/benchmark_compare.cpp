#include <iostream>
#include <string>
#include <iomanip>
#include <chrono>

#include <zerialize/zerialize.hpp>
#include <zerialize/protocols/flex.hpp>
#include <zerialize/protocols/msgpack.hpp>
#include <zerialize/protocols/json.hpp>
#include <zerialize/protocols/cbor.hpp>
#include <zerialize/tensor/xtensor.hpp>

#include <xtensor/core/xmath.hpp>

// Reflect-cpp includes
#include <rfl/json.hpp>
#include <rfl/flexbuf.hpp>
#include <rfl/msgpack.hpp>
#include <rfl/cbor.hpp>
#include <rfl.hpp>

using namespace zerialize;
using namespace std::chrono;

using std::cout, std::endl, std::string;
using std::setw, std::setprecision, std::right, std::left, std::fixed;


template <typename Buffer>
void print_bytes(const Buffer& buf) {
    if constexpr (std::is_same_v<Buffer, std::string>) {
        std::cout << buf << std::endl;
    } else {
        for (uint8_t b : buf) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(b) << " ";
        }
        std::cout << std::dec << std::endl;
    }
}

template <typename T, int N>
inline rfl::Bytestring to_bytestring(const xt::xtensor<T, N>& x)
{
    const std::byte* p   = reinterpret_cast<const std::byte*>(x.data());
    const std::size_t n  = x.size() * sizeof(T);

    // Fast path if Bytestring has a range constructor (it usually does):
    return rfl::Bytestring(p, p + n);

    // // If your Bytestring lacks that constructor, use this instead:
    // rfl::Bytestring out(n);
    // std::memcpy(out.data(), p, n);
    // return out;
}

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
// ???
// -------------------------


// -------------------------
// Test variation combinations

// Test across these serialization types
enum class SerializationType {
    Flex,
    MsgPack,
    Json,
    CBOR
};

// Test across these data variations
enum class DataType {
    SmallStruct,
    SmallStructAsVector,
    SmallTensorStruct,
    SmallTensorStructAsVector,
    MediumTensorStruct,
    MediumTensorStructAsVector,
    LargeTensorStruct
};

// Test across these competitors
enum class CompetitorType {
    Zerialize,
    ReflectCpp
};


// Tensor tooling for reflect
using TensorWrapper = rfl::Tuple<int, std::vector<size_t>, rfl::Bytestring>;

template <typename T>
auto tensorFromWrapper(const TensorWrapper& tw) {
    //const int dataType = tw.get<0>();
    const auto& sizes = tw.get<1>();
    const auto& blob = tw.get<2>();

    const T* data_typed = reinterpret_cast<const T*>(blob.data());
    const std::size_t tensor_size = blob.size() / sizeof(T);

    return xt::adapt(
        data_typed,
        tensor_size,
        xt::no_ownership(),
        sizes
    );
}


// -------------------------
// the message types we are testing

struct SmallStruct {
    int int_value;
    double double_value;
    std::string string_value;
    std::array<int, 10> array_value;
};

struct SmallTensorStruct {
    int int_value;
    double double_value;
    std::string string_value;
    std::vector<int> array_value;
    TensorWrapper tensor_value; // Small 4x4 tensor
};

struct MediumTensorStruct {
    int int_value;
    double double_value;
    std::string string_value;
    std::vector<int> array_value;
    TensorWrapper tensor_value; // Medium 10x2048 tensor
};

struct LargeTensorStruct {
    int int_value;
    double double_value;
    std::string string_value;
    std::vector<int> array_value;
    TensorWrapper tensor_value; // Large 3x1024x768 tensor
};


// -------------------------
// to string utility methods

template <SerializationType ST>
constexpr string st_to_string() {
    if constexpr (ST == SerializationType::Flex) { return "Flex"; } 
    else if constexpr (ST == SerializationType::MsgPack) { return "MsgPack"; }
    else if constexpr (ST == SerializationType::CBOR) { return "CBOR"; } 
    else { return "Json"; }
}

template <DataType DT>
constexpr string dt_to_string() {
    if constexpr (DT == DataType::SmallStruct) { return "SmallStruct"; } 
    else if constexpr (DT == DataType::SmallStructAsVector) { return "SmallStructAsVector"; } 
    else if constexpr (DT == DataType::SmallTensorStruct) { return "SmallTensorStruct 4x4 double";} 
    else if constexpr (DT == DataType::SmallTensorStructAsVector) { return "SmallTensorStructAsVector 4x4 double"; }
    else if constexpr (DT == DataType::MediumTensorStruct) { return "MediumTensorStruct 10x2048 float";} 
    else if constexpr (DT == DataType::MediumTensorStructAsVector) { return "MediumTensorStructAsVector 10x2048 float"; } 
    else if constexpr (DT == DataType::LargeTensorStruct) { return "LargeTensorStruct 3x1024x768 uint8"; } 
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
const size_t MediumN = 10;
xt::xtensor<float, 2> mediumXtensor({MediumN, 2048}, 3);
xt::xtensor<uint8_t, 3> largeXTensor({3, 1024, 768}, 3);


// -------------------------
// Zerialize serialization methods (DSL: serialize/zmap/zvec)

template <typename P>
ZBuffer get_zerialized_smallstruct() {
    return serialize<P>(
        zmap<"int_value","double_value","string_value","array_value">(
            42, 3.14159, "hello world", smallArray
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
        zmap<"int_value","double_value","string_value","array_value","tensor_value">(
            42, 3.14159, "hello world", smallArray, smallXtensor
        )
    );
}

template <typename P>
ZBuffer get_zerialized_smalltensorstructasvector() {
    return serialize<P>(
        zvec(42, 3.14159, "hello world", smallArray, smallXtensor)
    );
}

template <typename P>
ZBuffer get_zerialized_mediumtensorstruct() {
    return serialize<P>(
        zmap<"int_value","double_value","string_value","array_value","tensor_value">(
            42, 3.14159, "hello world", smallArray, mediumXtensor
        )
    );
}

template <typename P>
ZBuffer get_zerialized_mediumtensorstructasvector() {
    return serialize<P>(
        zvec(42, 3.14159, "hello world", smallArray, mediumXtensor)
    );
}

template <typename P>
ZBuffer get_zerialized_largetensorstruct() {
    return serialize<P>(
        zmap<"int_value","double_value","string_value","array_value","tensor_value">(
            42, 3.14159, "hello world", smallArray, largeXTensor
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
    } else if constexpr (DT == DataType::MediumTensorStruct) {
        return get_zerialized_mediumtensorstruct<P>();
    } else if constexpr (DT == DataType::MediumTensorStructAsVector) {
        return get_zerialized_mediumtensorstructasvector<P>();
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
    } else if constexpr (ST == SerializationType::CBOR) {
        return get_zerialized_data<zerialize::CBOR, DT>();
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
    smallArray
};

SmallTensorStruct testDataSmallTensorStruct {
    42, 
    3.14159, 
    "hello world",
    {1,2,3,4,5,6,7,8,9,10},
    TensorWrapper{
        11,
        { 4, 4 },
        to_bytestring<double, 2>(smallXtensor)
    }
};

MediumTensorStruct testDataMediumTensorStruct {
    42, 
    3.14159, 
    "hello world",
    {1,2,3,4,5,6,7,8,9,10},
    TensorWrapper{
        10,
        { MediumN, 2048 },
        to_bytestring<float, 2>(mediumXtensor)
    }
};


// Create large tensor data to match the zerialize version
// std::vector<size_t> large_shape = { 3, 1024, 768 };
// rfl::Bytestring large_vector = to_bytestring<uint8_t, 3>(largeXTensor);

LargeTensorStruct testDataLargeTensorStruct {
    42, 
    3.14159, 
    "hello world",
    {1,2,3,4,5,6,7,8,9,10},
    TensorWrapper{
        4, // uint8_t dtype
        { 3, 1024, 768 }, //large_shape,
        to_bytestring<uint8_t, 3>(largeXTensor) //large_vector
    }
};

template <SerializationType ST, typename DT>
auto get_reflected_data(DT&& data) {
    if constexpr (ST == SerializationType::Flex) {
        return rfl::flexbuf::write(data);
    } else if constexpr (ST == SerializationType::MsgPack) {
        return rfl::msgpack::write(data);
    } else if constexpr (ST == SerializationType::CBOR) {
        return rfl::cbor::write(data);
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
    } else if constexpr (ST == SerializationType::CBOR) {
        return rfl::cbor::write<rfl::NoFieldNames>(data);
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
    } else if constexpr (DT == DataType::MediumTensorStruct) {
        return get_reflected_data<ST>(testDataMediumTensorStruct);
    } else if constexpr (DT == DataType::MediumTensorStructAsVector) {
        return get_reflected_data_flat<ST>(testDataMediumTensorStruct);
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
    if constexpr (DT == DataType::SmallStruct) { return                     1000000; } 
    else if constexpr (DT == DataType::SmallStructAsVector) { return        1000000; } 
    else if constexpr (DT == DataType::SmallTensorStruct) { return          1000000; } 
    else if constexpr (DT == DataType::SmallTensorStructAsVector) { return  1000000; } 
    else if constexpr (DT == DataType::MediumTensorStruct) { return          100000; } 
    else if constexpr (DT == DataType::MediumTensorStructAsVector) { return  100000; } 
    else if constexpr (DT == DataType::LargeTensorStruct) { return            10000; } 
    else { return 1; }
}

// template <DataType DT>
// size_t num_iterations() {
//     if constexpr (DT == DataType::SmallStruct) { return 1; } 
//     else if constexpr (DT == DataType::SmallStructAsVector) { return 1; } 
//     else if constexpr (DT == DataType::SmallTensorStruct) { return 1; } 
//     else if constexpr (DT == DataType::SmallTensorStructAsVector) { return 1; } 
//     else if constexpr (DT == DataType::LargeTensorStruct) { return 1; } 
//     else { return 1; }
// }


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

template <SerializationType ST, DataType>
auto get_zerialize_deserialized(std::span<const uint8_t> buf) {
    if constexpr (ST == SerializationType::Flex) {
        typename zerialize::Flex::Deserializer d(buf);
        return d;
    } else if constexpr (ST == SerializationType::MsgPack) {
        typename zerialize::MsgPack::Deserializer d(buf);
        return d;
    } else if constexpr (ST == SerializationType::CBOR) {
        typename zerialize::CBOR::Deserializer d(buf);
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
    } else if constexpr (ST == SerializationType::CBOR) {
        if constexpr (Flat) {
            return rfl::cbor::read<DataType, rfl::NoFieldNames>(buf).value();
        } else {
            return rfl::cbor::read<DataType>(buf).value();
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
    } else if constexpr (DT == DataType::MediumTensorStruct) {
        return get_reflect_deserialized_object<ST, MediumTensorStruct, false>(buf);
    } else if constexpr (DT == DataType::MediumTensorStructAsVector) {
        return get_reflect_deserialized_object<ST, MediumTensorStruct, true>(buf);
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
    auto arr = deserializer["array_value"];
    auto tensor = xtensor::asXTensor<double, 2>(deserializer["tensor_value"]);
    size_t sum = xt::sum(tensor)();
    for (size_t i = 0; i < arr.arraySize(); i++) {
        sum += arr[i].asInt32();
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 179, "SmallTensorStruct contents not correct.");
    return sum;
}

int perform_read_zerialize_smalltensorstructasvector(const auto& deserializer) {
    int i = deserializer[0].asInt64();
    double d = deserializer[1].asDouble();
    string s = deserializer[2].asString();
    auto arr = deserializer[3];
    auto tensor = xtensor::asXTensor<double, 2>(deserializer[4]);
    size_t sum = xt::sum(tensor)();
    for (size_t i = 0; i < arr.arraySize(); i++) {
        sum += arr[i].asInt32();
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 179, "SmallTensorStruct contents not correct.");
    return sum;
}

int perform_read_zerialize_mediumtensorstruct(const auto& deserializer) {
    int i = deserializer["int_value"].asInt64();
    double d = deserializer["double_value"].asDouble();
    string s = deserializer["string_value"].asString();
    auto arr = deserializer["array_value"];
    auto tensor = xtensor::asXTensor<float, 2>(deserializer["tensor_value"]);
    size_t sum = xt::sum(tensor)();
    for (size_t i = 0; i < arr.arraySize(); i++) {
        sum += arr[i].asInt32();
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 55 + MediumN*2048*3, "MediumTensorStruct contents not correct.");
    return sum;
}

int perform_read_zerialize_mediumtensorstructasvector(const auto& deserializer) {
    int i = deserializer[0].asInt64();
    double d = deserializer[1].asDouble();
    string s = deserializer[2].asString();
    auto arr = deserializer[3];
    auto tensor = xtensor::asXTensor<float, 2>(deserializer[4]);
    size_t sum = xt::sum(tensor)();
    for (size_t i = 0; i < arr.arraySize(); i++) {
        sum += arr[i].asInt32();
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 55 + MediumN*2048*3, "MediumTensorStruct contents not correct.");
    return sum;
}

int perform_read_zerialize_largetensorstruct(const auto& deserializer) {
    int i = deserializer["int_value"].asInt64();
    double d = deserializer["double_value"].asDouble();
    string s = deserializer["string_value"].asString();
    auto arr = deserializer["array_value"];
    auto tensor = xtensor::asXTensor<uint8_t, 3>(deserializer["tensor_value"]);
    size_t sum = xt::sum(tensor)();
    for (size_t i = 0; i < arr.arraySize(); i++) {
        sum += arr[i].asInt32();
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 55 + 3*1024*768*3, "LargeTensorStruct contents not correct.");
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
    } else if constexpr (DT == DataType::MediumTensorStruct) { 
        return perform_read_zerialize_mediumtensorstruct(deserializer);
    } else if constexpr (DT == DataType::MediumTensorStructAsVector) { 
        return perform_read_zerialize_mediumtensorstructasvector(deserializer);
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
}

int perform_read_reflect_smalltensorstruct(const SmallTensorStruct& obj) {
    int i = obj.int_value;
    double d = obj.double_value;
    string s = obj.string_value;
    auto arr = obj.array_value;
    auto tensor = obj.tensor_value;
    auto actualTensor = tensorFromWrapper<double>(tensor); 
    size_t sum = xt::sum(actualTensor)();
    for (size_t i = 0; i < arr.size(); i++) {
        sum += arr[i];
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 179, "SmallStruct contents not correct.");
    return sum;
}

int perform_read_reflect_smalltensorstructasvector(const SmallTensorStruct& obj) {
    return perform_read_reflect_smalltensorstruct(obj);
}

int perform_read_reflect_mediumtensorstruct(const MediumTensorStruct& obj) {
    int i = obj.int_value;
    double d = obj.double_value;
    string s = obj.string_value;
    auto arr = obj.array_value;
    auto tensor = obj.tensor_value;
    auto actualTensor = tensorFromWrapper<float>(tensor); 
    size_t sum = xt::sum(actualTensor)();
    for (size_t i = 0; i < arr.size(); i++) {
        sum += arr[i];
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 55 + MediumN*2048*3, "MediumStruct contents not correct.");
    return sum;
}

int perform_read_reflect_mediumtensorstructasvector(const MediumTensorStruct& obj) {
    return perform_read_reflect_mediumtensorstruct(obj);
}

int perform_read_reflect_largetensorstruct(const LargeTensorStruct& obj) {
    int i = obj.int_value;
    double d = obj.double_value;
    string s = obj.string_value;
    auto arr = obj.array_value;
    auto tensor = obj.tensor_value;
    auto actualTensor = tensorFromWrapper<uint8_t>(tensor);
    size_t sum = xt::sum(actualTensor)();
    for (size_t i = 0; i < arr.size(); i++) {
        sum += arr[i];
    }
    release_assert(i == 42 && d == 3.14159 && s == "hello world" && sum == 55 + 3*1024*768*3, "LargeTensorStruct contents not correct.");
    return sum;
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
    } else if constexpr (DT == DataType::MediumTensorStruct) { 
        return perform_read_reflect_mediumtensorstruct(obj);
    } else if constexpr (DT == DataType::MediumTensorStructAsVector) { 
        return perform_read_reflect_mediumtensorstructasvector(obj);
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
        .deserializeAndReadTime = deSerializationTime + readTime,
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
        // << setw(18) << fixed << setprecision(3) << result.deserializeAndInstantiateTime 
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
        // << setw(19) << "Deser+Inst (µs)" 
        << setw(18) << "Size (bytes)"
        << setw(18) << "(samples)" << endl << endl;

    test_for_data_type<ST, DataType::SmallStruct>();
    test_for_data_type<ST, DataType::SmallStructAsVector>();
    if constexpr (ST != SerializationType::Json) {
        // We support binaries in json (via base64 encoding),
        // but reflectcpp does not. So just don't even compare.
        test_for_data_type<ST, DataType::SmallTensorStruct>();
        test_for_data_type<ST, DataType::SmallTensorStructAsVector>();
        test_for_data_type<ST, DataType::MediumTensorStruct>();
        test_for_data_type<ST, DataType::MediumTensorStructAsVector>();
        test_for_data_type<ST, DataType::LargeTensorStruct>();
    }
    cout << endl << endl;
}

int main() {
    std::cout << "Serialize:    produce bytes" << std::endl;
    std::cout << "Deserialize:  consume bytes" << std::endl;
    std::cout << "Read:         read and check every value from pre-deserialized, compute tensor element sums" << std::endl;
    std::cout << "Deser+Read:   deserialize, then read" << std::endl;
    std::cout << std::endl;
    test_for_serialization_type<SerializationType::Json>();
    test_for_serialization_type<SerializationType::Flex>();
    test_for_serialization_type<SerializationType::MsgPack>();
    test_for_serialization_type<SerializationType::CBOR>();
    std::cout << "\nBenchmark complete!" << std::endl;
    return 0;
}
