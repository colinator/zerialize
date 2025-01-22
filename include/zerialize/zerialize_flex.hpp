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


class FlexBuffer : public Buffer {
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
        , ref_(flexbuffers::GetRoot(buf_)) {
            
        }

    // Must copy for const reference
    FlexBuffer(const std::vector<uint8_t>& buf)
        : buf_(buf)
        , ref_(flexbuffers::GetRoot(buf_)) {
            
        }

    void finish() {
        fbb.Finish(); 
        buf_ = fbb.GetBuffer();
        ref_ = flexbuffers::GetRoot(buf_);
    }

    std::string to_string() const override {
        return "FlexBuffer size: " + std::to_string(buf().size());
    }

    const std::vector<uint8_t>& buf() const override {
        return buf_;
    }

    int32_t asInt32() const { 
        return ref_.AsInt32();
    }

    double asDouble() const { 
        return ref_.AsDouble();
    }

    std::string_view asString() const { 
        auto str = ref_.AsString();
        return std::string_view(str.c_str(), str.size());
    }

    FlexBuffer operator[] (const std::string& key) const { 
        return FlexBuffer(ref_.AsMap()[key]);
    }

    FlexBuffer operator[] (size_t index) const { 
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
        FlexBuffer buffer;
        return buffer;
    }

    BufferType serialize(const std::any& value) {
        FlexBuffer buffer;
        serializeValue(buffer.fbb, value);
        buffer.finish();
        return buffer;
    }

    BufferType serialize(std::initializer_list<std::any> list) {
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
        FlexBuffer buffer;
        serializeValue(buffer.fbb, std::forward<ValueType>(value));
        buffer.finish();
        return buffer;
    }

    template<typename... ValueTypes>
    BufferType serialize(ValueTypes&&... values) {
        FlexBuffer buffer;
        buffer.fbb.Vector([&]() {
            (serializeValue(buffer.fbb, std::forward<ValueTypes>(values)), ...);
        });
        buffer.finish();
        return buffer;
    }

    template<typename... ValueTypes>
    BufferType serializeMap(ValueTypes&&... values) {
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

}