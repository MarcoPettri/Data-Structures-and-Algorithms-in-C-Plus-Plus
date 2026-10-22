// File : C-7.2.cpp

/*
   C-7.2 Describe an efficient algorithm for converting a fully balanced string
    of parentheses into an equivalent tree. The tree associated with such a
    string is defined recursively. The outer-most pair of balanced parentheses
    is associated with the root and each substring inside this pair, defined by
    the substring between two balanced parentheses, is associated with a subtree
    of this root.

   ─────────────────────────────────────────────────────────────────────────────
   Solution
   ─────────────────────────────────────────────────────────────────────────────

   Algorithm  (Single-pass, iterative stack):

   A fully balanced parenthesis string is scanned left-to-right.  An explicit
   stack keeps track of *open* nodes (nodes whose matching ')' has not yet been
   seen).

     For each character:
       '('  → Create a new node, push it onto the stack.
       ')'  → Pop the top node (it is now complete).
              If the stack is non-empty, the popped node becomes the right-most
              child of the new stack top.
              If the stack is empty, the popped node is the root → return it.

   Complexity Analysis:
     * Running time : O(n) — each character triggers O(1) work.
     * Space usage  : O(h) auxiliary (stack depth = nesting depth of the
                      string), plus O(n) for the output tree itself.

*/

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "paren_tree.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Minimal test harness
// ─────────────────────────────────────────────────────────────────────────────

