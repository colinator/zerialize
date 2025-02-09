#pragma once

#include <any>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <span>
#include <zerialize/zerialize.hpp>
#include "flatbuffers/flexbuffers.h"


namespace zerialize {


class FlexBuffer : public DataBuffer<FlexBuffer> {
private:
    std::vector<uint8_t> buf_;
    flexbuffers::Reference ref_;
public:
    flexbuffers::Builder fbb;

    FlexBuffer() {}

    FlexBuffer(flexbuffers::Reference ref): ref_(ref) {}

    // Zero-copy view of existing data
    FlexBuffer(std::span<const uint8_t> data)
        : ref_(flexbuffers::GetRoot(data.data(), data.size())) { }

    // Zero-copy move of vector ownership
    FlexBuffer(std::vector<uint8_t>&& buf)
        : buf_(std::move(buf))
        , ref_(flexbuffers::GetRoot(buf_)) { }

    // Must copy for const reference
    FlexBuffer(const std::vector<uint8_t>& buf)
        : buf_(buf)
        , ref_(flexbuffers::GetRoot(buf_)) { }

    void finish() {
        fbb.Finish(); 
        buf_ = fbb.GetBuffer();
        ref_ = flexbuffers::GetRoot(buf_);
    }

    // Base DataBuffer class overrides
    
    std::string to_string() const override {
        return "FlexBuffer size: " + std::to_string(buf().size());
    }

    const std::vector<uint8_t>& buf() const override {
        return buf_;
    }

    // Satisfy the Deserializer concept

    bool isInt8() const { return ref_.GetType() == flexbuffers::FBT_INT; }
    bool isInt16() const { return ref_.GetType() == flexbuffers::FBT_INT; }
    bool isInt32() const { return ref_.GetType() == flexbuffers::FBT_INT; }
    bool isInt64() const { return ref_.GetType() == flexbuffers::FBT_INT; }
    bool isUInt8() const { return ref_.GetType() == flexbuffers::FBT_INT; }
    bool isUInt16() const { return ref_.GetType() == flexbuffers::FBT_INT; }
    bool isUInt32() const { return ref_.GetType() == flexbuffers::FBT_INT; }
    bool isUInt64() const { return ref_.GetType() == flexbuffers::FBT_INT; }
    bool isFloat() const { return ref_.GetType() == flexbuffers::FBT_FLOAT; }
    bool isDouble() const { return ref_.GetType() == flexbuffers::FBT_FLOAT; }
    bool isBool() const { return ref_.GetType() == flexbuffers::FBT_BOOL; }
    bool isString() const { return ref_.GetType() == flexbuffers::FBT_STRING; }
    bool isMap() const { return ref_.GetType() == flexbuffers::FBT_MAP; }
    bool isArray() const { return ref_.GetType() == flexbuffers::FBT_VECTOR; }

    int8_t asInt8() const {
        if (!isInt8()) { throw DeserializationError("not an int8"); }
        return static_cast<int8_t>(ref_.AsInt8());
    }

    int16_t asInt16() const {
        if (!isInt16()) { throw DeserializationError("not an int16"); }
        return static_cast<int16_t>(ref_.AsInt16());
    }

    int32_t asInt32() const {
        if (!isInt32()) { throw DeserializationError("not an int32"); }
        return ref_.AsInt32();
    }

    int64_t asInt64() const {
        if (!isInt64()) { throw DeserializationError("not an int64"); }
        return ref_.AsInt64();
    }

    uint8_t asUInt8() const {
        if (!isUInt8()) { throw DeserializationError("not a uint8"); }
        return static_cast<uint8_t>(ref_.AsUInt8());
    }

    uint16_t asUInt16() const {
        if (!isUInt16()) { throw DeserializationError("not a uint16"); }
        return static_cast<uint16_t>(ref_.AsUInt16());
    }

    uint32_t asUInt32() const {
        if (!isUInt32()) { throw DeserializationError("not a uint32"); }
        return static_cast<uint32_t>(ref_.AsUInt32());
    }

    uint64_t asUInt64() const {
        if (!isUInt64()) { throw DeserializationError("not a uint64"); }
        return static_cast<uint64_t>(ref_.AsUInt64());
    }

    float asFloat() const {
        if (!isFloat()) { throw DeserializationError("not a float"); }
        return static_cast<float>(ref_.AsFloat());
    }

    double asDouble() const { 
        if (!isDouble()) { throw DeserializationError("not a float"); }
        return ref_.AsDouble();
    }

    bool asBool() const {
        if (!isBool()) { throw DeserializationError("not a bool"); }
        return ref_.AsBool();
    }

    std::string_view asString() const {
        if (!isString()) { throw DeserializationError("not a string"); }
        auto str = ref_.AsString();
        return std::string_view(str.c_str(), str.size());
    }

    std::vector<std::string_view> mapKeys() const {
        if (!isMap()) { throw DeserializationError("not a map"); }
        std::vector<std::string_view> keys;
        for (size_t i=0; i < ref_.AsMap().Keys().size(); i++) {
            std::string_view key(
                ref_.AsMap().Keys()[i].AsString().c_str(), 
                ref_.AsMap().Keys()[i].AsString().size());
            keys.push_back(key);
        }
        return keys;
    }

