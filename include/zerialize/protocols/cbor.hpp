// CBOR protocol implemented with jsoncons (reader and writer)
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <limits>
#include <cmath>

#include <jsoncons/json.hpp>
#include <jsoncons_ext/cbor/cbor.hpp>

#include <zerialize/zbuffer.hpp>
#include <zerialize/errors.hpp>

namespace zerialize {
namespace cborjc {

// ========================== Writer (Serializer) ===============================

struct RootSerializer {
    std::vector<uint8_t> out_;
    jsoncons::cbor::cbor_bytes_encoder enc;
    bool wrote_root = false;

    RootSerializer()
        : out_()
        , enc(out_)
    {}

    ZBuffer finish() {
        if (!wrote_root) {
            enc.null_value();
            wrote_root = true;
        }
        return ZBuffer(std::move(out_));
    }
};

struct Serializer {
    RootSerializer* r;
    explicit Serializer(RootSerializer& rs) : r(&rs) {}

    // primitives
    void null()                  { r->enc.null_value(); r->wrote_root = true; }
    void boolean(bool v)         { r->enc.bool_value(v); r->wrote_root = true; }
    void int64(std::int64_t v)   { r->enc.int64_value(v); r->wrote_root = true; }
    void uint64(std::uint64_t v) { r->enc.uint64_value(v); r->wrote_root = true; }
    void double_(double v)       { r->enc.double_value(v); r->wrote_root = true; }
    void string(std::string_view sv) { r->enc.string_value(sv); r->wrote_root = true; }
    void binary(std::span<const std::byte> b) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(b.data());
        std::vector<uint8_t> tmp(p, p + b.size());
        r->enc.byte_string_value(tmp); r->wrote_root = true;
    }

    // containers
    void begin_array(std::size_t n) { r->enc.begin_array(n); r->wrote_root = true; }
    void end_array()                { r->enc.end_array(); }
    void begin_map(std::size_t n)   { r->enc.begin_object(n); r->wrote_root = true; }
    void end_map()                  { r->enc.end_object(); }
    void key(std::string_view k)    { r->enc.key(k); }
};

// ========================== Reader (Deserializer) =============================

class CborDeserializer {
    std::span<const uint8_t> buf_{};
    std::vector<uint8_t> owned_; // if non-empty, buf_ points into this
    std::size_t pos_ = 0; // start of this view's value

    static void ensure(bool cond, const char* msg) {
        if (!cond) throw DeserializationError(msg);
    }

    static uint64_t get_be(const uint8_t* p, std::size_t n) {
        uint64_t v = 0;
        for (std::size_t i = 0; i < n; ++i) v = (v << 8) | p[i];
        return v;
    }

    struct Head {
        uint8_t major;
        uint8_t addl;
        uint64_t val;   // length/value for definite forms
        std::size_t hlen; // header length in bytes
        bool indefinite;
    };

    Head read_head(std::size_t p) const {
        ensure(p < buf_.size(), "CBOR: truncated");
        uint8_t b = buf_[p];
        Head h{}; h.major = b >> 5; h.addl = b & 0x1F; h.indefinite = false; h.hlen = 1; h.val = 0;
        if (h.major == 7) {
            // simple/float encoding: header is 1 byte for floats; body size depends on addl
            if (h.addl == 25) { h.val = 2; }           // half (2 bytes)
            else if (h.addl == 26) { h.val = 4; }      // float32
            else if (h.addl == 27) { h.val = 8; }      // float64
            else if (h.addl == 24) {                   // simple value (next 1 byte)
                ensure(p+2 <= buf_.size(), "CBOR: truncated simple(24)");
                h.hlen = 2; h.val = 0;
            } else if (h.addl == 31) {
                h.indefinite = true;
            } else {
                // null/true/false/simple (no body)
                h.val = 0;
            }
            return h;
        }
        if (h.addl < 24) { h.val = h.addl; }
        else if (h.addl == 24) { ensure(p+2 <= buf_.size(), "CBOR: truncated u8"); h.val = buf_[p+1]; h.hlen = 2; }
        else if (h.addl == 25) { ensure(p+3 <= buf_.size(), "CBOR: truncated u16"); h.val = get_be(&buf_[p+1],2); h.hlen = 3; }
        else if (h.addl == 26) { ensure(p+5 <= buf_.size(), "CBOR: truncated u32"); h.val = get_be(&buf_[p+1],4); h.hlen = 5; }
        else if (h.addl == 27) { ensure(p+9 <= buf_.size(), "CBOR: truncated u64"); h.val = get_be(&buf_[p+1],8); h.hlen = 9; }
        else if (h.addl == 31) { h.indefinite = true; }
        else { throw DeserializationError("CBOR: reserved additional info"); }
        return h;
    }

