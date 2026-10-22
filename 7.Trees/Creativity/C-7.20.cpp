// File : C-7.20.cpp

/*
    C-7.20 Consider a variation of the linked data structure for binary trees
   where each node object has pointers to the node objects of the children but
   not to the node object of the parent. Describe an implementation of the
   functions of a binary tree with this data structure and analyze the time
   complexity for these functions.

   ─────────────────────────────────────────────────────────────────────────────
   Note: The complexity analysis is described in the comments for each function
   in the ParentlessBinaryTree.hpp file.
   ─────────────────────────────────────────────────────────────────────────────
*/

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "parentless_binary_tree.hpp"

namespace {

int g_failures = 0; ///< Number of failed checks so far.

/// Records a failure (with location) if @p cond is false.
#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ':' << __LINE__                                 \
                << ": CHECK failed: " #cond "\n";                              \
    }                                                                          \
  } while (false)

/// Checks that evaluating @p expr throws an exception of type @p ExType.
#define CHECK_THROWS_AS(expr, ExType)                                          \
  do {                                                                         \
    bool caught_ = false;                                                      \
    try {                                                                      \
      (void)(expr);                                                            \
    } catch (const ExType &) {                                                 \
      caught_ = true;                                                          \
    } catch (...) {                                                            \
    }                                                                          \
    CHECK(caught_);                                                            \
  } while (false)

using Tree = adt::ParentlessBinaryTree<int>;
using Pos = Tree::Position;
using Ints = std::vector<int>;

static_assert(!std::is_copy_constructible<Tree>::value,
              "tree must not be copyable");
static_assert(std::is_nothrow_move_constructible<Tree>::value,
              "tree must be movable");

/// Elements of @p ps, in order.
Ints values(const std::vector<Pos> &ps) {
  Ints v;
  v.reserve(ps.size());
  for (const Pos &p : ps)
    v.push_back(*p);
  return v;
}

/// Position holding element @p v (null if absent). Linear scan; test helper
/// only.
Pos find(const Tree &t, int v) {
  for (const Pos &p : t.positions()) {
    if (*p == v)
      return p;
  }
  return Pos{};
}

/**
 * Builds:
 *         1
 *        / \
 *       2   3
 *          / \
 *         4   5
 *            / \
 *           6   7
 */
Tree make_sample() {
  Tree t;
  t.addRoot(1);
  t.expandExternal(t.root(), 2, 3);
  const Pos n3 = t.root().right();
  t.expandExternal(n3, 4, 5);
  const Pos n5 = n3.right();
  t.expandExternal(n5, 6, 7);
  return t;
}

/// Structural invariants: proper tree, parent/child links and depths agree.
bool well_formed(const Tree &t) {
  const std::vector<Pos> ps = t.positions();
  if (ps.size() != t.size())
    return false;
  for (const Pos &p : ps) {
    if (p.left().isNull() != p.right().isNull())
      return false; // 0 or 2 children
    if (t.isRoot(p))
      continue;
    const Pos par = t.parent(p);
    if (par.isNull())
      return false;
    if (par.left() != p && par.right() != p)
      return false;
    if (t.depth(p) != t.depth(par) + 1)
      return false;
  }
  return true;
}

// ---------------------------------------------------------------- tests ---

void test_empty_tree() {
  const Tree t;
  CHECK(t.empty());
  CHECK(t.size() == 0);
  CHECK(t.root().isNull());
  CHECK(t.height() == 0);
  CHECK(t.positions().empty());
  CHECK(t.inorder().empty());
}

void test_constant_time_accessors() {
  Tree t;
  const Pos r = t.addRoot(10);
  CHECK(!t.empty());
  CHECK(t.size() == 1);
  CHECK(t.root() == r);
  CHECK(*r == 10);
  CHECK(r.isExternal());
  CHECK(t.isRoot(r));

  t.expandExternal(r, 20, 30);
  CHECK(t.size() == 3);
  CHECK(r.isInternal());
  CHECK(*r.left() == 20);
  CHECK(*r.right() == 30);
  CHECK(r.left().isExternal());
  CHECK(!t.isRoot(r.left()));

  *r.left() = 21; // element is mutable through a position
  CHECK(*r.left() == 21);
}

