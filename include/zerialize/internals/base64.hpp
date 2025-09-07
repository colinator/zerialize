#pragma once

#include <string>
#include <span>
#include <vector>
#include <string_view>
#include <cstddef>
#include <cstdint>
#include <zerialize/errors.hpp>

namespace zerialize {

/**
 * @brief Encode bytes as RFC 4648 Base64 (standard alphabet, with '=' padding).
 * Used by the json.hpp serializer (and any other serializer that can't natively
 * store blobs) to encode/decode blobs as strings.
 *
 * @param data Input bytes.
 * @return std::string Base64 text.
 *
 * @note This is the classic (not URL-safe) alphabet:
 *       ABC…XYZ abc…xyz 0…9 + /
 * @note Whitespace is not inserted; the output is a single line.
 */
inline std::string base64Encode(std::span<const std::byte> data) {
    static constexpr char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encoded;
    const size_t input_size = data.size();
    encoded.reserve(((input_size + 2) / 3) * 4);

    std::uint32_t value = 0; // rolling 24-bit buffer
    std::size_t   bits  = 0; // number of valid bits currently in 'value'

    std::size_t i = 0;
    while (i < input_size) {
        // Accumulate 8 bits
        value = (value << 8) | static_cast<std::uint8_t>(data[i++]);
        bits  += 8;

        // Emit as many 6-bit chunks as we can
        while (bits >= 6) {
            bits -= 6;
            encoded += base64_chars[(value >> bits) & 0x3F];
        }

        // Keep only the remaining (bits) LSBs; avoid value growing unbounded.
        if (bits > 0) value &= ((1u << bits) - 1);
    }

    // Flush remaining bits (0 < bits < 6)
    if (bits > 0) {
        encoded += base64_chars[(value << (6 - bits)) & 0x3F];
    }

    // Pad to multiple of 4
    while (encoded.size() % 4 != 0) {
        encoded += '=';
    }

    return encoded;
}

/**
 * @brief Decode RFC 4648 Base64 (standard alphabet, '=' padding).
 *
 * @param encoded Base64 text. Whitespace is NOT allowed (strict mode).
 * @return std::vector<std::byte> Decoded bytes.
 *
 * @throws DeserializationError on invalid characters or structure.
 * @note This is the classic (not URL-safe) alphabet:
 *       ABC…XYZ abc…xyz 0…9 + /
 * @note Padding '=' terminates decoding; trailing garbage is ignored.
 */
inline std::vector<std::byte> base64Decode(std::string_view encoded) {
    // 64 = invalid sentinel; other entries map ASCII -> 6-bit value.
    static constexpr std::uint8_t lookup[256] = {
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63, 52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,
        64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, 15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
        64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40, 41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64
    };

    std::vector<std::byte> out;
    out.reserve((encoded.size() / 4) * 3); // coarse upper bound

    std::uint32_t buf  = 0; // rolling 24-bit buffer of decoded 6-bit values
    int           bits = 0; // number of valid bits in 'buf'

    for (unsigned char c : std::string_view(encoded)) {
        if (c == '=') break;               // padding: stop decoding

        std::uint8_t v = lookup[c];
        if (v == 64) {
            throw DeserializationError("Invalid Base64 character");
        }

        buf  = (buf << 6) | v;
        bits += 6;

        // When we accumulate at least 8 bits, emit a byte.
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::byte>((buf >> bits) & 0xFF));
            // Keep only the remaining 'bits' LSBs (avoid buf growth).
            if (bits > 0) buf &= ((1u << bits) - 1);
        }
    }

    // Optionally: strict check leftover state. For classic Base64 the
    // leftover bits should be 0 after proper '=' padding. If you want strict mode:
    // if (bits != 0) throw DeserializationError("Truncated Base64 input");

    return out;
}

} // namespace zerialize