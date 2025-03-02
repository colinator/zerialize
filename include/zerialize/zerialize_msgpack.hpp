#pragma once

#include <zerialize/zerialize.hpp>
#include <msgpack.hpp>
#include <variant>

// Define endianness manually since we're not using Boost
// #if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
//   #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
//     #define MSGPACK_ENDIAN_LITTLE_BYTE 1
//   #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
//     #define MSGPACK_ENDIAN_BIG_BYTE 1
//   #endif
// #elif defined(_WIN32)
//   #define MSGPACK_ENDIAN_LITTLE_BYTE 1
// #endif

namespace zerialize {

using std::variant;

using MsgPackStream = msgpack::sbuffer;
using MsgPacker = msgpack::packer<MsgPackStream>;

class MsgPackBuffer : public DataBuffer<MsgPackBuffer> {
private:
    vector<uint8_t> buf_;
    msgpack::object_handle oh_;
    msgpack::object obj_;

public:
    MsgPackStream sbuf;

    MsgPackBuffer() {}

    MsgPackBuffer(const msgpack::object& obj): obj_(obj) {}

    // Zero-copy view of existing data
    MsgPackBuffer(span<const uint8_t> data) {
        if (data.size() > 0) {
            msgpack::unpack(oh_, reinterpret_cast<const char*>(data.data()), data.size());
            obj_ = oh_.get();
        }
    }

    // Zero-copy move of vector ownership
    MsgPackBuffer(vector<uint8_t>&& buf): buf_(std::move(buf)) {
        if (buf_.size() > 0) {
            msgpack::unpack(oh_, reinterpret_cast<const char*>(buf_.data()), buf_.size());
            obj_ = oh_.get();
        }
    }

    // Must copy for const reference
    MsgPackBuffer(const vector<uint8_t>& buf): buf_(buf) {
        if (buf_.size() > 0) {
            msgpack::unpack(oh_, reinterpret_cast<const char*>(buf_.data()), buf_.size());
            obj_ = oh_.get();
        }
    }

    void finish() {
        if (sbuf.size() > 0) {
            buf_.resize(sbuf.size());
            std::memcpy(buf_.data(), sbuf.data(), sbuf.size());
            
            // Parse the buffer to get the object
            msgpack::unpack(oh_, reinterpret_cast<const char*>(buf_.data()), buf_.size());
            obj_ = oh_.get();
        }
    }

    const vector<uint8_t>& buf() const override {
        return buf_;
    }

    string to_string() const override {
        std::stringstream ss;
        ss << "MsgPackBuffer " << buf().size() 
           << " bytes at: " << std::format("{}", static_cast<const void*>(buf_.data()));
        
        if (buf_.size() > 0) {
            ss << "\n" << debug_string(*this);
        }
        
        return ss.str();
    }

    bool isNull() const { 
        return obj_.type == msgpack::type::NIL; 
    }

    bool isInt() const { 
        return obj_.type == msgpack::type::NEGATIVE_INTEGER || obj_.type == msgpack::type::POSITIVE_INTEGER;
    }
    
    bool isUInt() const { 
        return obj_.type == msgpack::type::POSITIVE_INTEGER;
    }
    
    bool isFloat() const { 
        return obj_.type == msgpack::type::FLOAT;
    }
    
    bool isBool() const { 
        return obj_.type == msgpack::type::BOOLEAN;
    }
    
    bool isString() const { 
        return obj_.type == msgpack::type::STR;
    }
    
    bool isBlob() const { 
        return obj_.type == msgpack::type::BIN;
    }
    
    bool isMap() const { 
        return obj_.type == msgpack::type::MAP;
    }
    
    bool isArray() const { 
        return obj_.type == msgpack::type::ARRAY;
    }

    int8_t asInt8() const {
        if (!isInt() && !isUInt()) { throw DeserializationError("not an int"); }
        return static_cast<int8_t>(obj_.as<int64_t>());
    }

    int16_t asInt16() const {
        if (!isInt() && !isUInt()) { throw DeserializationError("not an int"); }
        return static_cast<int16_t>(obj_.as<int64_t>());
    }

    int32_t asInt32() const {
        if (!isInt() && !isUInt()) { throw DeserializationError("not an int"); }
        return static_cast<int32_t>(obj_.as<int64_t>());
    }

    int64_t asInt64() const {
        if (!isInt() && !isUInt()) { throw DeserializationError("not an int"); }
        return obj_.as<int64_t>();
    }

