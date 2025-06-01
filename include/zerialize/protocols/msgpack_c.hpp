#pragma once

#include <zerialize/zerialize.hpp>
#include <msgpack.h>

namespace zerialize {

// Buffer for MessagePack serialization
using MsgPackStream = msgpack_sbuffer;

class MsgPackBuffer : public DataBuffer<MsgPackBuffer> {
private:
    vector<uint8_t> buf_;
    msgpack_unpacked unpacked_;
    msgpack_object obj_;
    bool initialized_ = false;

public:
    MsgPackBuffer() {
        msgpack_unpacked_init(&unpacked_);
    }

    ~MsgPackBuffer() {
        if (initialized_) {
            msgpack_unpacked_destroy(&unpacked_);
        }
    }

    // Constructor from msgpack_object
    MsgPackBuffer(const msgpack_object& obj): obj_(obj) {
        msgpack_unpacked_init(&unpacked_);
        initialized_ = true;
    }

    // Zero-copy view of existing data
    MsgPackBuffer(span<const uint8_t> data) {
        msgpack_unpacked_init(&unpacked_);
        if (data.size() > 0) {
            msgpack_unpack_return ret = msgpack_unpack_next(
                &unpacked_, 
                reinterpret_cast<const char*>(data.data()), 
                data.size(), 
                NULL
            );
            if (ret == MSGPACK_UNPACK_SUCCESS) {
                obj_ = unpacked_.data;
                initialized_ = true;
            }
        }
    }

    // Zero-copy move of vector ownership
    MsgPackBuffer(vector<uint8_t>&& buf): buf_(std::move(buf)) {
        msgpack_unpacked_init(&unpacked_);
        if (buf_.size() > 0) {
            msgpack_unpack_return ret = msgpack_unpack_next(
                &unpacked_, 
                reinterpret_cast<const char*>(buf_.data()), 
                buf_.size(), 
                NULL
            );
            if (ret == MSGPACK_UNPACK_SUCCESS) {
                obj_ = unpacked_.data;
                initialized_ = true;
            }
        }
    }

    // Must copy for const reference
    MsgPackBuffer(const vector<uint8_t>& buf): buf_(buf) {
        msgpack_unpacked_init(&unpacked_);
        if (buf_.size() > 0) {
            msgpack_unpack_return ret = msgpack_unpack_next(
                &unpacked_, 
                reinterpret_cast<const char*>(buf_.data()), 
                buf_.size(), 
                NULL
            );
            if (ret == MSGPACK_UNPACK_SUCCESS) {
                obj_ = unpacked_.data;
                initialized_ = true;
            }
        }
    }

    // Copy constructor
    MsgPackBuffer(const MsgPackBuffer& other): buf_(other.buf_), obj_(other.obj_) {
        msgpack_unpacked_init(&unpacked_);
        if (buf_.size() > 0) {
            msgpack_unpack_return ret = msgpack_unpack_next(
                &unpacked_, 
                reinterpret_cast<const char*>(buf_.data()), 
                buf_.size(), 
                NULL
            );
            if (ret == MSGPACK_UNPACK_SUCCESS) {
                initialized_ = true;
            }
        }
    }

    // Move constructor
    MsgPackBuffer(MsgPackBuffer&& other) noexcept 
        : buf_(std::move(other.buf_)), obj_(other.obj_), initialized_(other.initialized_) {
        msgpack_unpacked_init(&unpacked_);
        if (initialized_) {
            // We need to re-unpack since we can't move msgpack_unpacked
            if (buf_.size() > 0) {
                msgpack_unpack_next(
                    &unpacked_, 
                    reinterpret_cast<const char*>(buf_.data()), 
                    buf_.size(), 
                    NULL
                );
            }
        }
        other.initialized_ = false;
    }

    // Assignment operator
    MsgPackBuffer& operator=(const MsgPackBuffer& other) {
        if (this != &other) {
            if (initialized_) {
                msgpack_unpacked_destroy(&unpacked_);
            }
            
            buf_ = other.buf_;
            obj_ = other.obj_;
            
            msgpack_unpacked_init(&unpacked_);
            if (buf_.size() > 0) {
                msgpack_unpack_return ret = msgpack_unpack_next(
                    &unpacked_, 
                    reinterpret_cast<const char*>(buf_.data()), 
                    buf_.size(), 
                    NULL
                );
                if (ret == MSGPACK_UNPACK_SUCCESS) {
                    initialized_ = true;
                }
            }
        }
        return *this;
    }

    // Move assignment operator
    MsgPackBuffer& operator=(MsgPackBuffer&& other) noexcept {
        if (this != &other) {
            if (initialized_) {
                msgpack_unpacked_destroy(&unpacked_);
            }
            
            buf_ = std::move(other.buf_);
            obj_ = other.obj_;
            initialized_ = other.initialized_;
            
            msgpack_unpacked_init(&unpacked_);
            if (initialized_ && buf_.size() > 0) {
                msgpack_unpack_next(
                    &unpacked_, 
                    reinterpret_cast<const char*>(buf_.data()), 
                    buf_.size(), 
                    NULL
                );
            }
            
            other.initialized_ = false;
        }
        return *this;
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
        return obj_.type == MSGPACK_OBJECT_NIL; 
    }

