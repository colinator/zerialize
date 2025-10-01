#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <span>
#include <stdexcept>
#include <sstream>
#include <iterator>
#include <type_traits>

#include <msgpack.h> // for the writer
#include <zerialize/zbuffer.hpp>
#include <zerialize/errors.hpp>


namespace zerialize {

// ===== helpers (big-endian readers) ==========================================
inline uint16_t mp_read_be16(const uint8_t* p) {
    return (uint16_t(p[0]) << 8) | uint16_t(p[1]);
}
inline uint32_t mp_read_be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
inline uint64_t mp_read_be64(const uint8_t* p) {
    return (uint64_t(p[0]) << 56) | (uint64_t(p[1]) << 48) | (uint64_t(p[2]) << 40) | (uint64_t(p[3]) << 32) |
           (uint64_t(p[4]) << 24) | (uint64_t(p[5]) << 16) | (uint64_t(p[6]) << 8) | uint64_t(p[7]);
}

// Forward decl
inline size_t mp_skip(std::span<const uint8_t>);

// ===== minimal element skipper (for iterators / indexing) ====================
inline size_t mp_skip(std::span<const uint8_t> v) {
    if (v.empty()) throw DeserializationError("msgpack: empty in skip");
    const uint8_t m = v[0];

    // Single byte: nil/bool/fixint/neg fixint
    if ((m <= 0x7f) || (m >= 0xe0) || m == 0xc0 || m == 0xc2 || m == 0xc3) return 1;

    // fixstr
    if ((m & 0xe0) == 0xa0) return 1 + (m & 0x1f);
    // fixarray
    if ((m & 0xf0) == 0x90) {
        size_t n = m & 0x0f, off = 1;
        for (size_t i = 0; i < n; ++i) { auto s = mp_skip(v.subspan(off)); off += s; }
        return off;
    }
    // fixmap
    if ((m & 0xf0) == 0x80) {
        size_t n = m & 0x0f, off = 1;
        for (size_t i = 0; i < n; ++i) {
            auto ks = mp_skip(v.subspan(off)); off += ks;
            auto vs = mp_skip(v.subspan(off)); off += vs;
        }
        return off;
    }

    switch (m) {
        // ints / floats
        case 0xcc: case 0xd0: return 2;
        case 0xcd: case 0xd1: return 3;
        case 0xce: case 0xd2: case 0xca: return 5;
        case 0xcf: case 0xd3: case 0xcb: return 9;

        // str
        case 0xd9: { if (v.size() < 2) throw DeserializationError("str8");  return 2 + v[1]; }
        case 0xda: { if (v.size() < 3) throw DeserializationError("str16"); return 3 + mp_read_be16(v.data()+1); }
        case 0xdb: { if (v.size() < 5) throw DeserializationError("str32"); return 5 + mp_read_be32(v.data()+1); }

        // bin
        case 0xc4: { if (v.size() < 2) throw DeserializationError("bin8");  return 2 + v[1]; }
        case 0xc5: { if (v.size() < 3) throw DeserializationError("bin16"); return 3 + mp_read_be16(v.data()+1); }
        case 0xc6: { if (v.size() < 5) throw DeserializationError("bin32"); return 5 + mp_read_be32(v.data()+1); }

        // arrays
        case 0xdc: {
            if (v.size() < 3) throw DeserializationError("array16");
            size_t n = mp_read_be16(v.data()+1), off = 3;
            for (size_t i=0;i<n;++i){ auto s=mp_skip(v.subspan(off)); off+=s; } return off;
        }
        case 0xdd: {
            if (v.size() < 5) throw DeserializationError("array32");
            size_t n = mp_read_be32(v.data()+1), off = 5;
            for (size_t i=0;i<n;++i){ auto s=mp_skip(v.subspan(off)); off+=s; } return off;
        }

        // maps
        case 0xde: {
            if (v.size() < 3) throw DeserializationError("map16");
            size_t n = mp_read_be16(v.data()+1), off = 3;
            for (size_t i=0;i<n;++i){ auto ks=mp_skip(v.subspan(off)); off+=ks; auto vs=mp_skip(v.subspan(off)); off+=vs; } return off;
        }
        case 0xdf: {
            if (v.size() < 5) throw DeserializationError("map32");
            size_t n = mp_read_be32(v.data()+1), off = 5;
            for (size_t i=0;i<n;++i){ auto ks=mp_skip(v.subspan(off)); off+=ks; auto vs=mp_skip(v.subspan(off)); off+=vs; } return off;
        }
        default: break;
    }
    throw DeserializationError("msgpack: unsupported marker");
}

// ===== Deserializer ==========================================================
class MsgPackDeserializer {
    // root ownership if constructed from bytes/vector
    std::vector<uint8_t> owned_;
    // view over current element
    std::span<const uint8_t> view_{};

