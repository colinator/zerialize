#pragma once

#include <zerialize/zerialize.hpp>
#include "flatbuffers/flexbuffers.h"

namespace zerialize {

class FlexBuffer : public DataBuffer<FlexBuffer> {
private:
    vector<uint8_t> buf_;
    flexbuffers::Reference ref_;
public:
    flexbuffers::Builder fbb;

    FlexBuffer() {}

    FlexBuffer(flexbuffers::Reference ref): ref_(ref) {}

    // Zero-copy view of existing data
    FlexBuffer(span<const uint8_t> data)
        : ref_(flexbuffers::GetRoot(data.data(), data.size())) { }

    // Zero-copy move of vector ownership
    FlexBuffer(vector<uint8_t>&& buf)
        : buf_(std::move(buf))
        , ref_(flexbuffers::GetRoot(buf_)) { }

    // Must copy for const reference
    FlexBuffer(const vector<uint8_t>& buf)
        : buf_(buf)
        , ref_(buf.size() > 0 ? flexbuffers::GetRoot(buf_) : flexbuffers::Reference()) { }

    void finish() {
        fbb.Finish(); 
        buf_ = fbb.GetBuffer();
        ref_ = flexbuffers::GetRoot(buf_);
    }

    // Base DataBuffer class overrides
    
    string to_string() const override {
        return "FlexBuffer " + std::to_string(buf().size()) +
            " bytes at: " + std::format("{}", static_cast<const void*>(buf_.data())) +
            "\n" + debug_string(*this);;
    }

    const vector<uint8_t>& buf() const override {
        return buf_;
    }

    // Satisfy the Deserializer concept

    bool isInt() const { return ref_.IsInt(); }
    bool isUInt() const { return ref_.IsUInt(); }
    bool isFloat() const { return ref_.IsFloat(); }
    bool isBool() const { return ref_.IsBool(); }
    bool isString() const { return ref_.IsString() ; }
    bool isBlob() const { return ref_.IsBlob() ; }
    bool isMap() const { return ref_.IsMap(); }
    bool isArray() const { return ref_.IsAnyVector(); }

    int8_t asInt8() const {
        if (!isInt()) { throw DeserializationError("not an int"); }
        return ref_.AsInt8();
    }

    int16_t asInt16() const {
        if (!isInt()) { throw DeserializationError("not an int"); }
        return ref_.AsInt16();
    }

    int32_t asInt32() const {
        if (!isInt()) { throw DeserializationError("not an int"); }
        return ref_.AsInt32();
    }

    int64_t asInt64() const {
        if (!isInt()) { throw DeserializationError("not an int"); }
        return ref_.AsInt64();
    }

