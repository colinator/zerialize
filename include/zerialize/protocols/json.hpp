#pragma once

#include <string>
#include <string_view>
#include <span>
#include <set>
#include <iterator>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <yyjson.h>
#include <zerialize/zbuffer.hpp>
#include <zerialize/errors.hpp>
#include <zerialize/internals/base64.hpp>

namespace zerialize {
namespace json {

const std::string blobTag("~b");
const std::string blobEncoding("base64");

// A forward range of string_view over an object's keys.
struct KeysView {
    yyjson_val* obj; // must be a yyjson object value

    struct iterator {
        yyjson_val*     owner = nullptr; // which object we're iterating
        yyjson_obj_iter it{};            // yyjson iterator state
        yyjson_val*     key = nullptr;   // current key node (nullptr == end)

        // C++20 iterator bits
        using iterator_concept  = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type        = std::string_view;
        using difference_type   = std::ptrdiff_t;
        using reference         = std::string_view;

        iterator() = default;

        explicit iterator(yyjson_val* o, bool to_end) : owner(o) {
            if (o) yyjson_obj_iter_init(o, &it);
            key = (to_end || !o) ? nullptr : yyjson_obj_iter_next(&it);
        }

        reference operator*() const {
            // JSON keys are strings
            return std::string_view(yyjson_get_str(key), yyjson_get_len(key));
        }

        iterator& operator++() {
            key = yyjson_obj_iter_next(&it);
            return *this;
        }

        // postfix ++ is required for input/forward iterators
        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const iterator& a, const iterator& b) {
            // tie equality to the same owner and same current key
            return a.owner == b.owner && a.key == b.key;
        }
        friend bool operator!=(const iterator& a, const iterator& b) {
            return !(a == b);
        }
    };

    iterator begin() const { return iterator{obj, /*to_end*/false}; }
    iterator end()   const { return iterator{obj, /*to_end*/true};  }
};

// ADL helpers make std::ranges::begin/end unambiguous on all lib implementations.
inline KeysView::iterator begin(const KeysView& kv) noexcept { return kv.begin(); }
inline KeysView::iterator end  (const KeysView& kv) noexcept { return kv.end();   }

class JsonDeserializer {
    yyjson_doc* doc_ = nullptr;         // owned when owns_doc_==true
    yyjson_val* cur_ = nullptr;          // view into doc_
    bool owns_doc_ = false;

    static void ensure(bool cond, const char* msg) {
        if (!cond) throw DeserializationError(msg);
    }

    // type-check helper (accepts function pointers to yyjson_is_* or lambdas)
    template<class F>
    void check(F&& pred, const char* what) const {
        if (!cur_ || !std::forward<F>(pred)(cur_))
            throw DeserializationError(std::string("Value is not a ") + what);
    }

public:
    // --- ctors: root (owning) ---
    explicit JsonDeserializer(std::string_view json) {
        static_assert(Reader<JsonDeserializer>, "Derived must satisfy Reader concept");

        doc_ = yyjson_read(json.data(), json.size(), 0);
        ensure(doc_, "Failed to parse JSON");
        cur_ = yyjson_doc_get_root(doc_);
        ensure(cur_, "Failed to get JSON root");
        owns_doc_ = true;
    }
    explicit JsonDeserializer(const uint8_t* data, std::size_t n)
        : JsonDeserializer(std::string_view(reinterpret_cast<const char*>(data), n)) {}

    explicit JsonDeserializer(std::span<const uint8_t> bytes)
        : JsonDeserializer(std::string_view(reinterpret_cast<const char*>(bytes.data()),
                                        bytes.size())) {}

    // default: empty object {}
    JsonDeserializer() : JsonDeserializer(std::string_view("{}", 2)) {}

    // --- view ctor (non-owning) ---
    JsonDeserializer(yyjson_val* v, yyjson_doc* d)
        : doc_(d), cur_(v), owns_doc_(false) { ensure(doc_ && cur_, "Null view"); }