    std::size_t skip(std::size_t p) const {
        auto h = read_head(p);
        std::size_t q = p + h.hlen;
        switch (h.major) {
            case 0: // uint
            case 1: // negint
                return q;
            case 2: // bstr
            case 3: // tstr
                if (!h.indefinite) {
                    ensure(q + h.val <= buf_.size(), "CBOR: truncated string");
                    return q + static_cast<std::size_t>(h.val);
                } else {
                    // chunks terminated by 0xFF
                    for (;;) {
                        ensure(q < buf_.size(), "CBOR: truncated indef str");
                        if (buf_[q] == 0xFF) return q + 1;
                        auto ch = read_head(q);
                        ensure(ch.major == h.major, "CBOR: wrong chunk type");
                        ensure(!ch.indefinite, "CBOR: nested indef chunks not allowed");
                        q += ch.hlen + static_cast<std::size_t>(ch.val);
                    }
                }
            case 4: // array
                if (!h.indefinite) {
                    for (uint64_t i = 0; i < h.val; ++i) q = skip(q);
                    return q;
                } else {
                    for (;;) {
                        ensure(q < buf_.size(), "CBOR: truncated indef array");
                        if (buf_[q] == 0xFF) return q + 1;
                        q = skip(q);
                    }
                }
            case 5: // map
                if (!h.indefinite) {
                    for (uint64_t i = 0; i < h.val; ++i) { q = skip(q); q = skip(q); }
                    return q;
                } else {
                    for (;;) {
                        ensure(q < buf_.size(), "CBOR: truncated indef map");
                        if (buf_[q] == 0xFF) return q + 1;
                        q = skip(q); // key
                        q = skip(q); // value
                    }
                }
            case 6: // tag
                // skip tag then the tagged value
                return skip(q);
            case 7: // floats/simple
                if (h.addl == 25) { ensure(q+2 <= buf_.size(), "CBOR: truncated f16"); return q+2; }
                if (h.addl == 26) { ensure(q+4 <= buf_.size(), "CBOR: truncated f32"); return q+4; }
                if (h.addl == 27) { ensure(q+8 <= buf_.size(), "CBOR: truncated f64"); return q+8; }
                if (h.addl == 24) { ensure(q+1 <= buf_.size(), "CBOR: truncated simple(24)"); return q+1; }
                // simple/bool/null (no body): header only
                return q;
            default:
                throw DeserializationError("CBOR: unknown major");
        }
    }

    Head head() const { return read_head(pos_); }

