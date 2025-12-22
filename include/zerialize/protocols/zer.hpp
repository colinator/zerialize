#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <stdexcept>
#include <sstream>
#include <optional>
#include <variant>
#include <utility>
#include <type_traits>
#include <limits>
#include <iterator>
#include <array>

#include <zerialize/zbuffer.hpp>
#include <zerialize/errors.hpp>

namespace zerialize {
namespace zer {

// =============================================================================
//  ZER v1: Lazy JSON-model envelope + aligned arena (see protocols/zer.txt)
// =============================================================================

inline constexpr std::uint32_t Magic = 0x564E455A; // 'ZENV' little-endian
inline constexpr std::uint16_t Version = 1;
inline constexpr std::uint32_t HeaderSize = 20;
inline constexpr std::uint32_t ArenaBaseAlign = 16;
inline constexpr std::uint32_t InlineMax = 12;
inline constexpr std::uint32_t RankMax = 8;

enum class Tag : std::uint8_t {
    Null      = 0,
    Bool      = 1,
    I64       = 2,
    F64       = 3,
    String    = 4,
    Array     = 5,
    Object    = 6,
    TypedArray= 7,
    U64       = 8,
};

enum class DType : std::uint16_t {
    I8  = 1,
    U8  = 2,
    I16 = 3,
    U16 = 4,
    I32 = 5,
    U32 = 6,
    I64 = 7,
    U64 = 8,
    F32 = 9,
    F64 = 10,
};

// ---- little-endian IO helpers (byte layouts, not packed structs) ----
inline std::uint16_t read_u16_le(const std::uint8_t* p) {
    return std::uint16_t(p[0]) | (std::uint16_t(p[1]) << 8);
}
inline std::uint32_t read_u32_le(const std::uint8_t* p) {
    return std::uint32_t(p[0]) |
           (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) |
           (std::uint32_t(p[3]) << 24);
}
inline std::uint64_t read_u64_le(const std::uint8_t* p) {
    return std::uint64_t(read_u32_le(p)) | (std::uint64_t(read_u32_le(p + 4)) << 32);
}

inline void append_u16_le(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(std::uint8_t(v & 0xff));
    out.push_back(std::uint8_t((v >> 8) & 0xff));
}
inline void append_u32_le(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(std::uint8_t(v & 0xff));
    out.push_back(std::uint8_t((v >> 8) & 0xff));
    out.push_back(std::uint8_t((v >> 16) & 0xff));
    out.push_back(std::uint8_t((v >> 24) & 0xff));
}
inline void append_u64_le(std::vector<std::uint8_t>& out, std::uint64_t v) {
    append_u32_le(out, std::uint32_t(v & 0xffffffffu));
    append_u32_le(out, std::uint32_t((v >> 32) & 0xffffffffu));
}

inline void write_u16_le_at(std::vector<std::uint8_t>& out, std::size_t at, std::uint16_t v) {
    out.at(at + 0) = std::uint8_t(v & 0xff);
    out.at(at + 1) = std::uint8_t((v >> 8) & 0xff);
}
inline void write_u32_le_at(std::vector<std::uint8_t>& out, std::size_t at, std::uint32_t v) {
    out.at(at + 0) = std::uint8_t(v & 0xff);
    out.at(at + 1) = std::uint8_t((v >> 8) & 0xff);
    out.at(at + 2) = std::uint8_t((v >> 16) & 0xff);
    out.at(at + 3) = std::uint8_t((v >> 24) & 0xff);
}

inline std::size_t align_up(std::size_t x, std::size_t a) {
    if (a == 0) return x;
    const std::size_t r = x % a;
    return r ? (x + (a - r)) : x;
}

// =============================================================================
//  Reader (Deserializer)
// =============================================================================

struct HeaderView {
    std::uint32_t magic = 0;
    std::uint16_t version = 0;
    std::uint16_t flags = 0;
    std::uint32_t root_ofs = 0;
    std::uint32_t env_size = 0;
    std::uint32_t arena_ofs = 0;
};

inline HeaderView parse_header(std::span<const std::uint8_t> buf) {
    if (buf.size() < HeaderSize) throw DeserializationError("zer: truncated header");
    HeaderView h{};
    h.magic     = read_u32_le(buf.data() + 0);
    h.version   = read_u16_le(buf.data() + 4);
    h.flags     = read_u16_le(buf.data() + 6);
    h.root_ofs  = read_u32_le(buf.data() + 8);
    h.env_size  = read_u32_le(buf.data() + 12);
    h.arena_ofs = read_u32_le(buf.data() + 16);
    return h;
}

class ZerValue;

class ZerViewBase {
protected:
    const std::uint8_t* buf_ = nullptr;
    std::size_t buf_len_ = 0;
    const std::uint8_t* env_ = nullptr;
    std::size_t env_size_ = 0;
    const std::uint8_t* arena_ = nullptr;
    std::size_t arena_len_ = 0;
    const std::uint8_t* vr_ = nullptr; // points to a ValueRef16 byte layout

