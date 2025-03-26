#pragma once

#include <zerialize/zerialize.hpp>
#include <nlohmann/json.hpp>

namespace zerialize {

inline string base64Encode(span<const uint8_t> data) {
    static constexpr char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    string encoded;
    size_t i = 0;
    uint32_t value = 0;

    for (uint8_t byte : data) {
        value = (value << 8) + byte;
        i += 8;
        while (i >= 6) {
            encoded += base64_chars[(value >> (i - 6)) & 0x3F];
            i -= 6;
        }
    }

    if (i > 0) {
        encoded += base64_chars[(value << (6 - i)) & 0x3F];
    }

    while (encoded.size() % 4 != 0) {
        encoded += '=';
    }

    return encoded;
}

inline vector<uint8_t> base64Decode(string_view encoded) {
    static constexpr uint8_t lookup[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
        64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };

    vector<uint8_t> decoded;
    uint32_t buffer = 0;
    int bits_collected = 0;

    for (char c : encoded) {
        if (c == '=') break; // Ignore padding

        uint8_t value = lookup[static_cast<uint8_t>(c)];
        if (value == 64) throw DeserializationError("Invalid Base64 character");

        buffer = (buffer << 6) | value;
        bits_collected += 6;

        if (bits_collected >= 8) {
            bits_collected -= 8;
            decoded.push_back(static_cast<uint8_t>((buffer >> bits_collected) & 0xFF));
        }
    }

    return decoded;
}

class JsonBuffer : public DataBuffer<JsonBuffer> {
private:
    vector<uint8_t> buf_;
    nlohmann::json json_;

public:

    nlohmann::json& json() { return json_; }

    JsonBuffer() {}

    JsonBuffer(const nlohmann::json& j): json_(j) {}

    // Zero-copy view of existing data
    JsonBuffer(span<const uint8_t> data)
        : buf_(data.begin(), data.end()),
          json_(nlohmann::json::parse(data.begin(), data.end())) { }

    // Zero-copy move of vector ownership
    JsonBuffer(vector<uint8_t>&& buf)
        : buf_(std::move(buf)),
          json_(nlohmann::json::parse(buf_)) { }

    // Must copy for const reference
    JsonBuffer(const vector<uint8_t>& buf)
        : buf_(buf),
          json_(nlohmann::json::parse(buf_)) { }

    JsonBuffer(vector<uint8_t>&& buf, nlohmann::json&& json)
        : buf_(buf)
        , json_(json) { }

    span<const uint8_t> buf() const override {
        return std::span<const uint8_t>(buf_);
    }

    string to_string() const override {
        return "JsonBuffer " + std::to_string(buf().size()) + 
            " bytes at: " + std::format("{}", static_cast<const void*>(buf_.data())) +
            " : " + json_.dump() + "\n" + debug_string(*this);
    }

    bool isNull() const { return json_.is_null(); }
    bool isInt() const { return json_.is_number_integer(); }
    bool isUInt() const { return json_.is_number_unsigned(); }
    bool isFloat() const { return json_.is_number_float(); }
    bool isBool() const { return json_.is_boolean(); }
    bool isString() const { return json_.is_string(); }
    bool isBlob() const { return json_.is_string(); }
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
    
    string asString() const { 
        if (!isString()) { throw DeserializationError("not a string"); }
        return json_.get<string>(); 
    }

    string_view asStringView() const { 
        if (!isString()) { throw DeserializationError("not a string"); }
        return json_.get<string_view>(); 
    }

    vector<uint8_t> asBlob() const {
        if (!isBlob()) { throw DeserializationError("not a blob"); }
        return base64Decode(json_.get<string_view>());
    }

    set<string_view> mapKeys() const {
        if (!isMap()) { throw DeserializationError("not a map"); }
        set<string_view> keys;
        for (const auto& it : json_.items()) {
            keys.insert(it.key());
        }
        return keys;
    }