    static double decode_f16(uint16_t h) {
        // Simple IEEE754 half to double conversion
        uint16_t sign = (h >> 15) & 1;
        uint16_t exp  = (h >> 10) & 0x1F;
        uint16_t frac = h & 0x3FF;
        if (exp == 0) {
            if (frac == 0) return sign ? -0.0 : 0.0;
            return (sign?-1:1) * std::ldexp(static_cast<double>(frac), -24);
        } else if (exp == 31) {
            return frac ? std::numeric_limits<double>::quiet_NaN() : (sign ? -INFINITY : INFINITY);
        }
        double mant = 1.0 + static_cast<double>(frac) / 1024.0;
        int e = static_cast<int>(exp) - 15;
        double val = std::ldexp(mant, e);
        return sign ? -val : val;
    }

public:
    // ---- ctors ----
    CborDeserializer() = default;
    explicit CborDeserializer(std::span<const uint8_t> bytes) : buf_(bytes), pos_(0) {}
    explicit CborDeserializer(const std::vector<uint8_t>& buf)
        : owned_(buf.begin(), buf.end())
        , pos_(0)
    {
        buf_ = std::span<const uint8_t>(owned_.data(), owned_.size());
    }
    explicit CborDeserializer(std::vector<uint8_t>&& buf)
        : owned_(std::move(buf))
        , pos_(0)
    {
        buf_ = std::span<const uint8_t>(owned_.data(), owned_.size());
    }
    explicit CborDeserializer(const uint8_t* data, std::size_t n) : CborDeserializer(std::span<const uint8_t>(data, n)) {}
    explicit CborDeserializer(const std::byte* data, std::size_t n) : CborDeserializer(reinterpret_cast<const uint8_t*>(data), n) {}

    // view ctor
    CborDeserializer(std::span<const uint8_t> buf, std::size_t start) : buf_(buf), pos_(start) {}

    // ---- predicates ----
    bool isNull()   const { auto h = head(); return h.major==7 && h.addl==22; }
    bool isBool()   const { auto h = head(); return h.major==7 && (h.addl==20 || h.addl==21); }
    bool isInt()    const { auto h = head(); return h.major==0 || h.major==1; }
    bool isUInt()   const { auto h = head(); return h.major==0; }
    bool isFloat()  const { auto h = head(); return h.major==7 && (h.addl==25 || h.addl==26 || h.addl==27); }
    bool isString() const { auto h = head(); return h.major==3; }
    bool isBlob()   const { auto h = head(); return h.major==2; }
    bool isMap()    const { auto h = head(); return h.major==5; }
    bool isArray()  const { auto h = head(); return h.major==4; }

    // ---- scalars ----
    int8_t   asInt8()   const { auto v = asInt64();  if (v < INT8_MIN  || v > INT8_MAX)  throw DeserializationError("int8 out of range");  return (int8_t)v; }
    int16_t  asInt16()  const { auto v = asInt64();  if (v < INT16_MIN || v > INT16_MAX) throw DeserializationError("int16 out of range"); return (int16_t)v; }
    int32_t  asInt32()  const { auto v = asInt64();  if (v < INT32_MIN || v > INT32_MAX) throw DeserializationError("int32 out of range"); return (int32_t)v; }
    int64_t  asInt64()  const {
        auto h = head();
        ensure(h.major==0 || h.major==1, "CBOR: not an integer");
        if (h.major==0) return static_cast<int64_t>(h.val);
        // negative: -1 - n
        if (h.val > uint64_t(INT64_MAX)) throw DeserializationError("int64 underflow");
        long long x = -1 - static_cast<long long>(h.val);
        return static_cast<int64_t>(x);
    }

    uint8_t  asUInt8()  const { auto v = asUInt64(); if (v > UINT8_MAX)  throw DeserializationError("uint8 out of range");  return (uint8_t)v; }
    uint16_t asUInt16() const { auto v = asUInt64(); if (v > UINT16_MAX) throw DeserializationError("uint16 out of range"); return (uint16_t)v; }
    uint32_t asUInt32() const { auto v = asUInt64(); if (v > UINT32_MAX) throw DeserializationError("uint32 out of range"); return (uint32_t)v; }
    uint64_t asUInt64() const { auto h = head(); ensure(h.major==0, "CBOR: not an unsigned integer"); return h.val; }

