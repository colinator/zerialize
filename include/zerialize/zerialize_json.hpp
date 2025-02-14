#pragma once

#include <zerialize/zerialize.hpp>
#include <nlohmann/json.hpp>
#include <any>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <span>
#include <sstream>

namespace zerialize {

class JsonBuffer : public DataBuffer<JsonBuffer> {
private:
    std::vector<uint8_t> buf_;
    nlohmann::json json_;

public:
    nlohmann::json& json() { return json_; }

    JsonBuffer() {}

    JsonBuffer(const nlohmann::json& j): json_(j) {}

    // Zero-copy view of existing data
    JsonBuffer(std::span<const uint8_t> data)
        : buf_(data.begin(), data.end()),
          json_(nlohmann::json::parse(data.begin(), data.end())) { }

    // Zero-copy move of vector ownership
    JsonBuffer(std::vector<uint8_t>&& buf)
        : buf_(std::move(buf)),
          json_(nlohmann::json::parse(buf_)) { }

    // Must copy for const reference
    JsonBuffer(const std::vector<uint8_t>& buf)
        : buf_(buf),
          json_(nlohmann::json::parse(buf_)) { }

    void finish() {
        std::string str = json_.dump();
        buf_ = std::vector<uint8_t>(str.begin(), str.end());
    }

    const std::vector<uint8_t>& buf() const override {
        return buf_;
    }

    std::string to_string() const override {
        return "JsonBuffer " + std::to_string(buf().size()) + 
            " bytes at: " + std::format("{}", static_cast<const void*>(buf_.data())) +
            " : " + json_.dump() + "\n" + debug_string(*this);
    }

    bool isInt() const { return json_.is_number_integer(); }
    bool isUInt() const { return json_.is_number_unsigned(); }
    bool isFloat() const { return json_.is_number_float(); }
    bool isBool() const { return json_.is_boolean(); }
    bool isString() const { return json_.is_string(); }
    bool isBlob() const { return false; } //json_.is_string(); }
    bool isMap() const { return json_.is_object(); }
    bool isArray() const { return json_.is_array(); }

    int8_t asInt8() const {
        if (!isInt()) { throw DeserializationError("not an int"); }
        return json_.get<int8_t>();
    }

    int16_t asInt16() const {
        if (!isInt()) { throw DeserializationError("not an int"); }
        return json_.get<int16_t>();
    }
    
    int32_t asInt32() const { 
        if (!isInt()) { throw DeserializationError("not an int"); }
        return json_.get<int32_t>(); 
    }

    int64_t asInt64() const { 
        if (!isInt()) { throw DeserializationError("not an int"); }
        return json_.get<int64_t>(); 
    }

    uint8_t asUInt8() const {
        if (!isUInt()) { throw DeserializationError("not a uint"); }
        return json_.get<uint8_t>();
    }

    uint16_t asUInt16() const {
        if (!isUInt()) { throw DeserializationError("not a uint"); }
        return json_.get<uint16_t>();
    }

    uint32_t asUInt32() const {
        if (!isUInt()) { throw DeserializationError("not a uint"); }
        return json_.get<uint32_t>();
    }

    uint64_t asUInt64() const {
        if (!isUInt()) { throw DeserializationError("not a uint"); }
        return json_.get<uint64_t>();
    }

    float asFloat() const {
        if (!isFloat()) { throw DeserializationError("not a float"); }
        return json_.get<float>();
    }

    double asDouble() const {
        if (!isFloat()) { throw DeserializationError("not a float"); }
        return json_.get<double>(); 
    }

    bool asBool() const {
        if (!isBool()) { throw DeserializationError("not a bool"); }
        return json_.get<bool>();
    }
    
    std::string_view asString() const { 
        if (!isString()) { throw DeserializationError("not a string"); }
        return json_.get<std::string_view>(); 
    }

    DataBlob asBlob() const { 
        if (!isBlob()) { throw DeserializationError("not a blob"); }
        return DataBlob{ .data = nullptr, .size = 0 };
    }