    // --- dtor / move only ---
    ~JsonDeserializer() {
        if (owns_doc_ && doc_) yyjson_doc_free(doc_);
    }
    JsonDeserializer(const JsonDeserializer&) = delete;
    JsonDeserializer& operator=(const JsonDeserializer&) = delete;

    JsonDeserializer(JsonDeserializer&& o) noexcept
        : doc_(o.doc_), cur_(o.cur_), owns_doc_(o.owns_doc_) {
        o.doc_ = nullptr; o.cur_ = nullptr; o.owns_doc_ = false;
    }
    JsonDeserializer& operator=(JsonDeserializer&& o) noexcept {
        if (this != &o) {
            if (owns_doc_ && doc_) yyjson_doc_free(doc_);
            doc_ = o.doc_; cur_ = o.cur_; owns_doc_ = o.owns_doc_;
            o.doc_ = nullptr; o.cur_ = nullptr; o.owns_doc_ = false;
        }
        return *this;
    }

    // --- predicates (strict bool) ---
    bool isNull()   const { return cur_ && yyjson_is_null(cur_); }
    bool isBool()   const { return cur_ && yyjson_is_bool(cur_); }
    bool isInt()    const { return cur_ && yyjson_is_int(cur_); }
    bool isUInt()   const { return cur_ && yyjson_is_uint(cur_); }
    bool isFloat()  const { return cur_ && yyjson_is_real(cur_); }
    bool isString() const { return cur_ && yyjson_is_str(cur_); }
    bool isMap()    const { return cur_ && yyjson_is_obj(cur_); }
    bool isArray()  const { return cur_ && yyjson_is_arr(cur_); }
    // Blob policy: base64 string
    bool isBlob()   const { 
        return isArray() && arraySize() == 3 && 
            (*this)[0].isString() && (*this)[0].asString() == blobTag &&
            (*this)[1].isString() && (*this)[1].asString() == blobEncoding && 
            (*this)[1].isString();
    }

    // --- scalars ---
    int8_t   asInt8()   const { check(yyjson_is_int,  "signed integer"); return static_cast<int8_t>(yyjson_get_sint(cur_)); }
    int16_t  asInt16()  const { check(yyjson_is_int,  "signed integer"); return static_cast<int16_t>(yyjson_get_sint(cur_)); }
    int32_t  asInt32()  const { check(yyjson_is_int,  "signed integer"); return static_cast<int32_t>(yyjson_get_sint(cur_)); }
    int64_t  asInt64()  const { check(yyjson_is_int,  "signed integer"); return yyjson_get_sint(cur_); }

    uint8_t  asUInt8()  const { check(yyjson_is_uint, "unsigned integer"); return static_cast<uint8_t>(yyjson_get_uint(cur_)); }
    uint16_t asUInt16() const { check(yyjson_is_uint, "unsigned integer"); return static_cast<uint16_t>(yyjson_get_uint(cur_)); }
    uint32_t asUInt32() const { check(yyjson_is_uint, "unsigned integer"); return static_cast<uint32_t>(yyjson_get_uint(cur_)); }
    uint64_t asUInt64() const { check(yyjson_is_uint, "unsigned integer"); return yyjson_get_uint(cur_); }

    float    asFloat()  const { check(yyjson_is_real, "float");  return static_cast<float>(yyjson_get_real(cur_)); }
    double   asDouble() const { check(yyjson_is_real, "float");  return yyjson_get_real(cur_); }

    bool     asBool()   const { check(yyjson_is_bool, "boolean"); return yyjson_get_bool(cur_); }

    std::string asString() const {
        check(yyjson_is_str, "string");
        return std::string(yyjson_get_str(cur_), yyjson_get_len(cur_));
    }
    std::string_view asStringView() const {
        check(yyjson_is_str, "string");
        return std::string_view(yyjson_get_str(cur_), yyjson_get_len(cur_));
    }

