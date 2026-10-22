// File : P-7.1.cpp

/*
    P-7.1 Write a program that takes as input a rooted tree T and a node v of T
   and converts T to another tree with the same set of node adjacencies but now
    rooted at v.
*/

/**
 * @file  P-7.1.cpp
 * @brief Command-line demo: re-root a small tree at a chosen node.
 *
 * Usage: reroot_demo [node-id]   (node-id in 0..7, default 6)
 */
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "../Creativity/general_tree.hpp"
#include "reroot.hpp"

namespace {

using Tree = adt::GeneralTree<int>;
using Pos = Tree::Position;

/// Prints @p tree as root(child child(...) ...), iteratively.
void print(const Tree &tree) {
  struct Frame {
    Pos node;
    std::size_t next;
  };

  std::vector<Frame> stack;
  if (!tree.isEmpty()) {
    std::cout << tree.root()->element();
    stack.push_back(Frame{tree.root(), 0});
  }
  while (!stack.empty()) {
    Frame &top = stack.back();
    const auto &kids = tree.children(top.node);
    if (top.next < kids.size()) {
      std::cout << (top.next == 0 ? "(" : " ");
      const Pos child = kids[top.next++].get();
      std::cout << child->element();
      stack.push_back(Frame{child, 0});
    } else {
      if (!kids.empty()) {
        std::cout << ')';
      }
      stack.pop_back();
    }
  }
}

} // namespace

int main(int argc, char **argv) {
  // Node i holds element i; parentOf[i] is the index of its parent.
  const std::vector<int> parentOf = {-1, 0, 0, 0, 1, 1, 3, 6};

  Tree tree;
  std::vector<Pos> pos;
  for (std::size_t i = 0; i < parentOf.size(); ++i) {
    pos.push_back(parentOf[i] < 0
                      ? tree.insertRoot(0)
                      : tree.addChild(pos[parentOf[i]], static_cast<int>(i)));
  }

  std::size_t v = 6;
  if (argc > 1) {
    v = std::strtoul(argv[1], nullptr, 10);
  }
  if (v >= pos.size()) {
    std::cerr << "node id must be in 0.." << pos.size() - 1 << '\n';
    return 1;
  }

  std::cout << "original:      ";
  print(tree);
  std::cout << '\n';

  const Tree rerooted = adt::rerootAt(tree, pos[v]);
  std::cout << "rerooted at " << v << ": ";
  print(rerooted);
  std::cout << '\n';
  return 0;
}