    std::vector<std::string_view> mapKeys() const {
        if (!isMap()) { throw DeserializationError("not a map"); }
        std::vector<std::string_view> keys;
        for (const auto& it : json_.items()) {
            keys.push_back(it.key());
        }
        return keys;
    }

    JsonBuffer operator[] (const std::string& key) const { 
        if (!isMap()) { throw DeserializationError("not a map"); }
        return JsonBuffer(json_[key]); 
    }

    size_t arraySize() const {
        if (!isArray()) { throw DeserializationError("not an array"); }
        return json_.size();
    }
    
    JsonBuffer operator[] (size_t index) const { 
        if (!isArray()) { throw DeserializationError("not an array"); }
        return JsonBuffer(json_[index]); 
    }
};

class Json {
private:
    void serializeValue(nlohmann::json& j, const char* val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, int8_t val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, int16_t val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, int32_t val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, int64_t val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, uint8_t val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, uint16_t val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, uint32_t val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, uint64_t val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, bool val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, float val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, double val) {
        j = val;
    }

    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T, std::string_view>>>
    void serializeValue(nlohmann::json& j, T&& val) {
        j = std::string(val);
    }

    template<typename T>
    void serializeValue(nlohmann::json& j, std::pair<const char*, T>&& keyVal) {
        j[keyVal.first] = nullptr;
        serializeValue(j[keyVal.first], std::forward<T>(keyVal.second));
    }

    void serializeValue(nlohmann::json& j, std::vector<std::any>&& val) {
        j = nlohmann::json::array();
        for (const std::any& v: val) {
            j.push_back(nullptr);
            serializeValue(j.back(), v);
        }
    }

    void serializeValue(nlohmann::json& j, const std::any& val) {
        if (val.type() == typeid(std::map<std::string, std::any>)) {
            std::map<std::string, std::any> m = std::any_cast<std::map<std::string, std::any>>(val);
            j = nlohmann::json::object();
            for (const auto& [key, value]: m) {
                j[key] = nullptr;
                serializeValue(j[key], value);
            }
        } else if (val.type() == typeid(std::vector<std::any>)) {
            std::vector<std::any> l = std::any_cast<std::vector<std::any>>(val);
            j = nlohmann::json::array();
            for (const std::any& v: l) {
                j.push_back(nullptr);
                serializeValue(j.back(), v);
            }
        } else if (val.type() == typeid(int8_t)) {
            serializeValue(j, std::any_cast<int8_t>(val));
        } else if (val.type() == typeid(int16_t)) {
            serializeValue(j, std::any_cast<int16_t>(val));
        } else if (val.type() == typeid(int32_t)) {
            serializeValue(j, std::any_cast<int32_t>(val));
        } else if (val.type() == typeid(int64_t)) {
            serializeValue(j, std::any_cast<int64_t>(val));
        } else if (val.type() == typeid(uint8_t)) {
            serializeValue(j, std::any_cast<uint8_t>(val));
        } else if (val.type() == typeid(uint16_t)) {
            serializeValue(j, std::any_cast<uint16_t>(val));
        } else if (val.type() == typeid(uint32_t)) {
            serializeValue(j, std::any_cast<uint32_t>(val));
        } else if (val.type() == typeid(uint64_t)) {
            serializeValue(j, std::any_cast<uint64_t>(val));
        } else if (val.type() == typeid(bool)) {
            serializeValue(j, std::any_cast<bool>(val));
        } else if (val.type() == typeid(double)) {
            serializeValue(j, std::any_cast<double>(val));
        } else if (val.type() == typeid(float)) {
            serializeValue(j, std::any_cast<float>(val));
        } else if (val.type() == typeid(const char*)) {
            serializeValue(j, std::any_cast<const char*>(val));
        } else if (val.type() == typeid(std::string)) {
            serializeValue(j, std::any_cast<std::string>(val));
        } else {
            throw std::runtime_error("Unsupported type in std::any");
        }
    }

