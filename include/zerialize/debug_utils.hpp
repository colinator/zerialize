#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <span>
#include "concepts.hpp"
#include "value_type.hpp"

namespace zerialize {

using std::stringstream, std::string, std::vector, std::span;
using std::string_view, std::endl;

// --------------
// debug streaming

inline string repeated_string(int num, const string& input) {
    string ret;
    ret.reserve(input.size() * num);
    while (num--)
        ret += input;
    return ret;
}

void debug_stream(stringstream & s, int tabLevel, const Deserializable auto& v) {
    auto valueType = to_value_type(v);
    auto tab = "  ";
    auto tabString = repeated_string(tabLevel, tab);

    if (v.isMap()) {
        s << "<Map> {" << endl;
        for (string_view key: v.mapKeys()) {
            const string sk(key);
            s << tabString << tab << "\"" << key << "\": ";
            debug_stream(s, tabLevel+1, v[sk]);
        }
        s << tabString << "}" << endl;
    } else if (v.isArray()) {
        s << "<Array> [" << endl;
        for (size_t i=0; i<v.arraySize(); i++) {
            s << tabString << tab;
            debug_stream(s, tabLevel+1, v[i]);
        }
        s << tabString << "]" << endl;
    } else if (is_primitive(valueType)) {
        if (v.isUInt()) {
            s << v.asUInt64();
        } else if (v.isInt()) {
            s << v.asInt64();
        } else if (v.isFloat()) {
            s << v.asDouble();
        } else if (v.isString()) {
            s << "\"" << v.asString() << "\"";
        } else if (v.isNull()) {
            s << "<null/>";
        } else if (v.isBool()) {
            s << v.asBool();
        } else if (v.isBlob()) {
            auto blob = v.asBlob();
            s << "<" << blob.size() << " bytes>";
        }
        s << " <" << value_type_to_string(valueType) << ">" << endl;
    }
}

string debug_string(const Deserializable auto& v) {
    stringstream s;
    debug_stream(s, 0, v);
    return s.str();
}

inline std::string blob_to_string(vector<uint8_t>& s) {
    std::string result = "<vec " + std::to_string(s.size()) + ": ";
    for (uint8_t v : s) {
        result += " " + std::to_string(v);
    }
    result += ">";
    return result;
}

inline std::string blob_to_string(std::span<const uint8_t> s) {
    std::string result = "<span " + std::to_string(s.size()) + ": ";
    for (uint8_t v : s) {
        result += " " + std::to_string(v);
    }
    result += ">";
    return result;
}

} // namespace zerialize
