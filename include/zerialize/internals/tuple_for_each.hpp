#pragma once

#include <tuple> 
#include <utility>
#include <type_traits>

namespace zerialize {

/**
 * @brief Visit each element in a std::tuple, in order, forwarding value category.
 *
 * @tparam I     Current index (callers never specify; recursion anchor).
 * @tparam Tuple Any std::tuple-like type.
 * @tparam F     Callable invocable with each element (by perfect forwarding).
 *
 * @param tuple  The tuple to iterate.
 * @param f      The function to apply to each element.
 *
 * @note This is `constexpr`-friendly and preserves lvalue/rvalue-ness of elements.
 */
template<std::size_t I = 0, typename Tuple, typename F>
constexpr void tuple_for_each(Tuple&& tuple, F&& f) {
    if constexpr (I < std::tuple_size_v<std::remove_reference_t<Tuple>>) {
        f(std::get<I>(std::forward<Tuple>(tuple)));
        tuple_for_each<I + 1>(std::forward<Tuple>(tuple), std::forward<F>(f));
    }
}

}
