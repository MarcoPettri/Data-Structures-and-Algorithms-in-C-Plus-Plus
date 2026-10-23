// File : C-8.13.cpp

/*
C-8.13 Suppose the binary tree T used to implement a heap can be accessed using
only the functions of the binary tree ADT. That is, we cannot assume T
is implemented as a vector. Given a pointer to the current last node, v,
describe an efficient algorithm for finding the insertion point (that is, the
new last node) using just the functions of the binary tree interface. Be
sure and handle all possible cases as illustrated in Figure 8.12. What is
the running time of this function?

─────────────────────────────────────────────────────────────────────────────
The complexity analysis is described in the comments for the function in
the C-8.23_binary_tree.hpp file.
─────────────────────────────────────────────────────────────────────────────
*/

#include "C-8.23_binary_tree.hpp"
#include <cassert>
#include <iostream>

// Helper function to create a node
template <typename T>
BinaryTreeNode<T> *createNode(const T &val,
                              BinaryTreeNode<T> *parent = nullptr) {
  auto *node = new BinaryTreeNode<T>{val, parent, nullptr, nullptr};
  return node;
}

// Helper to free tree memory recursively
template <typename T> void freeTree(BinaryTreeNode<T> *node) {
  if (!node)
    return;
  freeTree(node->left);
  freeTree(node->right);
  delete node;
}

void runTests() {
  std::cout << "Running complete binary tree insertion point tests...\n";

  // Test 1: Single node tree (Root only)
  {
    auto *root = createNode(1);
    auto *next = findNextInsertionPoint(root);
    // Next insertion point should be root's left child (which is currently
    // nullptr)
    assert(next == root->left);
    freeTree(root);
  }

  // Test 2: Tree with 2 nodes (Root and Left Child)
  //         1
  //       /
  //      2 (last)
  {
    auto *root = createNode(1);
    auto *leftChild = createNode(2, root);
    root->left = leftChild;

    auto *next = findNextInsertionPoint(leftChild);
    // Next insertion point should be root's right child
    assert(next == root->right);
    freeTree(root);
  }

  // Test 3: Tree with 3 nodes (Full tree of height 1)
  //         1
  //       /   \
  //      2     3 (last)
  {
    auto *root = createNode(1);
    auto *leftChild = createNode(2, root);
    auto *rightChild = createNode(3, root);
    root->left = leftChild;
    root->right = rightChild;

    auto *next = findNextInsertionPoint(rightChild);
    // Next insertion point should be left child of leftChild (Node 2's left
    // child) Since leftChild has no children yet, next should be
    // leftChild->left
    assert(next == leftChild->left);
    freeTree(root);
  }

  // Test 4: Tree with 4 nodes
  //         1
  //       /   \
  //      2     3
  //     /
  //    4 (last)
  {
    auto *root = createNode(1);
    auto *l = createNode(2, root);
    auto *r = createNode(3, root);
    auto *ll = createNode(4, l);
    root->left = l;
    root->right = r;
    l->left = ll;

    auto *next = findNextInsertionPoint(ll);
    // Next insertion point should be l->right (right child of node 2)
    assert(next == l->right);
    freeTree(root);
  }

  std::cout << "All complete binary tree insertion point tests passed "
               "successfully!\n";
}

int main() {
  runTests();
  return 0;
}