    float    asFloat()  const { return static_cast<float>(asDouble()); }
    double   asDouble() const {
        auto h = head(); ensure(isFloat(), "CBOR: not a float");
        std::size_t q = pos_ + h.hlen;
        if (h.addl == 25) { uint16_t v = (uint16_t)((buf_[q] << 8) | buf_[q+1]); return decode_f16(v); }
        if (h.addl == 26) { uint32_t v = (uint32_t)get_be(&buf_[q],4); float f; std::memcpy(&f,&v,4); return static_cast<double>(f); }
        uint64_t v = get_be(&buf_[q],8); double d; std::memcpy(&d,&v,8); return d;
    }
    bool     asBool()   const { auto h = head(); ensure(h.major==7 && (h.addl==20 || h.addl==21), "CBOR: not a bool"); return h.addl==21; }

    std::string      asString() const {
        auto h = head(); ensure(h.major==3, "CBOR: not a string");
        std::size_t q = pos_ + h.hlen;
        if (!h.indefinite) {
            ensure(q + h.val <= buf_.size(), "CBOR: truncated string");
            return std::string(reinterpret_cast<const char*>(&buf_[q]), static_cast<std::size_t>(h.val));
        } else {
            std::string out;
            for (;;) {
                ensure(q < buf_.size(), "CBOR: truncated indef tstr");
                if (buf_[q] == 0xFF) break;
                auto ch = read_head(q); ensure(ch.major==3 && !ch.indefinite, "CBOR: bad tstr chunk");
                q += ch.hlen; ensure(q + ch.val <= buf_.size(), "CBOR: trunc");
                out.append(reinterpret_cast<const char*>(&buf_[q]), static_cast<std::size_t>(ch.val));
                q += ch.val;
            }
            return out;
        }
    }
    std::string_view asStringView() const {
        auto h = head(); ensure(h.major==3, "CBOR: not a string");
        ensure(!h.indefinite, "CBOR: string is indefinite; use asString()");
        std::size_t q = pos_ + h.hlen; ensure(q + h.val <= buf_.size(), "CBOR: trunc tstr");
        return std::string_view(reinterpret_cast<const char*>(&buf_[q]), static_cast<std::size_t>(h.val));
    }

    std::span<const std::byte> asBlob() const {
        auto h = head(); ensure(h.major==2, "CBOR: not a byte string");
        ensure(!h.indefinite, "CBOR: chunked byte string; fetch via asBlobVec()");
        std::size_t q = pos_ + h.hlen; ensure(q + h.val <= buf_.size(), "CBOR: trunc bstr");
        auto* p = reinterpret_cast<const std::byte*>(&buf_[q]);
        return { p, static_cast<std::size_t>(h.val) };
    }
    std::vector<std::byte> asBlobVec() const {
        auto h = head(); ensure(h.major==2, "CBOR: not a byte string");
        std::size_t q = pos_ + h.hlen;
        std::vector<std::byte> out;
        if (!h.indefinite) {
            ensure(q + h.val <= buf_.size(), "CBOR: trunc bstr");
            auto* p = reinterpret_cast<const std::byte*>(&buf_[q]);
            out.insert(out.end(), p, p + h.val);
        } else {
            for (;;) {
                ensure(q < buf_.size(), "CBOR: trunc indef bstr");
                if (buf_[q] == 0xFF) break;
                auto ch = read_head(q); ensure(ch.major==2 && !ch.indefinite, "CBOR: bad bstr chunk");
                q += ch.hlen; ensure(q + ch.val <= buf_.size(), "CBOR: trunc chunk");
                auto* p = reinterpret_cast<const std::byte*>(&buf_[q]);
                out.insert(out.end(), p, p + ch.val);
                q += ch.val;
            }
        }
        return out;
    }