    uint8_t asUInt8() const {
        if (!isUInt() && !isInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint8_t>(obj_.as<uint64_t>());
    }

    uint16_t asUInt16() const {
        if (!isUInt() && !isInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint16_t>(obj_.as<uint64_t>());
    }

    uint32_t asUInt32() const {
        if (!isUInt() && !isInt()) { throw DeserializationError("not a uint"); }
        return static_cast<uint32_t>(obj_.as<uint64_t>());
    }

    uint64_t asUInt64() const {
        if (!isUInt() && !isInt()) { throw DeserializationError("not a uint"); }
        return obj_.as<uint64_t>();
    }

    float asFloat() const {
        if (!isFloat()) { throw DeserializationError("not a float"); }
        return static_cast<float>(obj_.as<double>());
    }

    double asDouble() const {
        if (!isFloat()) { throw DeserializationError("not a float"); }
        return obj_.as<double>();
    }

    bool asBool() const {
        if (!isBool()) { throw DeserializationError("not a bool"); }
        return obj_.as<bool>();
    }

    string asString() const {
        if (!isString()) { throw DeserializationError("not a string"); }
        return obj_.as<string>();
    }

    string_view asStringView() const {
        if (!isString()) { throw DeserializationError("not a string"); }
        std::size_t size;
        const char* ptr = obj_.via.str.ptr;
        size = obj_.via.str.size;
        return string_view(ptr, size);
    }

    span<const uint8_t> asBlob() const {
        if (!isBlob()) { throw DeserializationError("not a blob"); }
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(obj_.via.bin.ptr);
        std::size_t size = obj_.via.bin.size;
        return span<const uint8_t>(ptr, size);
    }

    set<string_view> mapKeys() const {
        if (!isMap()) { throw DeserializationError("not a map"); }
        set<string_view> keys;
        for (uint32_t i = 0; i < obj_.via.map.size; ++i) {
            const msgpack::object& key = obj_.via.map.ptr[i].key;
            if (key.type != msgpack::type::STR) {
                throw DeserializationError("map key is not a string");
            }
            keys.insert(string_view(key.via.str.ptr, key.via.str.size));
        }
        return keys;
    }

    MsgPackBuffer operator[] (const string& key) const {
        if (!isMap()) { throw DeserializationError("not a map"); }
        for (uint32_t i = 0; i < obj_.via.map.size; ++i) {
            const msgpack::object& k = obj_.via.map.ptr[i].key;
            if (k.type == msgpack::type::STR) {
                string_view sv(k.via.str.ptr, k.via.str.size);
                if (sv == key) {
                    return MsgPackBuffer(obj_.via.map.ptr[i].val);
                }
            }
        }
        throw DeserializationError("key not found in map: " + key);
    }

    size_t arraySize() const {
        if (!isArray()) { throw DeserializationError("not an array"); }
        return obj_.via.array.size;
    }

    MsgPackBuffer operator[] (size_t index) const {
        if (!isArray()) { throw DeserializationError("not an array"); }
        if (index >= obj_.via.array.size) {
            throw DeserializationError("array index out of bounds");
        }
        return MsgPackBuffer(obj_.via.array.ptr[index]);
    }
};

class MsgPackSerializer: public Serializer<MsgPackSerializer> {
private:

    MsgPacker packer;

    //variant<MsgPacker, std::reference_wrapper<MsgPacker>> packer;

    MsgPackStream& pack_stream;

public:
    // Make the base class overloads visible in the derived class
    using Serializer<MsgPackSerializer>::serialize;

    MsgPackSerializer(MsgPackBuffer& mb): packer(mb.sbuf), pack_stream(mb.sbuf) {}

    MsgPackSerializer(MsgPackStream& ps): packer(ps), pack_stream(ps) {}

    void serialize(std::nullptr_t) { packer.pack_nil(); }

    void serialize(int8_t val) { packer.pack(val); }
    void serialize(int16_t val) { packer.pack(val); }
    void serialize(int32_t val) { packer.pack(val); }
    void serialize(int64_t val) { packer.pack(val); }

    void serialize(uint8_t val) { packer.pack(val); }
    void serialize(uint16_t val) { packer.pack(val); }
    void serialize(uint32_t val) { packer.pack(val); }
    void serialize(uint64_t val) { packer.pack(val); }

    void serialize(bool val) { packer.pack(val); }
    void serialize(double val) { packer.pack(val); }
    
    void serialize(const char* val) { 
        packer.pack_str(strlen(val));
        packer.pack_str_body(val, strlen(val));
    }
    
    void serialize(const string& val) { 
        packer.pack_str(val.size());
        packer.pack_str_body(val.data(), val.size());
    }

    void serialize(const span<const uint8_t>& val) { 
        packer.pack_bin(val.size());
        packer.pack_bin_body(reinterpret_cast<const char*>(val.data()), val.size());
    }

    template<typename T, typename = enable_if_t<is_convertible_v<T, string_view>>>
    void serialize(T&& val) {
        string str(std::forward<T>(val));
        packer.pack_str(str.size());
        packer.pack_str_body(str.data(), str.size());
    }

    void serialize(const string& key, const any& value) {
        serialize(key);
        Serializer::serializeAny(value);
    }

    MsgPackSerializer serializerForKey(const string& key) {
        serialize(key);
        return MsgPackSerializer(pack_stream);
    }

    void serialize(const map<string, any>& m) {
        packer.pack_map(m.size());
        for (const auto& [key, value]: m) {
            serialize(key, value);
        }
    }

    void serialize(const vector<any>& l) {
        packer.pack_array(l.size());
        for (const auto& value: l) {
            serializeAny(value);
        }
    }

    template <typename F>
    requires InvocableSerializer<F, MsgPackSerializer&>
    void serialize(F&& f) {
        std::forward<F>(f)(*this);
    }

    template <typename F>
    requires InvocableSerializer<F, MsgPackSerializer&>
    void serializeMap(F&& f) {
        // for message pack map serialization, we need the size in advance
        SerializeCounter counter;
        std::forward<F>(f)(counter);
        packer.pack_map(counter.count);
        std::forward<F>(f)(*this);
    }

    template <typename F>
    requires InvocableSerializer<F, MsgPackSerializer&>
    void serializeVector(F&& f) {
        // for message pack array serialization, we need the size in advance
        SerializeCounter counter;
        std::forward<F>(f)(counter);
        packer.pack_array(counter.count);
        std::forward<F>(f)(*this);
    }
};

class MsgPack {
public:
    using BufferType = MsgPackBuffer;
    using Serializer = MsgPackSerializer;
    using SerializingFunction = MsgPackSerializer::SerializingFunction;

    static inline constexpr const char* Name = "MsgPack";
};

} // namespace zerialize
