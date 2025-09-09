#pragma once

#include <utility>

#include <zerialize/zbuffer.hpp>
#include <zerialize/concepts.hpp>
#include <zerialize/internals/serializers.hpp>
#include <zerialize/zbuilders.hpp>

namespace zerialize {

/*
 * serialize<P>(rootValue)
 * -----------------------
 * Top-level entry point for producing a serialized buffer using protocol P.
 *
 * Inputs:
 *   - P: a Protocol (see concepts.hpp) that ties together:
 *       * P::RootSerializer  : default-constructible, finish() -> ZBuffer
 *       * P::Serializer      : constructible from RootSerializer&, satisfies Writer
 *   - rootValue: either
 *       * a “builder” (e.g., zmap<...>(...), zvec(...)) — something callable with a Writer
 *       * a regular value (primitive or user type) that has a free `serialize(value, writer)`
 *         overload discoverable via ADL
 *
 * Behavior:
 *   - If rootValue is a Builder, we invoke it directly with the protocol Writer.
 *   - Otherwise, we ADL-dispatch to a free `serialize(value, writer)` function.
 *     The `using zerialize::serialize;` line is deliberate: it keeps our fallback
 *     overloads visible while still allowing ADL to find third-party customizations.
 *
 * Returns:
 *   - A ZBuffer containing the protocol-encoded bytes produced by P::RootSerializer::finish().
 *
 * Notes:
 *   - All exceptions thrown by user/third-party serialize overloads (or underlying protocol
 *     implementations) propagate to the caller. This API intentionally doesn’t swallow errors.
 */

template <Protocol P, class RootType>
inline ZBuffer serialize(RootType&& rootValue) {
    using RootSerializer = typename P::RootSerializer;
    using Serializer     = typename P::Serializer;
    using T              = std::remove_cvref_t<RootType>;

    RootSerializer rs{};
    Serializer w{rs};

    if constexpr (Builder<T>) {
        // Builder path: e.g., zmap<...>(...) / zvec(...)
        std::forward<RootType>(rootValue)(w);
    } else {
        // Value path: call free `serialize(value, writer)` via ADL (+ our fallbacks).
        using zerialize::serialize;      // enable our fallbacks
        serialize(std::forward<RootType>(rootValue), w);
    }
    return rs.finish();
}

// Overload for 0 arguments: creates empty serialization. 
// Not necessarily 0-byte; for example in json is "null".
template <Protocol P>
inline ZBuffer serialize() {
    typename P::RootSerializer rs{};
    return rs.finish();
}
 
} // namespace zerialize
