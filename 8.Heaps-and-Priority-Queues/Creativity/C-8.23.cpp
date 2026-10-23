// File : C-8.23.cpp

/*
    C-8.23 Let S be a set of n points in the plane with distinct integer x- and
   y- coordinates. Let T be a complete binary tree storing the points from S at
   its external nodes, such that the points are ordered left-to-right by in-
    creasing x-coordinates. For each node v in T , let S(v) denote the subset of
    S consisting of points stored in the subtree rooted at v. For the root r of
    T , define top(r) to be the point in S = S(r) with maximum y-coordinate.
    For every other node v, define top(r) to be the point in S with highest y-
    coordinate in S(v) that is not also the highest y-coordinate in S(u), where
    u is the parent of v in T (if such a point exists). Such labeling turns T
   into a priority search tree. Describe a linear-time algorithm for turning T
   into a priority search tree.
*/

#include "priority_search_tree.hpp"
#include <cassert>
#include <iostream>
#include <vector>

// Helper function to build a complete binary tree from sorted points
PSTNode *buildCompleteBinaryTree(const std::vector<Point> &points, size_t start,
                                 size_t end) {
  PSTNode *node = new PSTNode();
  if (start == end) {
    node->m1 = points[start];
    node->has_m1 = true;
    return node;
  }
  size_t mid = start + (end - start) / 2;
  node->left = buildCompleteBinaryTree(points, start, mid);
  node->right = buildCompleteBinaryTree(points, mid + 1, end);
  return node;
}

// Helper to free allocated tree memory
void freeTree(PSTNode *node) {
  if (!node)
    return;
  freeTree(node->left);
  freeTree(node->right);
  delete node;
}

// Helper to collect all assigned top points
void collectTopPoints(PSTNode *node, std::vector<Point> &tops) {
  if (!node)
    return;
  if (node->has_top) {
    tops.push_back(node->top);
  }
  collectTopPoints(node->left, tops);
  collectTopPoints(node->right, tops);
}

void runTests() {
  std::cout << "Running Priority Search Tree conversion tests...\n";

  // Set of points with distinct x and y coordinates, ordered left-to-right by x
  std::vector<Point> points = {{1, 2}, {3, 8}, {5, 1}, {7, 6}};

  // Step 1: Construct complete binary tree
  PSTNode *root = buildCompleteBinaryTree(points, 0, points.size() - 1);

  // Step 2: Convert to Priority Search Tree in O(n) time
  convertToPrioritySearchTree(root);

  // Verify root top point has maximum y among all points ({3, 8})
  assert(root->has_top);
  assert(root->top.x == 3 && root->top.y == 8);
  std::cout << "Root top check passed: (" << root->top.x << ", " << root->top.y
            << ")\n";

  // Collect all assigned top points across the 2n - 1 nodes (7 nodes total)
  std::vector<Point> assignedTops;
  collectTopPoints(root, assignedTops);
  assert(assignedTops.size() == 7);

  std::cout
      << "All Priority Search Tree conversion tests passed successfully!\n";

  // Clean up dynamic memory
  freeTree(root);
}

int main() {
  runTests();
  return 0;
}