    // Used for format conversion, from any other Deserializable.
    template<Deserializable SourceBufferType>
    void serializeValue(nlohmann::json& j, const SourceBufferType& value) {
        if (value.isMap()) {
            j = nlohmann::json::object();
            for (string_view key: value.mapKeys()) {
                // Copies the key. Lame if you ask me...
                const std::string s(key);
                j[s] = nullptr;
                serializeValue(j[s], value[s]);
            }
        } else if (value.isArray()) {
            j = nlohmann::json::array();
            for (size_t i=0; i<value.arraySize(); i++) {
                j.push_back(nullptr);
                serializeValue(j.back(),  value[i]);
            }
        } else if (value.isInt()) {
            serializeValue(j, value.asInt64());
        } else if (value.isUInt()) {
            serializeValue(j, value.asUInt64());
        }  else if (value.isFloat()) {
            serializeValue(j, value.asDouble());
        } else if (value.isBool()) {
            serializeValue(j, value.asBool());
        } else if (value.isString()) {
            serializeValue(j, value.asString());
        } else if (value.isBlob()) {
            //serializeValue(j, value.asBlob());
        } else {
            throw std::runtime_error("Unsupported source buffer value type");
        }
    }

public:
    using BufferType = JsonBuffer;

    Json() {}

    BufferType serialize() {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "Json::serialize()" << std::endl;
        }
        JsonBuffer buffer;
        buffer.json() = nlohmann::json::array();
        buffer.finish();
        return buffer;
    }

    BufferType serialize(const std::any& value) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "Json::serialize(value)" << std::endl;
        }
        JsonBuffer buffer;
        serializeValue(buffer.json(), value);
        buffer.finish();
        return buffer;
    }

    BufferType serialize(std::initializer_list<std::any> list) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "Json::serialize(initializer_list)" << std::endl;
        }
        JsonBuffer buffer;
        buffer.json() = nlohmann::json::array();
        for (const std::any& val : list) {
            buffer.json().push_back(nullptr);
            serializeValue(buffer.json().back(), val);
        }
        buffer.finish();
        return buffer;
    }

    BufferType serialize(std::initializer_list<std::pair<std::string, std::any>> list) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "Json::serialize(initializer list/map)" << std::endl;
        }
        JsonBuffer buffer;
        buffer.json() = nlohmann::json::object();
        for (const auto& [key, val] : list) {
            buffer.json()[key] = nullptr;
            serializeValue(buffer.json()[key], val);
        }
        buffer.finish();
        return buffer;
    }

    template<typename ValueType>
    BufferType serialize(ValueType&& value) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "Json::serialize(&&value)" << std::endl;
        }   
        JsonBuffer buffer;
        serializeValue(buffer.json(), std::forward<ValueType>(value));
        buffer.finish();
        return buffer;
    }

    template<typename... ValueTypes>
    BufferType serialize(ValueTypes&&... values) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "Json::serialize(&&values)" << std::endl;
        }
        JsonBuffer buffer;
        buffer.json() = nlohmann::json::array();
        (serializeValue(buffer.json().emplace_back(), std::forward<ValueTypes>(values)), ...);
        buffer.finish();
        return buffer;
    }

    template<typename... ValueTypes>
    BufferType serializeMap(ValueTypes&&... values) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "Json::serializeMap(&&values)" << std::endl;
        }
        JsonBuffer buffer;
        buffer.json() = nlohmann::json::object();
        ([&](auto&& pair) {
            buffer.json()[pair.first] = nullptr;
            serializeValue(buffer.json()[pair.first], std::forward<decltype(pair.second)>(pair.second));
        }(std::forward<ValueTypes>(values)), ...);
        buffer.finish();
        return buffer;
    }

    // A special serialize method that takes another buffer type.
    // Used for format conversion.
    template<Deserializable SourceBufferType>
    BufferType serialize(const SourceBufferType& value) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "Json::serialize(Deserializable &&value)" << std::endl;
        }   
        JsonBuffer buffer;
        serializeValue(buffer.json(), value);
        buffer.finish();
        return buffer;
    }
};

template<>
struct SerializerName<Json> {
    static constexpr const char* value = "JSON";
};

} // namespace zerialize



// Verified:
// {"a":3,"b":5.2,"c":"asdf","d":[7,8.2,{"e":2.613,"pi":3.14159}],"k":1028}
// JsonBuffer 72 bytes at: 0x13ae070a0