    ZerViewBase() = default;

    ZerViewBase(const std::uint8_t* buf, std::size_t len,
                const std::uint8_t* env, std::size_t env_size,
                const std::uint8_t* arena, std::size_t arena_len,
                const std::uint8_t* vr)
        : buf_(buf), buf_len_(len), env_(env), env_size_(env_size), arena_(arena), arena_len_(arena_len), vr_(vr) {}

    ZerViewBase(const ZerViewBase& parent, const std::uint8_t* vr)
        : buf_(parent.buf_), buf_len_(parent.buf_len_),
          env_(parent.env_), env_size_(parent.env_size_),
          arena_(parent.arena_), arena_len_(parent.arena_len_),
          vr_(vr) {}

    [[noreturn]] static void fail(std::string_view msg) {
        throw DeserializationError(std::string(msg));
    }
    static void require(bool ok, std::string_view msg) {
        if (!ok) fail(msg);
    }

    void require_vr() const {
        require(vr_ != nullptr, "zer: null ValueRef");
        require(vr_ >= env_ && vr_ + 16 <= env_ + env_size_, "zer: ValueRef out of bounds");
    }

    Tag tag() const { require_vr(); return Tag(vr_[0]); }
    std::uint8_t flags() const { require_vr(); return vr_[1]; }
    std::uint16_t aux() const { require_vr(); return read_u16_le(vr_ + 2); }
    std::uint32_t a() const { require_vr(); return read_u32_le(vr_ + 4); }
    std::uint32_t b() const { require_vr(); return read_u32_le(vr_ + 8); }
    std::uint32_t c() const { require_vr(); return read_u32_le(vr_ + 12); }

    std::string_view inline_bytes_view(std::size_t n) const {
        require(n <= InlineMax, "zer: inline payload too large");
        require_vr();
        return std::string_view(reinterpret_cast<const char*>(vr_ + 4), n);
    }

    std::string_view arena_bytes_view(std::uint32_t ofs, std::uint32_t len) const {
        require(ofs <= arena_len_, "zer: arena offset out of bounds");
        require(len <= arena_len_, "zer: arena length out of bounds");
        require(std::size_t(ofs) + std::size_t(len) <= arena_len_, "zer: arena span out of bounds");
        return std::string_view(reinterpret_cast<const char*>(arena_ + ofs), len);
    }

    std::span<const std::byte> arena_blob_view(std::uint32_t ofs, std::uint32_t len) const {
        require(ofs <= arena_len_, "zer: arena offset out of bounds");
        require(len <= arena_len_, "zer: arena length out of bounds");
        require(std::size_t(ofs) + std::size_t(len) <= arena_len_, "zer: arena span out of bounds");
        return std::span<const std::byte>(reinterpret_cast<const std::byte*>(arena_ + ofs), len);
    }

    const std::uint8_t* env_ptr_at(std::uint32_t ofs, std::size_t need) const {
        require(ofs <= env_size_, "zer: envelope offset out of bounds");
        require(need <= env_size_, "zer: envelope need out of bounds");
        require(std::size_t(ofs) + need <= env_size_, "zer: envelope span out of bounds");
        return env_ + ofs;
    }

    void require_flags_ok() const {
        const auto t = tag();
        const std::uint8_t fl = flags();
        if (t == Tag::String) {
            require((fl & ~std::uint8_t{1}) == 0, "zer: unknown ValueRef flags");
        } else {
            require(fl == 0, "zer: non-string ValueRef has flags set");
        }
    }

public:
    // Predicates
    bool isNull()   const { return tag() == Tag::Null; }
    bool isBool()   const { return tag() == Tag::Bool; }
    bool isInt()    const { return tag() == Tag::I64; }
    bool isUInt()   const { return tag() == Tag::U64; }
    bool isFloat()  const { return tag() == Tag::F64; }
    bool isString() const { return tag() == Tag::String; }
    bool isArray()  const { return tag() == Tag::Array; }
    bool isMap()    const { return tag() == Tag::Object; }
    bool isBlob()   const { return tag() == Tag::TypedArray && aux() == std::uint16_t(DType::U8); }

