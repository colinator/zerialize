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

    void finish() {
        string str = json_.dump();
        buf_ = vector<uint8_t>(str.begin(), str.end());
    }

    const vector<uint8_t>& buf() const override {
        return buf_;
    }

    string to_string() const override {
        return "JsonBuffer " + std::to_string(buf().size()) + 
            " bytes at: " + std::format("{}", static_cast<const void*>(buf_.data())) +
            " : " + json_.dump() + "\n" + debug_string(*this);
    }

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

    void copyMapKeys() const {
        for (const auto& it : json_.items()) {
            cachedMapKeys.insert(it.key());
        }
    }

    JsonBuffer operator[] (const string& key) const { 
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

    void serializeValue(nlohmann::json& j, const string& val) {
        j = val;
    }

    void serializeValue(nlohmann::json& j, const span<const uint8_t>& val) {
        std::string s = base64Encode(val);
        j = s;
    }

    template<typename T, typename = enable_if_t<is_convertible_v<T, string_view>>>
    void serializeValue(nlohmann::json& j, T&& val) {
        j = string(val);
    }

    template<typename T>
    void serializeValue(nlohmann::json& j, pair<const char*, T>&& keyVal) {
        j[keyVal.first] = nullptr;
        serializeValue(j[keyVal.first], std::forward<T>(keyVal.second));
    }

    void serializeValue(nlohmann::json& j, vector<any>&& val) {
        j = nlohmann::json::array();
        for (const any& v: val) {
            j.push_back(nullptr);
            serializeValue(j.back(), v);
        }
    }

    void serializeValue(nlohmann::json& j, const any& val) {
        if (val.type() == typeid(map<string, any>)) {
            map<string, any> m = any_cast<map<string, any>>(val);
            j = nlohmann::json::object();
            for (const auto& [key, value]: m) {
                j[key] = nullptr;
                serializeValue(j[key], value);
            }
        } else if (val.type() == typeid(vector<any>)) {
            vector<any> l = any_cast<vector<any>>(val);
            j = nlohmann::json::array();
            for (const any& v: l) {
                j.push_back(nullptr);
                serializeValue(j.back(), v);
            }
        } else if (val.type() == typeid(int8_t)) {
            serializeValue(j, any_cast<int8_t>(val));
        } else if (val.type() == typeid(int16_t)) {
            serializeValue(j, any_cast<int16_t>(val));
        } else if (val.type() == typeid(int32_t)) {
            serializeValue(j, any_cast<int32_t>(val));
        } else if (val.type() == typeid(int64_t)) {
            serializeValue(j, any_cast<int64_t>(val));
        } else if (val.type() == typeid(uint8_t)) {
            serializeValue(j, any_cast<uint8_t>(val));
        } else if (val.type() == typeid(uint16_t)) {
            serializeValue(j, any_cast<uint16_t>(val));
        } else if (val.type() == typeid(uint32_t)) {
            serializeValue(j, any_cast<uint32_t>(val));
        } else if (val.type() == typeid(uint64_t)) {
            serializeValue(j, any_cast<uint64_t>(val));
        } else if (val.type() == typeid(bool)) {
            serializeValue(j, any_cast<bool>(val));
        } else if (val.type() == typeid(double)) {
            serializeValue(j, any_cast<double>(val));
        } else if (val.type() == typeid(float)) {
            serializeValue(j, any_cast<float>(val));
        } else if (val.type() == typeid(span<const uint8_t>)) {
            serializeValue(j, any_cast<span<const uint8_t>>(val));
        } else if (val.type() == typeid(const char*)) {
            serializeValue(j, any_cast<const char*>(val));
        } else if (val.type() == typeid(string)) {
            serializeValue(j, any_cast<string>(val));
        } else {
            throw SerializationError("Unsupported type in any");
        }
    }

    // Used for format conversion, from any other Deserializable.
    template<Deserializable SourceBufferType>
    void serializeValue(nlohmann::json& j, const SourceBufferType& value) {
        if (value.isMap()) {
            j = nlohmann::json::object();
            for (string_view key: value.mapKeys()) {
                // Copies the key. Lame if you ask me...
                const string s(key);
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
            serializeValue(j, value.asBlob()); // ERROR HERE!!!! Howdoweknow?
        } else {
            throw SerializationError("Unsupported source buffer value type");
        }
    }

public:
    using BufferType = JsonBuffer;

    Json() {}

    BufferType serialize() {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "Json::serialize()" << endl;
        }
        JsonBuffer buffer;
        buffer.json() = nlohmann::json::array();
        buffer.finish();
        return buffer;
    }

    BufferType serialize(const any& value) {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "Json::serialize(value)" << endl;
        }
        JsonBuffer buffer;
        serializeValue(buffer.json(), value);
        buffer.finish();
        return buffer;
    }

    BufferType serialize(initializer_list<any> list) {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "Json::serialize(initializer_list)" << endl;
        }
        JsonBuffer buffer;
        buffer.json() = nlohmann::json::array();
        for (const any& val : list) {
            buffer.json().push_back(nullptr);
            serializeValue(buffer.json().back(), val);
        }
        buffer.finish();
        return buffer;
    }

    BufferType serialize(initializer_list<pair<string, any>> list) {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "Json::serialize(initializer list/map)" << endl;
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
            cout << "Json::serialize(&&value)" << endl;
        }   
        JsonBuffer buffer;
        serializeValue(buffer.json(), std::forward<ValueType>(value));
        buffer.finish();
        return buffer;
    }

    template<typename... ValueTypes>
    BufferType serialize(ValueTypes&&... values) {
        if constexpr (DEBUG_TRACE_CALLS) {
            cout << "Json::serialize(&&values)" << endl;
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
            cout << "Json::serializeMap(&&values)" << endl;
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
            cout << "Json::serialize(Deserializable &&value)" << endl;
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



// Verified size is correct:
// {"a":3,"b":5.2,"c":"asdf","d":[7,8.2,{"e":2.613,"pi":3.14159}],"k":1028}
// JsonBuffer 72 bytes at: 0x13ae070a0
