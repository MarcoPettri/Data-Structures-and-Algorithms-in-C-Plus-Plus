/**
 * @file standard_trie.hpp
 * @brief Standard (uncompressed) trie for a set of ASCII strings (exercise P-12.6).
 *
 * Every node represents one character; a node is flagged as terminal when the
 * path from the root to it spells a stored string. This is equivalent to the
 * textbook's end-of-string marker, without spending a node on it.
 *
 * Children are kept in a small sorted vector per node and searched with binary
 * search, so memory is proportional to the number of nodes (not 128 pointers
 * per node) and lookup is O(m log sigma) for a string of length m.
 *
 */

#ifndef DS_STANDARD_TRIE_HPP
#define DS_STANDARD_TRIE_HPP

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "common.hpp"

namespace ds {

/**
 * @brief A standard trie over ASCII strings.
 *
 * @par Example
 * @code
 * ds::StandardTrie trie{"bear", "bell", "bid"};
 * trie.contains("bell");   // true
 * trie.contains("be");     // false (prefix only)
 * trie.hasPrefix("be");    // true
 * @endcode
 */
class StandardTrie {
public:
  /// Constructs an empty trie.
  StandardTrie() = default;

  /**
     * @brief Builds a trie from a list of strings.
     * @param words  Strings to store; duplicates are ignored.
     * @throws std::invalid_argument if any string contains a non-ASCII character.
     * @complexity O(total length * log sigma).
     */
  explicit StandardTrie(const std::vector<std::string> &words);

  /**
     * @brief Builds a trie from a brace-enclosed list, e.g. `StandardTrie{"a", "ab"}`.
     * @param words  Strings to store.
     * @throws std::invalid_argument if any string contains a non-ASCII character.
     */
  StandardTrie(std::initializer_list<std::string_view> words);

  /**
     * @brief Builds a trie from an iterator range of string-like values.
     * @tparam InputIt  Iterator whose value converts to std::string_view.
     * @param first,last  Range of strings to store.
     * @throws std::invalid_argument if any string contains a non-ASCII character.
     */
  template <typename InputIt,
            typename = detail::EnableIfStringIterator<InputIt>>
  StandardTrie(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      insert(std::string_view(*first));
    }
  }

  /**
     * @brief Inserts a string (no effect if already stored).
     * @param word  ASCII string; may be empty.
     * @throws std::invalid_argument if @p word contains a non-ASCII character
     *         (the trie is left unchanged).
     * @complexity O(m log sigma) for a string of length m.
     */
  void insert(std::string_view word);

  /**
     * @brief Tests whether a string is stored in the trie.
     * @param word  String to look up.
     * @return true if @p word was inserted.
     * @complexity O(m log sigma).
     */
  [[nodiscard]] bool contains(std::string_view word) const noexcept;

  /**
     * @brief Tests whether some stored string starts with @p prefix.
     * @param prefix  Prefix to look up (the empty prefix matches iff the trie is non-empty).
     * @complexity O(m log sigma).
     */
  [[nodiscard]] bool hasPrefix(std::string_view prefix) const noexcept;

  /// @return Number of distinct stored strings.
  [[nodiscard]] std::size_t size() const noexcept { return wordCount_; }

  /// @return true if no string is stored.
  [[nodiscard]] bool empty() const noexcept { return wordCount_ == 0; }

  /// @return Number of trie nodes, including the root.
  [[nodiscard]] std::size_t nodeCount() const noexcept { return nodes_.size(); }

private:
  static constexpr std::uint32_t kNoNode = UINT32_MAX;

  /// Edge to a child, labelled with one character.
  struct Edge {
    unsigned char key;
    std::uint32_t child;
  };

  /// Trie node; `edges` is sorted by `key`.
  struct Node {
    std::vector<Edge> edges;
    bool terminal = false;
  };

  /// @return Index of the first edge in @p node whose key is not less than @p key.
  [[nodiscard]] static std::size_t lowerBound(const Node &node,
                                              unsigned char key) noexcept;

  /// @return Node reached by spelling @p s from the root, or kNoNode.
  [[nodiscard]] std::uint32_t walk(std::string_view s) const noexcept;

  std::vector<Node> nodes_ = std::vector<Node>(1); ///< nodes_[0] is the root.
  std::size_t wordCount_ = 0;
};

} // namespace ds

#endif // DS_STANDARD_TRIE_HPP
