/**
 * @file compressed_trie.hpp
 * @brief Compressed (radix / Patricia-style) trie for ASCII strings (exercise P-12.7).
 *
 * Chains of single-child nodes are merged, so every internal node that is not
 * terminal has at least two children and the trie has O(n) nodes for n stored
 * strings, independent of their lengths.
 *
 * Edge labels are not stored as separate strings: each edge keeps a
 * `[begin, end)` range into one shared character pool (the textbook's
 * "(i, j, k)" substring trick, with a single pool instead of per-word indices).
 *
 */

#ifndef DS_COMPRESSED_TRIE_HPP
#define DS_COMPRESSED_TRIE_HPP

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "common.hpp"

namespace ds {

/**
     * @brief A compressed trie over ASCII strings.
     *
     * @par Example
     * @code
     * ds::CompressedTrie trie{"bear", "bell", "bid", "bull", "buy", "sell", "stock", "stop"};
     * trie.contains("stock");  // true
     * trie.contains("sto");    // false
     * trie.nodeCount();        // 14 (a standard trie needs 22)
     * @endcode
     */
class CompressedTrie {
public:
  /// Constructs an empty trie.
  CompressedTrie() = default;

  /**
         * @brief Builds a trie from a list of strings.
         * @param words  Strings to store; duplicates are ignored.
         * @throws std::invalid_argument if any string contains a non-ASCII character.
         * @complexity O(total length * log sigma).
         */
  explicit CompressedTrie(const std::vector<std::string> &words);

  /**
         * @brief Builds a trie from a brace-enclosed list, e.g. `CompressedTrie{"a", "ab"}`.
         * @param words  Strings to store.
         * @throws std::invalid_argument if any string contains a non-ASCII character.
*/
  CompressedTrie(std::initializer_list<std::string_view> words);

  /**
         * @brief Builds a trie from an iterator range of string-like values.
         * @tparam InputIt  Iterator whose value converts to std::string_view.
         * @param first,last  Range of strings to store.
         * @throws std::invalid_argument if any string contains a non-ASCII character.
         */
  template <typename InputIt,
            typename = detail::EnableIfStringIterator<InputIt>>
  CompressedTrie(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      insert(std::string_view(*first));
    }
  }

  /**
         * @brief Inserts a string, splitting an edge when it diverges mid-label.
         * @param word  ASCII string; may be empty.
         * @throws std::invalid_argument if @p word contains a non-ASCII character
         *         (the trie is left unchanged).
         * @throws std::length_error if the label pool would exceed 2^32 - 1 characters.
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
  /// Edge whose label is `pool_[begin, end)`; its first character is the sort key.
  struct Edge {
    std::uint32_t begin;
    std::uint32_t end;
    std::uint32_t child;
  };

  /// Trie node; `edges` is sorted by the first character of each label.
  struct Node {
    std::vector<Edge> edges;
    bool terminal = false;
  };

  [[nodiscard]] unsigned char firstChar(const Edge &e) const noexcept {
    return static_cast<unsigned char>(pool_[e.begin]);
  }

  /// @return Index of the first edge of @p node whose first character is not less than @p key.
  [[nodiscard]] std::size_t lowerBound(const Node &node,
                                       unsigned char key) const noexcept;

  /// @return The edge of @p node starting with @p key, or nullptr.
  [[nodiscard]] const Edge *findEdge(const Node &node,
                                     unsigned char key) const noexcept;

  /// Flags @p node as the end of a stored string.
  void markTerminal(std::uint32_t node);

  /// Appends @p rest to the pool and hangs a new terminal leaf under @p parent at edge slot @p idx.
  void addLeaf(std::uint32_t parent, std::size_t idx, std::string_view rest);

  /**
         * @brief Splits the edge `nodes_[parent].edges[idx]` after @p k label characters.
         * @return The new intermediate node.
         */
  std::uint32_t splitEdge(std::uint32_t parent, std::size_t idx, std::size_t k);

  std::vector<Node> nodes_ = std::vector<Node>(1); ///< nodes_[0] is the root.
  std::string pool_; ///< Shared storage for all edge labels.
  std::size_t wordCount_ = 0;
};

} // namespace ds

#endif // DS_COMPRESSED_TRIE_HPP