    // ---- map interface ----
    bool contains(std::string_view key) const {
        auto h = head(); if (!(h.major==5)) return false;
        std::size_t q = pos_ + h.hlen;
        if (!h.indefinite) {
            for (uint64_t i=0;i<h.val;++i) {
                // key
                auto kh = read_head(q);
                std::string_view ksv;
                if (kh.major==3 && !kh.indefinite) {
                    ksv = std::string_view(reinterpret_cast<const char*>(&buf_[q+kh.hlen]), static_cast<std::size_t>(kh.val));
                } else {
                    // fallback: decode key to string
                    CborDeserializer keyv(buf_, q);
                    std::string ks = keyv.asString();
                    if (ks == key) return true;
                    q = skip(q); // advance over key
                    q = skip(q); // skip value
                    continue;
                }
                if (ksv == key) return true;
                q = skip(q); // key
                q = skip(q); // value
            }
            return false;
        } else {
            for (;;) {
                ensure(q < buf_.size(), "CBOR: trunc indef map");
                if (buf_[q] == 0xFF) return false;
                std::string k = CborDeserializer(buf_, q).asString();
                q = skip(q); // key
                if (k == key) return true;
                q = skip(q); // value
            }
        }
    }

    struct KeysView {
        std::span<const uint8_t> buf;
        std::size_t start; // pos at start of map head
        struct iterator {
            std::span<const uint8_t> buf{};
            std::size_t q = 0; // current cursor (at next key head or break)
            uint64_t remaining = 0; // for definite maps
            bool indefinite = false;
            mutable std::string scratch;

            using iterator_category = std::forward_iterator_tag;
            using iterator_concept  = std::forward_iterator_tag;
            using value_type        = std::string_view;
            using difference_type   = std::ptrdiff_t;
            using reference         = std::string_view;

            struct H { uint8_t major, addl; uint64_t val; std::size_t hlen; bool indefinite; };
            static H read_head(std::span<const uint8_t> b, std::size_t p) {
                uint8_t bb = b[p];
                H h{}; h.major = bb>>5; h.addl=bb&0x1F; h.hlen=1; h.val=0; h.indefinite=false;
                if (h.major==7) {
                    if (h.addl==25) { h.val=2; }
                    else if (h.addl==26) { h.val=4; }
                    else if (h.addl==27) { h.val=8; }
                    else if (h.addl==24) { h.hlen=2; }
                    else if (h.addl==31) { h.indefinite=true; }
                    return h;
                }
                if (h.addl<24) { h.val=h.addl; }
                else if (h.addl==24) { h.val=b[p+1]; h.hlen=2; }
                else if (h.addl==25) { h.val=(uint64_t)b[p+1]<<8 | b[p+2]; h.hlen=3; }
                else if (h.addl==26) { h.val=((uint64_t)b[p+1]<<24)|((uint64_t)b[p+2]<<16)|((uint64_t)b[p+3]<<8)|b[p+4]; h.hlen=5; }
                else if (h.addl==27) { h.val=((uint64_t)b[p+1]<<56)|((uint64_t)b[p+2]<<48)|((uint64_t)b[p+3]<<40)|((uint64_t)b[p+4]<<32)|((uint64_t)b[p+5]<<24)|((uint64_t)b[p+6]<<16)|((uint64_t)b[p+7]<<8)|b[p+8]; h.hlen=9; }
                else if (h.addl==31) { h.indefinite=true; }
                return h;
            }
            static std::size_t skip(std::span<const uint8_t> b, std::size_t p) {
                auto h = read_head(b,p); std::size_t q=p+h.hlen;
                switch (h.major) {
                    case 0: case 1: return q;
                    case 2: case 3:
                        if (!h.indefinite) return q + (std::size_t)h.val;
                        for (;;) { if (b[q]==0xFF) return q+1; auto ch=read_head(b,q); q += ch.hlen + (std::size_t)ch.val; }
                    case 4:
                        if (!h.indefinite) { for (uint64_t i=0;i<h.val;++i) q=skip(b,q); return q; }
                        for(;;){ if(b[q]==0xFF) return q+1; q=skip(b,q);}        
                    case 5:
                        if (!h.indefinite) { for (uint64_t i=0;i<h.val;++i){ q=skip(b,q); q=skip(b,q);} return q; }
                        for(;;){ if(b[q]==0xFF) return q+1; q=skip(b,q); q=skip(b,q);} 
                    case 6: return skip(b,q);
                    case 7:
                        if (h.addl==25) return q+2; if (h.addl==26) return q+4; if (h.addl==27) return q+8; return q;
                }
                return q;
            }

