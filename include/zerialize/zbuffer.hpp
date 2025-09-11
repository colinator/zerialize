#pragma once

#include <iomanip>
#include <cctype>
#include <string>
#include <sstream>
#include <vector>
#include <span>
#include <variant>
#include <functional>

namespace zerialize {

/*
 * ZBuffer
 * --------
 * A flexible, RAII-managed byte buffer abstraction used throughout zerialize.
 * When you call `serialize`, this is what you get back.
 *
 * Key points:
 *   • Encapsulates a contiguous block of bytes with unique ownership semantics.
 *   • Can be constructed from:
 *       - A moved `std::vector<uint8_t>` (takes ownership of its storage).
 *       - A raw pointer (`uint8_t*`, `char*`, or `void*`) + size + custom deleter.
 *         Useful for integrating with C APIs, malloc/new allocations, or foreign libraries.
 *   • Internally stores either:
 *       - An owned `std::vector<uint8_t>`.
 *       - A managed pointer (`unique_ptr<uint8_t, function<void(uint8_t*)>>`) + length.
 *   • Always provides a uniform view via:
 *       - `.data()`   → raw pointer
 *       - `.size()`   → length in bytes
 *       - `.buf()`    → `std::span<const uint8_t>` (preferred)
 *
 * Intended usage:
 *   ZBuffer acts as the "glue" between serializers and deserializers,
 *   making it easy to hand off raw protocol bytes while abstracting away
 *   ownership rules. 
 *
 * Example:
 *   // From a std::vector
 *   std::vector<uint8_t> bytes = {...};
 *   zerialize::ZBuffer buf(std::move(bytes));
 *
 *   // From a malloc’d buffer
 *   void* raw = malloc(1024);
 *   zerialize::ZBuffer buf(raw, 1024, zerialize::ZBuffer::Deleters::Free);
 *
 *   // As a span
 *   auto view = buf.buf();
 *
 *   // Copy out
 *   std::vector<uint8_t> copy = buf.to_vector_copy();
 *
 * Notes:
 *   • Copy construction is disabled (unique ownership).
 *   • Move construction/assignment is supported.
 *   • Provides helper methods for debugging (`to_string`, `to_debug_string`).
 */

class ZBuffer {
public:

    // --- Deleter Helpers ---
    // Provides convenient access to common deleters
    struct Deleters {
        // Deleter for memory allocated with `malloc`, `calloc`, `realloc`
        // Takes void* to match std::free signature
        static constexpr auto Free = [](void* ptr) { std::free(ptr); };

        // Deleter for memory allocated with `new uint8_t[...]`
        static constexpr auto DeleteArray = [](std::uint8_t* ptr) { delete[] ptr; };

        // No-op deleter for data that shouldn't be freed by ZBuffer (e.g., stack, static)
        // Use with caution! Ensure the data outlives the ZBuffer.
        static constexpr auto NoOp = [](std::uint8_t*) { };
    };
    

private:
    // --- Storage Strategies ---

    // 1. Data owned by a std::vector
    using Ownedvector = std::vector<uint8_t>;

    // 2. Data owned by a raw pointer with a custom deleter
    // We use unique_ptr with a type-erased deleter (std::function)
    struct ManagedPtr {
        // unique_ptr stores the pointer and the deleter.
        // The deleter lambda captures any necessary context (like original type).
        std::unique_ptr<uint8_t, std::function<void(uint8_t*)>> ptr;
        size_t count = 0;

        ManagedPtr(uint8_t* data_to_own, size_t size, std::function<void(uint8_t*)> deleter)
            : ptr(data_to_own, std::move(deleter)), count(size) {}

        // Default constructor needed for variant default construction (if needed)
        ManagedPtr() = default;
    };

    // --- Variant holding the active storage strategy ---
    std::variant<Ownedvector, ManagedPtr> storage_;

public:
    // --- Constructors ---

    // Default constructor: Creates an empty buffer
    ZBuffer() noexcept 
        : storage_(std::in_place_type<Ownedvector>) {}

    // Constructor from a moved std::std::vector
    // Takes ownership of the std::vector's data.
    ZBuffer(std::vector<uint8_t>&& vec) noexcept
        : storage_(std::in_place_type<Ownedvector>, std::move(vec)) {}

    // Constructor taking ownership of a raw uint8_t* pointer with a custom deleter.
    // The deleter MUST correctly free the provided pointer.
    ZBuffer(uint8_t* data_to_own, size_t size, std::function<void(uint8_t*)> deleter)
        : storage_(std::in_place_type<ManagedPtr>, data_to_own, size, std::move(deleter))
    {
        if (size > 0 && data_to_own == nullptr) {
            throw std::invalid_argument("ZBuffer: Non-zero size requires a non-null pointer");
        }
        if (!std::get<ManagedPtr>(storage_).ptr.get_deleter()) { // Check if deleter is valid (!= nullptr)
            throw std::invalid_argument("ZBuffer: A valid deleter function must be provided");
        }
    }