namespace {

int failures = 0;

/// @cond INTERNAL
#define CHECK_EQ(actual, expected)                                             \
  do {                                                                         \
    const auto a_ = (actual);                                                  \
    const auto e_ = (expected);                                                \
    if (a_ != e_) {                                                            \
      ++failures;                                                              \
      std::cerr << "FAIL line " << __LINE__ << ": " #actual " = " << a_        \
                << ", expected " << e_ << '\n';                                \
    }                                                                          \
  } while (0)

#define CHECK_THROWS(expr)                                                     \
  do {                                                                         \
    bool threw_ = false;                                                       \
    try {                                                                      \
      (void)(expr);                                                            \
    } catch (const std::exception &) {                                         \
      threw_ = true;                                                           \
    }                                                                          \
    if (!threw_) {                                                             \
      ++failures;                                                              \
      std::cerr << "FAIL line " << __LINE__                                    \
                << ": expected exception for: " #expr "\n";                    \
    }                                                                          \
  } while (0)
/// @endcond

// ─────────────────────────────────────────────────────────────────────────────
// Individual test cases
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Empty string produces a nullptr root. */
void testEmptyString() {
  auto root = c72::parseParenTree("");
  CHECK_EQ(root == nullptr, true);
}

/** @brief "()" → single node with no children. */
void testSingleNode() {
  auto root = c72::parseParenTree("()");
  CHECK_EQ(root != nullptr, true);
  CHECK_EQ(root->children.size(), std::size_t{0});
  CHECK_EQ(c72::countNodes(root.get()), std::size_t{1});
  CHECK_EQ(c72::height(root.get()), 0);
}

/** @brief "(()())" → root with exactly two leaf children. */
void testTwoChildren() {
  auto root = c72::parseParenTree("(()())");
  CHECK_EQ(root != nullptr, true);
  CHECK_EQ(root->children.size(), std::size_t{2});
  CHECK_EQ(c72::countNodes(root.get()), std::size_t{3});
  CHECK_EQ(c72::height(root.get()), 1);
  // Both children must be leaves.
  CHECK_EQ(root->children[0]->children.empty(), true);
  CHECK_EQ(root->children[1]->children.empty(), true);
}

/** @brief "((())())" → root; left child has one child; right child is a leaf.
 */
void testAsymmetricTree() {
  //   root
  //   /  \.
  //  A    B
  // /
  // C
  auto root = c72::parseParenTree("((())())");
  CHECK_EQ(root != nullptr, true);
  CHECK_EQ(root->children.size(), std::size_t{2});
  CHECK_EQ(c72::countNodes(root.get()), std::size_t{4});
  CHECK_EQ(c72::height(root.get()), 2);

  const auto &A = root->children[0];
  const auto &B = root->children[1];
  CHECK_EQ(A->children.size(), std::size_t{1});     // A has child C
  CHECK_EQ(B->children.size(), std::size_t{0});     // B is a leaf
  CHECK_EQ(A->children[0]->children.empty(), true); // C is a leaf
}

/** @brief "(((())))" → linear chain depth 3. */
void testLinearChain() {
  auto root = c72::parseParenTree("(((())))");
  CHECK_EQ(root != nullptr, true);
  CHECK_EQ(c72::countNodes(root.get()), std::size_t{4});
  CHECK_EQ(c72::height(root.get()), 3);
  // Each node has exactly one child except the deepest.
  const c72::Node *cur = root.get();
  for (int depth = 0; depth < 3; ++depth) {
    CHECK_EQ(cur->children.size(), std::size_t{1});
    cur = cur->children[0].get();
  }
  CHECK_EQ(cur->children.empty(), true);
}

/** @brief Three siblings: "(()()())". */
void testThreeChildren() {
  auto root = c72::parseParenTree("(()()())");
  CHECK_EQ(root != nullptr, true);
  CHECK_EQ(root->children.size(), std::size_t{3});
  CHECK_EQ(c72::countNodes(root.get()), std::size_t{4});
  CHECK_EQ(c72::height(root.get()), 1);
}

/**
 * @brief Complex balanced string:
 *        "((()())(()))"
 *
 *   root
 *   /  \.
 *  A    B
 * / \.   \.
 *C   D    E
 */
void testComplexTree() {
  auto root = c72::parseParenTree("((()())(()))");
  CHECK_EQ(root != nullptr, true);
  CHECK_EQ(root->children.size(), std::size_t{2});
  CHECK_EQ(c72::countNodes(root.get()), std::size_t{6});
  CHECK_EQ(c72::height(root.get()), 2);

  const auto &A = root->children[0];
  const auto &B = root->children[1];
  CHECK_EQ(A->children.size(), std::size_t{2});
  CHECK_EQ(B->children.size(), std::size_t{1});
}

/** @brief DFS label order is correct (labels assigned 1..n in pre-order). */
void testLabelOrder() {
  //  "(()())"  pre-order: root=1, leftChild=2, rightChild=3
  auto root = c72::parseParenTree("(()())");
  CHECK_EQ(root->label, 1);
  CHECK_EQ(root->children[0]->label, 2);
  CHECK_EQ(root->children[1]->label, 3);
}

/** @brief Whitespace characters in the string are silently ignored. */
void testWhitespaceIgnored() {
  auto root = c72::parseParenTree("( () () )");
  CHECK_EQ(root != nullptr, true);
  CHECK_EQ(root->children.size(), std::size_t{2});
}

/** @brief Unmatched ')' must throw std::invalid_argument. */
void testUnmatchedClose() { CHECK_THROWS(c72::parseParenTree(")")); }

/** @brief Unmatched '(' must throw std::invalid_argument. */
void testUnmatchedOpen() { CHECK_THROWS(c72::parseParenTree("(")); }

/**
 * @brief Deep nesting (10 000 levels) must not overflow the call stack.
 *        Verifies the iterative implementation.
 */
void testDeepNesting() {
  constexpr int depth = 10'000;
  std::string s(depth, '(');
  s.append(depth, ')');
  auto root = c72::parseParenTree(s);
  CHECK_EQ(c72::countNodes(root.get()), std::size_t(depth));
  CHECK_EQ(c72::height(root.get()), depth - 1);
}

/** @brief Single-level wide tree with 1 000 siblings. */
void testWideTree() {
  constexpr int width = 1'000;
  std::string s = "(";
  for (int i = 0; i < width; ++i)
    s += "()";
  s += ')';
  auto root = c72::parseParenTree(s);
  CHECK_EQ(root->children.size(), std::size_t(width));
  CHECK_EQ(c72::countNodes(root.get()), std::size_t(width + 1));
  CHECK_EQ(c72::height(root.get()), 1);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────

int main() {
  testEmptyString();
  testSingleNode();
  testTwoChildren();
  testAsymmetricTree();
  testLinearChain();
  testThreeChildren();
  testComplexTree();
  testLabelOrder();
  testWhitespaceIgnored();
  testUnmatchedClose();
  testUnmatchedOpen();
  testDeepNesting();
  testWideTree();

  if (failures == 0) {
    std::cout << "All tests passed.\n";
    return EXIT_SUCCESS;
  }
  std::cerr << failures << " check(s) failed.\n";
  return EXIT_FAILURE;
}