void test_parent_sibling_depth() {
  const Tree t = make_sample();
  CHECK(t.parent(t.root()).isNull());
  CHECK(*t.parent(find(t, 2)) == 1);
  CHECK(*t.parent(find(t, 6)) == 5);
  CHECK(*t.sibling(find(t, 2)) == 3);
  CHECK(*t.sibling(find(t, 4)) == 5);
  CHECK(*t.sibling(find(t, 6)) == 7);

  const std::size_t expected[] = {0, 1, 1, 2, 2, 3, 3};
  for (int v = 1; v <= 7; ++v) {
    CHECK(t.depth(find(t, v)) == expected[v - 1]);
  }
}

void test_traversals_and_height() {
  const Tree t = make_sample();
  CHECK(t.size() == 7);
  CHECK(t.height() == 3);
  CHECK(values(t.positions()) == (Ints{1, 2, 3, 4, 5, 6, 7})); // preorder
  CHECK(values(t.inorder()) == (Ints{2, 1, 4, 3, 6, 5, 7}));
}

void test_remove_above_external() {
  Tree t = make_sample();

  // Parent is the root: the sibling subtree becomes the new tree.
  Pos promoted = t.removeAboveExternal(find(t, 2));
  CHECK(*promoted == 3);
  CHECK(t.root() == promoted);
  CHECK(t.size() == 5);
  CHECK(values(t.positions()) == (Ints{3, 4, 5, 6, 7}));
  CHECK(t.depth(find(t, 6)) == 2); // everything moved up one level
  CHECK(well_formed(t));

  // Parent has a grandparent: the grandparent must be relinked.
  promoted = t.removeAboveExternal(find(t, 6));
  CHECK(*promoted == 7);
  CHECK(t.size() == 3);
  CHECK(values(t.positions()) == (Ints{3, 4, 7}));
  CHECK(t.parent(promoted) == t.root());
  CHECK(well_formed(t));

  // Down to a single node.
  t.removeAboveExternal(find(t, 4));
  CHECK(t.size() == 1);
  CHECK(*t.root() == 7);
  CHECK(t.root().isExternal());
}

void test_random_trees() {
  constexpr std::size_t kNone = static_cast<std::size_t>(-1);

  for (std::uint32_t seed = 1; seed <= 30; ++seed) {
    std::mt19937 rng(seed);
    Tree t;

    // Model: node i holds element i; par[i] / dep[i] are its parent index and
    // depth.
    std::vector<Pos> pos;
    std::vector<std::size_t> par, dep, leaves;
    pos.push_back(t.addRoot(0));
    par.push_back(kNone);
    dep.push_back(0);
    leaves.push_back(0);

    const std::size_t expansions = 1 + (seed * 13) % 60;
    for (std::size_t i = 0; i < expansions; ++i) {
      std::uniform_int_distribution<std::size_t> pick(0, leaves.size() - 1);
      const std::size_t k = pick(rng);
      const std::size_t idx = leaves[k];
      leaves[k] = leaves.back();
      leaves.pop_back();

      const int li = static_cast<int>(pos.size());
      t.expandExternal(pos[idx], li, li + 1);
      const Pos l = pos[idx].left();
      const Pos r = pos[idx].right();
      pos.push_back(l);
      par.push_back(idx);
      dep.push_back(dep[idx] + 1);
      pos.push_back(r);
      par.push_back(idx);
      dep.push_back(dep[idx] + 1);
      leaves.push_back(pos.size() - 2);
      leaves.push_back(pos.size() - 1);
    }

    bool ok = (t.size() == pos.size());
    for (std::size_t i = 0; i < pos.size(); ++i) {
      ok = ok && (*pos[i] == static_cast<int>(i));
      ok = ok && (t.depth(pos[i]) == dep[i]);
      if (par[i] == kNone)
        ok = ok && t.parent(pos[i]).isNull();
      else
        ok = ok && (t.parent(pos[i]) == pos[par[i]]);
    }
    CHECK(ok);
    CHECK(well_formed(t));

    // Remove random leaves (with their parents) until a single node is left.
    while (t.size() > 1) {
      std::vector<Pos> candidates;
      for (const Pos &p : t.positions()) {
        if (p.isExternal() && !t.isRoot(p))
          candidates.push_back(p);
      }
      std::uniform_int_distribution<std::size_t> pick(0, candidates.size() - 1);
      t.removeAboveExternal(candidates[pick(rng)]);
      if (!well_formed(t)) {
        CHECK(false);
        break;
      }
    }
    CHECK(t.size() == 1);
  }
}

