#pragma once

#include <zerialize/zerialize.hpp>
#include <msgpack.h>

namespace zerialize {

// Helpers for reading big-endian numbers from a byte array.
inline uint16_t read_be16(const uint8_t* data) {
    return (static_cast<uint16_t>(data[0]) << 8) | data[1];
}

inline uint32_t read_be32(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8)  |
           data[3];
}

inline uint64_t read_be64(const uint8_t* data) {
    return (static_cast<uint64_t>(data[0]) << 56) |
           (static_cast<uint64_t>(data[1]) << 48) |
           (static_cast<uint64_t>(data[2]) << 40) |
           (static_cast<uint64_t>(data[3]) << 32) |
           (static_cast<uint64_t>(data[4]) << 24) |
           (static_cast<uint64_t>(data[5]) << 16) |
           (static_cast<uint64_t>(data[6]) << 8)  |
           data[7];
}

// Forward declaration: given a span starting at a MessagePack element,
// return how many bytes that element occupies.
inline size_t skip_element(span<const uint8_t> view);

inline size_t skip_element(span<const uint8_t> view) {
    if (view.empty()) throw DeserializationError("empty view in skip_element");
    uint8_t marker = view[0];
    size_t offset = 0;
    // Single-byte types: positive fixint, negative fixint, nil, booleans.
    if ((marker <= 0x7f) || (marker >= 0xe0) || marker == 0xc0 || marker == 0xc2 || marker == 0xc3)
        return 1;
    // fixstr: marker 0xa0-0xbf; lower 5 bits = length.
    if (marker >= 0xa0 && marker <= 0xbf) {
        size_t len = marker & 0x1f;
        return 1 + len;
    }
    // fixarray: marker 0x90-0x9f.
    if (marker >= 0x90 && marker <= 0x9f) {
        size_t count = marker & 0x0f;
        offset = 1;
        for (size_t i = 0; i < count; i++) {
            size_t elemSize = skip_element(span<const uint8_t>(view.data() + offset, view.size() - offset));
            offset += elemSize;
        }
        return offset;
    }
    // fixmap: marker 0x80-0x8f.
    if (marker >= 0x80 && marker <= 0x8f) {
        size_t count = marker & 0x0f;
        offset = 1;
        for (size_t i = 0; i < count; i++) {
            size_t keySize = skip_element(span<const uint8_t>(view.data() + offset, view.size() - offset));
            offset += keySize;
            size_t valSize = skip_element(span<const uint8_t>(view.data() + offset, view.size() - offset));
            offset += valSize;
        }
        return offset;
    }
    // Other markers:
    switch (marker) {
        case 0xcc: // uint8
        case 0xd0: // int8
            return 2;
        case 0xcd: // uint16
        case 0xd1: // int16
            return 3;
        case 0xce: // uint32
        case 0xd2: // int32
        case 0xca: // float32
            return 5;
        case 0xcf: // uint64
        case 0xd3: // int64
        case 0xcb: // float64
            return 9;
        case 0xd9: { // str8
            if (view.size() < 2) throw DeserializationError("insufficient data for str8");
            size_t len = view[1];
            return 2 + len;
        }
        case 0xda: { // str16
            if (view.size() < 3) throw DeserializationError("insufficient data for str16");
            size_t len = read_be16(view.data() + 1);
            return 3 + len;
        }
        case 0xdb: { // str32
            if (view.size() < 5) throw DeserializationError("insufficient data for str32");
            size_t len = read_be32(view.data() + 1);
            return 5 + len;
        }
        case 0xdc: { // array16
            if (view.size() < 3) throw DeserializationError("insufficient data for array16");
            size_t count = read_be16(view.data() + 1);
            offset = 3;
            for (size_t i = 0; i < count; i++) {
                size_t elemSize = skip_element(span<const uint8_t>(view.data() + offset, view.size() - offset));
                offset += elemSize;
            }
            return offset;
        }
        case 0xdd: { // array32
            if (view.size() < 5) throw DeserializationError("insufficient data for array32");
            size_t count = read_be32(view.data() + 1);
            offset = 5;
            for (size_t i = 0; i < count; i++) {
                size_t elemSize = skip_element(span<const uint8_t>(view.data() + offset, view.size() - offset));
                offset += elemSize;
            }
            return offset;
        }
        case 0xde: { // map16
            if (view.size() < 3) throw DeserializationError("insufficient data for map16");
            size_t count = read_be16(view.data() + 1);
            offset = 3;
            for (size_t i = 0; i < count; i++) {
                size_t keySize = skip_element(span<const uint8_t>(view.data() + offset, view.size() - offset));
                offset += keySize;
                size_t valSize = skip_element(span<const uint8_t>(view.data() + offset, view.size() - offset));
                offset += valSize;
            }
            return offset;
        }
        case 0xdf: { // map32
            if (view.size() < 5) throw DeserializationError("insufficient data for map32");
            size_t count = read_be32(view.data() + 1);
            offset = 5;
            for (size_t i = 0; i < count; i++) {
                size_t keySize = skip_element(span<const uint8_t>(view.data() + offset, view.size() - offset));
                offset += keySize;
                size_t valSize = skip_element(span<const uint8_t>(view.data() + offset, view.size() - offset));
                offset += valSize;
            }
            return offset;
        }
        case 0xc4: { // bin8
            if (view.size() < 2) throw DeserializationError("insufficient data for bin8");
            size_t len = view[1];
            return 2 + len;
        }
        case 0xc5: { // bin16
            if (view.size() < 3) throw DeserializationError("insufficient data for bin16");
            size_t len = read_be16(view.data() + 1);
            return 3 + len;
        }
        case 0xc6: { // bin32
            if (view.size() < 5) throw DeserializationError("insufficient data for bin32");
            size_t len = read_be32(view.data() + 1);
            return 5 + len;
        }
        default:
            throw DeserializationError("unsupported marker in skip_element");
    }
}