    std::vector<std::byte> asBlob() const {
        ensure(isBlob(), "not a blob");
        return base64Decode((*this)[2].asStringView());
    }

    // --- map interface ---
    bool contains(std::string_view key) const {
        if (!isMap()) return false;
        return yyjson_obj_getn(cur_, key.data(), key.size()) != nullptr;
    }

    KeysView mapKeys() const {
        check(yyjson_is_obj, "map/object");
        return KeysView{cur_};
    }

    JsonDeserializer operator[](std::string_view key) const {
        check(yyjson_is_obj, "map/object");
        yyjson_val* v = yyjson_obj_getn(cur_, key.data(), key.size());
        if (!v) throw DeserializationError("Key not found: " + std::string(key));
        return JsonDeserializer(v, doc_); // view
    }

    // --- array interface ---
    std::size_t arraySize() const {
        check(yyjson_is_arr, "array");
        return yyjson_arr_size(cur_);
    }
    JsonDeserializer operator[](std::size_t idx) const {
        check(yyjson_is_arr, "array");
        yyjson_val* v = yyjson_arr_get(cur_, idx);
        if (!v) throw DeserializationError("Array index out of range");
        return JsonDeserializer(v, doc_); // view
    }

    // --- debug helper ---
    std::string to_string(bool pretty = true) const {
        if (!cur_) return "null";
        size_t len = 0;
        yyjson_write_flag flg = pretty ? YYJSON_WRITE_PRETTY : 0;
        char* s = yyjson_val_write(cur_, flg, &len);
        std::string out = s ? std::string(s, len) : "(yyjson_val_write failed)";
        if (s) free(s);
        return out;
    }

    // expose raw (if needed for advanced ops)
    yyjson_doc* raw_doc() const { return doc_; }
    yyjson_val* raw_val() const { return cur_; }
};



struct RootSerializer {
    yyjson_mut_doc* doc = nullptr;
    yyjson_mut_val* root = nullptr;
    bool wrote_root = false;

    struct Ctx {
        enum K { Arr, Obj } k;
        yyjson_mut_val* node;        // array or object node
        std::string pending_key;     // for objects: set by key(), consumed by next value
    };
    std::vector<Ctx> st;

    RootSerializer() : doc(yyjson_mut_doc_new(nullptr)) {
        if (!doc) throw std::bad_alloc{};
    }

    // Consume and produce the final buffer.
    ZBuffer finish() {
        if (!root) {
            root = yyjson_mut_null(doc);
            yyjson_mut_doc_set_root(doc, root);
        }

        size_t len = 0;
        char* s = yyjson_mut_write(doc, 0, &len);
        if (!s) {
            yyjson_mut_doc_free(doc);
            doc = nullptr;
            throw std::runtime_error("yyjson_mut_write failed");
        }

        // The returned buffer 's' is allocated via malloc and independent of the document.
        // Hand ownership directly to ZBuffer to avoid a copy.
        yyjson_mut_doc_free(doc);
        doc = nullptr;
        return ZBuffer(static_cast<void*>(s), len, ZBuffer::Deleters::Free);
    }
};

struct Serializer {
    RootSerializer* r;

    explicit Serializer(RootSerializer& rs) : r(&rs) {}

    // ── primitives ──────────────────────────────────────────────
    void null()                 { push_value(yyjson_mut_null(doc())); }
    void boolean(bool v)        { push_value(yyjson_mut_bool(doc(), v)); }
    void int64(std::int64_t v)  { push_value(yyjson_mut_sint(doc(), v)); }
    void uint64(std::uint64_t v){ push_value(yyjson_mut_uint(doc(), v)); }
    void double_(double v)      { push_value(yyjson_mut_real(doc(), v)); }
    void string(std::string_view sv) {
        push_value(yyjson_mut_strn(doc(), sv.data(), sv.size()));
    }
    // Encode binary as array of 0..255
    void binary(std::span<const std::byte> b) {
        std::string s = base64Encode(b);

        // A 'binary' will actually be an array, the first element 
        // will be a string with the magic string "~b"
        begin_array(3);
        this->string(blobTag);
        this->string(blobEncoding);
        push_value(yyjson_mut_strncpy(doc(), s.data(), s.size()));
        end_array();
    }

