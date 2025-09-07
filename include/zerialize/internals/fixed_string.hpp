#pragma once

#include <iostream>
#include <string_view>
#include <utility>

namespace zerialize {

/**
 * @brief Compile-time string wrapper suitable for NTTP (non-type template parameters).
 *
 * This type is *structural* and trivially copyable, so it can be used like:
 *
 *   template<fixed_string Key>
 *   struct field {};
 *
 *   using F = field<"key">;   // OK
 *
 * It copies the source literal including the trailing '\0'. `view()` drops
 * the final '\0' and returns a `std::string_view` of length `N-1`.
 *
 * @tparam N Size of the char buffer, including the trailing '\0'.
 */
template <std::size_t N>
struct fixed_string {

    /// Raw storage including the trailing '\0'.
    char data[N];

    /// @brief Build from a string literal of exactly N chars (including '\0').
    /// @note `consteval` ensures this only accepts compile-time literals.
    consteval fixed_string(const char (&s)[N]) : data{} {
        for (std::size_t i = 0; i < N; ++i) data[i] = s[i];
    }

    /// @brief View without the trailing '\0'.
    constexpr std::string_view view() const { return {data, N - 1}; }

    /// @brief C-string pointer (includes the trailing '\0').
    constexpr const char* c_str() const noexcept { return data.data(); }

    /// @brief Length excluding the trailing '\0'.
    static constexpr std::size_t size() noexcept { return N - 1; }

    /// @brief Three-way comparison by contents (enables ==, <, etc. in C++20).
    friend constexpr auto operator<=>(const fixed_string&, const fixed_string&) = default;
};

// CTAD: allow `fixed_string fs{"key"}` without specifying <N>.
template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

}