// A minimal MsgPackDeserializer that parses MessagePack data dynamically.
// The root buffer owns its data and sets its view to cover it; non-root buffers
// have only a view over the appropriate subrange.
// class MsgPackDeserializer : public DataBuffer<MsgPackDeserializer> {
class MsgPackDeserializer : public Deserializer<MsgPackDeserializer> {
public:
    MsgPackDeserializer() 
        : Deserializer<MsgPackDeserializer>() {}

    MsgPackDeserializer(span<const uint8_t> data)
        : Deserializer<MsgPackDeserializer>(data)
    {}

    MsgPackDeserializer(vector<uint8_t>&& buf)
        : Deserializer<MsgPackDeserializer>(std::move(buf))
    {}

    MsgPackDeserializer(const vector<uint8_t>& buf)
        : Deserializer<MsgPackDeserializer>(std::move(buf))
    {}

    string to_string() const override {
        return "MsgPackDeserializer " + std::to_string(buf().size()) +
            " bytes at: " + std::format("{}", static_cast<const void*>(buf().data())) +
            "\n" + debug_string(*this);
    }

    // --- Type predicates ---

    bool isNull() const {
        if (view_.empty()) return false;
        return view_[0] == 0xc0;
    }

    bool isBool() const {
        if (view_.empty()) return false;
        return (view_[0] == 0xc2 || view_[0] == 0xc3);
    }

    bool isInt() const {
        if (view_.empty()) return false;
        uint8_t marker = view_[0];
        // Positive fixint or negative fixint.
        if (marker <= 0x7f || marker >= 0xe0) return true;
        return (marker == 0xd0 || marker == 0xd1 || marker == 0xd2 || marker == 0xd3);
    }

    bool isUInt() const {
        if (view_.empty()) return false;
        uint8_t marker = view_[0];
        if (marker <= 0x7f) return true;
        return (marker == 0xcc || marker == 0xcd || marker == 0xce || marker == 0xcf);
    }

    bool isFloat() const {
        if (view_.empty()) return false;
        uint8_t marker = view_[0];
        return (marker == 0xca || marker == 0xcb);
    }

