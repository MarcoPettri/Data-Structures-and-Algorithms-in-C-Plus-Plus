/**
 * @file standard_trie.cpp
 * @brief Member definitions of ds::StandardTrie.
 */

#include "standard_trie.hpp"

#include <algorithm>

namespace ds {

StandardTrie::StandardTrie(const std::vector<std::string> &words) {
  for (const auto &w : words) {
    insert(w);
  }
}

StandardTrie::StandardTrie(std::initializer_list<std::string_view> words) {
  for (const auto w : words) {
    insert(w);
  }
}

std::size_t StandardTrie::lowerBound(const Node &node,
                                     unsigned char key) noexcept {
  const auto it = std::lower_bound(
      node.edges.begin(), node.edges.end(), key,
      [](const Edge &e, unsigned char k) { return e.key < k; });
  return static_cast<std::size_t>(it - node.edges.begin());
}

void StandardTrie::insert(std::string_view word) {
  detail::requireAscii(
      word, "StandardTrie::insert");  // validate before mutating anything

  std::uint32_t cur = 0;
  for (const char ch : word) {
    const auto key = static_cast<unsigned char>(ch);
    const std::size_t idx = lowerBound(nodes_[cur], key);
    const auto &edges = nodes_[cur].edges;

    if (idx < edges.size() && edges[idx].key == key) {
      cur = edges[idx].child;
      continue;
    }

    const std::uint32_t created = detail::toU32(nodes_.size());
    nodes_.emplace_back();  // may reallocate: re-fetch the parent afterwards
    auto &parentEdges = nodes_[cur].edges;
    parentEdges.insert(parentEdges.begin() + static_cast<std::ptrdiff_t>(idx),
                       Edge{key, created});
    cur = created;
  }

  if (!nodes_[cur].terminal) {
    nodes_[cur].terminal = true;
    ++wordCount_;
  }
}

std::uint32_t StandardTrie::walk(std::string_view s) const noexcept {
  std::uint32_t cur = 0;
  for (const char ch : s) {
    const auto key = static_cast<unsigned char>(ch);
    const Node &node = nodes_[cur];
    const std::size_t idx = lowerBound(node, key);
    if (idx == node.edges.size() || node.edges[idx].key != key) {
      return kNoNode;
    }
    cur = node.edges[idx].child;
  }
  return cur;
}

bool StandardTrie::contains(std::string_view word) const noexcept {
  const std::uint32_t node = walk(word);
  return node != kNoNode && nodes_[node].terminal;
}

bool StandardTrie::hasPrefix(std::string_view prefix) const noexcept {
  if (wordCount_ == 0) {
    return false;
  }
  return walk(prefix) != kNoNode;
}

}  // namespace ds