            reference operator*() const {
                auto kh = read_head(buf, q);
                if (kh.major==3 && !kh.indefinite) {
                    return std::string_view(reinterpret_cast<const char*>(&buf[q+kh.hlen]), (std::size_t)kh.val);
                }
                // fallback: materialize
                CborDeserializer v(std::span<const uint8_t>(buf.data(), buf.size()), q);
                scratch = v.asString();
                return std::string_view(scratch);
            }
            iterator& operator++() {
                if (indefinite) {
                    if (buf[q]==0xFF) return *this; // already at end
                    q = skip(buf,q); // key
                    q = skip(buf,q); // value
                } else {
                    if (remaining==0) return *this;
                    q = skip(buf,q); // key
                    q = skip(buf,q); // value
                    --remaining;
                }
                return *this;
            }
            iterator operator++(int) { auto t=*this; ++(*this); return t; }
            friend bool operator==(const iterator& a, const iterator& b){ return a.q==b.q && a.buf.data()==b.buf.data(); }
        };

        iterator begin() const {
            iterator it; it.buf = buf; auto h = iterator::read_head(buf, start); std::size_t q = start + h.hlen; it.q = q; it.indefinite = h.indefinite; it.remaining = h.indefinite ? 0 : (uint64_t)h.val; return it;
        }
        iterator end()   const {
            iterator it; it.buf = buf; auto h = iterator::read_head(buf, start); if (!h.indefinite) {
                // compute end pos
                std::size_t q = start + h.hlen; for (uint64_t i=0;i<h.val;++i){ q = iterator::skip(buf,q); q = iterator::skip(buf,q);} it.q = q; it.indefinite=false; it.remaining=0; return it;
            } else {
                // find break
                std::size_t q = start + h.hlen; for(;;){ if (buf[q]==0xFF){ it.q=q+1; break; } q = iterator::skip(buf,q); q = iterator::skip(buf,q);} it.indefinite=true; return it;
            }
        }
    };

    KeysView mapKeys() const { auto h=head(); ensure(h.major==5, "CBOR: not a map"); return KeysView{ buf_, pos_ }; }

    CborDeserializer operator[](std::string_view key) const {
        auto h = head(); ensure(h.major==5, "CBOR: not a map");
        std::size_t q = pos_ + h.hlen;
        if (!h.indefinite) {
            for (uint64_t i=0;i<h.val;++i) {
                CborDeserializer kview(buf_, q);
                std::string k = kview.asString();
                q = skip(q);
                if (k == key) return CborDeserializer(buf_, q);
                q = skip(q);
            }
        } else {
            for(;;){ ensure(q<buf_.size(), "CBOR: trunc indef map"); if(buf_[q]==0xFF) break; CborDeserializer kview(buf_, q); std::string k = kview.asString(); q = skip(q); if (k==key) return CborDeserializer(buf_, q); q = skip(q);}        
        }
        throw DeserializationError("CBOR: key not found: " + std::string(key));
    }

    // ---- array interface ----
    std::size_t arraySize() const {
        auto h = head(); ensure(h.major==4, "CBOR: not an array");
        if (!h.indefinite) return static_cast<std::size_t>(h.val);
        std::size_t q = pos_ + h.hlen; std::size_t c=0; for(;;){ ensure(q<buf_.size(), "CBOR: trunc indef arr"); if(buf_[q]==0xFF) break; q = skip(q); ++c; } return c;
    }
    CborDeserializer operator[](std::size_t idx) const {
        auto h = head(); ensure(h.major==4, "CBOR: not an array");
        std::size_t q = pos_ + h.hlen;
        if (!h.indefinite) {
            if (idx >= h.val) throw DeserializationError("CBOR: index OOB");
            for (std::size_t i=0;i<idx;++i) q = skip(q);
            return CborDeserializer(buf_, q);
        } else {
            for (std::size_t i=0;;++i){ ensure(q<buf_.size(), "CBOR: trunc indef arr"); if (buf_[q]==0xFF) break; if (i==idx) return CborDeserializer(buf_, q); q = skip(q);}            
            throw DeserializationError("CBOR: index OOB");
        }
    }