    bool isString() const {
        if (view_.empty()) return false;
        uint8_t marker = view_[0];
        if (marker >= 0xa0 && marker <= 0xbf) return true; // fixstr
        return (marker == 0xd9 || marker == 0xda || marker == 0xdb);
    }

    bool isBlob() const { 
        if (view_.empty()) return false;
        uint8_t marker = view_[0];
        return (marker == 0xc4 || marker == 0xc5 || marker == 0xc6);
    }

    bool isArray() const {
        if (view_.empty()) return false;
        uint8_t marker = view_[0];
        if (marker >= 0x90 && marker <= 0x9f) return true;
        return (marker == 0xdc || marker == 0xdd);
    }

    bool isMap() const {
        if (view_.empty()) return false;
        uint8_t marker = view_[0];
        if (marker >= 0x80 && marker <= 0x8f) return true;
        return (marker == 0xde || marker == 0xdf);
    }

    // --- Conversion methods ---
    // Each method checks that the type marker matches, then reads the appropriate bytes.
    int8_t asInt8() const {
        if (!(isInt() || isUInt())) throw DeserializationError("MsgPackDeserializer: not an int (asInt8)");
        uint8_t marker = view_[0];
        if (marker <= 0x7f) return static_cast<int8_t>(marker);
        if (marker >= 0xe0) return static_cast<int8_t>(marker);
        if (marker == 0xd0 || marker == 0xcc) {
            if (view_.size() < 2) throw DeserializationError("insufficient data for int8");
            return static_cast<int8_t>(view_[1]);
        }
        throw DeserializationError("MsgPackDeserializer: not int8");
    }

    int16_t asInt16() const {
        if (!(isInt() || isUInt())) throw DeserializationError("MsgPackDeserializer: not an int (asInt16)");
        uint8_t marker = view_[0];
        if (marker == 0xd1 || marker == 0xcd) {
            if (view_.size() < 3) throw DeserializationError("insufficient data for int16");
            return static_cast<int16_t>(read_be16(view_.data() + 1));
        }
        return static_cast<int16_t>(asInt8());
    }

    int32_t asInt32() const {
        if (!(isInt() || isUInt())) throw DeserializationError("MsgPackDeserializer: not an int (asInt32)");
        uint8_t marker = view_[0];
        if (marker == 0xd2 || marker == 0xce) {
            if (view_.size() < 5) throw DeserializationError("insufficient data for int32");
            return static_cast<int32_t>(read_be32(view_.data() + 1));
        }
        return static_cast<int32_t>(asInt16());
    }

    int64_t asInt64() const {
        if (!(isInt() || isUInt())) throw DeserializationError("MsgPackDeserializer: not an int (asInt64)");
        uint8_t marker = view_[0];
        if (marker == 0xd3 || marker == 0xcf) {
            if (view_.size() < 9) throw DeserializationError("insufficient data for int64");
            return static_cast<int64_t>(read_be64(view_.data() + 1));
        }
        return static_cast<int64_t>(asInt32());
    }

    uint8_t asUInt8() const {
        if (!isUInt()) throw DeserializationError("MsgPackDeserializer: not a uint (asUInt8)");
        uint8_t marker = view_[0];
        if (marker <= 0x7f) return marker;
        if (marker == 0xcc) {
            if (view_.size() < 2) throw DeserializationError("insufficient data for uint8");
            return view_[1];
        }
        throw DeserializationError("MsgPackDeserializer: not uint8");
    }

    uint16_t asUInt16() const {
        if (!isUInt()) throw DeserializationError("MsgPackDeserializer: not a uint (asUInt16)");
        uint8_t marker = view_[0];
        if (marker == 0xcd) {
            if (view_.size() < 3) throw DeserializationError("insufficient data for uint16");
            return read_be16(view_.data() + 1);
        }
        return static_cast<uint16_t>(asUInt8());
    }

    uint32_t asUInt32() const {
        if (!isUInt()) throw DeserializationError("MsgPackDeserializer: not a uint (asUInt32)");
        uint8_t marker = view_[0];
        if (marker == 0xce) {
            if (view_.size() < 5) throw DeserializationError("insufficient data for uint32");
            return read_be32(view_.data() + 1);
        }
        return static_cast<uint32_t>(asUInt16());
    }

