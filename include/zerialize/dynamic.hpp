#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <memory>
#include <functional>

#include <zerialize/concepts.hpp>

namespace zerialize::dyn {

// Type-erased Writer proxy so we can call `serialize(value, proxy)` for any
// underlying Writer without knowing its concrete type.
struct WriterView {
    void* ctx{};
    void (*null_fn)(void*){};
    void (*bool_fn)(void*, bool){};
    void (*i64_fn)(void*, std::int64_t){};
    void (*u64_fn)(void*, std::uint64_t){};
    void (*dbl_fn)(void*, double){};
    void (*str_fn)(void*, std::string_view){};
    void (*bin_fn)(void*, std::span<const std::byte>){};
    void (*key_fn)(void*, std::string_view){};
    void (*begin_arr_fn)(void*, std::size_t){};
    void (*end_arr_fn)(void*){};
    void (*begin_map_fn)(void*, std::size_t){};
    void (*end_map_fn)(void*){};

    template<zerialize::Writer W>
    static WriterView make(W& w) {
        return WriterView{
            &w,
            [](void* c){ static_cast<W*>(c)->null(); },
            [](void* c, bool b){ static_cast<W*>(c)->boolean(b); },
            [](void* c, std::int64_t v){ static_cast<W*>(c)->int64(v); },
            [](void* c, std::uint64_t v){ static_cast<W*>(c)->uint64(v); },
            [](void* c, double v){ static_cast<W*>(c)->double_(v); },
            [](void* c, std::string_view sv){ static_cast<W*>(c)->string(sv); },
            [](void* c, std::span<const std::byte> bin){ static_cast<W*>(c)->binary(bin); },
            [](void* c, std::string_view sv){ static_cast<W*>(c)->key(sv); },
            [](void* c, std::size_t n){ static_cast<W*>(c)->begin_array(n); },
            [](void* c){ static_cast<W*>(c)->end_array(); },
            [](void* c, std::size_t n){ static_cast<W*>(c)->begin_map(n); },
            [](void* c){ static_cast<W*>(c)->end_map(); }
        };
    }

    void null()               { null_fn(ctx); }
    void boolean(bool b)      { bool_fn(ctx, b); }
    void int64(std::int64_t v){ i64_fn(ctx, v); }
    void uint64(std::uint64_t v){ u64_fn(ctx, v); }
    void double_(double v)    { dbl_fn(ctx, v); }
    void string(std::string_view sv){ str_fn(ctx, sv); }
    void binary(std::span<const std::byte> bin){ bin_fn(ctx, bin); }
    void key(std::string_view sv){ key_fn(ctx, sv); }
    void begin_array(std::size_t n){ begin_arr_fn(ctx, n); }
    void end_array() { end_arr_fn(ctx); }
    void begin_map(std::size_t n){ begin_map_fn(ctx, n); }
    void end_map(){ end_map_fn(ctx); }
};

// Erased serializable payload that will call serialize(v, WriterView).
struct Serializable {
    std::shared_ptr<void> storage;
    std::function<void(WriterView&)> emit;

    template<class T>
    static Serializable make(T&& v) {
        using V = std::decay_t<T>;
        auto ptr = std::make_shared<V>(std::forward<T>(v));
        auto fn = [ptr](WriterView& wv) {
            using zerialize::serialize;
            serialize(*ptr, wv);
        };
        return Serializable{ptr, std::move(fn)};
    }

    template<class W>
    void operator()(W& w) const {
        auto view = WriterView::make(w);
        emit(view);
    }
};

// Marker for null when you want an explicit type.
struct Null {};

// Owning binary payload (used when a protocol supports blobs).
struct Binary {
    std::vector<std::byte> data;
};

// A small owning dynamic value for serialization-only use.
// Supports primitives, strings, binary blobs, arrays, maps, and an
// explicit "serializable" slot that can emit any type with an ADL-visible
// serialize(T, Writer) overload (e.g., xtensor/eigen).
class Value {
public:
    using Array = std::vector<Value>;
    using Map   = std::vector<std::pair<std::string, Value>>;
    using Serializable = dyn::Serializable;