    // helpers: decode headers
    static void str_info(std::span<const uint8_t> v, const uint8_t*& p, size_t& len) {
        const uint8_t m = v[0];
        if ((m & 0xe0) == 0xa0) { len = (m & 0x1f); p = v.data()+1; return; }
        if (m == 0xd9) { len = v[1]; p=v.data()+2; return; }
        if (m == 0xda) { len = mp_read_be16(v.data()+1); p=v.data()+3; return; }
        if (m == 0xdb) { len = mp_read_be32(v.data()+1); p=v.data()+5; return; }
        throw DeserializationError("msgpack: not a string");
    }
    static void bin_info(std::span<const uint8_t> v, const uint8_t*& p, size_t& len) {
        const uint8_t m = v[0];
        if (m == 0xc4) { len = v[1]; p=v.data()+2; return; }
        if (m == 0xc5) { len = mp_read_be16(v.data()+1); p=v.data()+3; return; }
        if (m == 0xc6) { len = mp_read_be32(v.data()+1); p=v.data()+5; return; }
        throw DeserializationError("msgpack: not a bin");
    }
    static void arr_info(std::span<const uint8_t> v, size_t& count, size_t& off) {
        const uint8_t m = v[0];
        if ((m & 0xf0) == 0x90) { count = (m & 0x0f); off = 1; return; }
        if (m == 0xdc) { count = mp_read_be16(v.data()+1); off = 3; return; }
        if (m == 0xdd) { count = mp_read_be32(v.data()+1); off = 5; return; }
        throw DeserializationError("msgpack: not an array");
    }
    static void map_info(std::span<const uint8_t> v, size_t& count, size_t& off) {
        const uint8_t m = v[0];
        if ((m & 0xf0) == 0x80) { count = (m & 0x0f); off = 1; return; }
        if (m == 0xde) { count = mp_read_be16(v.data()+1); off = 3; return; }
        if (m == 0xdf) { count = mp_read_be32(v.data()+1); off = 5; return; }
        throw DeserializationError("msgpack: not a map");
    }

