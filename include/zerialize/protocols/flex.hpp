#pragma once

#include <flatbuffers/flexbuffers.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <stdexcept>

namespace zerialize {
namespace flex {

// ========================== Writer (Serializer) ===============================

struct RootSerializer {
    ::flexbuffers::Builder fbb;
    bool finished_  = false;
    bool wrote_root_ = false;

    // track container "starts" for EndMap/EndVector
    struct Ctx {
        enum K { Arr, Obj } k;
        std::size_t start;
    };
    std::vector<Ctx> st;

    RootSerializer() = default;

    ZBuffer finish() {
        if (!finished_) {
            if (!wrote_root_) fbb.Null();  // ensure we wrote a root
            fbb.Finish();
            finished_ = true;
        }
        // FlexBuffers returns a const ref; we “steal” it (your prior pattern)
        const std::vector<uint8_t>& buf = fbb.GetBuffer();
        auto& hack = const_cast<std::vector<uint8_t>&>(buf);
        return ZBuffer(std::move(hack));
    }
};

struct Serializer {
    RootSerializer* r;

    explicit Serializer(RootSerializer& rs) : r(&rs) {}

    // ---- primitives ----
    void null()                  { r->fbb.Null();  r->wrote_root_ = true; }
    void boolean(bool v)         { r->fbb.Bool(v); r->wrote_root_ = true; }
    void int64(std::int64_t v)   { r->fbb.Int(v);  r->wrote_root_ = true; }
    void uint64(std::uint64_t v) { r->fbb.UInt(v); r->wrote_root_ = true; }
    void double_(double v)       { r->fbb.Double(v); r->wrote_root_ = true; }
    void string(std::string_view sv) {
        r->fbb.String(sv.data(), sv.size()); r->wrote_root_ = true;
    }
    void binary(std::span<const std::byte> b) {
        auto ptr = reinterpret_cast<const std::uint8_t*>(b.data());
        r->fbb.Blob(ptr, b.size()); r->wrote_root_ = true;
    }

    // ---- structures ----
    void begin_array(std::size_t /*reserve*/) {
        std::size_t start = r->fbb.StartVector();
        r->st.push_back({RootSerializer::Ctx::Arr, start});
        r->wrote_root_ = true;
    }
    void end_array() {
        ensure_in(RootSerializer::Ctx::Arr, "end_array");
        auto start = r->st.back().start;
        r->st.pop_back();
        // typed=false, fixed=false (generic JSON-like array)
        (void)r->fbb.EndVector(start, /*typed=*/false, /*fixed=*/false);
    }

    void begin_map(std::size_t /*reserve*/) {
        std::size_t start = r->fbb.StartMap();
        r->st.push_back({RootSerializer::Ctx::Obj, start});
        r->wrote_root_ = true;
    }
    void end_map() {
        ensure_in(RootSerializer::Ctx::Obj, "end_map");
        auto start = r->st.back().start;
        r->st.pop_back();
        (void)r->fbb.EndMap(start);
    }

    void key(std::string_view k) {
        // FlexBuffers requires a key immediately before its value
        r->fbb.Key(k.data(), k.size());
    }

private:
    void ensure_in(RootSerializer::Ctx::K want, const char* fn) const {
        if (r->st.empty() || r->st.back().k != want)
            throw std::logic_error(std::string(fn) + " called outside correct container");
    }
};

// Optional protocol binder
// ========================== Reader (Deserializer) =============================

class FlexDeserializer {
protected:
    // If we construct from a vector, we own the bytes here.
    std::vector<uint8_t> owned_;
    // If we construct as a non-owning view, we keep a span here.
    std::span<const uint8_t> view_;
    // Reference into whichever storage is active.
    ::flexbuffers::Reference ref_{};