    // ── structures ──────────────────────────────────────────────
    void begin_array(std::size_t /*n*/) {
        yyjson_mut_val* arr = yyjson_mut_arr(doc());
        if (!arr) throw std::bad_alloc{};
        push_value(arr);                       // attach to parent/root first
        r->st.push_back({RootSerializer::Ctx::Arr, arr, {}}); // then track context
    }
    void end_array() {
        ensure_in(RootSerializer::Ctx::Arr, "end_array");
        r->st.pop_back();
    }

    void begin_map(std::size_t /*n*/) {
        yyjson_mut_val* obj = yyjson_mut_obj(doc());
        if (!obj) throw std::bad_alloc{};
        push_value(obj);
        r->st.push_back({RootSerializer::Ctx::Obj, obj, {}});
    }
    void end_map() {
        ensure_in(RootSerializer::Ctx::Obj, "end_map");
        if (!r->st.back().pending_key.empty())
            throw std::logic_error("end_map() while awaiting value for key()");
        r->st.pop_back();
    }

    void key(std::string_view k) {
        ensure_in(RootSerializer::Ctx::Obj, "key");
        auto& c = r->st.back();
        if (!c.pending_key.empty())
            throw std::logic_error("key() called twice without value");
        c.pending_key.assign(k.data(), k.size());
     }

private:
    yyjson_mut_doc* doc() const { return r->doc; }

    void push_value(yyjson_mut_val* v) {
        if (!v) throw std::bad_alloc{};

        if (r->st.empty()) {
            // set root once
            if (r->wrote_root) throw std::logic_error("multiple root values");
            r->root = v;
            yyjson_mut_doc_set_root(doc(), v);
            r->wrote_root = true;
            return;
        }

        auto& c = r->st.back();
        if (c.k == RootSerializer::Ctx::Arr) {
            if (!yyjson_mut_arr_add_val(c.node, v))
                throw std::bad_alloc{};
        } else {

            // current: c.pending_key holds a std::string we set in key()
            // auto& c = r->st.back();
            if (c.pending_key.empty())
                throw std::logic_error("value added to object without key()");
    
            // optional sanity check during debug
            #ifndef NDEBUG
            if (c.pending_key.find('\0') != std::string::npos) {
                throw std::logic_error("embedded NUL in key() input");
            }
            #endif
    
            // EITHER use the convenience helper if your yyjson has it:
            #if defined(YYJSON_VERSION_HEX) && (YYJSON_VERSION_HEX >= 0x000e00) // 0.14.0+
            if (!yyjson_mut_obj_add_str(doc(), c.node, c.pending_key.c_str(), v))
                throw std::bad_alloc{};
            #else
            // OR the portable two-step using a *cpy* string node (always null-terminated):
            yyjson_mut_val* k = yyjson_mut_strcpy(doc(), c.pending_key.c_str());
            if (!k || !yyjson_mut_obj_add(c.node, k, v))
                throw std::bad_alloc{};
            #endif
    
            c.pending_key.clear();
    }
    }

    void ensure_in(RootSerializer::Ctx::K want, const char* fn) const {
        if (r->st.empty() || r->st.back().k != want)
            throw std::logic_error(std::string(fn) + " called outside correct container");
    }
};

} // namespace json

struct JSON {
    static inline constexpr const char* Name = "Json";
    using Deserializer   = json::JsonDeserializer;
    using RootSerializer = json::RootSerializer;
    using Serializer     = json::Serializer;
};

} // namespace zerialize
