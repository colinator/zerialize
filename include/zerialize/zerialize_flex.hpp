#pragma once

#include <zerialize/zerialize.hpp>
#include "flatbuffers/flexbuffers.h"

namespace zerialize {

class FlexBuffer : public DataBuffer<FlexBuffer> {
private:
    vector<uint8_t> buf_;
    flexbuffers::Reference ref_;
    span<const uint8_t> view_;

public:

    FlexBuffer() {}

    FlexBuffer(flexbuffers::Reference ref): ref_(ref) {}

    // Zero-copy view of existing data
    FlexBuffer(span<const uint8_t> data)
        : ref_(data.size() > 0 ? flexbuffers::GetRoot(data.data(), data.size()) : flexbuffers::Reference())
        , view_(data) { }

    // Zero-copy move of vector ownership
    FlexBuffer(vector<uint8_t>&& buf)
        : buf_(std::move(buf))
        , ref_(buf_.size() > 0 ? flexbuffers::GetRoot(buf_) : flexbuffers::Reference())
        , view_(buf_) { }

    // Must copy for const reference
    FlexBuffer(const vector<uint8_t>& buf)
        : buf_(buf)
        , ref_(buf.size() > 0 ? flexbuffers::GetRoot(buf_) : flexbuffers::Reference()) 
        , view_(buf_) { }

    span<const uint8_t> buf() const override {
        return view_;
    }

    string to_string() const override {
        return "FlexBuffer " + std::to_string(buf().size()) +
            " bytes at: " + std::format("{}", static_cast<const void*>(buf_.data())) +
            "\n" + debug_string(*this);
    }

    bool isNull() const { return ref_.IsNull(); }
    bool isInt() const { return ref_.IsInt(); }
    bool isUInt() const { return ref_.IsUInt(); }
    bool isFloat() const { return ref_.IsFloat(); }
    bool isBool() const { return ref_.IsBool(); }
    bool isString() const { return ref_.IsString() ; }
    bool isBlob() const { return ref_.IsBlob() ; }
    bool isMap() const { return ref_.IsMap(); }
    bool isArray() const { return ref_.IsAnyVector(); }

    int8_t asInt8() const {
        //if (!isInt()) { throw DeserializationError("not an int"); }
        return ref_.AsInt8();
    }

    int16_t asInt16() const {
        //if (!isInt()) { throw DeserializationError("not an int"); }
        return ref_.AsInt16();
    }

    int32_t asInt32() const {
        //if (!isInt()) { throw DeserializationError("not an int"); }
        return ref_.AsInt32();
    }

    int64_t asInt64() const {
        //if (!isInt()) { throw DeserializationError("not an int"); }
        return ref_.AsInt64();
    }

    uint8_t asUInt8() const {
        //if (!isUInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint8_t>(ref_.AsUInt8());
    }

    uint16_t asUInt16() const {
        //if (!isUInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint16_t>(ref_.AsUInt16());
    }

    uint32_t asUInt32() const {
        //if (!isUInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint32_t>(ref_.AsUInt32());
    }

    uint64_t asUInt64() const {
        //if (!isUInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint64_t>(ref_.AsUInt64());
    }

    float asFloat() const {
        //if (!isFloat()) { throw DeserializationError("not a float"); }
        return static_cast<float>(ref_.AsFloat());
    }

    double asDouble() const { 
        //if (!isFloat()) { throw DeserializationError("not a float"); }
        return ref_.AsDouble();
    }

    bool asBool() const {
        //if (!isBool()) { throw DeserializationError("not a bool"); }
        return ref_.AsBool();
    }

    string asString() const {
        //if (!isString()) { throw DeserializationError("not a string"); }
        auto str = ref_.AsString();
        return str.str();
    }

    string_view asStringView() const {
        //if (!isString()) { throw DeserializationError("not a string"); }
        auto str = ref_.AsString();
        return string_view(str.c_str(), str.size());
    }

    span<const uint8_t> asBlob() const {
       //if (!isBlob()) { throw DeserializationError("not a blob"); }
        auto b = ref_.AsBlob();
        return span<const uint8_t>(b.data(), b.size());
    }

    set<string_view> mapKeys() const {
        //if (!isMap()) { throw DeserializationError("not a map"); }
        set<string_view> keys;
        for (size_t i=0; i < ref_.AsMap().Keys().size(); i++) {
            string_view key(
                ref_.AsMap().Keys()[i].AsString().c_str(), 
                ref_.AsMap().Keys()[i].AsString().size());
            keys.insert(key);
        }
        return keys;
    }