    // Scalars
    bool asBool() const {
        require(tag() == Tag::Bool, "zer: value is not a bool");
        require_flags_ok();
        auto v = aux();
        require(v == 0 || v == 1, "zer: invalid bool aux");
        return v == 1;
    }

    std::int64_t asInt64() const {
        require(tag() == Tag::I64 || tag() == Tag::U64, "zer: value is not an integer");
        require_flags_ok();
        const std::uint64_t bits = std::uint64_t(a()) | (std::uint64_t(b()) << 32);
        if (tag() == Tag::U64) {
            require(bits <= std::uint64_t(std::numeric_limits<std::int64_t>::max()), "zer: uint64 out of range for int64");
            return static_cast<std::int64_t>(bits);
        }
        return static_cast<std::int64_t>(bits);
    }

    std::uint64_t asUInt64() const {
        require(tag() == Tag::U64 || tag() == Tag::I64, "zer: value is not an integer");
        require_flags_ok();
        const std::uint64_t bits = std::uint64_t(a()) | (std::uint64_t(b()) << 32);
        if (tag() == Tag::I64) {
            const std::int64_t si = static_cast<std::int64_t>(bits);
            require(si >= 0, "zer: int64 out of range for uint64");
            return static_cast<std::uint64_t>(si);
        }
        return bits;
    }

    double asDouble() const {
        require(tag() == Tag::F64, "zer: value is not a float");
        require_flags_ok();
        const std::uint64_t bits = std::uint64_t(a()) | (std::uint64_t(b()) << 32);
        double out;
        static_assert(sizeof(out) == sizeof(bits));
        std::memcpy(&out, &bits, sizeof(out));
        return out;
    }

    float asFloat() const { return static_cast<float>(asDouble()); }

    std::int8_t asInt8() const {
        auto v = asInt64();
        if (v < std::numeric_limits<std::int8_t>::min() || v > std::numeric_limits<std::int8_t>::max())
            throw DeserializationError("zer: int8 out of range");
        return static_cast<std::int8_t>(v);
    }
    std::int16_t asInt16() const {
        auto v = asInt64();
        if (v < std::numeric_limits<std::int16_t>::min() || v > std::numeric_limits<std::int16_t>::max())
            throw DeserializationError("zer: int16 out of range");
        return static_cast<std::int16_t>(v);
    }
    std::int32_t asInt32() const {
        auto v = asInt64();
        if (v < std::numeric_limits<std::int32_t>::min() || v > std::numeric_limits<std::int32_t>::max())
            throw DeserializationError("zer: int32 out of range");
        return static_cast<std::int32_t>(v);
    }

    std::uint8_t asUInt8() const {
        auto v = asUInt64();
        if (v > std::numeric_limits<std::uint8_t>::max()) throw DeserializationError("zer: uint8 out of range");
        return static_cast<std::uint8_t>(v);
    }
    std::uint16_t asUInt16() const {
        auto v = asUInt64();
        if (v > std::numeric_limits<std::uint16_t>::max()) throw DeserializationError("zer: uint16 out of range");
        return static_cast<std::uint16_t>(v);
    }
    std::uint32_t asUInt32() const {
        auto v = asUInt64();
        if (v > std::numeric_limits<std::uint32_t>::max()) throw DeserializationError("zer: uint32 out of range");
        return static_cast<std::uint32_t>(v);
    }

    std::string_view asStringView() const {
        require(tag() == Tag::String, "zer: value is not a string");
        require_flags_ok();
        const bool inline_data = (flags() & 1) != 0;
        if (inline_data) {
            const auto len = aux();
            require(len <= InlineMax, "zer: inline string length too large");
            return inline_bytes_view(len);
        }
        return arena_bytes_view(a(), b());
    }

    std::string asString() const {
        auto sv = asStringView();
        return std::string(sv.data(), sv.size());
    }

    std::span<const std::byte> asBlob() const {
        require(isBlob(), "zer: value is not a blob");
        require_flags_ok();

        const std::uint32_t shape_ofs = c();
        const auto* sp = env_ptr_at(shape_ofs, 4);
        const std::uint32_t rank = read_u32_le(sp);
        require(rank <= RankMax, "zer: blob rank too large");
        require(rank == 1, "zer: blob must be rank 1");
        (void)env_ptr_at(shape_ofs, 4 + 8 * std::size_t(rank));
        const std::uint64_t dim0 = read_u64_le(env_ + shape_ofs + 4);
        require(dim0 == b(), "zer: blob shape length mismatch");

        return arena_blob_view(a(), b());
    }