    // pretty printer
    static std::string esc(std::string_view s) {
        std::string out; out.reserve(s.size()+4);
        for (unsigned char ch : s) {
            switch (ch){
                case '"': out+="\\\""; break; case '\\': out+="\\\\"; break;
                case '\b': out+="\\b"; break; case '\f': out+="\\f"; break;
                case '\n': out+="\\n"; break; case '\r': out+="\\r"; break;
                case '\t': out+="\\t"; break;
                default:
                    if (ch < 0x20) { char b[7]; std::snprintf(b,sizeof(b),"\\u%04x",ch); out+=b; }
                    else out.push_back(char(ch));
            }
        } return out;
    }
    static const char* tname(const MsgPackDeserializer& v) {
        if (v.isNull()) return "null";
        if (v.isBool()) return "bool";
        if (v.isInt())  return "int";
        if (v.isUInt()) return "uint";
        if (v.isFloat())return "float";
        if (v.isString())return "str";
        if (v.isBlob()) return "bin";
        if (v.isMap())  return "map";
        if (v.isArray())return "arr";
        return "any";
    }
    static void dump(std::ostringstream& os, const MsgPackDeserializer& v, int pad) {
        auto ind = [&](int n){ for(int i=0;i<n;++i) os.put(' '); };
        if (v.isNull()) { os << "null: null"; return; }
        if (v.isBool()) { os << "bool: " << (v.asBool()?"true":"false"); return; }
        if (v.isInt())  { os << "int: "  << v.asInt64(); return; }
        if (v.isUInt()) { os << "uint: " << v.asUInt64(); return; }
        if (v.isFloat()){ os << "float: " << v.asDouble(); return; }
        if (v.isString()){ auto sv=v.asStringView(); os << "str: \""<<esc(sv)<<"\""; return; }
        if (v.isBlob()){ auto b=v.asBlob(); os << "bin[size="<<b.size()<<"]"; return; }
        if (v.isMap()){
            os << "map {\n";
            size_t n=0, off=0; v.map_info(v.view_, n, off);
            for(size_t i=0;i<n;++i){
                MsgPackDeserializer k(v.view_.subspan(off, mp_skip(v.view_.subspan(off))));
                off += mp_skip(v.view_.subspan(off));
                MsgPackDeserializer val(v.view_.subspan(off, mp_skip(v.view_.subspan(off))));
                off += mp_skip(v.view_.subspan(off));
                ind(pad+2);
                os << '"' << esc(k.asStringView()) << "\": ";
                dump(os, val, pad+2);
                if (i+1<n) os << ',';
                os << '\n';
            }
            ind(pad); os << '}';
            return;
        }
        if (v.isArray()){
            os << "arr [\n";
            size_t n=0, off=0; v.arr_info(v.view_, n, off);
            for(size_t i=0;i<n;++i){
                MsgPackDeserializer e(v.view_.subspan(off, mp_skip(v.view_.subspan(off))));
                off += mp_skip(v.view_.subspan(off));
                ind(pad+2); dump(os, e, pad+2);
                if (i+1<n) os << ',';
                os << '\n';
            }
            ind(pad); os << ']';
            return;
        }
        os << tname(v);
    }

public:
    // ---- ctors ----
    MsgPackDeserializer() = default;

    explicit MsgPackDeserializer(std::span<const uint8_t> bytes)
      : owned_(bytes.begin(), bytes.end()), view_(owned_) {}

    explicit MsgPackDeserializer(const std::vector<uint8_t>& buf)
      : owned_(buf), view_(owned_) {}

    explicit MsgPackDeserializer(std::vector<uint8_t>&& buf)
      : owned_(std::move(buf)), view_(owned_) {}

    // subview ctor
    explicit MsgPackDeserializer(std::span<const uint8_t> sub, bool /*tag*/) : view_(sub) {}

    // ---- type predicates ----
    bool isNull()  const { return !view_.empty() && view_[0] == 0xc0; }
    bool isBool()  const { return !view_.empty() && (view_[0] == 0xc2 || view_[0] == 0xc3); }
    bool isInt()   const {
        if (view_.empty()) return false;
        uint8_t m=view_[0]; return (m <= 0x7f) || (m >= 0xe0) || (m==0xd0||m==0xd1||m==0xd2||m==0xd3);
    }
    bool isUInt()  const {
        if (view_.empty()) return false;
        uint8_t m=view_[0]; return (m <= 0x7f) || (m==0xcc||m==0xcd||m==0xce||m==0xcf);
    }
    bool isFloat() const { if (view_.empty()) return false; uint8_t m=view_[0]; return m==0xca||m==0xcb; }
    bool isString()const {
        if (view_.empty()) return false; uint8_t m=view_[0];
        return ((m & 0xe0) == 0xa0) || m==0xd9 || m==0xda || m==0xdb;
    }
    bool isBlob()  const { if (view_.empty()) return false; uint8_t m=view_[0]; return m==0xc4||m==0xc5||m==0xc6; }
    bool isArray() const {
        if (view_.empty()) return false; uint8_t m=view_[0];
        return ((m & 0xf0) == 0x90) || m==0xdc || m==0xdd;
    }
    bool isMap()   const {
        if (view_.empty()) return false; uint8_t m=view_[0];
        return ((m & 0xf0) == 0x80) || m==0xde || m==0xdf;
    }

