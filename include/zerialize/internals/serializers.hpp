#pragma once

#include <map>
#include <unordered_map>
#include <array>
#include <list>
#include <zerialize/concepts.hpp>

//──────────────────────────────────────────────────────────────────────────────
//  serializers.hpp
//
//  This header defines the default `serialize` overloads for primitive types,
//  strings, binary blobs, and common STL containers. Each overload takes a
//  `Writer` that conforms to the zerialize::Writer concept and emits values
//  into the underlying serialization protocol.
//
//  The functions here provide the "baseline vocabulary" of serialization for
//  built-in C++ types and standard containers. More specialized types (e.g.,
//  user-defined classes, third-party libraries like xtensor) can provide their
//  own `serialize` overloads via ADL, defined in the same namespace as the
//  type being serialized.
//
//  In short: this file establishes the core serialization rules that all
//  protocols (JSON, MsgPack, Flex, etc.) can rely on out of the box.
//──────────────────────────────────────────────────────────────────────────────

namespace zerialize {

// Primitive serialize functions
template<Writer W> void serialize(std::nullptr_t, W& w) { w.null(); }
template<Writer W> void serialize(bool v, W& w) { w.boolean(v); }
template<Writer W> void serialize(char v, W& w) { w.int64(v); }
template<Writer W> void serialize(signed char v, W& w) { w.int64(v); }
template<Writer W> void serialize(unsigned char v, W& w) { w.uint64(v); }
template<Writer W> void serialize(short v, W& w) { w.int64(v); }
template<Writer W> void serialize(unsigned short v, W& w) { w.uint64(v); }
template<Writer W> void serialize(int v, W& w) {  w.int64(v); }
template<Writer W> void serialize(unsigned v, W& w) { w.uint64(v); }
template<Writer W> void serialize(long v, W& w) { w.int64(v); }
template<Writer W> void serialize(unsigned long v, W& w) { w.uint64(v); }
template<Writer W> void serialize(long long v, W& w) { w.int64(v); }
template<Writer W> void serialize(unsigned long long v, W& w) { w.uint64(v); }
template<Writer W> void serialize(float v, W& w) { w.double_(v); }
template<Writer W> void serialize(double v, W& w) { w.double_(v); }

// String types
template<Writer W> void serialize(const char* v, W& w) { w.string(v); }
template<Writer W> void serialize(std::string_view v, W& w) { w.string(v); }
template<Writer W> void serialize(const std::string& v, W& w) { w.string(v); }

// Binary data
template<Writer W> void serialize(const std::vector<std::byte>& v, W& w) { w.binary(v); }
template<Writer W> void serialize(std::span<const std::byte> v, W& w) { w.binary(v); }

// Containers
template<Writer W, class T>
void serialize(const std::list<T>& l, W& w) {
    w.begin_array(l.size());
    using zerialize::serialize;
    for (const auto& item : l) {
        serialize(item, w);
    }
    w.end_array();
}

template<Writer W, class T>
void serialize(const std::vector<T>& vec, W& w) {
    w.begin_array(vec.size());
    using zerialize::serialize;
    for (const auto& item : vec) {
        serialize(item, w);
    }
    w.end_array();
}

template<Writer W, class T, size_t N>
void serialize(const std::array<T, N>& arr, W& w) {
    w.begin_array(arr.size());
    using zerialize::serialize;
    for (const auto& item : arr) {
        serialize(item, w);
    }
    w.end_array();
}

template<Writer W, class T> 
void serialize(std::initializer_list<T> list, W& w) {
    w.begin_array(list.size());
    using zerialize::serialize;
    for (const auto& item : list) {
        serialize(item, w);
    }
    w.end_array();
}

template<Writer W, class K, class V> 
void serialize(const std::map<K, V>& map, W& w) {
    w.begin_map(map.size());
    using zerialize::serialize;
    for (const auto& [key, val] : map) {
        w.key(key);
        serialize(val, w);
    }
    w.end_map();
}

template<Writer W, class K, class V>
void serialize(const std::unordered_map<K, V>& map, W& w) {
    w.begin_map(map.size());
    using zerialize::serialize;
    for (const auto& [key, val] : map) {
        w.key(key);
        serialize(val, w);
    }
    w.end_map();
}

} // namespace zerialize