    std::size_t arraySize() const {
        require(tag() == Tag::Array, "zer: not an array");
        require_flags_ok();
        const std::uint32_t arr_ofs = a();
        const auto* p = env_ptr_at(arr_ofs, 4);
        const std::uint32_t count = read_u32_le(p);
        (void)env_ptr_at(arr_ofs, 4 + 16 * std::size_t(count));
        return static_cast<std::size_t>(count);
    }

    // mapKeys() must be a forward range of string_view, zero-alloc.
    struct KeysView {
        const ZerViewBase* self = nullptr;
        const std::uint8_t* p = nullptr;
        std::uint32_t count = 0;

        struct iterator {
            const ZerViewBase* self = nullptr;
            const std::uint8_t* cur = nullptr;
            std::uint32_t i = 0;
            std::uint32_t n = 0;
            using iterator_category = std::forward_iterator_tag;
            using iterator_concept  = std::forward_iterator_tag;
            using value_type        = std::string_view;
            using difference_type   = std::ptrdiff_t;
            using reference         = std::string_view;

            reference operator*() const {
                if (i >= n) ZerViewBase::fail("zer: KeysView deref out of range");
                const auto key_len = read_u16_le(cur);
                (void)self->env_ptr_at(std::uint32_t(cur - self->env_), 4 + key_len + 16);
                return std::string_view(reinterpret_cast<const char*>(cur + 4), key_len);
            }

            iterator& operator++() {
                if (i >= n) return *this;
                const auto key_len = read_u16_le(cur);
                const std::size_t adv = 4 + std::size_t(key_len) + 16;
                const auto cur_ofs = std::uint32_t(cur - self->env_);
                cur = self->env_ptr_at(cur_ofs + std::uint32_t(adv), 0);
                ++i;
                return *this;
            }
            iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
            friend bool operator==(const iterator& a, const iterator& b) { return a.self==b.self && a.i==b.i && a.n==b.n; }
        };

        iterator begin() const { return iterator{self, p, 0, count}; }
        iterator end() const {
            auto it = iterator{self, p, count, count};
            return it;
        }
    };

    KeysView mapKeys() const {
        require(tag() == Tag::Object, "zer: not a map");
        require_flags_ok();
        const std::uint32_t obj_ofs = a();
        const auto* p = env_ptr_at(obj_ofs, 4);
        const std::uint32_t count = read_u32_le(p);
        const auto* entries = env_ptr_at(obj_ofs + 4, 0);
        return KeysView{this, entries, count};
    }

    bool contains(std::string_view key) const {
        if (!isMap()) return false;
        (void)require_flags_ok();
        const std::uint32_t obj_ofs = a();
        const auto* p = env_ptr_at(obj_ofs, 4);
        const std::uint32_t count = read_u32_le(p);
        std::size_t ofs = obj_ofs + 4;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto* e = env_ptr_at(static_cast<std::uint32_t>(ofs), 4);
            const std::uint16_t key_len = read_u16_le(e);
            (void)read_u16_le(e + 2); // reserved
            const auto* key_bytes = env_ptr_at(static_cast<std::uint32_t>(ofs + 4), key_len);
            if (key_len == key.size() &&
                std::memcmp(key_bytes, key.data(), key.size()) == 0) return true;
            ofs += 4 + std::size_t(key_len) + 16;
            (void)env_ptr_at(static_cast<std::uint32_t>(ofs), 0);
        }
        return false;
    }

    ZerValue operator[](std::size_t idx) const;
    ZerValue operator[](std::string_view key) const;