    bool isInt() const { 
        return obj_.type == MSGPACK_OBJECT_NEGATIVE_INTEGER || obj_.type == MSGPACK_OBJECT_POSITIVE_INTEGER;
    }
    
    bool isUInt() const { 
        return obj_.type == MSGPACK_OBJECT_POSITIVE_INTEGER;
    }
    
    bool isFloat() const { 
        return obj_.type == MSGPACK_OBJECT_FLOAT || obj_.type == MSGPACK_OBJECT_FLOAT32;
    }
    
    bool isBool() const { 
        return obj_.type == MSGPACK_OBJECT_BOOLEAN;
    }
    
    bool isString() const { 
        return obj_.type == MSGPACK_OBJECT_STR;
    }
    
    bool isBlob() const { 
        return obj_.type == MSGPACK_OBJECT_BIN;
    }
    
    bool isMap() const { 
        return obj_.type == MSGPACK_OBJECT_MAP;
    }
    
    bool isArray() const { 
        return obj_.type == MSGPACK_OBJECT_ARRAY;
    }

    int8_t asInt8() const {
        if (!isInt() && !isUInt()) { throw DeserializationError("not an int"); }
        if (obj_.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            return static_cast<int8_t>(obj_.via.u64);
        } else {
            return static_cast<int8_t>(obj_.via.i64);
        }
    }

    int16_t asInt16() const {
        if (!isInt() && !isUInt()) { throw DeserializationError("not an int"); }
        if (obj_.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            return static_cast<int16_t>(obj_.via.u64);
        } else {
            return static_cast<int16_t>(obj_.via.i64);
        }
    }

    int32_t asInt32() const {
        if (!isInt() && !isUInt()) { throw DeserializationError("not an int"); }
        if (obj_.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            return static_cast<int32_t>(obj_.via.u64);
        } else {
            return static_cast<int32_t>(obj_.via.i64);
        }
    }

    int64_t asInt64() const {
        if (!isInt() && !isUInt()) { throw DeserializationError("not an int"); }
        if (obj_.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            return static_cast<int64_t>(obj_.via.u64);
        } else {
            return obj_.via.i64;
        }
    }

    uint8_t asUInt8() const {
        if (!isUInt() && !isInt()) { throw DeserializationError("not a uint"); }
        if (obj_.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            return static_cast<uint8_t>(obj_.via.u64);
        } else {
            return static_cast<uint8_t>(obj_.via.i64);
        }
    }

    uint16_t asUInt16() const {
        if (!isUInt() && !isInt()) { throw DeserializationError("not a uint"); }
        if (obj_.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            return static_cast<uint16_t>(obj_.via.u64);
        } else {
            return static_cast<uint16_t>(obj_.via.i64);
        }
    }

    uint32_t asUInt32() const {
        if (!isUInt() && !isInt()) { throw DeserializationError("not a uint"); }
        if (obj_.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            return static_cast<uint32_t>(obj_.via.u64);
        } else {
            return static_cast<uint32_t>(obj_.via.i64);
        }
    }

    uint64_t asUInt64() const {
        if (!isUInt() && !isInt()) { throw DeserializationError("not a uint"); }
        if (obj_.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
            return obj_.via.u64;
        } else {
            return static_cast<uint64_t>(obj_.via.i64);
        }
    }

    float asFloat() const {
        if (!isFloat()) { throw DeserializationError("not a float"); }
        if (obj_.type == MSGPACK_OBJECT_FLOAT32) {
            return obj_.via.f64;  // In C API, both float32 and float64 use f64 field
        } else {
            return static_cast<float>(obj_.via.f64);
        }
    }

    double asDouble() const {
        if (!isFloat()) { throw DeserializationError("not a float"); }
        return obj_.via.f64;
    }

    bool asBool() const {
        if (!isBool()) { throw DeserializationError("not a bool"); }
        return obj_.via.boolean;
    }

    string asString() const {
        if (!isString()) { throw DeserializationError("not a string"); }
        return string(obj_.via.str.ptr, obj_.via.str.size);
    }

    string_view asStringView() const {
        if (!isString()) { throw DeserializationError("not a string"); }
        return string_view(obj_.via.str.ptr, obj_.via.str.size);
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
            const msgpack_object& key = obj_.via.map.ptr[i].key;
            if (key.type != MSGPACK_OBJECT_STR) {
                throw DeserializationError("map key is not a string");
            }
            keys.insert(string_view(key.via.str.ptr, key.via.str.size));
        }
        return keys;
    }

