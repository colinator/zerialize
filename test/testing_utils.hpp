#pragma once
#include <iostream>
#include <utility>
#include <vector>
#include <span>
#include <string_view>
#include <type_traits>

namespace zerialize {

using std::string;
using std::cout;
using std::endl;

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

template<class P, class F>
ZBuffer make_buffer_from_builder(F&& build) {
    using Root   = typename P::RootSerializer;
    using Writer = typename P::Serializer;
    Root rs{};
    Writer w{rs};
    std::forward<F>(build)(w);           // user populates the writer
    return rs.finish();
}

template<class P>
typename P::Deserializer make_reader_from(const ZBuffer& zb) {
    // Prefer vector copy if available
    if constexpr (requires { typename P::Deserializer{ zb.buf() }; }) {
        return typename P::Deserializer(zb.buf());
    } else if constexpr (requires { typename P::Deserializer{ zb.data(), zb.size() }; }) {
        return typename P::Deserializer(
            reinterpret_cast<const std::uint8_t*>(zb.data()), zb.size());
    } else {
        static_assert(sizeof(P) == 0,
            "Deserializer must be constructible from vector<uint8_t> or (ptr,size)");
    }
}

template<class P>
bool run_all_reader_ctors(const ZBuffer& zb,
                          const std::function<bool(const typename P::Deserializer&)>& test)
{
    bool ok = true;

    // 1) Copy-construct from vector
    {
        auto rd = make_reader_from<P>(zb);
        ok = ok && test(rd);
        //std::cout << rd.to_string() << std::endl;
        if (!ok) return false;
    }

    // 2) Construct from span (if supported)
    if constexpr (requires { typename P::Deserializer{ std::span<const std::uint8_t>{} }; }) {
        auto v = zb.buf();
        std::span<const std::uint8_t> sp{ v.data(), v.size() };
        typename P::Deserializer rd(sp);
        ok = ok && test(rd);
        if (!ok) return false;
    }

    // 3) Move vector (if supported)
    if constexpr (requires { typename P::Deserializer{ std::vector<std::uint8_t>{} }; }) {
        auto v = zb.buf();
        std::vector<std::uint8_t> mv(v.begin(), v.end());
        typename P::Deserializer rd(std::move(mv));
        ok = ok && test(rd);
        if (!ok) return false;
    }

    // 4) Pointer/size (if supported)
    if constexpr (requires { typename P::Deserializer{ (const std::uint8_t*)nullptr, std::size_t{} }; }) {
        auto v = zb.buf();
        typename P::Deserializer rd(v.data(), v.size());
        ok = ok && test(rd);
        if (!ok) return false;
    }

    return ok;
}

//------------------------------------------------------------------------------
// Main test entry #1: builder -> P::Deserializer -> test
// - build_fn:  either ZBuffer() OR (Writer&)->void
// - test_fn:   bool(const P::Deserializer&)
//------------------------------------------------------------------------------
template <class P, class BuildFn, class TestFn>
bool test_serialization(string name, BuildFn&& build_fn, TestFn&& test_fn)
{
    const string banner = string("TEST <") + P::Name + "> --- " + name + " ---";
    cout << "START " << banner << endl;

    // 1) Produce a ZBuffer
    ZBuffer zb;
    if constexpr (std::is_invocable_r_v<ZBuffer, BuildFn>) {
        zb = std::forward<BuildFn>(build_fn)();
    } else {
        // assume (Writer&)->void builder
        zb = make_buffer_from_builder<P>(std::forward<BuildFn>(build_fn));
    }

    cout << "serialized buffer size: " << zb.buf().size() << endl;

    // 2) Build a reader and run predicate across all ctor variants
    bool res = run_all_reader_ctors<P>(zb,
        [&](const typename P::Deserializer& rd){ return std::forward<TestFn>(test_fn)(rd); });

    cout << (res ? "   OK " : " FAIL ") << banner << "\n\n";
    if (!res) throw std::runtime_error("test failed!!! " + banner);
    return res;
}

//------------------------------------------------------------------------------
// Optional test entry #2: cross-protocol check via translate<DP>(src)
// - Requires your translate bridge: DstP::Deserializer translate<DstP>(SrcV)
// - xlate_check: bool(const DstP::Deserializer&)
//------------------------------------------------------------------------------
template <class SrcP, class DstP, class BuildFn, class CheckFn>
bool test_translate(string name, BuildFn&& build_src, CheckFn&& xlate_check)
{
    const string banner = string("XLATE <") + SrcP::Name + " -> " + DstP::Name + "> --- " + name + " ---";
    cout << "START " << banner << endl;

    // Build a source buffer and source reader
    ZBuffer src_zb;
    if constexpr (std::is_invocable_r_v<ZBuffer, BuildFn>) {
        src_zb = std::forward<BuildFn>(build_src)();
    } else {
        src_zb = make_buffer_from_builder<SrcP>(std::forward<BuildFn>(build_src));
    }
    auto src_rd = make_reader_from<SrcP>(src_zb);

    // translate
    auto dst_rd = translate<DstP>(src_rd);

    bool ok = xlate_check(dst_rd);
    cout << (ok ? "   OK " : " FAIL ") << banner << "\n\n";
    if (!ok) throw std::runtime_error("xlate test failed!!! " + banner);
    return ok;
}

//------------------------------------------------------------------------------
// Small, reusable matchers for tests (use or ignore as you like)
//------------------------------------------------------------------------------

template<class V>
bool expect_array_size(const V& v, std::size_t n) {
    return v.isArray() && v.arraySize() == n;
}

template<class V>
bool expect_string_eq(const V& v, std::string_view s) {
    return v.isString() && v.asStringView() == s;
}

template<class V>
bool expect_int_eq(const V& v, std::int64_t x) {
    return (v.isInt() || v.isUInt()) && v.asInt64() == x;
}

template<class V>
bool expect_double_eq(const V& v, double d, double eps = 1e-12) {
    if (!v.isFloat()) return false;
    auto got = v.asDouble();
    return (got == d) || (std::abs(got - d) <= eps);
}

template<class V>
bool expect_map_has(const V& v, std::string_view k) {
    if (!v.isMap()) return false;
    for (std::string_view kk : v.mapKeys()) if (kk == k) return true;
    return false;
}

//------------------------------------------------------------------------------
// Example usage (comment out or keep as docs)
//
// test_serialization<JSON>("simple object",
//     [](auto& w) {
//         w.begin_map(3);
//         w.key("a"); w.int64(1);
//         w.key("b"); w.string("hi");
//         w.key("c"); w.begin_array(2); w.boolean(true); w.null(); w.end_array();
//         w.end_map();
//     },
//     [](const JSON::Deserializer& d) {
//         return d.isMap()
//             && expect_int_eq(d["a"], 1)
//             && expect_string_eq(d["b"], "hi")
//             && d["c"].isArray() && d["c"].arraySize()==2 && d["c"][0].asBool()==true;
//     }
// );
//
// test_translate<JSON, Flex>("json->flex roundtrip",
//     [](auto& w) {
//         w.begin_map(1);
//         w.key("x"); w.begin_array(2); w.uint64(7); w.string("s"); w.end_array();
//         w.end_map();
//     },
//     [](const Flex::Deserializer& f) {
//         return f.isMap()
//             && f["x"].isArray()
//             && f["x"].arraySize()==2
//             && f["x"][0].asUInt64()==7
//             && f["x"][1].asString()=="s";
//     }
// );
//------------------------------------------------------------------------------

} // namespace zerialize