    std::string to_string() const {
        std::ostringstream os;
        os << "Zer(";
        const auto t = tag();
        switch (t) {
            case Tag::Null: os << "null"; break;
            case Tag::Bool: os << (asBool() ? "true" : "false"); break;
            case Tag::I64:  os << asInt64(); break;
            case Tag::U64:  os << asUInt64(); break;
            case Tag::F64:  os << asDouble(); break;
            case Tag::String: os << "str[len=" << asStringView().size() << "]"; break;
            case Tag::Array: os << "arr[n=" << arraySize() << "]"; break;
            case Tag::Object: os << "map"; break;
            case Tag::TypedArray: os << (isBlob() ? "blob[len=" : "typed[len=") << b() << "]"; break;
            default: os << "unknown"; break;
        }
        os << ")";
        return os.str();
    }
};

class ZerValue final : public ZerViewBase {
public:
    ZerValue(const ZerViewBase& parent, const std::uint8_t* vr)
        : ZerViewBase(parent, vr) {}
};

inline ZerValue ZerViewBase::operator[](std::size_t idx) const {
    require(tag() == Tag::Array, "zer: not an array");
    require_flags_ok();
    const std::uint32_t arr_ofs = a();
    const auto* p = env_ptr_at(arr_ofs, 4);
    const std::uint32_t count = read_u32_le(p);
    require(idx < count, "zer: array index out of bounds");
    const std::size_t elem_ofs = std::size_t(arr_ofs) + 4 + 16 * idx;
    const auto* vr = env_ptr_at(static_cast<std::uint32_t>(elem_ofs), 16);
    return ZerValue(*this, vr);
}

inline ZerValue ZerViewBase::operator[](std::string_view key) const {
    require(tag() == Tag::Object, "zer: not a map");
    require_flags_ok();
    const std::uint32_t obj_ofs = a();
    const auto* p = env_ptr_at(obj_ofs, 4);
    const std::uint32_t count = read_u32_le(p);
    std::size_t ofs = obj_ofs + 4;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto* e = env_ptr_at(static_cast<std::uint32_t>(ofs), 4);
        const std::uint16_t key_len = read_u16_le(e);
        (void)read_u16_le(e + 2); // reserved
        const auto* key_bytes = env_ptr_at(static_cast<std::uint32_t>(ofs + 4), key_len);
        const auto* value_vr = env_ptr_at(static_cast<std::uint32_t>(ofs + 4 + key_len), 16);
        if (key_len == key.size() && std::memcmp(key_bytes, key.data(), key.size()) == 0) {
            return ZerValue(*this, value_vr);
        }
        ofs += 4 + std::size_t(key_len) + 16;
        (void)env_ptr_at(static_cast<std::uint32_t>(ofs), 0);
    }
    throw DeserializationError("zer: key not found: " + std::string(key));
}

class ZerDeserializer final : public ZerViewBase {
    std::vector<std::uint8_t> owned_;
    std::span<const std::uint8_t> view_{};

    void init_from(std::span<const std::uint8_t> buf) {
        auto h = parse_header(buf);
        if (h.magic != Magic) throw DeserializationError("zer: bad magic");
        if (h.version != Version) throw DeserializationError("zer: unsupported version");
        if (h.flags != 1) throw DeserializationError("zer: flags invalid (expected little-endian bit0)");

        require(h.env_size <= buf.size(), "zer: env_size out of bounds");
        require(h.root_ofs < h.env_size, "zer: root_ofs out of bounds");
        require(h.arena_ofs <= buf.size(), "zer: arena_ofs out of bounds");
        require((h.arena_ofs % ArenaBaseAlign) == 0, "zer: arena_ofs not aligned");
        require(h.arena_ofs >= HeaderSize + h.env_size, "zer: arena_ofs overlaps envelope");
        require(HeaderSize + h.env_size <= h.arena_ofs, "zer: envelope exceeds arena_ofs");

        buf_ = buf.data();
        buf_len_ = buf.size();
        env_ = buf_ + HeaderSize;
        env_size_ = h.env_size;
        arena_ = buf_ + h.arena_ofs;
        arena_len_ = buf.size() - h.arena_ofs;

        require(h.root_ofs <= env_size_ - 16, "zer: root ValueRef out of bounds");
        vr_ = env_ + h.root_ofs;
    }

public:
    ZerDeserializer() = default;

    explicit ZerDeserializer(const std::vector<std::uint8_t>& buf) : owned_(buf) {
        view_ = std::span<const std::uint8_t>(owned_.data(), owned_.size());
        init_from(view_);
    }
    explicit ZerDeserializer(std::vector<std::uint8_t>&& buf) : owned_(std::move(buf)) {
        view_ = std::span<const std::uint8_t>(owned_.data(), owned_.size());
        init_from(view_);
    }
    explicit ZerDeserializer(std::span<const std::uint8_t> buf) : view_(buf) {
        init_from(view_);
    }
    explicit ZerDeserializer(const std::uint8_t* data, std::size_t n) : view_(data, n) {
        init_from(view_);
    }
    explicit ZerDeserializer(const std::byte* data, std::size_t n)
        : ZerDeserializer(reinterpret_cast<const std::uint8_t*>(data), n) {}

