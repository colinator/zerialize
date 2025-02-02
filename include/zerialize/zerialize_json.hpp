#pragma once

#include <zerialize/zerialize.hpp>
#include <nlohmann/json.hpp>
#include <any>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <span>

namespace zerialize {

class JsonBuffer : public Buffer {
private:
    std::vector<uint8_t> buf_;
    nlohmann::json json_;
public:
    nlohmann::json& json() { return json_; }

    JsonBuffer() {}

    JsonBuffer(const nlohmann::json& j): json_(j) {}

    // Zero-copy view of existing data
    JsonBuffer(std::span<const uint8_t> data) {
        json_ = nlohmann::json::parse(data.begin(), data.end());
    }

    // Zero-copy move of vector ownership
    JsonBuffer(std::vector<uint8_t>&& buf)
        : buf_(std::move(buf)) {
        json_ = nlohmann::json::parse(buf_);
    }

    // Must copy for const reference
    JsonBuffer(const std::vector<uint8_t>& buf)
        : buf_(buf) {
        json_ = nlohmann::json::parse(buf_);
    }

    void finish() {
        std::string str = json_.dump();
        buf_ = std::vector<uint8_t>(str.begin(), str.end());
    }

    std::string to_string() const override {
        return json_.dump();
    }

    const std::vector<uint8_t>& buf() const override {
        return buf_;
    }

    int32_t asInt32() const { 
        return json_.get<int32_t>();
    }

    double asDouble() const { 
        return json_.get<double>();
    }

    std::string_view asString() const { 
        return json_.get<std::string_view>();
    }

    JsonBuffer operator[] (const std::string& key) const { 
        return JsonBuffer(json_[key]);
    }

    JsonBuffer operator[] (size_t index) const { 
        return JsonBuffer(json_[index]);
    }
};

class Json {
private:
    void serializeValue(nlohmann::json& j, const char* val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, int val) {
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
        if (val.type() == typeid(int)) {
            j = std::any_cast<int>(val);
        } else if (val.type() == typeid(double)) {
            j = std::any_cast<double>(val);
        } else if (val.type() == typeid(float)) {
            j = std::any_cast<float>(val);
        } else if (val.type() == typeid(const char*)) {
            j = std::any_cast<const char*>(val);
        } else if (val.type() == typeid(std::string)) {
            j = std::any_cast<std::string>(val);
        } else if (val.type() == typeid(std::vector<std::any>)) {
            std::vector<std::any> l = std::any_cast<std::vector<std::any>>(val);
            j = nlohmann::json::array();
            for (const std::any& v: l) {
                j.push_back(nullptr);
                serializeValue(j.back(), v);
            }
        } else if (val.type() == typeid(std::map<std::string, std::any>)) {
            std::map<std::string, std::any> m = std::any_cast<std::map<std::string, std::any>>(val);
            j = nlohmann::json::object();
            for (const auto& [key, value]: m) {
                j[key] = nullptr;
                serializeValue(j[key], value);
            }
        } else {
            throw std::runtime_error("Unsupported type in std::any");
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
};

template<>
struct SerializerName<Json> {
    static constexpr const char* value = "JSON";
};

} // namespace zerialize