    static std::string json_escape(std::string_view s) {
        std::string out;
        out.reserve(s.size() + 4);
        for (unsigned char ch : s) {
            switch (ch) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b";  break;
                case '\f': out += "\\f";  break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:
                    if (ch < 0x20) {
                        char buf[7];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", ch);
                        out += buf;
                    } else {
                        out.push_back(static_cast<char>(ch));
                    }
            }
        }
        return out;
    }

    static void indent(std::ostringstream& os, int n) {
        for (int i = 0; i < n; ++i) os.put(' ');
    }

    static const char* type_code(const ::flexbuffers::Reference& r) {
        if (r.IsNull())   return "null";
        if (r.IsBool())   return "bool";
        if (r.IsInt())    return "int";
        if (r.IsUInt())   return "uint";
        if (r.IsFloat())  return "float";
        if (r.IsString()) return "str";
        if (r.IsBlob())   return "blob";
        if (r.IsMap())    return "map";
        if (r.IsAnyVector()) return "arr";
        return "any";
    }

    static void dump_rec(std::ostringstream& os, const ::flexbuffers::Reference& r, int pad) {
        auto t = type_code(r);
        if (r.IsNull()) {
            os << t << "|null";
        } else if (r.IsBool()) {
            os << t << "| " << (r.AsBool() ? "true" : "false");
        } else if (r.IsInt()) {
            os << t << "|" << r.AsInt64();
        } else if (r.IsUInt()) {
            os << t << "|" << r.AsUInt64();
        } else if (r.IsFloat()) {
            os << t << "|" << r.AsDouble();
        } else if (r.IsString()) {
            auto sv = r.AsString();
            os << t << "|\"" << json_escape(std::string_view(sv.c_str(), sv.size())) << '"';
        } else if (r.IsBlob()) {
            auto b = r.AsBlob();
            os << t << "[size=" << b.size() << "]";
        } else if (r.IsMap()) {
            os << t << " {\n";
            auto m = r.AsMap();
            auto keys = m.Keys();
            auto vals = m.Values();
            for (size_t i = 0; i < keys.size(); ++i) {
                indent(os, pad + 2);
                auto ks = keys[i].AsString();
                os << '"' << json_escape(std::string_view(ks.c_str(), ks.size())) << "\": ";
                dump_rec(os, vals[i], pad + 2);
                if (i + 1 < keys.size()) os << ',';
                os << '\n';
            }
            indent(os, pad);
            os << '}';
        } else if (r.IsAnyVector()) {
            os << t << " [\n";
            auto v = r.AsVector();
            for (size_t i = 0; i < v.size(); ++i) {
                indent(os, pad + 2);
                dump_rec(os, v[i], pad + 2);
                if (i + 1 < v.size()) os << ',';
                os << '\n';
            }
            indent(os, pad);
            os << ']';
        } else {
            os << t; // fallback
        }
    }

public:
    // -------- constructors (owning) --------
    explicit FlexDeserializer(const std::vector<uint8_t>& buf)
      : owned_(buf),
        ref_(owned_.empty() ? ::flexbuffers::Reference{} : ::flexbuffers::GetRoot(owned_)) {}

    explicit FlexDeserializer(std::vector<uint8_t>&& buf)
      : owned_(std::move(buf)),
        ref_(owned_.empty() ? ::flexbuffers::Reference{} : ::flexbuffers::GetRoot(owned_)) {}

    // -------- constructors (non-owning / zero-copy) --------
    explicit FlexDeserializer(std::span<const uint8_t> bytes)
      : view_(bytes),
        ref_(view_.empty() ? ::flexbuffers::Reference{} : ::flexbuffers::GetRoot(view_.data(), view_.size())) {}

    explicit FlexDeserializer(const uint8_t* data, std::size_t n)
      : view_(data, n),
        ref_(n == 0 ? ::flexbuffers::Reference{} : ::flexbuffers::GetRoot(data, n)) {}

    explicit FlexDeserializer(const std::byte* data, std::size_t n)
      : FlexDeserializer(reinterpret_cast<const uint8_t*>(data), n) {}