    using ZerViewBase::isNull;
    using ZerViewBase::isBool;
    using ZerViewBase::isInt;
    using ZerViewBase::isUInt;
    using ZerViewBase::isFloat;
    using ZerViewBase::isString;
    using ZerViewBase::isBlob;
    using ZerViewBase::isMap;
    using ZerViewBase::isArray;
    using ZerViewBase::asInt8;
    using ZerViewBase::asInt16;
    using ZerViewBase::asInt32;
    using ZerViewBase::asInt64;
    using ZerViewBase::asUInt8;
    using ZerViewBase::asUInt16;
    using ZerViewBase::asUInt32;
    using ZerViewBase::asUInt64;
    using ZerViewBase::asFloat;
    using ZerViewBase::asDouble;
    using ZerViewBase::asString;
    using ZerViewBase::asStringView;
    using ZerViewBase::asBool;
    using ZerViewBase::asBlob;
    using ZerViewBase::mapKeys;
    using ZerViewBase::contains;
    using ZerViewBase::arraySize;
    using ZerViewBase::operator[];
    using ZerViewBase::to_string;
};

// =============================================================================
//  Writer (Builder)
// =============================================================================

struct RootSerializer {
    struct ArrayCtx {
        std::vector<std::uint8_t> payload; // [u32 count][ValueRef16...]
        std::uint32_t count = 0;
    };
    struct MapCtx {
        std::vector<std::uint8_t> payload; // [u32 count][entries...]
        std::uint32_t count = 0;
        std::optional<std::size_t> pending_value_patch; // offset within payload
    };

    std::vector<std::variant<ArrayCtx, MapCtx>> st_;
    std::vector<std::uint8_t> env_;     // finalized envelope payloads + root ValueRef16
    std::vector<std::uint8_t> arena_;   // arena bytes
    std::optional<std::uint32_t> root_ofs_;
    std::uint32_t inline_threshold_ = InlineMax;

    RootSerializer() = default;

    void set_inline_string_threshold(std::uint32_t t) {
        if (t > InlineMax) throw SerializationError("zer: inline string threshold must be <= 12");
        inline_threshold_ = t;
    }

    ZBuffer finish() {
        if (!st_.empty()) throw SerializationError("zer: finish() called with unterminated container");
        if (!root_ofs_) {
            // Default root = null
            auto vr = make_vr(Tag::Null, 0, 0, 0, 0, 0);
            write_root_vr(vr);
        }

        if (env_.size() > std::numeric_limits<std::uint32_t>::max()) throw SerializationError("zer: envelope too large");
        if (arena_.size() > std::numeric_limits<std::uint32_t>::max()) throw SerializationError("zer: arena too large");

        const std::uint32_t env_size = static_cast<std::uint32_t>(env_.size());
        const std::size_t arena_ofs = align_up(HeaderSize + std::size_t(env_size), ArenaBaseAlign);
        if (arena_ofs > std::numeric_limits<std::uint32_t>::max())
            throw SerializationError("zer: arena_ofs overflow");

        std::vector<std::uint8_t> out;
        out.resize(arena_ofs + arena_.size(), 0);

        auto write_header32 = [&](std::size_t at, std::uint32_t v) {
            out.at(at + 0) = std::uint8_t(v & 0xff);
            out.at(at + 1) = std::uint8_t((v >> 8) & 0xff);
            out.at(at + 2) = std::uint8_t((v >> 16) & 0xff);
            out.at(at + 3) = std::uint8_t((v >> 24) & 0xff);
        };
        auto write_header16 = [&](std::size_t at, std::uint16_t v) {
            out.at(at + 0) = std::uint8_t(v & 0xff);
            out.at(at + 1) = std::uint8_t((v >> 8) & 0xff);
        };

        write_header32(0, Magic);
        write_header16(4, Version);
        write_header16(6, 1); // flags: bit0 little-endian
        write_header32(8, *root_ofs_);
        write_header32(12, env_size);
        write_header32(16, static_cast<std::uint32_t>(arena_ofs));

        std::memcpy(out.data() + HeaderSize, env_.data(), env_.size());
        std::memcpy(out.data() + arena_ofs, arena_.data(), arena_.size());

        return ZBuffer(std::move(out));
    }

    // ---- streaming encoding helpers (called by Serializer) ----
    static std::array<std::uint8_t, 16> make_vr(Tag tag, std::uint8_t flags, std::uint16_t aux,
                                               std::uint32_t a, std::uint32_t b, std::uint32_t c) {
        std::array<std::uint8_t, 16> out{};
        out[0] = std::uint8_t(tag);
        out[1] = flags;
        out[2] = std::uint8_t(aux & 0xff);
        out[3] = std::uint8_t((aux >> 8) & 0xff);
        out[4] = std::uint8_t(a & 0xff);
        out[5] = std::uint8_t((a >> 8) & 0xff);
        out[6] = std::uint8_t((a >> 16) & 0xff);
        out[7] = std::uint8_t((a >> 24) & 0xff);
        out[8] = std::uint8_t(b & 0xff);
        out[9] = std::uint8_t((b >> 8) & 0xff);
        out[10] = std::uint8_t((b >> 16) & 0xff);
        out[11] = std::uint8_t((b >> 24) & 0xff);
        out[12] = std::uint8_t(c & 0xff);
        out[13] = std::uint8_t((c >> 8) & 0xff);
        out[14] = std::uint8_t((c >> 16) & 0xff);
        out[15] = std::uint8_t((c >> 24) & 0xff);
        return out;
    }