    FlexBuffer operator[] (const string_view key) const {
        //if (!isMap()) { throw DeserializationError("not a map"); }
        string s(key);
        return FlexBuffer(ref_.AsMap()[s]);
    }

    size_t arraySize() const {
        //if (!isArray()) { throw DeserializationError("not an array"); }
        return ref_.AsVector().size();
    }

    FlexBuffer operator[] (size_t index) const {
        // if (!isArray()) {
        //     throw DeserializationError("not an array");
        // }
        auto vec = ref_.AsVector();
        auto val = vec[index];
        auto ret = FlexBuffer(val);
        return ret;
        // if (!isArray()) { throw DeserializationError("not an array"); }
        //return FlexBuffer(ref_.AsVector()[index]);
    }
};

class FlexRootSerializer {
public:
    flexbuffers::Builder fbb;

    FlexBuffer finish() {
        if (fbb.StartVector() > 0) {
            fbb.Finish(); 

            // Move the data out of the builder into the Buffer class

            // You might think this is unsafe...
            const std::vector<uint8_t>& orig = fbb.GetBuffer();

            // ...and this even more so...
            std::vector<uint8_t>& nonConstOrig = const_cast<std::vector<uint8_t>&>(orig);

            // ...but we'll be fine...
            return FlexBuffer(std::move(nonConstOrig));
        }
        return FlexBuffer();
    }
};

class FlexSerializer: public Serializer<FlexSerializer> {
private:
    flexbuffers::Builder& fbb;

public:

    // Make the base class overloads visible in the derived class
    using Serializer<FlexSerializer>::serialize;

    FlexSerializer(const FlexSerializer& o) noexcept: Serializer<FlexSerializer>(), fbb(o.fbb) { }

    FlexSerializer(FlexRootSerializer& fb) noexcept: Serializer(), fbb(fb.fbb) {}

    void serialize(std::nullptr_t) noexcept { fbb.Null(); }

    void serialize(int8_t val) noexcept { fbb.Int(val); }
    void serialize(int16_t val) noexcept { fbb.Int(val); }
    void serialize(int32_t val) noexcept { fbb.Int(val); }
    void serialize(int64_t val) noexcept { fbb.Int(val); }

    void serialize(uint8_t val) noexcept { fbb.UInt(val); }
    void serialize(uint16_t val) noexcept { fbb.UInt(val); }
    void serialize(uint32_t val) noexcept { fbb.UInt(val); }
    void serialize(uint64_t val) noexcept { fbb.UInt(val); }

    void serialize(bool val) noexcept { fbb.Bool(val); }
    void serialize(double val) noexcept { fbb.Double(val); }
    void serialize(const char* val) noexcept { fbb.String(val); }
    void serialize(const string& val) noexcept { fbb.String(val); }

    void serialize(const span<const uint8_t>& val) noexcept { 
        fbb.Blob(val.data(), val.size()); 
    }

    template<typename T, typename = enable_if_t<is_convertible_v<T, string_view>>>
    void serialize(T&& val) noexcept {
        // fbb.String(std::forward<T>(val));
        // LAME. must provide l-value.
        string a(std::forward<T>(val));
        fbb.String(a);
    }

    void serialize(const string& key, const any& value) noexcept {
        fbb.Key(key);
        Serializer::serializeAny(value);
    }

    FlexSerializer serializerForKey(const string_view& key) noexcept {
        fbb.Key(string(key));
        return *this;
    }

    void serialize(const map<string, any>& m) noexcept {
        fbb.Map([&]() {
            for (const auto& [key, value]: m) {
                serialize(key, value);
            }
        });
    }

    void serialize(const vector<any>& l) {
        fbb.Vector([&]() {
            for (auto value: l) {
                Serializer::serializeAny(value);
            }
        });
    }
    
    template <typename F>
    requires InvocableSerializer<F, FlexSerializer&>
    void serialize(F&& f) noexcept {
        std::forward<F>(f)(*this);
    }

    template <typename F>
    requires InvocableSerializer<F, FlexSerializer&>
    void serializeMap(F&& f) noexcept {
        fbb.Map([&]() {
            std::forward<F>(f)(*this);
        });
    }

    template <typename F>
    requires InvocableSerializer<F, FlexSerializer&>
    void serializeVector(F&& f) noexcept {
        fbb.Vector([&]() {
            std::forward<F>(f)(*this);
        });
    }
};

class Flex {
public:
    using BufferType = FlexBuffer;
    using Serializer = FlexSerializer;
    using RootSerializer = FlexRootSerializer;
    using SerializingFunction = FlexSerializer::SerializingFunction;

    static inline constexpr const char* Name = "FLEX";
};

} // namespace zerialize
