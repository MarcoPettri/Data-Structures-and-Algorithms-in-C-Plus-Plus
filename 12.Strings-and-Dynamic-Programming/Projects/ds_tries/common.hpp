/**
 * @file common.hpp
 * @brief Small helpers shared by the trie classes (ASCII validation, index checks).
 */

#ifndef DS_COMMON_HPP
#define DS_COMMON_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace ds::detail {

/// @return true if @p c is a 7-bit ASCII character (code point 0..127).
[[nodiscard]] inline bool isAscii(char c) noexcept {
  return static_cast<unsigned char>(c) < 128U;
}

/// @return true if every character of @p s is 7-bit ASCII.
[[nodiscard]] inline bool isAscii(std::string_view s) noexcept {
  return std::all_of(s.begin(), s.end(), [](char c) { return isAscii(c); });
}

/**
 * @brief Throws if @p s contains a non-ASCII character.
 * @param s    String to validate.
 * @param who  Name of the calling function, used in the message.
 * @throws std::invalid_argument on a non-ASCII character.
 */
inline void requireAscii(std::string_view s, const char *who) {
  if (!isAscii(s)) {
    throw std::invalid_argument(std::string(who) +
                                ": input must contain only ASCII characters");
  }
}

/**
 * @brief Narrows a size to a 32-bit index (the tries use 32-bit links to save memory).
 * @throws std::length_error if @p value does not fit in 32 bits.
 */
[[nodiscard]] inline std::uint32_t toU32(std::size_t value) {
  if (value > std::numeric_limits<std::uint32_t>::max()) {
    throw std::length_error("ds: structure exceeds 2^32 - 1 elements");
  }
  return static_cast<std::uint32_t>(value);
}

/// SFINAE helper: enabled when dereferencing @p It yields something convertible to std::string_view.
template <typename It>
using EnableIfStringIterator = std::enable_if_t<
    std::is_convertible_v<decltype(*std::declval<It &>()), std::string_view>>;

}  // namespace ds::detail

#endif  // DS_COMMON_HPP
