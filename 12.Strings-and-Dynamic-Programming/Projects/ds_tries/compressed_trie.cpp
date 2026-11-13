/**
 * @file compressed_trie.cpp
 * @brief Member definitions of ds::CompressedTrie.
 */

#include "compressed_trie.hpp"

#include <algorithm>
#include <string>

namespace ds {

CompressedTrie::CompressedTrie(const std::vector<std::string> &words) {
  for (const auto &w : words) {
    insert(w);
  }
}

CompressedTrie::CompressedTrie(std::initializer_list<std::string_view> words) {
  for (const auto w : words) {
    insert(w);
  }
}

std::size_t CompressedTrie::lowerBound(const Node &node,
                                       unsigned char key) const noexcept {
  const auto it = std::lower_bound(
      node.edges.begin(), node.edges.end(), key,
      [this](const Edge &e, unsigned char k) { return firstChar(e) < k; });
  return static_cast<std::size_t>(it - node.edges.begin());
}

const CompressedTrie::Edge *
CompressedTrie::findEdge(const Node &node, unsigned char key) const noexcept {
  const std::size_t idx = lowerBound(node, key);
  if (idx < node.edges.size() && firstChar(node.edges[idx]) == key) {
    return &node.edges[idx];
  }
  return nullptr;
}

void CompressedTrie::markTerminal(std::uint32_t node) {
  if (!nodes_[node].terminal) {
    nodes_[node].terminal = true;
    ++wordCount_;
  }
}

void CompressedTrie::addLeaf(std::uint32_t parent, std::size_t idx,
                             std::string_view rest) {
  const std::uint32_t begin = detail::toU32(pool_.size());
  const std::uint32_t end = detail::toU32(pool_.size() + rest.size());
  const std::uint32_t leaf = detail::toU32(nodes_.size());

  pool_.append(rest);
  nodes_.emplace_back();  // may reallocate: re-fetch the parent afterwards
  nodes_[leaf].terminal = true;
  ++wordCount_;

  auto &edges = nodes_[parent].edges;
  edges.insert(edges.begin() + static_cast<std::ptrdiff_t>(idx),
               Edge{begin, end, leaf});
}

std::uint32_t CompressedTrie::splitEdge(std::uint32_t parent, std::size_t idx,
                                        std::size_t k) {
  const std::uint32_t mid = detail::toU32(nodes_.size());

  Edge tail{};
  {
    Edge &edge = nodes_[parent].edges[idx];
    const auto split = static_cast<std::uint32_t>(edge.begin + k);
    tail = Edge{split, edge.end, edge.child};  // remainder of the old label
    edge.end = split; // head keeps the first k characters
    edge.child = mid;
  }

  nodes_.emplace_back(); // `edge` is not used past this point
  nodes_[mid].edges.push_back(tail);
  return mid;
}

void CompressedTrie::insert(std::string_view word) {
  detail::requireAscii(
      word, "CompressedTrie::insert"); // validate before mutating anything

  std::uint32_t cur = 0;
  std::size_t pos = 0;

  for (;;) {
    if (pos == word.size()) {
      markTerminal(cur);
      return;
    }

    const auto key = static_cast<unsigned char>(word[pos]);
    const std::size_t idx = lowerBound(nodes_[cur], key);
    const auto &edges = nodes_[cur].edges;

    if (idx == edges.size() || firstChar(edges[idx]) != key) {
      addLeaf(cur, idx, word.substr(pos)); // no edge starts with this character
      return;
    }

    const Edge &edge = edges[idx];
    const std::size_t labelLen = edge.end - edge.begin;
    const std::size_t limit = std::min(labelLen, word.size() - pos);

    std::size_t k = 1; // the first character is known to match
    while (k < limit && pool_[edge.begin + k] == word[pos + k]) {
      ++k;
    }

    if (k == labelLen) { // whole label consumed: descend
      cur = edge.child;
      pos += k;
      continue;
    }

    cur = splitEdge(cur, idx, k); // diverges (or ends) inside the label
    pos += k;
  }
}

bool CompressedTrie::contains(std::string_view word) const noexcept {
  std::uint32_t cur = 0;
  std::size_t pos = 0;

  while (pos < word.size()) {
    const Edge *edge =
        findEdge(nodes_[cur], static_cast<unsigned char>(word[pos]));
    if (edge == nullptr) {
      return false;
    }
    const std::size_t len = edge->end - edge->begin;
    if (word.size() - pos < len ||
        std::char_traits<char>::compare(pool_.data() + edge->begin,
                                        word.data() + pos, len) != 0) {
      return false;
    }
    cur = edge->child;
    pos += len;
  }
  return nodes_[cur].terminal;
}

bool CompressedTrie::hasPrefix(std::string_view prefix) const noexcept {
  if (wordCount_ == 0) {
    return false;
  }

  std::uint32_t cur = 0;
  std::size_t pos = 0;

  while (pos < prefix.size()) {
    const Edge *edge =
        findEdge(nodes_[cur], static_cast<unsigned char>(prefix[pos]));
    if (edge == nullptr) {
      return false;
    }
    const std::size_t len = edge->end - edge->begin;
    const std::size_t cmp = std::min(len, prefix.size() - pos);
    if (std::char_traits<char>::compare(pool_.data() + edge->begin,
                                        prefix.data() + pos, cmp) != 0) {
      return false;
    }
    cur = edge->child;
    pos += cmp; // a prefix may end in the middle of an edge label
  }
  return true;
}

} // namespace ds