    // ---- debug ----
    std::string to_string() const {
        std::ostringstream os; dump_rec(os, pos_, 0); return os.str();
    }
private:
    static void indent(std::ostringstream& os, int n){ for(int i=0;i<n;++i) os.put(' ');}    
    const char* type_code(std::size_t p) const {
        auto h = read_head(p);
        switch (h.major){
            case 0: return "uint"; case 1: return "int"; case 2: return "blob"; case 3: return "str"; case 4: return "arr"; case 5: return "map";
            case 7: if (h.addl==20||h.addl==21) return "bool"; if(h.addl==22) return "null"; if(h.addl==25||h.addl==26||h.addl==27) return "float"; return "any";
            default: return "any";
        }
    }
    void dump_rec(std::ostringstream& os, std::size_t p, int pad) const {
        auto h = read_head(p); auto t = type_code(p); std::size_t q = p + h.hlen;
        if (h.major==0) { os << t << '|' << h.val; }
        else if (h.major==1) { long long x = -1 - (long long)h.val; os << t << '|' << x; }
        else if (h.major==7 && (h.addl==25||h.addl==26||h.addl==27)) { os << t << '|' << CborDeserializer(buf_, p).asDouble(); }
        else if (h.major==7 && (h.addl==20||h.addl==21)) { os << t << "| " << (h.addl==21?"true":"false"); }
        else if (h.major==7 && h.addl==22) { os << t << "|null"; }
        else if (h.major==3) {
            if (!h.indefinite) { os << t << "|\"" << std::string(reinterpret_cast<const char*>(&buf_[q]), (std::size_t)h.val) << '"'; }
            else { os << t << "|\"" << CborDeserializer(buf_, p).asString() << '"'; }
        } else if (h.major==2) {
            std::size_t total = 0; if (!h.indefinite) total = (std::size_t)h.val; else { for(;;){ if(buf_[q]==0xFF) break; auto ch = read_head(q); total += (std::size_t)ch.val; q += ch.hlen + (std::size_t)ch.val; } }
            os << t << "[size=" << total << "]";
        } else if (h.major==4) {
            os << t << " [\n"; std::size_t n = arraySize(); std::size_t e = p + h.hlen; for (std::size_t i=0;i<n;++i){ indent(os, pad+2); dump_rec(os, e, pad+2); if (i+1<n) os << ","; os << "\n"; e = skip(e);} indent(os, pad); os << ']';
        } else if (h.major==5) {
            os << t << " {\n"; std::size_t q2 = p + h.hlen; std::size_t count = h.indefinite ? arraySize() : (std::size_t)h.val; // rough
            std::size_t i=0; while (true){ if (h.indefinite && buf_[q2]==0xFF) { q2++; break; } indent(os, pad+2); os << '"' << CborDeserializer(buf_, q2).asString() << "\": "; q2 = skip(q2); dump_rec(os, q2, pad+2); q2 = skip(q2); if (++i < count) os << ","; os << "\n"; if (!h.indefinite && i>=count) break; }
            indent(os, pad); os << '}';
        } else {
            os << t;
        }
    }
};

} // namespace cborjc

struct CBOR {
    static inline constexpr const char* Name = "CBOR";
    using Deserializer   = cborjc::CborDeserializer;
    using RootSerializer = cborjc::RootSerializer;
    using Serializer     = cborjc::Serializer;
};

} // namespace zerialize
