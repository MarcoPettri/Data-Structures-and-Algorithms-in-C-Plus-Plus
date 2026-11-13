/**
 * @file suffix_trie.cpp
 * @brief member definitions of ds::SuffixTrie (Ukkonen's algorithm).
 */

#include "suffix_trie.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "common.hpp"

namespace ds {

SuffixTrie::SuffixTrie(std::string_view text) {
  detail::requireAscii(text, "SuffixTrie");
  if (text.size() > std::numeric_limits<std::uint32_t>::max() / 2 - 2) {
    throw std::length_error("SuffixTrie: text too long");
  }
  text_.reserve(text.size() + 1);
  text_.append(text);
  text_.push_back(kSentinel);
  build();
}

std::uint32_t SuffixTrie::newNode(std::size_t start, std::uint32_t end,
                                  std::size_t suffix) {
  const std::uint32_t id = detail::toU32(nodes_.size());
  Node node;
  node.start = detail::toU32(start);
  node.end = end;
  node.suffix = detail::toU32(suffix);
  nodes_.push_back(std::move(node));
  return id;
}

std::size_t SuffixTrie::lowerBound(const Node &node,
                                   unsigned char key) noexcept {
  const auto it = std::lower_bound(
      node.children.begin(), node.children.end(), key,
      [](const Child &c, unsigned char k) { return c.key < k; });
  return static_cast<std::size_t>(it - node.children.begin());
}

void SuffixTrie::insertChild(std::uint32_t parent, std::size_t idx,
                             unsigned char key, std::uint32_t child) {
  auto &kids = nodes_[parent].children;
  kids.insert(kids.begin() + static_cast<std::ptrdiff_t>(idx),
              Child{key, child});
}

void SuffixTrie::build() {
  const std::size_t n = text_.size();
  nodes_.reserve(2 * n + 1); // a suffix tree has at most 2n nodes
  nodes_.emplace_back();     // root

  // Active point of Ukkonen's algorithm.
  std::uint32_t activeNode = 0;
  std::size_t activeEdge =
      0; // index into text_ of the first character of the active edge
  std::size_t activeLen = 0;
  std::size_t remainder = 0; // suffixes still to be inserted explicitly

  for (std::size_t i = 0; i < n; ++i) {
    const auto ci = static_cast<unsigned char>(text_[i]);
    ++remainder;
    std::uint32_t lastNew = kNone; // internal node awaiting its suffix link

    while (remainder > 0) {
      if (activeLen == 0) {
        activeEdge = i;
      }
      const auto key = static_cast<unsigned char>(text_[activeEdge]);
      const std::size_t idx = lowerBound(nodes_[activeNode], key);
      const auto &kids = nodes_[activeNode].children;
      const bool found = idx < kids.size() && kids[idx].key == key;
      const std::uint32_t next = found ? kids[idx].node : kNone;

      if (!found) {
        // Rule 2: no edge for this character: add a leaf directly.
        const std::uint32_t leaf = newNode(i, kLeafEnd, i + 1 - remainder);
        insertChild(activeNode, idx, key, leaf);
        if (lastNew != kNone) {
          nodes_[lastNew].link = activeNode;
          lastNew = kNone;
        }
      } else {
        const std::uint32_t nextStart = nodes_[next].start;
        const std::size_t nextEnd =
            nodes_[next].end == kLeafEnd
                ? i + 1
                : static_cast<std::size_t>(nodes_[next].end);
        const std::size_t edgeLen = std::min(nextEnd, i + 1) - nextStart;

        if (activeLen >= edgeLen) { // walk down
          activeEdge += edgeLen;
          activeLen -= edgeLen;
          activeNode = next;
          continue;
        }

        if (static_cast<unsigned char>(text_[nextStart + activeLen]) == ci) {
          // Rule 3: the character is already on the edge; stop this phase.
          if (lastNew != kNone && activeNode != 0) {
            nodes_[lastNew].link = activeNode;
            lastNew = kNone;
          }
          ++activeLen;
          break;
        }

        // Rule 2 with a split in the middle of the edge.
        const std::uint32_t split =
            newNode(nextStart, detail::toU32(nextStart + activeLen), 0);
        nodes_[activeNode].children[idx].node = split;

        const std::uint32_t leaf = newNode(i, kLeafEnd, i + 1 - remainder);
        insertChild(split, 0, ci, leaf); // split has no children yet

        nodes_[next].start = detail::toU32(nextStart + activeLen);
        const auto nextKey =
            static_cast<unsigned char>(text_[nodes_[next].start]);
        insertChild(split, nextKey < ci ? 0 : 1, nextKey,
                    next); // keep children sorted

        if (lastNew != kNone) {
          nodes_[lastNew].link = split;
        }
        lastNew = split;
      }

      --remainder;
      if (activeNode == 0 && activeLen > 0) {
        --activeLen;
        activeEdge = i - remainder + 1;
      } else if (activeNode != 0) {
        activeNode = nodes_[activeNode].link; // root when no link was set
      }
    }
  }
}

std::uint32_t SuffixTrie::locate(std::string_view pattern) const noexcept {
  if (!detail::isAscii(pattern)) {
    return kNone; // would otherwise be able to match the sentinel
  }

  std::uint32_t cur = 0;
  std::size_t pos = 0;
  while (pos < pattern.size()) {
    const Node &node = nodes_[cur];
    const auto key = static_cast<unsigned char>(pattern[pos]);
    const std::size_t idx = lowerBound(node, key);
    if (idx == node.children.size() || node.children[idx].key != key) {
      return kNone;
    }

    const std::uint32_t childId = node.children[idx].node;
    const Node &child = nodes_[childId];
    const std::size_t end = child.end == kLeafEnd ? text_.size() : child.end;
    const std::size_t len = end - child.start;
    const std::size_t cmp = std::min(len, pattern.size() - pos);

    if (std::char_traits<char>::compare(text_.data() + child.start,
                                        pattern.data() + pos, cmp) != 0) {
      return kNone;
    }
    pos += cmp; // the match may end in the middle of an edge
    cur = childId;
  }
  return cur;
}

bool SuffixTrie::contains(std::string_view pattern) const noexcept {
  return locate(pattern) != kNone;
}

std::size_t SuffixTrie::count(std::string_view pattern) const {
  std::size_t total = 0;
  forEachOccurrence(pattern, [&total](std::size_t) { ++total; });
  return total;
}

std::optional<std::size_t>
SuffixTrie::findFirst(std::string_view pattern) const {
  std::optional<std::size_t> best;
  forEachOccurrence(pattern, [&best](std::size_t idx) {
    if (!best || idx < *best) {
      best = idx;
    }
  });
  return best;
}

std::vector<std::size_t> SuffixTrie::findAll(std::string_view pattern) const {
  std::vector<std::size_t> out;
  forEachOccurrence(pattern, [&out](std::size_t idx) { out.push_back(idx); });
  std::sort(out.begin(), out.end());
  return out;
}

} // namespace ds
