#pragma once

#include <cstddef>
#include <cstdint>

namespace zerialize {
namespace tensor {

enum class TensorViewReason {
    Ok = 0,
    NotSpanBacked,
    Misaligned,
};

// Metadata about whether a tensor/matrix wrapper is backed by a zero-copy view
// or an owning copy, and why.
struct TensorViewInfo {
    bool zero_copy = false;
    TensorViewReason reason = TensorViewReason::Ok;

    std::size_t required_alignment = 0;
    std::uintptr_t address = 0;
    std::size_t byte_size = 0;
};

} // namespace tensor
} // namespace zerialize