    Value() : data_(std::monostate{}) {}
    Value(Null) : data_(std::monostate{}) {}
    Value(std::nullptr_t) : data_(std::monostate{}) {}
    Value(bool b) : data_(b) {}

    template<std::integral T>
    requires (!std::same_as<std::remove_cvref_t<T>, bool>)
    Value(T v) {
        if constexpr (std::is_signed_v<T>) {
            data_ = static_cast<std::int64_t>(v);
        } else {
            data_ = static_cast<std::uint64_t>(v);
        }
    }

    Value(double d) : data_(d) {}
    Value(float d) : data_(static_cast<double>(d)) {}

    Value(std::string s) : data_(std::move(s)) {}
    Value(std::string_view sv) : data_(std::string(sv)) {}
    Value(const char* s) : data_(std::string(s)) {}

    Value(const std::vector<std::byte>& blob) : data_(blob) {}
    Value(std::vector<std::byte>&& blob) : data_(std::move(blob)) {}
    Value(std::span<const std::byte> blob)
        : data_(std::vector<std::byte>(blob.begin(), blob.end())) {}
    Value(Binary blob) : data_(std::move(blob.data)) {}

    Value(Array arr) : data_(std::move(arr)) {}
    Value(Map map) : data_(std::move(map)) {}
    Value(Serializable s) : data_(std::move(s)) {}

    static Value array(Array arr) { return Value(std::move(arr)); }
    static Value map(Map map)     { return Value(std::move(map)); }

    static Value blob(std::span<const std::byte> blob) {
        return Value(blob);
    }

    template<class T>
    static Value serializable(T&& v) {
        return Value(Serializable::make(std::forward<T>(v)));
    }

    const auto& storage() const { return data_; }

private:
    using Storage = std::variant<
        std::monostate,
        bool,
        std::int64_t,
        std::uint64_t,
        double,
        std::string,
        std::vector<std::byte>,
        Array,
        Map,
        Serializable
    >;

    Storage data_;
};

inline Value array(std::initializer_list<Value> values) {
    return Value::array(Value::Array(values));
}

inline Value map(std::initializer_list<std::pair<std::string, Value>> entries) {
    return Value::map(Value::Map(entries));
}

template<class T>
Value serializable(T&& v) {
    return Value::serializable(std::forward<T>(v));
}

} // namespace zerialize::dyn

namespace zerialize::dyn {

// Serialize dyn::Value into any Writer (kept in dyn for ADL friendliness).
template<zerialize::Writer W>
void serialize(const Value& v, W& w) {
    using zerialize::serialize; // enable ADL for nested custom types if needed

    std::visit([&](auto&& arg) {
        using T = std::remove_cvref_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
            w.null();
        } else if constexpr (std::is_same_v<T, bool>) {
            w.boolean(arg);
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            w.int64(arg);
        } else if constexpr (std::is_same_v<T, std::uint64_t>) {
            w.uint64(arg);
        } else if constexpr (std::is_same_v<T, double>) {
            w.double_(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
            w.string(arg);
        } else if constexpr (std::is_same_v<T, std::vector<std::byte>>) {
            w.binary(std::span<const std::byte>(arg.data(), arg.size()));
        } else if constexpr (std::is_same_v<T, Value::Array>) {
            w.begin_array(arg.size());
            for (const auto& child : arg) {
                serialize(child, w);
            }
            w.end_array();
        } else if constexpr (std::is_same_v<T, Value::Map>) {
            w.begin_map(arg.size());
            for (const auto& [k, child] : arg) {
                w.key(k);
                serialize(child, w);
            }
            w.end_map();
        } else if constexpr (std::is_same_v<T, Value::Serializable>) {
            arg(w);
        }
    }, v.storage());
}

} // namespace zerialize::dyn

namespace zerialize {

using DynamicValue = dyn::Value;

} // namespace zerialize
