#pragma once
//
// concepts.hpp — Core zerialize concepts for (de)serialization
//
// This header defines the fundamental C++20 concepts and types used across the
// library:
//
//   • BlobView        — “blob-like” types that can be viewed as a
//                       std::span<const std::byte>.
//   • StringViewRange — forward range whose value_type is std::string_view,
//                       used to constrain the return type of 'keys' on map-like
//                       elements.
//   • Reader          — the reader/“value view” concept: type checks,
//                       scalar accessors, map/array/blobs.
//   • Writer          — the minimal serializer surface (primitives,
//                       begin/end array/map, keys, etc.).
//   • Builder         — a small tag-based concept for DSL builders
//                       (zmap/zvec/etc.) that emit into a Writer.
//   • RootSerializer  — default-constructible, finish() → ZBuffer.
//   • SerializerFor   — Writer constructible from RootSerializer&.
//   • Protocol        — ties the above together and requires a Name.
//
// Notes:
//
// – BlobView is flexible: it accepts either
//   (1) exactly std::span<const std::byte> for zero-copy views, or
//   (2) any type exposing data() and size() compatible with
//       (const std::byte*, std::size_t), such as std::vector<std::byte>
//       or std::array<std::byte, N>.
//   This allows deserializers to return either a non-owning span or an
//   owning container without breaking the concept.
//
// – StringViewRange requires value_type == std::string_view. This allows
//   zero-alloc key iteration (e.g., JSON/Flex key views). If you want to
//   relax it later, use convertible_to<std::string_view> instead.
//   StringViewRange is what 'keys' of Reader objets should return,
//   instead of vector, list, set, etc.
//
// – Reader demands exact return types (same_as<…>) to make
//   overload resolution predictable and avoid surprising implicit
//   conversions in user code.
//

#include <concepts>      // std::same_as, std::convertible_to, std::constructible_from
#include <ranges>        // std::ranges::forward_range, size, etc.
#include <string_view>
#include <span>
#include <cstddef>
#include <cstdint>

#include <zerialize/zbuffer.hpp>

