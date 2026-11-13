/**
 * @file suffix_trie.hpp
 * @brief Compressed suffix trie of an ASCII string with pattern matching (exercise P-12.8).
 *
 * @note The exercise text asks for a "prefix trie" with pattern matching. A trie
 *       of only the prefixes of one string is a single chain and can only answer
 *       "is this a prefix?". Matching a pattern *anywhere* in the string needs the
 *       trie of all **suffixes** (a pattern occurs in the text iff it is a prefix
 *       of some suffix), which is what this class builds.
 *
 * Construction uses Ukkonen's online algorithm: O(n log sigma) time and O(n)
 * nodes for a text of length n. Edge labels are `[start, end)` ranges into the
 * stored text, so no substring is ever copied. A unique sentinel (byte 0x80,
 * outside ASCII) is appended internally so that every suffix ends at a leaf.
 *
 */

#ifndef DS_SUFFIX_TRIE_HPP
#define DS_SUFFIX_TRIE_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ds {

/**
 * @brief Compressed suffix trie (suffix tree) of an ASCII text.
 *
 * @par Example
 * @code
 * ds::SuffixTrie trie("banana");
 * trie.contains("nan");          // true
 * trie.findAll("ana");           // {1, 3}
 * trie.findFirst("na");          // 2
 * trie.count("a");               // 3
 * @endcode
 */
class SuffixTrie {
public:
  /**
     * @brief Builds the suffix trie of @p text.
     * @param text  ASCII text; may be empty and may contain NUL characters.
     * @throws std::invalid_argument if @p text contains a non-ASCII character.
     * @throws std::length_error if @p text is longer than about 2^32 characters.
     * @complexity O(n log sigma) time, O(n) space.
     */
  explicit SuffixTrie(std::string_view text);

  /// @return The indexed text (without the internal sentinel).
  [[nodiscard]] std::string_view text() const noexcept {
    return std::string_view(text_).substr(0, text_.size() - 1);
  }

  /// @return Length of the indexed text.
  [[nodiscard]] std::size_t textSize() const noexcept {
    return text_.size() - 1;
  }

  /// @return Number of trie nodes, including the root and the leaves.
  [[nodiscard]] std::size_t nodeCount() const noexcept { return nodes_.size(); }

  /**
     * @brief Tests whether @p pattern occurs as a substring of the text.
     * @param pattern  Pattern to search (empty always matches; non-ASCII never matches).
     * @complexity O(m log sigma) for a pattern of length m.
     */
  [[nodiscard]] bool contains(std::string_view pattern) const noexcept;

  /**
     * @brief Counts the occurrences of @p pattern (overlapping occurrences included).
     * @note The empty pattern occurs at every position `0..n`, i.e. `n + 1` times.
     * @complexity O(m log sigma + occ).
     */
  [[nodiscard]] std::size_t count(std::string_view pattern) const;

  /**
     * @brief Returns the smallest starting index of @p pattern, if any.
     * @complexity O(m log sigma + occ).
     */
  [[nodiscard]] std::optional<std::size_t>
  findFirst(std::string_view pattern) const;

  /**
     * @brief Returns all starting indices of @p pattern in increasing order.
     * @complexity O(m log sigma + occ log occ).
     */
  [[nodiscard]] std::vector<std::size_t>
  findAll(std::string_view pattern) const;

  /**
     * @brief Calls @p visit(std::size_t index) for every occurrence of @p pattern.
     *
     * The visiting order is unspecified (it follows the lexicographic order of
     * the suffixes). Use this to avoid allocating when only aggregating.
     *
     * @tparam Visitor  Callable accepting a `std::size_t`.
     * @param pattern  Pattern to search.
     * @param visit    Callback invoked once per occurrence.
     * @complexity O(m log sigma + occ).
     */
  template <typename Visitor>
  void forEachOccurrence(std::string_view pattern, Visitor &&visit) const {
    const std::uint32_t locus = locate(pattern);
    if (locus == kNone) {
      return;
    }

    // Every leaf below the locus is a suffix that starts with the pattern.
    std::vector<std::uint32_t> stack{locus};
    while (!stack.empty()) {
      const Node &node = nodes_[stack.back()];
      stack.pop_back();

      if (node.end == kLeafEnd) {
        visit(static_cast<std::size_t>(node.suffix));
        continue;
      }
      for (const Child &child : node.children) {
        stack.push_back(child.node);
      }
    }
  }

private:
  static constexpr char kSentinel =
      static_cast<char>(0x80);  ///< Never valid in ASCII input.
  static constexpr std::uint32_t kNone =
      std::numeric_limits<std::uint32_t>::max();
  static constexpr std::uint32_t kLeafEnd =
      kNone;  ///< `end` value marking a leaf ("open" edge).

    /// Child link keyed by the first character of the edge label.
  struct Child {
    unsigned char key;
    std::uint32_t node;
  };

  /// Node; the label of the edge entering it is `text_[start, end)`.
  struct Node {
    std::uint32_t start = 0;
    std::uint32_t end = 0;
    std::uint32_t link =
        0;    ///< Suffix link (internal nodes only); root by default.
    std::uint32_t suffix = 0; ///< Starting index of the suffix (leaves only).
    std::vector<Child> children; ///< Sorted by `key`.
  };

  /// Runs Ukkonen's algorithm over text_.
  void build();

  /// Appends a node and returns its index.
  std::uint32_t newNode(std::size_t start, std::uint32_t end,
                        std::size_t suffix);

  /// @return Index of the first child of @p node whose key is not less than @p key.
  [[nodiscard]] static std::size_t lowerBound(const Node &node,
                                              unsigned char key) noexcept;

  /// Inserts a child link at slot @p idx (obtained from lowerBound()).
  void insertChild(std::uint32_t parent, std::size_t idx, unsigned char key,
                   std::uint32_t child);

  /**
     * @brief Finds the node whose subtree contains exactly the occurrences of @p pattern.
     * @return Node index, or kNone if the pattern does not occur.
     */
  [[nodiscard]] std::uint32_t locate(std::string_view pattern) const noexcept;

  std::string text_; ///< Indexed text followed by kSentinel.
  std::vector<Node> nodes_; ///< nodes_[0] is the root.
};

} // namespace ds

// #include "ds/suffix_trie.tpp"

#endif // DS_SUFFIX_TRIE_HPP