    // Constructor taking ownership of a raw char* pointer with a custom deleter.
    // The provided deleter should expect a char*.
    ZBuffer(char* data_to_own, size_t size, std::function<void(char*)> char_deleter)
        // Delegate to the uint8_t* constructor, wrapping the deleter
        : ZBuffer(reinterpret_cast<uint8_t*>(data_to_own), size,
            // Create a lambda that captures the original char* deleter
            // and performs the cast back before calling it.
            [cd = std::move(char_deleter)](uint8_t* ptr_to_delete) {
                if (ptr_to_delete) { // Avoid casting/deleting null
                    cd(reinterpret_cast<char*>(ptr_to_delete));
                }
            }) // End of arguments to delegating constructor
    {}

    // Constructor taking ownership of a raw void* pointer with a custom *void* deleter
    // Useful for C APIs returning void* (like from malloc).
    ZBuffer(void* data_to_own, size_t size, std::function<void(void*)> void_deleter)
        : ZBuffer(static_cast<uint8_t*>(data_to_own), size,
            [vd = std::move(void_deleter)](uint8_t* ptr_to_delete) {
                if (ptr_to_delete) {
                    vd(static_cast<void*>(ptr_to_delete)); // Cast back to void* for deleter
                }
            })
    {}

    // Delete copy constructor and assignment - ZBuffer manages unique ownership
    ZBuffer(const ZBuffer&) = delete;
    ZBuffer& operator=(const ZBuffer&) = delete;

    // Default move constructor and assignment are okay (variant handles it)
    ZBuffer(ZBuffer&&) noexcept = default;
    ZBuffer& operator=(ZBuffer&&) noexcept = default;


    // --- Accessors ---

    [[nodiscard]] size_t size() const noexcept {
        return std::visit([](const auto& storage_impl) -> size_t {
            using T = std::decay_t<decltype(storage_impl)>;
            if constexpr (std::is_same_v<T, Ownedvector>) {
                return storage_impl.size();
            } else if constexpr (std::is_same_v<T, ManagedPtr>) {
                return storage_impl.count;
            } else {
                return 0; // Should be unreachable
            }
        }, storage_);
    }

    [[nodiscard]] bool owned() const noexcept {
        return std::holds_alternative<Ownedvector>(storage_);
    }

    [[nodiscard]] const uint8_t* data() const noexcept {
        return std::visit([](const auto& storage_impl) -> const uint8_t* {
            using T = std::decay_t<decltype(storage_impl)>;
            if constexpr (std::is_same_v<T, Ownedvector>) {
                return storage_impl.data(); // std::vector::data() is fine in C++20
            } else if constexpr (std::is_same_v<T, ManagedPtr>) {
                return storage_impl.ptr.get(); // unique_ptr::get()
            } else {
                return nullptr; // Should be unreachable
            }
        }, storage_);
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }

    [[nodiscard]] std::span<const uint8_t> buf() const noexcept {
        return std::span<const uint8_t>(data(), size());
    }

    std::string to_string() const {
        return std::string("<ZBuffer ") + std::to_string(size()) + " bytes, owned=" + (owned() ? "true" : "false") + ">";
    }

    // Returns the buffer data as a string for debugging purposes.
    // For text-based formats like JSON, this will be human-readable.
    // For binary formats, this may contain non-printable characters.
    std::string to_debug_string() const {
        const uint8_t* ptr = data();
        const size_t count = size();
        if (ptr && count > 0) {
            // Convert the raw bytes to a string
            // for (size_t i=0; i<count; i++) {
            //     std::cout << ptr[i];
            // }
            return std::string(reinterpret_cast<const char*>(ptr), count);
        } else {
            // Return an empty string if the buffer is empty or invalid
            return std::string();
        }
    }

    // Creates a new std::std::vector containing a copy of the buffer's data.
    std::vector<uint8_t> to_vector_copy() const {
        const uint8_t* ptr = data();
        const size_t count = size();
        if (ptr && count > 0) {
            // Use the iterator range constructor to copy the data
            return std::vector<uint8_t>(ptr, ptr + count);
        } else {
            // Return an empty std::vector if the buffer is empty or invalid
            return std::vector<uint8_t>();
        }
    }

    // Pretty hexdump (like `hexdump -C`): offset, hex, and ASCII column.
    // Requires <iomanip> and <cctype>.
    std::string hexdump(std::size_t bytes_per_row = 16) const {
        std::ostringstream os;
        const uint8_t* p = data();
        const std::size_t n = size();
        if (!p || n == 0) {
            return "(empty)\n";
        }
    
        // Save stream formatting
        std::ios::fmtflags f = os.flags();
        char old_fill = os.fill();
    
        for (std::size_t i = 0; i < n; i += bytes_per_row) {
            // Offset
            os << std::setw(8) << std::setfill('0') << std::hex << i << "  ";
    
            // Hex bytes (with an extra space between the two 8-byte halves)
            for (std::size_t j = 0; j < bytes_per_row; ++j) {
                if (i + j < n) {
                    os << std::setw(2) << std::setfill('0') << std::hex
                       << static_cast<int>(p[i + j]) << ' ';
                } else {
                    os << "   ";
                }
                if (j == 7) os << ' ';
            }
    
            // ASCII column
            os << " |";
            for (std::size_t j = 0; j < bytes_per_row && (i + j) < n; ++j) {
                unsigned char c = static_cast<unsigned char>(p[i + j]);
                os << (std::isprint(c) ? static_cast<char>(c) : '.');
            }
            os << "|\n";
        }
    
        // Restore stream formatting
        os.flags(f);
        os.fill(old_fill);
    
        return os.str();
    }
};

}