namespace zerialize {

//─────────────────────────────  Blob-like  ─────────────────────────────

template<class B>
concept BlobView =
    std::same_as<std::remove_cvref_t<B>, std::span<const std::byte>> ||
    requires (const B& b) {
        { std::data(b) } -> std::convertible_to<const std::byte*>;
        { std::size(b) } -> std::convertible_to<std::size_t>;
    };

//────────────────────────────  Key ranges  ─────────────────────────────

template<class R>
concept StringViewRange =
    std::ranges::forward_range<R> &&
    std::same_as<
        std::remove_cvref_t<std::ranges::range_value_t<R>>,
        std::string_view
    >;

//──────────────────────────  Reader  ───────────────────────────
//
// A read-only “value view” surface. Conforming types expose exact
// predicates, exact-typed scalar accessors, random access for arrays,
// key access for maps (with a zero-alloc key range), and a blob view.
//
template<class V>
concept Reader =
    requires (const V& v, std::string_view key, std::size_t index) {

      // --- Type predicates (must be exact bool; no proxies) ---
      { v.isNull()   } -> std::same_as<bool>;
      { v.isInt()    } -> std::same_as<bool>;
      { v.isUInt()   } -> std::same_as<bool>;
      { v.isFloat()  } -> std::same_as<bool>;
      { v.isString() } -> std::same_as<bool>;
      { v.isBlob()   } -> std::same_as<bool>;
      { v.isBool()   } -> std::same_as<bool>;
      { v.isMap()    } -> std::same_as<bool>;
      { v.isArray()  } -> std::same_as<bool>;

      // --- Scalar accessors (exact return types) ---
      { v.asInt8()      } -> std::same_as<std::int8_t>;
      { v.asInt16()     } -> std::same_as<std::int16_t>;
      { v.asInt32()     } -> std::same_as<std::int32_t>;
      { v.asInt64()     } -> std::same_as<std::int64_t>;
      { v.asUInt8()     } -> std::same_as<std::uint8_t>;
      { v.asUInt16()    } -> std::same_as<std::uint16_t>;
      { v.asUInt32()    } -> std::same_as<std::uint32_t>;
      { v.asUInt64()    } -> std::same_as<std::uint64_t>;
      { v.asFloat()     } -> std::same_as<float>;
      { v.asDouble()    } -> std::same_as<double>;
      { v.asString()    } -> std::same_as<std::string>;
      { v.asStringView()} -> std::same_as<std::string_view>;
      { v.asBool()      } -> std::same_as<bool>;

      // --- Map interface (zero-alloc keys) ---
      { v.mapKeys()    } -> StringViewRange;
      { v.contains(key)} -> std::same_as<bool>;
      { v[key]         } -> std::same_as<V>;

      // --- Array interface ---
      { v.arraySize()  } -> std::same_as<std::size_t>;
      { v[index]       } -> std::same_as<V>;

      // --- Blob view ---
      { v.asBlob()     } -> BlobView;
  };

//───────────────────────────────  Writer  ──────────────────────────────
//
// Minimal serializer surface. Implementations should encode the given
// primitives and container boundaries into their target format.
//
template<class W>
concept Writer =
    requires (W& w,
              std::string_view sv,
              std::span<const std::byte> bin,
              std::size_t n,
              std::int64_t i,
              std::uint64_t u,
              double d) {

        // primitives
        { w.null() }                     -> std::same_as<void>;
        { w.boolean(true) }              -> std::same_as<void>;
        { w.int64(i) }                   -> std::same_as<void>;
        { w.uint64(u) }                  -> std::same_as<void>;
        { w.double_(d) }                 -> std::same_as<void>;
        { w.string(sv) }                 -> std::same_as<void>;
        { w.binary(bin) }                -> std::same_as<void>;
        { w.key(sv) }                    -> std::same_as<void>;

        // container boundaries
        { w.begin_array(n) }             -> std::same_as<void>;
        { w.end_array() }                -> std::same_as<void>;
        { w.begin_map(n) }               -> std::same_as<void>;
        { w.end_map() }                  -> std::same_as<void>;
    };

//──────────────────────────────  Builders  ─────────────────────────────
//
// A Builder is a callable that emits exactly one value into a Writer.
// zmap/zvec return BuilderWrapper instances that conform to Builder; 
// the Writer calls them to generate structured content. This tag-based 
// approach avoids false positives we can get with unconstrained operator() 
// templates.
//

template<class T>
concept Builder =
    requires { std::bool_constant<std::remove_cvref_t<T>::is_builder>{}; } &&
    ( std::remove_cvref_t<T>::is_builder == true );

//────────────────────────────  Protocols  ─────────────────────────────
//
// A Protocol bundles the concrete RootSerializer and Serializer types,
// plus a human-readable Name.
//
template<class RS>
concept RootSerializer =
    std::default_initializable<RS> &&
    requires (RS& rs) {
        { rs.finish() } -> std::same_as<ZBuffer>;
    };

// Writer constructible from RootSerializer&.
template<class S, class RS>
concept SerializerFor =
    Writer<S> && std::constructible_from<S, RS&>;

// Protocol ties it all together. Example:
// struct JSON {
//   static inline constexpr const char* Name = "Json";
//   using Deserializer   = json::JsonDeserializer;
//   using RootSerializer = json::RootSerializer;
//   using Serializer     = json::Serializer;
// };
template<class P>
concept Protocol =
    // Associated types must exist
    requires { typename P::RootSerializer; typename P::Serializer; typename P::Deserializer; } &&

    // Root + Writer
    RootSerializer<typename P::RootSerializer> &&
    SerializerFor<typename P::Serializer, typename P::RootSerializer> &&

    // Reader/Deserializer side
    Reader<typename P::Deserializer> &&

    // A human-readable name
    requires { { P::Name } -> std::convertible_to<const char*>; };

} // namespace zerialize