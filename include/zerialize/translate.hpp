#pragma once

#include <zerialize/concepts.hpp>

namespace zerialize {

/*
 * translate.hpp
 * -------------
 * Generic bridging between a Reader (Deserializer) and a Writer (Serializer).
 *
 * Provides:
 *   - `write_value(v, w)`: Recursively walk a Reader value `v` and emit it
 *     into a Writer `w`.
 *   - `translate<DstP>(src)`: Convert any Reader `src` into a destination
 *     Protocol’s Deserializer, going through its RootSerializer/Writer.
 *
 * Example:
 *   // Suppose you have FlexBuffers data in `flex`.
 *   auto json = zerialize::translate<JSON>(flex);
 *   std::cout << json.to_string() << std::endl;
 *
 *   // Round-trip between protocols:
 *   auto flex2 = zerialize::translate<Flex>(json);
 *
 * These helpers are especially useful for testing or migrating between
 * protocols, since they don’t need to know about the schema — they
 * operate on the Reader/Writer concepts directly.
 */

// ==== Generic bridge: Reader -> Writer ===============================
template<class V, class W>
inline void write_value(const V& v, W& w) {
    if (v.isNull())      { w.null(); return; }
    if (v.isBool())      { w.boolean(v.asBool()); return; }
    if (v.isInt())       { w.int64(v.asInt64()); return; }
    if (v.isUInt())      { w.uint64(v.asUInt64()); return; }
    if (v.isFloat())     { w.double_(v.asDouble()); return; }
    if (v.isString())    { w.string(v.asStringView()); return; }
 
    if (v.isBlob()) {
        auto b = v.asBlob();                    // span<const std::byte>
        w.binary(b);
        return;
    }

    if (v.isMap()) {
        // We want stable iteration; use the order the reader exposes.
        // mapKeys() returns a forward range of string_view (your StringViewRange).
        std::size_t count = 0;
        for (auto /*unused*/ _ : v.mapKeys()) (void)_, ++count;

        w.begin_map(count);
        for (std::string_view k : v.mapKeys()) {
            w.key(std::string(k));
            write_value(v[k], w);
        }
        w.end_map();
        return;
    }

    if (v.isArray()) {
        const std::size_t n = v.arraySize();
        w.begin_array(n);
        for (std::size_t i = 0; i < n; ++i) write_value(v[i], w);
        w.end_array();
        return;
    }

    // If a backend adds more types later:
    throw std::runtime_error("write_value: unsupported source type");
}

// ==== Convenience: convert from a source Reader to a destination Protocol
// Returns DstP::BufferType (e.g., JsonDeserializer, FlexDeserializer, ...)

template<class DstP, class SrcV>
requires Protocol<DstP> && Reader<SrcV>
inline typename DstP::Deserializer translate(const SrcV& src) {
    using Root   = typename DstP::RootSerializer;
    using Writer = typename DstP::Serializer;

    Root   rs{};
    Writer w{rs};
    write_value(src, w);

    ZBuffer out = rs.finish();

    // Construct the destination BufferType from the finished bytes.
    // Match your earlier pattern that used to_vector_copy().
    if constexpr (requires { out.to_vector_copy(); }) {
        return typename DstP::Deserializer(out.to_vector_copy());
    } else if constexpr (requires { out.data(); out.size(); }) {
        return typename DstP::Deserializer(
            reinterpret_cast<const std::uint8_t*>(out.data()), out.size());
    } else {
        static_assert(sizeof(DstP) == 0,
            "DstP::BufferType needs a ctor from vector<uint8_t> or (ptr,size)");
    }
}

// Optional: if you sometimes have raw bytes instead of a Reader instance
// you can provide thin wrappers that first construct the reader and then call convert:

template<class DstP, class SrcBufferType>
requires Protocol<DstP>
inline typename DstP::Deserializer translate_bytes(const SrcBufferType& src_bytes) {
    return translate<DstP>(SrcBufferType(src_bytes));
}

} // namespace zerialize