    // ---- scalar access ----
    std::int8_t asInt8() const {
        auto val = asInt64();
        if (val < std::numeric_limits<std::int8_t>::min() || val > std::numeric_limits<std::int8_t>::max()) {
            throw DeserializationError("int8 out of range");
        }
        return static_cast<std::int8_t>(val);
    }
    
    std::int16_t asInt16() const {
        auto val = asInt64();
        if (val < std::numeric_limits<std::int16_t>::min() || val > std::numeric_limits<std::int16_t>::max()) {
            throw DeserializationError("int16 out of range");
        }
        return static_cast<std::int16_t>(val);
    }
    
    std::int32_t asInt32() const {
        auto val = asInt64();
        if (val < std::numeric_limits<std::int32_t>::min() || val > std::numeric_limits<std::int32_t>::max()) {
            throw DeserializationError("int32 out of range");
        }
        return static_cast<std::int32_t>(val);
    }
    
    std::int64_t asInt64() const {
        if (view_.empty()) throw DeserializationError("int64");
        uint8_t m=view_[0];
        if (m <= 0x7f) return int64_t(m);
        if (m >= 0xe0) return int8_t(m);
        if (m==0xd0){ if(view_.size()<2) throw DeserializationError("i8");  return int8_t(view_[1]); }
        if (m==0xd1){ if(view_.size()<3) throw DeserializationError("i16"); return int16_t(mp_read_be16(view_.data()+1)); }
        if (m==0xd2){ if(view_.size()<5) throw DeserializationError("i32"); return int32_t(mp_read_be32(view_.data()+1)); }
        if (m==0xd3){ if(view_.size()<9) throw DeserializationError("i64"); return int64_t(mp_read_be64(view_.data()+1)); }
        // unsigned widths:
        if (m==0xcc){ if(view_.size()<2) throw DeserializationError("u8");  return uint8_t(view_[1]); }
        if (m==0xcd){ if(view_.size()<3) throw DeserializationError("u16"); return uint16_t(mp_read_be16(view_.data()+1)); }
        if (m==0xce){ if(view_.size()<5) throw DeserializationError("u32"); return uint32_t(mp_read_be32(view_.data()+1)); }
        if (m==0xcf){ if(view_.size()<9) throw DeserializationError("u64"); return uint64_t(mp_read_be64(view_.data()+1)); }
        throw DeserializationError("not int");
    }
    
    std::uint8_t asUInt8() const {
        auto val = asUInt64();
        if (val > std::numeric_limits<std::uint8_t>::max()) {
            throw DeserializationError("uint8 out of range");
        }
        return static_cast<std::uint8_t>(val);
    }
    
    std::uint16_t asUInt16() const {
        auto val = asUInt64();
        if (val > std::numeric_limits<std::uint16_t>::max()) {
            throw DeserializationError("uint16 out of range");
        }
        return static_cast<std::uint16_t>(val);
    }
    
    std::uint32_t asUInt32() const {
        auto val = asUInt64();
        if (val > std::numeric_limits<std::uint32_t>::max()) {
            throw DeserializationError("uint32 out of range");
        }
        return static_cast<std::uint32_t>(val);
    }
    
    std::uint64_t asUInt64() const {
        if (view_.empty()) throw DeserializationError("uint64");
        uint8_t m=view_[0];
        if (m <= 0x7f) return m;
        if (m==0xcc){ if(view_.size()<2) throw DeserializationError("u8");  return uint8_t(view_[1]); }
        if (m==0xcd){ if(view_.size()<3) throw DeserializationError("u16"); return mp_read_be16(view_.data()+1); }
        if (m==0xce){ if(view_.size()<5) throw DeserializationError("u32"); return mp_read_be32(view_.data()+1); }
        if (m==0xcf){ if(view_.size()<9) throw DeserializationError("u64"); return mp_read_be64(view_.data()+1); }
        // fall back for signed fix/neg
        return uint64_t(asInt64());
    }
    