    uint64_t asUInt64() const {
        if (!isUInt()) throw DeserializationError("MsgPackDeserializer: not a uint (asUInt64)");
        uint8_t marker = view_[0];
        if (marker == 0xcf) {
            if (view_.size() < 9) throw DeserializationError("insufficient data for uint64");
            return read_be64(view_.data() + 1);
        }
        return static_cast<uint64_t>(asUInt32());
    }

    float asFloat() const {
        if (!isFloat()) throw DeserializationError("MsgPackDeserializer: not a float");
        uint8_t marker = view_[0];
        if (marker == 0xca) {
            if (view_.size() < 5) throw DeserializationError("insufficient data for float32");
            uint32_t be = read_be32(view_.data() + 1);
            float f;
            std::memcpy(&f, &be, sizeof(float));
            return f;
        }
        if (marker == 0xcb) {
            if (view_.size() < 9) throw DeserializationError("insufficient data for float64");
            uint64_t be = read_be64(view_.data() + 1);
            double d;
            std::memcpy(&d, &be, sizeof(double));
            return static_cast<float>(d);
        }
        throw DeserializationError("MsgPackDeserializer: unknown float type");
    }

    double asDouble() const {
        if (!isFloat()) throw DeserializationError("MsgPackDeserializer: not a float");
        uint8_t marker = view_[0];
        if (marker == 0xcb) {
            if (view_.size() < 9) throw DeserializationError("insufficient data for float64");
            uint64_t be = read_be64(view_.data() + 1);
            double d;
            std::memcpy(&d, &be, sizeof(double));
            return d;
        }
        if (marker == 0xca) {
            return static_cast<double>(asFloat());
        }
        throw DeserializationError("MsgPackDeserializer: unknown float type");
    }

    bool asBool() const {
        if (!isBool()) throw DeserializationError("MsgPackDeserializer: not a bool");
        return view_[0] == 0xc3;
    }

    string asString() const {
        if (!isString()) throw DeserializationError("MsgPackDeserializer: not a string");
        uint8_t marker = view_[0];
        size_t str_len = 0;
        const uint8_t* str_ptr = nullptr;
        if (marker >= 0xa0 && marker <= 0xbf) {
            str_len = marker & 0x1f;
            str_ptr = view_.data() + 1;
        } else if (marker == 0xd9) {
            if (view_.size() < 2) throw DeserializationError("insufficient data for str8");
            str_len = view_[1];
            str_ptr = view_.data() + 2;
        } else if (marker == 0xda) {
            if (view_.size() < 3) throw DeserializationError("insufficient data for str16");
            str_len = read_be16(view_.data() + 1);
            str_ptr = view_.data() + 3;
        } else if (marker == 0xdb) {
            if (view_.size() < 5) throw DeserializationError("insufficient data for str32");
            str_len = read_be32(view_.data() + 1);
            str_ptr = view_.data() + 5;
        } else {
            throw DeserializationError("MsgPackDeserializer: unknown string type");
        }
        return string(reinterpret_cast<const char*>(str_ptr), str_len);
    }

    string_view asStringView() const {
        if (!isString()) throw DeserializationError("MsgPackDeserializer: not a string");
        uint8_t marker = view_[0];
        size_t str_len = 0;
        const uint8_t* str_ptr = nullptr;
        if (marker >= 0xa0 && marker <= 0xbf) {
            str_len = marker & 0x1f;
            str_ptr = view_.data() + 1;
        } else if (marker == 0xd9) {
            if (view_.size() < 2) throw DeserializationError("insufficient data for str8");
            str_len = view_[1];
            str_ptr = view_.data() + 2;
        } else if (marker == 0xda) {
            if (view_.size() < 3) throw DeserializationError("insufficient data for str16");
            str_len = read_be16(view_.data() + 1);
            str_ptr = view_.data() + 3;
        } else if (marker == 0xdb) {
            if (view_.size() < 5) throw DeserializationError("insufficient data for str32");
            str_len = read_be32(view_.data() + 1);
            str_ptr = view_.data() + 5;
        } else {
            throw DeserializationError("MsgPackDeserializer: unknown string type");
        }
        return string_view(reinterpret_cast<const char*>(str_ptr), str_len);
    }