    uint8_t asUInt8() const {
        if (!isUInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint8_t>(ref_.AsUInt8());
    }

    uint16_t asUInt16() const {
        if (!isUInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint16_t>(ref_.AsUInt16());
    }

    uint32_t asUInt32() const {
        if (!isUInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint32_t>(ref_.AsUInt32());
    }

    uint64_t asUInt64() const {
        if (!isUInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint64_t>(ref_.AsUInt64());
    }

    float asFloat() const {
        if (!isFloat()) { throw DeserializationError("not a float"); }
        return static_cast<float>(ref_.AsFloat());
    }

    double asDouble() const { 
        if (!isFloat()) { throw DeserializationError("not a float"); }
        return ref_.AsDouble();
    }

    bool asBool() const {
        if (!isBool()) { throw DeserializationError("not a bool"); }
        return ref_.AsBool();
    }

    string asString() const {
        if (!isString()) { throw DeserializationError("not a string"); }
        auto str = ref_.AsString();
        return str.str();
    }

    string_view asStringView() const {
        if (!isString()) { throw DeserializationError("not a string"); }
        auto str = ref_.AsString();
        return string_view(str.c_str(), str.size());
    }

    span<const uint8_t> asBlob() const {
        if (!isBlob()) { throw DeserializationError("not a blob"); }
        auto b = ref_.AsBlob();
        return span<const uint8_t>(b.data(), b.size());
    }

    void copyMapKeys() const {
        for (size_t i=0; i < ref_.AsMap().Keys().size(); i++) {
            string_view key(
                ref_.AsMap().Keys()[i].AsString().c_str(), 
                ref_.AsMap().Keys()[i].AsString().size());
            cachedMapKeys.insert(key);
        }
    }

    FlexBuffer operator[] (const string& key) const {
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
public:

    using BufferType = FlexBuffer;

private:

    void serializeValue(flexbuffers::Builder& fbb, const char* val) {
        fbb.String(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, int8_t val) {
        fbb.Int(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, int16_t val) {
        fbb.Int(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, int32_t val) {
        fbb.Int(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, int64_t val) {
        fbb.Int(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, uint8_t val) {
        fbb.UInt(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, uint16_t val) {
        fbb.UInt(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, uint32_t val) {
        fbb.UInt(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, uint64_t val) {
        fbb.UInt(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, bool val) {
        fbb.Bool(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, float val) {
        fbb.Float(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, double val) {
        fbb.Double(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, const string& val) {
        fbb.String(val);
    }

    void serializeValue(flexbuffers::Builder& fbb, const span<const uint8_t>& val) {
        fbb.Blob(val.data(), val.size());
    }

    template<typename T, typename = enable_if_t<is_convertible_v<T, string_view>>>
    void serializeValue(flexbuffers::Builder& fbb, T&& val) {
        // fbb.String(std::forward<T>(val));
        // LAME. must provide l-value.
        string a(std::forward<T>(val));
        fbb.String(a);
    }

    template<typename T>
    void serializeValue(flexbuffers::Builder& fbb, pair<const char*, T>&& keyVal) {
        fbb.Key(keyVal.first);
        serializeValue(fbb, std::forward<T>(keyVal.second));
    }

    void serializeValue(flexbuffers::Builder& fbb, vector<any>&& val) {
        fbb.Vector([&]() {
            for (const any& v: val) {
                serializeValue(fbb, v);
            }
        });
    }

    void serializeValue(flexbuffers::Builder& fbb, const any& val) {
        if (val.type() == typeid(map<string, any>)) {
            map<string, any> m = any_cast<map<string, any>>(val);
            fbb.Map([&]() {
                for (const auto& [key, value]: m) {
                    fbb.Key(key);
                    serializeValue(fbb, value);
                }
            });
        } else if (val.type() == typeid(vector<any>)) {
            vector<any> l = any_cast<vector<any>>(val);
            fbb.Vector([&]() {
                for (const any& v: l) {
                    serializeValue(fbb, v);
                }
            });
        } else if (val.type() == typeid(int8_t)) {
            serializeValue(fbb, any_cast<int8_t>(val));
        } else if (val.type() == typeid(int16_t)) {
            serializeValue(fbb, any_cast<int16_t>(val));
        } else if (val.type() == typeid(int32_t)) {
            serializeValue(fbb, any_cast<int32_t>(val));
        } else if (val.type() == typeid(int64_t)) {
            serializeValue(fbb, any_cast<int64_t>(val));
        } else if (val.type() == typeid(uint8_t)) {
            serializeValue(fbb, any_cast<uint8_t>(val));
        } else if (val.type() == typeid(uint16_t)) {
            serializeValue(fbb, any_cast<uint16_t>(val));
        } else if (val.type() == typeid(uint32_t)) {
            serializeValue(fbb, any_cast<uint32_t>(val));
        } else if (val.type() == typeid(uint64_t)) {
            serializeValue(fbb, any_cast<uint64_t>(val));
        } else if (val.type() == typeid(bool)) {
            serializeValue(fbb, any_cast<bool>(val));
        } else if (val.type() == typeid(double)) {
            serializeValue(fbb, any_cast<double>(val));
        } else if (val.type() == typeid(float)) {
            serializeValue(fbb, any_cast<float>(val));
        } else if (val.type() == typeid(span<const uint8_t>)) {
            serializeValue(fbb, any_cast<span<const uint8_t>>(val));
        } else if (val.type() == typeid(const char*)) {
            serializeValue(fbb, any_cast<const char*>(val));
        } else if (val.type() == typeid(string)) {
            serializeValue(fbb, any_cast<string>(val));
        // } else if (val.type() == typeid(Composite)) {
        //     throw SerializationError("Composite type in any");
        } else {
            throw SerializationError("Unsupported type in any");
        }
    }


    // Used for format conversion, from any other Deserializable.
    //void serializeValue(flexbuffers::Builder& fbb, const Deserializable auto& value) {
    template<Deserializable SourceBufferType>
    void serializeValue(flexbuffers::Builder& fbb, const SourceBufferType& value) {
        if (value.isMap()) {
            fbb.Map([&]() {
                for (string_view key: value.mapKeys()) {
                    // Copies the key. Lame if you ask me...
                    const string s(key);
                    fbb.Key(s);
                    serializeValue(fbb, value[s]);
                }
            });
        } else if (value.isArray()) {
            fbb.Vector([&]() {
                for (size_t i=0; i<value.arraySize(); i++) {
                    serializeValue(fbb, value[i]);
                }
            });
        } else if (value.isInt()) {
            serializeValue(fbb, value.asInt64());
        } else if (value.isUInt()) {
            serializeValue(fbb, value.asUInt64());
        } else if (value.isFloat()) {
            serializeValue(fbb, value.asDouble());
        } else if (value.isBool()) {
            serializeValue(fbb, value.asBool());
        } else if (value.isString()) {
            serializeValue(fbb, value.asString());
        } else if (value.isBlob()) {
            span<const uint8_t> s = value.asBlob();
            serializeValue(fbb, s);
        } else {
            //serializeValue(fbb, value.template serialize<Flex>(*this));
            throw SerializationError("Unsupported source buffer value type");
        }
    }

public:

    Flex() {}

    BufferType serialize() {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "FlexBuffer::serialize nothing" << endl;
        }
        FlexBuffer buffer;
        return buffer;
    }

    BufferType serialize(const any& value) {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "FlexBuffer::serialize any: " << value.type().name() << endl;
        }
        FlexBuffer buffer;
        serializeValue(buffer.fbb, value);
        buffer.finish();
        return buffer;
    }

    BufferType serialize(initializer_list<any> list) {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "FlexBuffer::serialize initializer list" << endl;
        }
        FlexBuffer buffer;
        buffer.fbb.Vector([&]() {
            for (const any& val : list) {
                serializeValue(buffer.fbb, val);
            }
        });
        buffer.finish();
        return buffer;
    }

    // BufferType serialize(const map<string, any>& map) {
    //     if constexpr (DEBUG_TRACE_CALLS) {
    //         cout << "FlexBuffer::serialize (const map<string, any>&) " << endl;
    //     }
    //     FlexBuffer buffer;
    //     buffer.fbb.Map([&]() {
    //         for (const auto& [key, val] : map) {
    //             buffer.fbb.Key(key);
    //             serializeValue(buffer.fbb, val);
    //         }
    //     });
    //     buffer.finish();
    //     return buffer;
    // }

    BufferType serialize(initializer_list<pair<string, any>> list) {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "FlexBuffer::serialize initializer list(map)" << endl;
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
            cout << "FlexBuffer::serialize value&&" << endl;
        }
        FlexBuffer buffer;
        serializeValue(buffer.fbb, std::forward<ValueType>(value));
        buffer.finish();
        return buffer;
    }

    template<typename... ValueTypes>
    BufferType serialize(ValueTypes&&... values) {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "FlexBuffer::serialize values &&" << endl;
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
            cout << "FlexBuffer::serialize ValueTypes" << endl;
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

    // A special serialize method that takes another buffer type.
    // Used for format conversion.
    template<Deserializable SourceBufferType>
    BufferType serialize(const SourceBufferType& value) {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "Flex::serialize(Deserializable &&value)" << endl;
        }   
        FlexBuffer buffer;
        serializeValue(buffer.fbb, value);
        buffer.finish();
        return buffer;
    }
};


template<>
struct SerializerName<Flex> {
    static constexpr const char* value = "FLEX";
};

} // namespace zerialize