    float asFloat() const {
        return static_cast<float>(asDouble());
    }
    
    double asDouble() const {
        if (!isFloat()) throw DeserializationError("not float");
        uint8_t m=view_[0];
        if (m==0xca){ if(view_.size()<5) throw DeserializationError("f32"); uint32_t bits=mp_read_be32(view_.data()+1); float f; std::memcpy(&f,&bits,4); return double(f); }
        if (m==0xcb){ if(view_.size()<9) throw DeserializationError("f64"); uint64_t bits=mp_read_be64(view_.data()+1); double d; std::memcpy(&d,&bits,8); return d; }
        throw DeserializationError("float?");
    }
    
    bool asBool() const { if (!isBool()) throw DeserializationError("not bool"); return view_[0]==0xc3; }

    std::string      asString()     const { auto sv = asStringView(); return std::string(sv); }
    std::string_view asStringView() const {
        const uint8_t* p=nullptr; size_t n=0; str_info(view_, p, n);
        return std::string_view(reinterpret_cast<const char*>(p), n);
    }

    std::span<const std::byte> asBlob() const {
        const uint8_t* p=nullptr; size_t n=0; bin_info(view_, p, n);
        return { reinterpret_cast<const std::byte*>(p), n };
    }

    // ---- map interface ----

    struct KeysView {
        // store the map payload view and precomputed count/offset
        std::span<const uint8_t> v{};
        size_t count = 0;
        size_t payload_off = 0;

        struct iterator {
            // default-constructible → forward iterator
            std::span<const uint8_t> v{};
            size_t i = 0;
            size_t off = 0; // byte offset at current key

            using iterator_category = std::forward_iterator_tag;
            using iterator_concept  = std::forward_iterator_tag;
            using value_type        = std::string_view;
            using difference_type   = std::ptrdiff_t;
            using reference         = std::string_view;

            iterator() = default;
            iterator(std::span<const uint8_t> vv, size_t idx, size_t start_off)
                : v(vv), i(idx), off(start_off) {}

            reference operator*() const {
                // key at off
                auto key_span = v.subspan(off);
                size_t key_sz = mp_skip(key_span);
                MsgPackDeserializer kd(key_span.first(key_sz), true);
                // keys must be strings in our concept
                return kd.asStringView();
            }
            iterator& operator++() {
                // advance past key and value
                auto key_span = v.subspan(off);
                size_t key_sz = mp_skip(key_span);
                size_t val_sz = mp_skip(v.subspan(off + key_sz));
                off += key_sz + val_sz;
                ++i;
                return *this;
            }
            iterator operator++(int){ auto tmp=*this; ++(*this); return tmp; }

            friend bool operator==(const iterator& a, const iterator& b) {
                return a.v.data()==b.v.data() && a.i==b.i && a.off==b.off;
            }
        };

        iterator begin() const { return iterator{ v, 0, payload_off }; }
        iterator end()   const {
            // compute byte offset of end by walking all entries
            size_t off = payload_off;
            for (size_t i=0;i<count;++i) {
                size_t ks = mp_skip(v.subspan(off));
                off += ks;
                size_t vs = mp_skip(v.subspan(off));
                off += vs;
            }
            return iterator{ v, count, off };
        }
    };

    KeysView mapKeys() const {
        if (!isMap()) throw DeserializationError("not map");
        size_t n=0, off=0; map_info(view_, n, off);
        return KeysView{ view_, n, off };
    }

    bool contains(std::string_view key) const {
        if (!isMap()) return false;
        size_t n=0, off=0; map_info(view_, n, off);
        for (size_t i=0;i<n;++i) {
            auto kspan = view_.subspan(off);
            size_t ksz = mp_skip(kspan);
            MsgPackDeserializer kd(kspan.first(ksz), true);
            off += ksz;
            auto vspan = view_.subspan(off);
            size_t vsz = mp_skip(vspan);
            if (kd.isString() && kd.asStringView() == key) {
                return true;
            }
            off += vsz;
        }
        return false;
    }