    // For simplicity, asBlob() supports only the bin types.
    span<const uint8_t> asBlob() const {
        if (view_.empty()) throw DeserializationError("MsgPackDeserializer: empty blob");
        uint8_t marker = view_[0];
        size_t blob_len = 0;
        const uint8_t* blob_ptr = nullptr;
        if (marker == 0xc4) {
            if (view_.size() < 2) throw DeserializationError("insufficient data for bin8");
            blob_len = view_[1];
            blob_ptr = view_.data() + 2;
        } else if (marker == 0xc5) {
            if (view_.size() < 3) throw DeserializationError("insufficient data for bin16");
            blob_len = read_be16(view_.data() + 1);
            blob_ptr = view_.data() + 3;
        } else if (marker == 0xc6) {
            if (view_.size() < 5) throw DeserializationError("insufficient data for bin32");
            blob_len = read_be32(view_.data() + 1);
            blob_ptr = view_.data() + 5;
        } else {
            throw DeserializationError("MsgPackDeserializer: unsupported blob type");
        }
        return span<const uint8_t>(blob_ptr, blob_len);
    }

    // Map: get the keys in the map.
    std::set<string_view> mapKeys() const {
        if (!isMap()) throw DeserializationError("MsgPackDeserializer: not a map");
        std::set<string_view> keys;
        size_t offset = 0;
        uint8_t marker = view_[0];
        size_t map_size = 0;
        if (marker >= 0x80 && marker <= 0x8f) {
            map_size = marker & 0x0f;
            offset = 1;
        } else if (marker == 0xde) {
            if (view_.size() < 3) throw DeserializationError("insufficient data for map16");
            map_size = read_be16(view_.data() + 1);
            offset = 3;
        } else if (marker == 0xdf) {
            if (view_.size() < 5) throw DeserializationError("insufficient data for map32");
            map_size = read_be32(view_.data() + 1);
            offset = 5;
        } else {
            throw DeserializationError("MsgPackDeserializer: unknown map type");
        }
        for (size_t i = 0; i < map_size; i++) {
            MsgPackDeserializer keyBuffer(span<const uint8_t>(view_.data() + offset, view_.size() - offset));
            size_t keySize = skip_element(keyBuffer.view_);
            string_view keyStr = keyBuffer.asStringView();
            offset += keySize;
            size_t valSize =skip_element(span<const uint8_t>(view_.data() + offset, view_.size() - offset));
            // (We don’t use the value here; just collect the key.)
            keys.insert(keyStr);
            offset += valSize;
        }
        return keys;
    }

    size_t arraySize() const {
        if (!isArray()) throw DeserializationError("MsgPackDeserializer: not an array");
        uint8_t marker = view_[0];
        size_t array_size = 0;
        if (marker >= 0x90 && marker <= 0x9f) {
            array_size = marker & 0x0f;
        } else if (marker == 0xdc) {
            if (view_.size() < 3) throw DeserializationError("insufficient data for array16");
            array_size = read_be16(view_.data() + 1);
        } else if (marker == 0xdd) {
            if (view_.size() < 5) throw DeserializationError("insufficient data for array32");
            array_size = read_be32(view_.data() + 1);
        } else {
            throw DeserializationError("MsgPackDeserializer: unknown array type");
        }
        return array_size;
    }

