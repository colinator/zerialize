#pragma once

#include <tuple>
#include <utility>
#include <type_traits>
#include <string_view>
#include <array>

#include <zerialize/concepts.hpp>
#include <zerialize/internals/fixed_string.hpp>
#include <zerialize/internals/tuple_for_each.hpp>
#include <zerialize/internals/serializers.hpp>

namespace zerialize {

/*
 * zbuilders.hpp
 * -------------
 * Small builder DSL for constructing values into any protocol Writer.
 *
 * What is a “builder”?
 *   A callable object that, when invoked with a Writer, emits a value (primitive,
 *   array, or map). Builders compose (e.g., a zvec can contain a zmap).
 *
 * Top-level usage (with your Protocol P):
 *
 *   // Arrays:
 *   auto buf = serialize<P>( zvec(1, 2.5, "x") );
 *
 *   // Maps with compile-time keys (zero runtime key strings):
 *   auto buf2 = serialize<P>( zmap<"id","name">(42, "Ada") );
 *
 *   // Nesting:
 *   auto buf3 = serialize<P>( zmap<"nums","meta">(
 *       zvec(1,2,3),
 *       zmap<"ok">(true)
 *   ));
 *
 * How are non-builder values serialized?
 *   Via ADL: inside zvec/zmap we do `using zerialize::serialize; serialize(v, w);`
 *   so third-party overloads `serialize(T&, W&)` in T’s namespace are considered.
 */

// A tiny wrapper that marks a callable as a “builder”.
template<class F>
struct BuilderWrapper {
    static constexpr bool is_builder = true;
    F fn;
    template<Writer W>
    void operator()(W& w) noexcept(noexcept(fn(w))) { fn(w); }
};

//────────────────────────  zvec: array builder  ───────────────────
// Usage: zvec(1, 2.5, "x", zmap<"k">(42))
template<typename... Ts>
constexpr auto zvec(Ts&&... xs)
{
    // Capture by value with perfect forwarding into a tuple.
    auto l = [data = std::forward_as_tuple(std::forward<Ts>(xs)...)]
             <Writer W>(W& w) mutable
    {
        w.begin_array(sizeof...(Ts));

        // Visit each captured element in order.
        tuple_for_each(data, [&w](auto&& v) {
            using V = std::remove_reference_t<decltype(v)>;

            if constexpr (Builder<V>) {
                // Nested builder (already a callable that takes a Writer).
                v(w);
            } else {
                // Regular value: dispatch to free `serialize(value, writer)` via ADL.
                using zerialize::serialize;
                serialize(v, w);
            }
        });

        w.end_array();
    };

    return BuilderWrapper{std::move(l)};
}


//────────────────────────  zmap: map builder  ─────────────────────
// Compile-time keys (fast path, zero runtime key storage).
// Usage: zmap<"a","b">(3, 5.2)  →  {"a":3, "b":5.2}
template<fixed_string... Keys, typename... Ts>
constexpr auto zmap(Ts&&... xs)
{
    static_assert(sizeof...(Keys) == sizeof...(Ts),
                  "zmap: number of keys must match number of values");

    auto l = [data = std::forward_as_tuple(std::forward<Ts>(xs)...)]
             <Writer W>(W& w) mutable
    {
        constexpr std::size_t N = sizeof...(Keys);
        // Materialize compile-time keys as string_view array once.
        constexpr auto key_views = std::array<std::string_view, N>{ Keys.view()... };

        w.begin_map(N);

        // Unroll keys/values in lockstep.
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (( // key I
               w.key(key_views[I]),
               // value I
               [&]<typename V>(V&& val) {
                   using TVal = std::remove_reference_t<V>;
                   if constexpr (Builder<TVal>) {
                       val(w);     // nested builder
                   } else {
                       using zerialize::serialize;
                       serialize(val, w); // ADL for user types
                   }
               }(std::get<I>(data))
             ), ...);
        }(std::make_index_sequence<N>{});

        w.end_map();
    };

    return BuilderWrapper{std::move(l)};
}

} // namespace zerialize