    MsgPackDeserializer operator[](std::string_view key) const {
        if (!isMap()) throw DeserializationError("not map");
        size_t n=0, off=0; map_info(view_, n, off);
        for (size_t i=0;i<n;++i) {
            auto kspan = view_.subspan(off);
            size_t ksz = mp_skip(kspan);
            MsgPackDeserializer kd(kspan.first(ksz), true);
            off += ksz;
            auto vspan = view_.subspan(off);
            size_t vsz = mp_skip(vspan);
            if (kd.isString() && kd.asStringView() == key) {
                return MsgPackDeserializer(vspan.first(vsz), true);
            }
            off += vsz;
        }
        throw DeserializationError("key not found: " + std::string(key));
    }

    // ---- array interface ----
    size_t arraySize() const {
        size_t n=0, off=0; arr_info(view_, n, off); (void)off; return n;
    }

    MsgPackDeserializer operator[](size_t idx) const {
        size_t n=0, off=0; arr_info(view_, n, off);
        if (idx >= n) throw DeserializationError("index OOB");
        for (size_t i=0;i<idx;++i) off += mp_skip(view_.subspan(off));
        size_t sz = mp_skip(view_.subspan(off));
        return MsgPackDeserializer(view_.subspan(off, sz), true);
    }

    // ---- debug ----
    std::string to_string() const {
        std::ostringstream os; dump(os, *this, 0); return os.str();
    }

    // expose raw view if you need it
    std::span<const uint8_t> raw_view() const { return view_; }
};

// ===== Writer (msgpack-c) =====================================================
class MsgPackRootSerializer {
public:
    msgpack_sbuffer sbuf{};
    msgpack_packer  pk{};

    MsgPackRootSerializer() {
        msgpack_sbuffer_init(&sbuf);
        msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
    }
    ~MsgPackRootSerializer() { msgpack_sbuffer_destroy(&sbuf); }

    ZBuffer finish() {
        if (sbuf.size == 0) return ZBuffer();
        // steal buffer
        size_t n = sbuf.size;
        char*  d = sbuf.data;
        sbuf.data = nullptr; sbuf.size = 0; sbuf.alloc = 0;
        return ZBuffer(d, n, ZBuffer::Deleters::Free);
    }
};

class MsgPackSerializer {
    msgpack_packer& pk_;
public:
    explicit MsgPackSerializer(MsgPackRootSerializer& rs) : pk_(rs.pk) {}

    // primitives
    void null()                 { msgpack_pack_nil(&pk_); }
    void boolean(bool v)        { v ? msgpack_pack_true(&pk_) : msgpack_pack_false(&pk_); }
    void int64(std::int64_t v)  { msgpack_pack_int64(&pk_, v); }
    void uint64(std::uint64_t v){ msgpack_pack_uint64(&pk_, v); }
    void double_(double v)      { msgpack_pack_double(&pk_, v); }
    void string(std::string_view sv) {
        msgpack_pack_str(&pk_, sv.size());
        msgpack_pack_str_body(&pk_, sv.data(), sv.size());
    }
    void binary(std::span<const std::byte> b) {
        auto p = reinterpret_cast<const char*>(b.data());
        msgpack_pack_bin(&pk_, b.size());
        msgpack_pack_bin_body(&pk_, p, b.size());
    }

    // arrays/maps (MsgPack needs sizes up-front)
    void begin_array(std::size_t n) { msgpack_pack_array(&pk_, n); }
    void end_array()                { /* no-op */ }

    void begin_map(std::size_t n)   { msgpack_pack_map(&pk_, n); }
    void end_map()                  { /* no-op */ }

    void key(std::string_view k)    { string(k); }
};

struct MsgPack {
    static inline constexpr const char* Name = "MsgPack";
    using Deserializer   = MsgPackDeserializer; 
    using RootSerializer = MsgPackRootSerializer;
    using Serializer     = MsgPackSerializer;
};

} // namespace zerialize