    std::uint32_t append_env_payload(std::span<const std::uint8_t> bytes) {
        const std::size_t ofs = env_.size();
        if (ofs > std::numeric_limits<std::uint32_t>::max()) throw SerializationError("zer: envelope offset overflow");
        env_.insert(env_.end(), bytes.begin(), bytes.end());
        return static_cast<std::uint32_t>(ofs);
    }

    std::uint32_t arena_alloc(std::size_t len, std::size_t align) {
        const std::size_t want_align = std::max<std::size_t>(1, align);
        const std::size_t aligned = align_up(arena_.size(), want_align);
        if (aligned > arena_.size()) arena_.resize(aligned, 0);
        const std::size_t ofs = arena_.size();
        arena_.resize(ofs + len, 0);
        if (ofs > std::numeric_limits<std::uint32_t>::max()) throw SerializationError("zer: arena offset overflow");
        if (len > std::numeric_limits<std::uint32_t>::max()) throw SerializationError("zer: arena length overflow");
        return static_cast<std::uint32_t>(ofs);
    }

    std::uint32_t emit_shape_rank1(std::uint64_t dim0) {
        std::array<std::uint8_t, 12> tmp{};
        tmp[0] = 1; tmp[1] = 0; tmp[2] = 0; tmp[3] = 0; // rank u32
        for (int i = 0; i < 8; ++i) tmp[4 + i] = std::uint8_t((dim0 >> (8 * i)) & 0xff);
        return append_env_payload(tmp);
    }

    void write_root_vr(const std::array<std::uint8_t, 16>& vr) {
        if (root_ofs_) throw SerializationError("zer: multiple root values");
        root_ofs_ = append_env_payload(vr);
    }

    void deliver_vr(const std::array<std::uint8_t, 16>& vr) {
        if (st_.empty()) {
            write_root_vr(vr);
            return;
        }
        auto& top = st_.back();
        if (auto* a = std::get_if<ArrayCtx>(&top)) {
            a->payload.insert(a->payload.end(), vr.begin(), vr.end());
            ++a->count;
            return;
        }
        auto* m = std::get_if<MapCtx>(&top);
        if (!m) throw SerializationError("zer: internal container stack error");
        if (!m->pending_value_patch) throw SerializationError("zer: map value without key()");
        const std::size_t at = *m->pending_value_patch;
        if (at + 16 > m->payload.size()) throw SerializationError("zer: internal map patch out of bounds");
        std::memcpy(m->payload.data() + at, vr.data(), 16);
        m->pending_value_patch.reset();
    }
};

struct Serializer {
    RootSerializer* r;
    explicit Serializer(RootSerializer& rs) : r(&rs) {}