    JsonBuffer operator[] (const string_view key) const { 
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

class JsonRootSerializer {
public:
    nlohmann::json json;

    JsonBuffer finish() {
        string str = json.dump();
        auto buf = vector<uint8_t>(str.begin(), str.end());
        return JsonBuffer(std::move(buf), std::move(json));
    }
};

class JsonSerializer: public Serializer<JsonSerializer> {
private:
    nlohmann::json& j;
    bool serializingVector;

public:

    // Make the base class overloads visible in the derived class
    using Serializer<JsonSerializer>::serialize;

    JsonSerializer(nlohmann::json& j, bool serializingVector = false)
        : j(j)
        , serializingVector(serializingVector) {}

    JsonSerializer(JsonRootSerializer& jb, bool serializingVector = false)
        : j(jb.json)
        , serializingVector(serializingVector) {}

    nlohmann::json& elem() {
        if (serializingVector) {
            j.push_back(nullptr);
            return j.back();
        }
        return j;
    }

    template <typename T>
    void writeT(T val) {
        elem() = val;
    }

    void serialize(std::nullptr_t) { writeT(nullptr); }

    void serialize(int8_t val) { writeT(val); }
    void serialize(int16_t val) { writeT(val); }
    void serialize(int32_t val) { writeT(val); }
    void serialize(int64_t val) { writeT(val); }

    void serialize(uint8_t val) { writeT(val); }
    void serialize(uint16_t val) { writeT(val); }
    void serialize(uint32_t val) { writeT(val); }
    void serialize(uint64_t val) { writeT(val); }

    void serialize(bool val) { writeT(val); }
    void serialize(double val) { writeT(val); }
    void serialize(const char* val) { writeT(val); }
    void serialize(const string& val) { writeT(val); }

    void serialize(const span<const uint8_t>& val) { 
        std::string s = base64Encode(val);
        writeT(s);
    }

    template<typename T, typename = enable_if_t<is_convertible_v<T, string_view>>>
    void serialize(T&& val) {
        writeT(string(val));
    }

    void serialize(const string& key, const any& value) {
        if (serializingVector) { throw SerializationError("Cannot serialize key/value to vector"); }
        j[key] = nullptr;
        JsonSerializer s(j[key]);
        s.serializeAny(value);
    }

    JsonSerializer serializerForKey(const string_view& key) {
        if (serializingVector) { throw SerializationError("Cannot serialize key/value to vector"); }
        j[key] = nullptr;
        return JsonSerializer(j[key]);
    }

    void serialize(const map<string, any>& m) {
        writeT(nlohmann::json::object());
        for (const auto& [key, value]: m) {
            j[key] = nullptr;
            JsonSerializer s(j[key]);
            s.serialize(key, value);
        }
    }

    void serialize(const vector<any>& l) {
        nlohmann::json& ja = elem();
        ja = nlohmann::json::array();
        for (auto v: l) {
            JsonSerializer s(ja, true);
            s.serializeAny(v);
        }
    }

    template <typename F>
    requires InvocableSerializer<F, JsonSerializer&>
    void serialize(F&& f) {
        std::forward<F>(f)(*this);
    }

    template <typename F>
    requires InvocableSerializer<F, JsonSerializer&>
    void serializeMap(F&& f) {
        nlohmann::json& ja = elem();
        ja = nlohmann::json::object();
        JsonSerializer s(ja);
        std::forward<F>(f)(s);
    }

    template <typename F>
    requires InvocableSerializer<F, JsonSerializer&>
    void serializeVector(F&& f) {
        nlohmann::json& ja = elem();
        ja = nlohmann::json::array();
        JsonSerializer s(ja, true);
        std::forward<F>(f)(s);
    }
};


class Json {
public:
    using BufferType = JsonBuffer;
    using Serializer = JsonSerializer;
    using RootSerializer = JsonRootSerializer;
    using SerializingFunction = JsonSerializer::SerializingFunction;

    static inline constexpr const char* Name = "JSON";
};

} // namespace zerialize


// Verified size is correct:
// {"a":3,"b":5.2,"c":"asdf","d":[7,8.2,{"e":2.613,"pi":3.14159}],"k":1028}
// JsonBuffer 72 bytes at: 0x13ae070a0