    MsgPackBuffer operator[] (const string& key) const {
        if (!isMap()) { throw DeserializationError("not a map"); }
        for (uint32_t i = 0; i < obj_.via.map.size; ++i) {
            const msgpack_object& k = obj_.via.map.ptr[i].key;
            if (k.type == MSGPACK_OBJECT_STR) {
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

class MsgPackRootSerializer {
public:
    MsgPackStream sbuf;

    MsgPackRootSerializer() {
        msgpack_sbuffer_init(&sbuf);
    }

    ~MsgPackRootSerializer() {
        msgpack_sbuffer_destroy(&sbuf);
    }

    MsgPackBuffer finish() {
        if (sbuf.size > 0) {
            size_t size = sbuf.size;
            uint8_t* data = reinterpret_cast<uint8_t*>(sbuf.data);
            
            // Create a copy of the data since we're going to destroy the sbuffer
            std::vector<uint8_t> vec(data, data + size);
            
            // Reset the sbuffer without freeing the data (we've copied it)
            sbuf.size = 0;
            sbuf.data = nullptr;
            sbuf.alloc = 0;
            
            return MsgPackBuffer(std::move(vec));
        }
        return MsgPackBuffer();
    }
};

class MsgPackSerializer: public Serializer<MsgPackSerializer> {
private:
    msgpack_packer packer;
    MsgPackStream* pack_stream;

public:
    // Make the base class overloads visible in the derived class
    using Serializer<MsgPackSerializer>::serialize;

    MsgPackSerializer(MsgPackRootSerializer& mb) {
        pack_stream = &mb.sbuf;
        msgpack_packer_init(&packer, pack_stream, msgpack_sbuffer_write);
    }

    MsgPackSerializer(MsgPackStream* ps) {
        pack_stream = ps;
        msgpack_packer_init(&packer, pack_stream, msgpack_sbuffer_write);
    }

    void serialize(std::nullptr_t) { 
        msgpack_pack_nil(&packer); 
    }

    void serialize(int8_t val) { 
        msgpack_pack_int8(&packer, val); 
    }
    
    void serialize(int16_t val) { 
        msgpack_pack_int16(&packer, val); 
    }
    
    void serialize(int32_t val) { 
        msgpack_pack_int32(&packer, val); 
    }
    
    void serialize(int64_t val) { 
        msgpack_pack_int64(&packer, val); 
    }

    void serialize(uint8_t val) { 
        msgpack_pack_uint8(&packer, val); 
    }
    
    void serialize(uint16_t val) { 
        msgpack_pack_uint16(&packer, val); 
    }
    
    void serialize(uint32_t val) { 
        msgpack_pack_uint32(&packer, val); 
    }
    
    void serialize(uint64_t val) { 
        msgpack_pack_uint64(&packer, val); 
    }

    void serialize(bool val) { 
        if (val) {
            msgpack_pack_true(&packer);
        } else {
            msgpack_pack_false(&packer);
        }
    }
    
    void serialize(double val) { 
        msgpack_pack_double(&packer, val); 
    }
    
    void serialize(const char* val) { 
        size_t len = strlen(val);
        msgpack_pack_str(&packer, len);
        msgpack_pack_str_body(&packer, val, len);
    }
    
    void serialize(const string& val) { 
        msgpack_pack_str(&packer, val.size());
        msgpack_pack_str_body(&packer, val.data(), val.size());
    }

    void serialize(const span<const uint8_t>& val) { 
        msgpack_pack_bin(&packer, val.size());
        msgpack_pack_bin_body(&packer, reinterpret_cast<const char*>(val.data()), val.size());
    }

    template<typename T, typename = enable_if_t<is_convertible_v<T, string_view>>>
    void serialize(T&& val) {
        string str(std::forward<T>(val));
        msgpack_pack_str(&packer, str.size());
        msgpack_pack_str_body(&packer, str.data(), str.size());
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
        msgpack_pack_map(&packer, m.size());
        for (const auto& [key, value]: m) {
            serialize(key, value);
        }
    }

    void serialize(const vector<any>& l) {
        msgpack_pack_array(&packer, l.size());
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
        msgpack_pack_map(&packer, counter.count);
        std::forward<F>(f)(*this);
    }

    template <typename F>
    requires InvocableSerializer<F, MsgPackSerializer&>
    void serializeVector(F&& f) {
        // for message pack array serialization, we need the size in advance
        SerializeCounter counter;
        std::forward<F>(f)(counter);
        msgpack_pack_array(&packer, counter.count);
        std::forward<F>(f)(*this);
    }
};

class MsgPack {
public:
    using BufferType = MsgPackBuffer;
    using Serializer = MsgPackSerializer;
    using RootSerializer = MsgPackRootSerializer;
    using SerializingFunction = MsgPackSerializer::SerializingFunction;

    static inline constexpr const char* Name = "MsgPack";
};

} // namespace zerialize