    // Array indexing: returns the element at the given index.
    MsgPackDeserializer operator[](size_t index) const {
        if (!isArray()) throw DeserializationError("MsgPackDeserializer: not an array");
        size_t offset = 0;
        uint8_t marker = view_[0];
        size_t array_size = 0;
        if (marker >= 0x90 && marker <= 0x9f) {
            array_size = marker & 0x0f;
            offset = 1;
        } else if (marker == 0xdc) {
            if (view_.size() < 3) throw DeserializationError("insufficient data for array16");
            array_size = read_be16(view_.data() + 1);
            offset = 3;
        } else if (marker == 0xdd) {
            if (view_.size() < 5) throw DeserializationError("insufficient data for array32");
            array_size = read_be32(view_.data() + 1);
            offset = 5;
        } else {
            throw DeserializationError("MsgPackDeserializer: unknown array type");
        }
        if (index >= array_size) throw DeserializationError("MsgPackDeserializer: array index out of bounds");
        for (size_t i = 0; i < index; i++) {
            size_t elemSize = skip_element(span<const uint8_t>(view_.data() + offset, view_.size() - offset));
            offset += elemSize;
        }
        size_t elemSize = skip_element(span<const uint8_t>(view_.data() + offset, view_.size() - offset));
        return MsgPackDeserializer(span<const uint8_t>(view_.data() + offset, elemSize));
    }

    // Map indexing: returns the value corresponding to the given key.
    MsgPackDeserializer operator[](const string_view& key) const {
        if (!isMap()) throw DeserializationError("MsgPackDeserializer: not a map");
        size_t offset = 0;
        uint8_t marker = view_[0];
        size_t map_size = 0;
        if (marker >= 0x80 && marker <= 0x8f) {
            map_size = marker & 0x0f;
            offset = 1;
        } else if (marker == 0xde) {
            if (view_.size() < 3) throw DeserializationError("insufficient data for map16");
            map_size = read_be16(view_.data() + 1);
            offset = 3;
        } else if (marker == 0xdf) {
            if (view_.size() < 5) throw DeserializationError("insufficient data for map32");
            map_size = read_be32(view_.data() + 1);
            offset = 5;
        } else {
            throw DeserializationError("MsgPackDeserializer: unknown map type");
        }
        for (size_t i = 0; i < map_size; i++) {
            MsgPackDeserializer keyBuffer(span<const uint8_t>(view_.data() + offset, view_.size() - offset));
            size_t keySize = skip_element(keyBuffer.view_);
            string_view keyStr = keyBuffer.asStringView();
            offset += keySize;
            size_t valSize = skip_element(span<const uint8_t>(view_.data() + offset, view_.size() - offset));
            if (string(keyStr) == key) {
                return MsgPackDeserializer(span<const uint8_t>(view_.data() + offset, valSize));
            }
            offset += valSize;
        }
        throw DeserializationError("MsgPackDeserializer: key not found in map: " + string(key));
    }
};

class MsgPackRootSerializer {
public:
    msgpack_sbuffer sbuf;
    msgpack_packer packer;

    MsgPackRootSerializer() {
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer_init(&packer, &sbuf, msgpack_sbuffer_write);
    }

    ~MsgPackRootSerializer() {
        msgpack_sbuffer_destroy(&sbuf);
    }

    ZBuffer finish() {
        if (sbuf.size > 0) {

            // Take ownership of sbuf.data
            size_t size = sbuf.size;
            char* data = sbuf.data;

            // Prevent sbuffer from double-freeing
            sbuf.data = nullptr;
            sbuf.size = 0;
            sbuf.alloc = 0;

            return ZBuffer(data, size, ZBuffer::Deleters::Free);
        }
        return ZBuffer();
    }
};

// NOTE! Marking the serialization functions as 'noexcept' speeds it up
// by ~30% according to one measurement in benchmark_compare. But, do
// we really want that?

class MsgPackSerializer: public Serializer<MsgPackSerializer> {
private:
    msgpack_packer& packer;

public:
    // Make the base class overloads visible in the derived class
    using Serializer<MsgPackSerializer>::serialize;

    MsgPackSerializer(MsgPackRootSerializer& mb) noexcept 
        : packer(mb.packer)  {}

    MsgPackSerializer(msgpack_packer& ps) noexcept 
        : packer(ps) {}

    inline void serialize(std::nullptr_t) noexcept { 
        msgpack_pack_nil(&packer); 
    }