    FlexBuffer operator[] (const std::string& key) const {
        if (!isMap()) { throw DeserializationError("not a map"); }
        return FlexBuffer(ref_.AsMap()[key]);
    }

    size_t arraySize() const {
        if (!isArray()) { throw DeserializationError("not an array"); }
        return ref_.AsVector().size();
    }

    FlexBuffer operator[] (size_t index) const {
        if (!isArray()) { throw DeserializationError("not an array"); }
        return FlexBuffer(ref_.AsVector()[index]);
    }
};

class Flex {
private:

    void serializeValue(flexbuffers::Builder& fbb, const char* val) {
        fbb.String(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, int val) {
        fbb.Int(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, long long val) {
        fbb.UInt(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, float val) {
        fbb.Float(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, double val) {
        fbb.Double(val);
    }

    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T, std::string_view>>>
    void serializeValue(flexbuffers::Builder& fbb, T&& val) {
        fbb.String(std::forward<T>(val));
    }

    template<typename T>
    void serializeValue(flexbuffers::Builder& fbb, std::pair<const char*, T>&& keyVal) {
        fbb.Key(keyVal.first);
        serializeValue(fbb, std::forward<T>(keyVal.second));
    }

    void serializeValue(flexbuffers::Builder& fbb, std::vector<std::any>&& val) {
        fbb.Vector([&]() {
            for (const std::any& v: val) {
                serializeValue(fbb, v);
            }
        });
    }

    void serializeValue(flexbuffers::Builder& fbb, const std::any& val) {
        if (val.type() == typeid(int)) {
            fbb.Int(std::any_cast<int>(val));
        } else if (val.type() == typeid(double)) {
            fbb.Double(std::any_cast<double>(val));
        } else if (val.type() == typeid(float)) {
            fbb.Float(std::any_cast<float>(val));
        } else if (val.type() == typeid(const char*)) {
            fbb.String(std::any_cast<const char*>(val));
        } else if (val.type() == typeid(std::string)) {
            fbb.String(std::any_cast<std::string>(val));
        } else if (val.type() == typeid(std::vector<std::any>)) {
            std::vector<std::any> l = std::any_cast<std::vector<std::any>>(val);
            fbb.Vector([&]() {
                for (const std::any& v: l) {
                    serializeValue(fbb, v);
                }
            });
        } else if (val.type() == typeid(std::map<std::string, std::any>)) {
            std::map<std::string, std::any> m = std::any_cast<std::map<std::string, std::any>>(val);
            fbb.Map([&]() {
                for (const auto& [key, value]: m) {
                    fbb.Key(key);
                    serializeValue(fbb, value);
                }
            });
        } else {
            throw std::runtime_error("Unsupported type in std::any");
        }
    }

public:

    using BufferType = FlexBuffer;

    Flex() {}

    BufferType serialize() {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "FlexBuffer::serialize nothing" << std::endl;
        }
        FlexBuffer buffer;
        return buffer;
    }

    BufferType serialize(const std::any& value) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "FlexBuffer::serialize any: " << value.type().name() << std::endl;
        }
        FlexBuffer buffer;
        serializeValue(buffer.fbb, value);
        buffer.finish();
        return buffer;
    }

    BufferType serialize(std::initializer_list<std::any> list) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "FlexBuffer::serialize initializer list" << std::endl;
        }
        FlexBuffer buffer;
        buffer.fbb.Vector([&]() {
            for (const std::any& val : list) {
                serializeValue(buffer.fbb, val);
            }
        });
        buffer.finish();
        return buffer;
    }

    BufferType serialize(std::initializer_list<std::pair<std::string, std::any>> list) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "FlexBuffer::serialize initializer list(map)" << std::endl;
        }
        FlexBuffer buffer;
        buffer.fbb.Map([&]() {
            for (const auto& [key, val] : list) {
                buffer.fbb.Key(key);
                serializeValue(buffer.fbb, val);
            }
        });
        buffer.finish();
        return buffer;
    }

    template<typename ValueType>
    BufferType serialize(ValueType&& value) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "FlexBuffer::serialize value&&" << std::endl;
        }
        FlexBuffer buffer;
        serializeValue(buffer.fbb, std::forward<ValueType>(value));
        buffer.finish();
        return buffer;
    }

    template<typename... ValueTypes>
    BufferType serialize(ValueTypes&&... values) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "FlexBuffer::serialize values &&" << std::endl;
        }
        FlexBuffer buffer;
        buffer.fbb.Vector([&]() {
            (serializeValue(buffer.fbb, std::forward<ValueTypes>(values)), ...);
        });
        buffer.finish();
        return buffer;
    }

    template<typename... ValueTypes>
    BufferType serializeMap(ValueTypes&&... values) {
        if constexpr (DEBUG_TRACE_CALLS) {
            std::cout << "FlexBuffer::serialize ValueTypes" << std::endl;
        }
        FlexBuffer buffer;
        buffer.fbb.Map([&]() {
            ([&](auto&& pair) {
                buffer.fbb.Key(pair.first);
                serializeValue(buffer.fbb, std::forward<decltype(pair.second)>(pair.second));
            }(std::forward<ValueTypes>(values)), ...);
        });
        buffer.finish();
        return buffer;
    }
};


template<>
struct SerializerName<Flex> {
    static constexpr const char* value = "FLEX";
};

} // namespace zerialize
