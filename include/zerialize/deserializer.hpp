#pragma once

#include <vector>
#include <span>
#include <string>
#include "concepts.hpp"
#include "errors.hpp"

namespace zerialize {

using std::vector, std::span, std::string;

// --------------
// The Deserializer class exists to:
// 1. enforce a common compile-time concept interface for derived types: they
//    must conform to Deserializable.
// 2. own a source of data for the concrete child classes: either a vector or a span.

template <typename Derived>
class Deserializer {
protected:
    vector<uint8_t> buf_;
    span<const uint8_t> view_;

public:

    // --- Constructors ---

    Deserializer() noexcept {}

    // Constructor 1: Takes ownership by moving a vector. Safe lifetime.
    explicit Deserializer(vector<uint8_t>&& vec) noexcept
        : buf_(std::move(vec)),
          view_(buf_.begin(), buf_.end())
    {
        static_assert(Deserializable<Derived>, "Derived must satisfy Deserializable concept");
    }

    // Constructor 2: Copies from an existing vector via a const reference.
    explicit Deserializer(const vector<uint8_t>& vec) noexcept
        : buf_(vec),
          view_(buf_.begin(), buf_.end())
    {
        static_assert(Deserializable<Derived>, "Derived must satisfy Deserializable concept");
    }

    // Constructor 3: Borrows via an existing span.
    // WARNING: Caller MUST ensure the lifetime of the data viewed by 'view'
    // exceeds this Deserializer.
    explicit Deserializer(std::span<const uint8_t> view) noexcept
        : view_(view)
    {
        static_assert(Deserializable<Derived>, "Derived must satisfy Deserializable concept");
    }

    span<const uint8_t> buf() const { 
        return view_; 
    }
    
    size_t size() const { 
        return view_.size(); 
    }

    virtual string to_string() const { 
        return "<Deserializer size: " + std::to_string(size()) + ">"; 
    }

    template<typename T>
    T as() const {
        const Derived* d = static_cast<const Derived*>(this);
        if constexpr (std::is_same_v<T, int8_t>) {
            return d->asInt8();
        } else if constexpr (std::is_same_v<T, int16_t>) {
            return d->asInt16();
        } else if constexpr (std::is_same_v<T, int32_t>) {
            return d->asInt32();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return d->asInt64();
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            return d->asUInt8();
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            return d->asUInt16();
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            return d->asUInt32();
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            return d->asUInt64();
        } else if constexpr (std::is_same_v<T, float>) {
            return d->asFloat();
        } else if constexpr (std::is_same_v<T, double>) {
            return d->asDouble();
        } else if constexpr (std::is_same_v<T, bool>) {
            return d->asBool();
        } else if constexpr (std::is_same_v<T, string>) {
            return d->asString();
        } else if constexpr (std::is_same_v<T, string_view>) {
            return d->asStringView();
        } else {
            static_assert(false, "Unsupported type in as<T>()");
        }
    }
};

} // namespace zerialize