    // -------- view ctor from Reference (nested access) --------
    explicit FlexDeserializer(::flexbuffers::Reference r) : ref_(r) {}

    // -------- predicates --------
    bool isNull()   const { return ref_.IsNull(); }
    bool isBool()   const { return ref_.IsBool(); }
    bool isInt()    const { return ref_.IsInt(); }
    bool isUInt()   const { return ref_.IsUInt(); }
    bool isFloat()  const { return ref_.IsFloat(); }
    bool isString() const { return ref_.IsString(); }
    bool isBlob()   const { return ref_.IsBlob(); }
    bool isMap()    const { return ref_.IsMap(); }
    bool isArray()  const { return ref_.IsAnyVector() && !ref_.IsMap(); }

    // -------- scalars --------
    int8_t   asInt8()   const { return ref_.AsInt8(); }
    int16_t  asInt16()  const { return ref_.AsInt16(); }
    int32_t  asInt32()  const { return ref_.AsInt32(); }
    int64_t  asInt64()  const { return ref_.AsInt64(); }

    uint8_t  asUInt8()  const { return static_cast<uint8_t>(ref_.AsUInt8()); }
    uint16_t asUInt16() const { return static_cast<uint16_t>(ref_.AsUInt16()); }
    uint32_t asUInt32() const { return static_cast<uint32_t>(ref_.AsUInt32()); }
    uint64_t asUInt64() const { return static_cast<uint64_t>(ref_.AsUInt64()); }

    float    asFloat()  const { return static_cast<float>(ref_.AsFloat()); }
    double   asDouble() const { return ref_.AsDouble(); }
    bool     asBool()   const { return ref_.AsBool(); }

    std::string      asString()     const { auto s = ref_.AsString(); return s.str(); }
    std::string_view asStringView() const { auto s = ref_.AsString(); return {s.c_str(), s.size()}; }

    std::span<const std::byte> asBlob() const {
        auto b = ref_.AsBlob();
        auto* p = reinterpret_cast<const std::byte*>(b.data());
        return {p, b.size()};
    }

    // Zero-alloc forward range of keys (StringViewRange-compatible)
    struct KeysView {
        ::flexbuffers::Map map; // non-owning view over backing bytes

        struct iterator {
            // default-constructible → satisfies forward_iterator
            const ::flexbuffers::Map* map = nullptr;
            std::size_t i = 0;

            using iterator_category = std::forward_iterator_tag;
            using iterator_concept  = std::forward_iterator_tag;
            using value_type        = std::string_view;
            using difference_type   = std::ptrdiff_t;
            using reference         = std::string_view;

            iterator() = default;
            iterator(const ::flexbuffers::Map* m, std::size_t idx) : map(m), i(idx) {}

            reference operator*() const {
                auto keys = map->Keys();
                auto s    = keys[i].AsString();
                return std::string_view(s.c_str(), s.size());
            }

            iterator& operator++() { ++i; return *this; }
            // (optional) post-increment to be thorough
            iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

            friend bool operator==(const iterator& a, const iterator& b) {
                return a.map == b.map && a.i == b.i;
            }
        };

        // Provide both const and non-const begin/end (libc++ checks both T& and const T&)
        iterator begin() { return iterator{&map, 0}; }
        iterator end()   { auto keys = map.Keys(); return iterator{&map, keys.size()}; }

        iterator begin() const { return iterator{&map, 0}; }
        iterator end()   const { auto keys = map.Keys(); return iterator{&map, keys.size()}; }
    };

    inline KeysView mapKeys() const { return KeysView{ ref_.AsMap() }; }

    bool contains(std::string_view key) const {
        auto m = ref_.AsMap();
        std::string k(key);
        auto r = m[k];
        return !r.IsNull();
    }

    FlexDeserializer operator[](std::string_view key) const {
        auto m = ref_.AsMap();
        std::string k(key);
        return FlexDeserializer(m[k]);
    }