    void null() { r->deliver_vr(RootSerializer::make_vr(Tag::Null, 0, 0, 0, 0, 0)); }
    void boolean(bool v) { r->deliver_vr(RootSerializer::make_vr(Tag::Bool, 0, v ? 1 : 0, 0, 0, 0)); }
    void int64(std::int64_t v) {
        const std::uint64_t bits = static_cast<std::uint64_t>(v);
        r->deliver_vr(RootSerializer::make_vr(
            Tag::I64, 0, 0,
            static_cast<std::uint32_t>(bits & 0xffffffffu),
            static_cast<std::uint32_t>((bits >> 32) & 0xffffffffu),
            0));
    }
    void uint64(std::uint64_t v) {
        r->deliver_vr(RootSerializer::make_vr(
            Tag::U64, 0, 0,
            static_cast<std::uint32_t>(v & 0xffffffffu),
            static_cast<std::uint32_t>((v >> 32) & 0xffffffffu),
            0));
    }
    void double_(double v) {
        std::uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(v));
        std::memcpy(&bits, &v, sizeof(bits));
        r->deliver_vr(RootSerializer::make_vr(
            Tag::F64, 0, 0,
            static_cast<std::uint32_t>(bits & 0xffffffffu),
            static_cast<std::uint32_t>((bits >> 32) & 0xffffffffu),
            0));
    }

    void string(std::string_view sv) {
        if (sv.size() <= std::min<std::size_t>(r->inline_threshold_, InlineMax)) {
            auto out = RootSerializer::make_vr(Tag::String, 1, static_cast<std::uint16_t>(sv.size()), 0, 0, 0);
            if (!sv.empty()) std::memcpy(out.data() + 4, sv.data(), sv.size());
            r->deliver_vr(out);
            return;
        }
        if (sv.size() > std::numeric_limits<std::uint32_t>::max()) throw SerializationError("zer: string too large");
        const auto ofs = r->arena_alloc(sv.size(), 1);
        if (!sv.empty()) std::memcpy(r->arena_.data() + ofs, sv.data(), sv.size());
        r->deliver_vr(RootSerializer::make_vr(Tag::String, 0, 0, ofs, static_cast<std::uint32_t>(sv.size()), 0));
    }
    void binary(std::span<const std::byte> b) {
        if (b.size() > std::numeric_limits<std::uint32_t>::max()) throw SerializationError("zer: blob too large");
        const std::uint32_t byte_len = static_cast<std::uint32_t>(b.size());
        const std::uint32_t arena_ofs = r->arena_alloc(byte_len, ArenaBaseAlign);
        if (byte_len) std::memcpy(r->arena_.data() + arena_ofs, b.data(), byte_len);
        const std::uint32_t shape_ofs = r->emit_shape_rank1(byte_len);
        r->deliver_vr(RootSerializer::make_vr(
            Tag::TypedArray, 0, static_cast<std::uint16_t>(DType::U8),
            arena_ofs, byte_len, shape_ofs));
    }

    void begin_array(std::size_t reserve) {
        RootSerializer::ArrayCtx ctx{};
        ctx.payload.reserve(4 + reserve * 16);
        append_u32_le(ctx.payload, 0); // count placeholder
        r->st_.push_back(std::move(ctx));
    }
    void end_array() {
        if (r->st_.empty() || !std::holds_alternative<RootSerializer::ArrayCtx>(r->st_.back()))
            throw SerializationError("zer: end_array outside array");
        auto ctx = std::move(std::get<RootSerializer::ArrayCtx>(r->st_.back()));
        r->st_.pop_back();
        if (ctx.count > std::numeric_limits<std::uint32_t>::max()) throw SerializationError("zer: array too large");
        write_u32_le_at(ctx.payload, 0, ctx.count);
        const std::uint32_t payload_ofs = r->append_env_payload(ctx.payload);
        r->deliver_vr(RootSerializer::make_vr(Tag::Array, 0, 0, payload_ofs, 0, 0));
    }

    void begin_map(std::size_t reserve) {
        RootSerializer::MapCtx ctx{};
        ctx.payload.reserve(4 + reserve * (4 + 8 + 16));
        append_u32_le(ctx.payload, 0); // count placeholder
        r->st_.push_back(std::move(ctx));
    }
    void end_map() {
        if (r->st_.empty() || !std::holds_alternative<RootSerializer::MapCtx>(r->st_.back()))
            throw SerializationError("zer: end_map outside map");
        auto ctx = std::move(std::get<RootSerializer::MapCtx>(r->st_.back()));
        if (ctx.pending_value_patch) throw SerializationError("zer: end_map with dangling key()");
        r->st_.pop_back();
        write_u32_le_at(ctx.payload, 0, ctx.count);
        const std::uint32_t payload_ofs = r->append_env_payload(ctx.payload);
        r->deliver_vr(RootSerializer::make_vr(Tag::Object, 0, 0, payload_ofs, 0, 0));
    }

    void key(std::string_view k) {
        if (r->st_.empty() || !std::holds_alternative<RootSerializer::MapCtx>(r->st_.back()))
            throw SerializationError("zer: key() outside map");
        auto& ctx = std::get<RootSerializer::MapCtx>(r->st_.back());
        if (ctx.pending_value_patch) throw SerializationError("zer: key() called twice without value");
        if (k.size() > std::numeric_limits<std::uint16_t>::max()) throw SerializationError("zer: key too long");
        append_u16_le(ctx.payload, static_cast<std::uint16_t>(k.size()));
        append_u16_le(ctx.payload, 0);
        ctx.payload.insert(ctx.payload.end(), k.begin(), k.end());
        const std::size_t patch = ctx.payload.size();
        ctx.payload.resize(ctx.payload.size() + 16, 0);
        ctx.pending_value_patch = patch;
        ++ctx.count;
    }
};

} // namespace zer

struct Zer {
    static inline constexpr const char* Name = "Zer";
    using Deserializer   = zer::ZerDeserializer;
    using RootSerializer = zer::RootSerializer;
    using Serializer     = zer::Serializer;
};

} // namespace zerialize