void test_deep_path_no_stack_overflow() {
  constexpr std::size_t kDepth =
      100000; // a recursive implementation would overflow
  Tree t;
  Pos cur = t.addRoot(0);
  for (std::size_t i = 1; i <= kDepth; ++i) {
    t.expandExternal(cur, static_cast<int>(i), -1);
    cur = cur.left();
  }
  CHECK(t.size() == 2 * kDepth + 1);
  CHECK(t.height() == kDepth);
  CHECK(t.depth(cur) == kDepth); // a single Theta(n) search
  CHECK(t.positions().size() == t.size());
  CHECK(t.inorder().size() == t.size());
  // The destructor also runs on this degenerate tree when `t` leaves scope.
}

void test_move_semantics() {
  Tree a = make_sample();
  Tree b(std::move(a));
  CHECK(a.empty()); // moved-from trees are empty by design
  CHECK(b.size() == 7);

  Tree c;
  c.addRoot(99);
  c = std::move(b);
  CHECK(b.empty());
  CHECK(c.size() == 7);
  CHECK(values(c.positions()) == (Ints{1, 2, 3, 4, 5, 6, 7}));

  c.clear();
  CHECK(c.empty());
  CHECK(c.root().isNull());
}

void test_invalid_usage_throws() {
  Tree t;
  CHECK_THROWS_AS(t.expandExternal(Pos{}, 1, 2),
                  std::invalid_argument); // null position
  CHECK_THROWS_AS(t.parent(Pos{}), std::invalid_argument);

  const Pos r = t.addRoot(1);
  CHECK_THROWS_AS(t.addRoot(2), std::logic_error); // second root
  CHECK_THROWS_AS(t.sibling(r), std::logic_error); // root has no sibling
  CHECK_THROWS_AS(t.removeAboveExternal(r),
                  std::logic_error); // root has no parent

  t.expandExternal(r, 2, 3);
  CHECK_THROWS_AS(t.expandExternal(r, 4, 5),
                  std::logic_error); // already internal
  CHECK_THROWS_AS(t.removeAboveExternal(r), std::logic_error); // not external

  Tree other;
  const Pos foreign = other.addRoot(99);
  CHECK_THROWS_AS(t.depth(foreign), std::invalid_argument); // not in this tree
  CHECK_THROWS_AS(t.removeAboveExternal(foreign), std::invalid_argument);
}

struct TestCase {
  const char *name;
  void (*fn)();
};

} // namespace

int main() {
  const TestCase tests[] = {
      {"empty tree", test_empty_tree},
      {"O(1) accessors", test_constant_time_accessors},
      {"parent / sibling / depth", test_parent_sibling_depth},
      {"traversals and height", test_traversals_and_height},
      {"removeAboveExternal", test_remove_above_external},
      {"random trees vs. model", test_random_trees},
      {"deep path (no stack overflow)", test_deep_path_no_stack_overflow},
      {"move semantics", test_move_semantics},
      {"invalid usage throws", test_invalid_usage_throws},
  };

  int failed_tests = 0;
  for (const auto &tc : tests) {
    const int before = g_failures;
    tc.fn();
    const bool passed = (g_failures == before);
    std::cout << (passed ? "[ PASS ] " : "[ FAIL ] ") << tc.name << '\n';
    if (!passed)
      ++failed_tests;
  }

  std::cout << (failed_tests == 0 ? "\nAll tests passed.\n"
                                  : "\nSome tests FAILED.\n");
  return failed_tests == 0 ? 0 : 1;
}