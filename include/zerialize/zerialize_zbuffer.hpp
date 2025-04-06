#pragma once

#include <string>
#include <vector>
#include <span>

namespace zerialize {

using std::vector, std::span, std::string;


// --------------
// A buffer type (basically an array of bytes) that
// can get it's data from different sources: moving in
// a std::vector, or taking ownership of a raw pointer.
// Re-exposes data as a span<uint8_t>.

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
    using OwnedVector = vector<uint8_t>;

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
    std::variant<OwnedVector, ManagedPtr> storage_;

public:
    // --- Constructors ---

    // Default constructor: Creates an empty buffer
    ZBuffer() noexcept 
        : storage_(std::in_place_type<OwnedVector>) {}

    // Constructor from a moved std::vector
    // Takes ownership of the vector's data.
    ZBuffer(std::vector<uint8_t>&& vec) noexcept
        : storage_(std::in_place_type<OwnedVector>, std::move(vec)) {}

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
            if constexpr (std::is_same_v<T, OwnedVector>) {
                return storage_impl.size();
            } else if constexpr (std::is_same_v<T, ManagedPtr>) {
                return storage_impl.count;
            } else {
                return 0; // Should be unreachable
            }
        }, storage_);
    }

    [[nodiscard]] const uint8_t* data() const noexcept {
            return std::visit([](const auto& storage_impl) -> const uint8_t* {
            using T = std::decay_t<decltype(storage_impl)>;
            if constexpr (std::is_same_v<T, OwnedVector>) {
                return storage_impl.data(); // vector::data() is fine in C++20
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

    [[nodiscard]] span<const uint8_t> buf() const noexcept {
        return span<const uint8_t>(data(), size());
    }

    std::string to_string() const {
        return string("<ZBuffer, size=") + std::to_string(size()) + ">";
    }

    // Creates a new vector containing a copy of the buffer's data.
    vector<uint8_t> to_vector_copy() const {
        const uint8_t* ptr = data();
        const size_t count = size();
        if (ptr && count > 0) {
            // Use the iterator range constructor to copy the data
            return vector<uint8_t>(ptr, ptr + count);
        } else {
            // Return an empty vector if the buffer is empty or invalid
            return vector<uint8_t>();
        }
    }
};

}