    inline void serialize(int8_t val) noexcept { 
        msgpack_pack_int8(&packer, val); 
    }
    
    inline void serialize(int16_t val) noexcept { 
        msgpack_pack_int16(&packer, val); 
    }
    
    inline void serialize(int32_t val) noexcept { 
        msgpack_pack_int32(&packer, val); 
    }
    
    inline void serialize(int64_t val) noexcept { 
        msgpack_pack_int64(&packer, val); 
    }

    inline void serialize(uint8_t val) noexcept { 
        msgpack_pack_uint8(&packer, val); 
    }
    
    inline void serialize(uint16_t val) noexcept { 
        msgpack_pack_uint16(&packer, val); 
    }
    
    inline void serialize(uint32_t val) noexcept { 
        msgpack_pack_uint32(&packer, val); 
    }
    
    inline void serialize(uint64_t val) noexcept { 
        msgpack_pack_uint64(&packer, val); 
    }

    inline void serialize(bool val) noexcept { 
        if (val) {
            msgpack_pack_true(&packer);
        } else {
            msgpack_pack_false(&packer);
        }
    }
    
    inline void serialize(double val) noexcept { 
        msgpack_pack_double(&packer, val); 
    }
    
    inline void serialize(string_view val) noexcept {
        msgpack_pack_str(&packer, val.size());
        msgpack_pack_str_body(&packer, val.data(), val.size());
    }

    inline void serialize(const string& val) noexcept {
        serialize(string_view(val));
    }

    inline void serialize(const char* val) noexcept {
        serialize(string_view(val));
    }

    inline void serialize(const span<const uint8_t>& val) noexcept { 
        msgpack_pack_bin(&packer, val.size());
        msgpack_pack_bin_body(&packer, reinterpret_cast<const char*>(val.data()), val.size());
    }

    template<typename T, typename = enable_if_t<is_convertible_v<T, string_view>>>
    inline void serialize(T&& val) noexcept {
        msgpack_pack_str(&packer, val.size());
        msgpack_pack_str_body(&packer, val.data(), val.size());
    }

    template <typename K, typename = std::enable_if_t<std::is_convertible_v<K, std::string_view>>>
    inline void serialize(const K& key, const any& value) noexcept {
        serialize(std::string_view(key));
        Serializer::serializeAny(value);
    }

    inline MsgPackSerializer serializerForKey(const string_view& key) noexcept {
        serialize(key);
        return *this;
        //return MsgPackSerializer(packer);
    }

    inline void serialize(const map<string, any>& m) noexcept {
        msgpack_pack_map(&packer, m.size());
        for (const auto& [key, value]: m) {
            serialize(key, value);
        }
    }

    inline void serialize(const vector<any>& l) noexcept {
        msgpack_pack_array(&packer, l.size());
        for (const auto& value: l) {
            serializeAny(value);
        }
    }

    template <typename F> requires InvocableSerializer<F, MsgPackSerializer&>
    inline void serialize(F&& f) noexcept {
        std::forward<F>(f)(*this);
    }

    template <typename F> requires InvocableSerializer<F, MsgPackSerializer&>
    inline void serializeMap(F&& f) noexcept {
        // for message pack map serialization, we need the size in advance
        SerializeCounter counter;
        std::forward<F>(f)(counter);
        msgpack_pack_map(&packer, counter.count);
        std::forward<F>(f)(*this);
    }

    template <typename F> requires InvocableSerializer<F, MsgPackSerializer&>
    inline void serializeVector(F&& f) noexcept {
        // for message pack array serialization, we need the size in advance
        SerializeCounter counter;
        std::forward<F>(f)(counter);
        msgpack_pack_array(&packer, counter.count);
        std::forward<F>(f)(*this);
    }
};

class MsgPack {
public:
    using BufferType = MsgPackDeserializer;
    using Serializer = MsgPackSerializer;
    using RootSerializer = MsgPackRootSerializer;
    using SerializingFunction = MsgPackSerializer::SerializingFunction;

    static inline constexpr const char* Name = "MsgPack";
};

} // namespace zerialize