    std::size_t arraySize() const { return ref_.AsVector().size(); }
    FlexDeserializer operator[](std::size_t idx) const {
        auto v = ref_.AsVector();
        return FlexDeserializer(v[idx]);
    }

    // -------- pretty printer with type codes --------
    std::string to_string() const {
        std::ostringstream os;
        dump_rec(os, ref_, 0);
        return os.str();
    }
};

} // namespace flex

struct Flex {
    static inline constexpr const char* Name = "Flex";
    using Deserializer   = flex::FlexDeserializer;
    using RootSerializer = flex::RootSerializer;
    using Serializer     = flex::Serializer;
};

} // namespace zerialize


namespace zerialize {
namespace flex {
namespace debugging {

    static void print_ref(flexbuffers::Reference r, int indent = 0);

    static void print_indent(int n) { while (n--) std::cout << ' '; }

    static void print_map(flexbuffers::Map m, int indent) {
        auto keys = m.Keys();
        auto vals = m.Values();
        std::cout << "{\n";
        for (size_t i = 0; i < m.size(); ++i) {
            print_indent(indent + 2);
            std::string k = keys[i].AsString().str();
            std::cout << std::quoted(k) << ": ";
            print_ref(vals[i], indent + 2);
            if (i + 1 < m.size()) std::cout << ",";
            std::cout << "\n";
        }
        print_indent(indent);
        std::cout << "}";
    }

    static void print_vector(flexbuffers::Vector v, int indent) {
        std::cout << "[\n";
        for (size_t i = 0; i < v.size(); ++i) {
            print_indent(indent + 2);
            print_ref(v[i], indent + 2);
            if (i + 1 < v.size()) std::cout << ",";
            std::cout << "\n";
        }
        print_indent(indent);
        std::cout << "]";
    }

    static void print_blob(flexbuffers::Blob b) {
        std::cout << "<blob:" << b.size() << " bytes>";
    }

    static void print_ref(flexbuffers::Reference r, int indent) {
        using T = flexbuffers::Type;
        switch (r.GetType()) {
            case T::FBT_NULL:   std::cout << "null"; break;
            case T::FBT_BOOL:   std::cout << (r.AsBool() ? "true" : "false"); break;
            case T::FBT_INT:    std::cout << r.AsInt64(); break;
            case T::FBT_UINT:   std::cout << r.AsUInt64(); break;
            case T::FBT_FLOAT:  std::cout << r.AsDouble(); break;

            // String may return a char* via .str(); wrap it so quoted works everywhere
            case T::FBT_STRING:
                std::cout << std::quoted(std::string(r.AsString().str()));
                break;

            // Key often is just const char*; wrap it too
            case T::FBT_KEY:
                std::cout << std::quoted(std::string(r.AsKey()));
                break;

            case T::FBT_BLOB:
                print_blob(r.AsBlob());
                break;

            case T::FBT_VECTOR:
            case T::FBT_VECTOR_INT:
            case T::FBT_VECTOR_UINT:
            case T::FBT_VECTOR_FLOAT:
            case T::FBT_VECTOR_BOOL:
            case T::FBT_VECTOR_KEY:
                print_vector(r.AsVector(), indent);
                break;

            case T::FBT_MAP:
                print_map(r.AsMap(), indent);
                break;

            default:
                std::cout << "<unknown>";
                break;
        }
    }

    // Original pointer-based overload
    static void dump_flex(const uint8_t* data, size_t size) {
        auto root = flexbuffers::GetRoot(data, size);
        print_ref(root);
        std::cout << "\n";
    }

    // Convenience overload for span (so you can pass k.buf() directly)
    static void dump_flex(std::span<const uint8_t> bytes) {
        dump_flex(bytes.data(), bytes.size());
    }

} // namespace debugging
} // namespace flex
} // namespace zerialize