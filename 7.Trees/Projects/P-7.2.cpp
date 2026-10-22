// File : P-7.2.cpp

/*
    P-7.2 Give a fully generic implementation of the class LinkedBinaryTree
   using class templates and taking into account error conditions
*/
/**
 * @brief Unit tests for adt::LinkedBinaryTree.
 *
 */
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "linked_binary_tree.hpp"

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

using Tree = adt::LinkedBinaryTree<int>;
using Pos = Tree::Position;
using CPos = Tree::ConstPosition;
using Ints = std::vector<int>;
using adt::Traversal;

// Properties checked at compile time.
static_assert(std::is_nothrow_default_constructible<Tree>::value, "");
static_assert(std::is_nothrow_move_constructible<Tree>::value, "");
static_assert(std::is_nothrow_move_assignable<Tree>::value, "");
static_assert(std::is_nothrow_swappable<Tree>::value, "");
static_assert(std::is_convertible<Pos, CPos>::value,
              "a read/write position converts to a read-only one");
static_assert(!std::is_convertible<CPos, Pos>::value,
              "a read-only position must not convert to read/write");
static_assert(std::is_same<decltype(*std::declval<Pos>()), int &>::value, "");
static_assert(std::is_same<decltype(*std::declval<CPos>()), const int &>::value,
              "");

/// Elements of @p ps, in order.
template <typename PositionT> Ints values(const std::vector<PositionT> &ps) {
  Ints v;
  v.reserve(ps.size());
  for (const PositionT &p : ps) {
    v.push_back(*p);
  }
  return v;
}

/// Position holding @p value (null if absent). Linear scan; test helper only.
Pos find(Tree &t, int value) {
  for (const Pos &p : t.positions()) {
    if (*p == value) {
      return p;
    }
  }
  return Pos{};
}

/**
 * Builds:
 *          1
 *        /   \
 *       2     3
 *      / \   / \
 *     4   5 6   7
 *        / \
 *       8   9
 */
Tree makeSample() {
  Tree t;
  t.addRoot(1);
  t.expandExternal(t.root(), 2, 3);
  t.expandExternal(t.root().left(), 4, 5);
  t.expandExternal(t.root().right(), 6, 7);
  t.expandExternal(t.root().left().right(), 8, 9);
  return t;
}

/// Structural invariants, checked through the public interface only.
bool wellFormed(const Tree &t) {
  const std::vector<CPos> pre = t.positions();
  if (pre.size() != t.size() ||
      t.positions(Traversal::Inorder).size() != t.size() ||
      t.positions(Traversal::Postorder).size() != t.size() ||
      t.positions(Traversal::LevelOrder).size() != t.size()) {
    return false;
  }
  for (const CPos &p : pre) {
    if (!t.owns(p)) {
      return false;
    }
    if (p.isExternal() == p.isInternal()) {
      return false;
    }
    if (p.isRoot() != (p == t.root())) {
      return false;
    }
    if (p.isInternal()) {
      const CPos l = p.left();
      const CPos r = p.right();
      if (l.parent() != p || r.parent() != p) {
        return false;
      }
      if (l.sibling() != r || r.sibling() != l) {
        return false;
      }
      if (t.depth(l) != t.depth(p) + 1) {
        return false;
      }
    }
  }
  return true;
}

// ----------------------------------------------------------------- tests ---

void testEmptyTree() {
  Tree t;
  const Tree &ct = t;
  CHECK(t.empty());
  CHECK(t.size() == 0);
  CHECK(t.root().isNull());
  CHECK(ct.root().isNull());
  CHECK(t.positions().empty());
  CHECK(ct.positions(Traversal::LevelOrder).empty());
  CHECK(!t.owns(t.root()));
  CHECK_THROWS_AS(t.height(), adt::InvalidOperation);
  CHECK_THROWS_AS(*t.root(), adt::InvalidPosition);
  CHECK_THROWS_AS(t.root().left(), adt::InvalidPosition);
  CHECK_THROWS_AS(t.root().isRoot(), adt::InvalidPosition);
  CHECK_THROWS_AS(t.depth(t.root()), adt::InvalidPosition);
}

void testAddRootAndAccessors() {
  Tree t;
  const Pos r = t.addRoot(10);
  CHECK(!t.empty());
  CHECK(t.size() == 1);
  CHECK(t.root() == r);
  CHECK(*r == 10);
  CHECK(r.isRoot());
  CHECK(r.isExternal());
  CHECK(!r.isInternal());
  CHECK(t.owns(r));
  CHECK(t.depth(r) == 0);
  CHECK(t.height() == 0);

  *r = 11; // the element is mutable through a position
  CHECK(*t.root() == 11);

  t.expandExternal(r, 20, 30);
  CHECK(t.size() == 3);
  CHECK(r.isInternal());
  CHECK(*r.left() == 20);
  CHECK(*r.right() == 30);
  CHECK(r.left().parent() == r);
  CHECK(r.left().sibling() == r.right());
  CHECK(t.height() == 1);
  CHECK(wellFormed(t));
}

void testDefaultElements() {
  Tree t;
  const Pos r = t.addRoot(); // value-initialized elements
  t.expandExternal(r);
  CHECK(t.size() == 3);
  CHECK(*r == 0);
  CHECK(*r.left() == 0);
  CHECK(*r.right() == 0);
}

void testConstCorrectness() {
  Tree t = makeSample();
  const Tree &ct = t;
  const CPos cr = ct.root();
  CHECK(*cr == 1);
  const CPos converted = t.root(); // Position -> ConstPosition
  CHECK(converted == cr);
  CHECK(t.root() == cr); // mixed comparisons
  CHECK(cr == t.root());
  CHECK(ct.depth(cr.left()) == 1);
  CHECK(values(ct.positions()) == (Ints{1, 2, 4, 5, 8, 9, 3, 6, 7}));
}

void testBoundaryViolations() {
  Tree t = makeSample();
  const Pos root = t.root();
  const Pos leaf = find(t, 4);

  CHECK_THROWS_AS(root.parent(), adt::BoundaryViolation);
  CHECK_THROWS_AS(root.sibling(), adt::BoundaryViolation);
  CHECK_THROWS_AS(leaf.left(), adt::BoundaryViolation);
  CHECK_THROWS_AS(leaf.right(), adt::BoundaryViolation);

  CHECK(*leaf.parent() == 2);
  CHECK(*leaf.sibling() == 5);

  // The hierarchy can be caught at any level.
  CHECK_THROWS_AS(root.parent(), adt::TreeError);
  CHECK_THROWS_AS(root.parent(), std::logic_error);
}

void testTraversalsHeightAndDepth() {
  Tree t = makeSample();
  CHECK(t.size() == 9);
  CHECK(t.height() == 3);

  CHECK(values(t.positions()) == (Ints{1, 2, 4, 5, 8, 9, 3, 6, 7}));
  CHECK(values(t.positions(Traversal::Preorder)) ==
        (Ints{1, 2, 4, 5, 8, 9, 3, 6, 7}));
  CHECK(values(t.positions(Traversal::Inorder)) ==
        (Ints{4, 2, 8, 5, 9, 1, 6, 3, 7}));
  CHECK(values(t.positions(Traversal::Postorder)) ==
        (Ints{4, 8, 9, 5, 2, 6, 7, 3, 1}));
  CHECK(values(t.positions(Traversal::LevelOrder)) ==
        (Ints{1, 2, 3, 4, 5, 6, 7, 8, 9}));

  const std::size_t expectedDepth[] = {0, 1, 1, 2, 2, 2, 2, 3, 3};
  for (int v = 1; v <= 9; ++v) {
    CHECK(t.depth(find(t, v)) == expectedDepth[v - 1]);
  }
  CHECK(t.height(find(t, 2)) == 2);
  CHECK(t.height(find(t, 3)) == 1);
  CHECK(t.height(find(t, 9)) == 0);

  CHECK_THROWS_AS(t.positions(static_cast<Traversal>(99)),
                  std::invalid_argument);
}

void testRemoveAboveExternal() {
  Tree t = makeSample();

  // Leaf 4: its parent 2 is replaced by the sibling 5.
  Pos promoted = t.removeAboveExternal(find(t, 4));
  CHECK(*promoted == 5);
  CHECK(promoted.parent() == t.root());
  CHECK(t.root().left() == promoted);
  CHECK(t.size() == 7);
  CHECK(values(t.positions()) == (Ints{1, 5, 8, 9, 3, 6, 7}));
  CHECK(t.depth(find(t, 8)) == 2); // the subtree moved up one level
  CHECK(wellFormed(t));

  // Leaf 6: its parent 3 is replaced by 7.
  promoted = t.removeAboveExternal(find(t, 6));
  CHECK(*promoted == 7);
  CHECK(t.size() == 5);
  CHECK(values(t.positions()) == (Ints{1, 5, 8, 9, 7}));
  CHECK(wellFormed(t));

  // The parent is the root: the sibling subtree becomes the whole tree.
  promoted = t.removeAboveExternal(find(t, 7));
  CHECK(*promoted == 5);
  CHECK(t.root() == promoted);
  CHECK(t.size() == 3);
  CHECK(values(t.positions()) == (Ints{5, 8, 9}));
  CHECK_THROWS_AS(t.root().parent(), adt::BoundaryViolation);
  CHECK(wellFormed(t));

  // Down to a single node; the root has no parent to remove.
  promoted = t.removeAboveExternal(find(t, 8));
  CHECK(*promoted == 9);
  CHECK(t.size() == 1);
  CHECK(t.root().isExternal());
  CHECK_THROWS_AS(t.removeAboveExternal(t.root()), adt::BoundaryViolation);
  CHECK(t.size() == 1); // the failed call changed nothing
}

void testInvalidOperations() {
  Tree t;
  const Pos r = t.addRoot(1);
  CHECK_THROWS_AS(t.addRoot(2), adt::InvalidOperation);
  CHECK(t.size() == 1);

  CHECK_THROWS_AS(t.expandExternal(Pos{}, 1, 2), adt::InvalidPosition);
  CHECK_THROWS_AS(t.removeAboveExternal(Pos{}), adt::InvalidPosition);
  CHECK_THROWS_AS(t.depth(Pos{}), adt::InvalidPosition);

  t.expandExternal(r, 2, 3);
  CHECK_THROWS_AS(t.expandExternal(r, 4, 5), adt::InvalidOperation);
  CHECK_THROWS_AS(t.removeAboveExternal(r), adt::InvalidOperation);
  CHECK(t.size() == 3); // failed calls changed nothing
  CHECK(wellFormed(t));
}

void testForeignPositions() {
  Tree a = makeSample();
  Tree b = makeSample();
  const Pos pa = find(a, 4);
  const Pos pb = find(b, 4);

  CHECK(a.owns(pa));
  CHECK(!a.owns(pb));
  CHECK(!b.owns(pa));
  CHECK_THROWS_AS(a.expandExternal(pb, 1, 2), adt::InvalidPosition);
  CHECK_THROWS_AS(a.removeAboveExternal(pb), adt::InvalidPosition);
  CHECK_THROWS_AS(a.depth(pb), adt::InvalidPosition);
  CHECK_THROWS_AS(a.height(pb), adt::InvalidPosition);
  CHECK(a.size() == 9 && b.size() == 9);

  // A copy is a different tree: positions of the original do not transfer.
  Tree c(a);
  CHECK(!c.owns(pa));
  CHECK_THROWS_AS(c.depth(pa), adt::InvalidPosition);
  CHECK(c.depth(find(c, 4)) == 2);

  // Moving carries the identity with the nodes: positions stay valid.
  Tree d(std::move(a));
  CHECK(a.empty());
  CHECK(!a.owns(pa));
  CHECK(d.owns(pa));
  CHECK(d.depth(pa) == 2);
  CHECK_THROWS_AS(a.depth(pa), adt::InvalidPosition);

  // So does swapping.
  d.swap(b);
  CHECK(b.owns(pa));
  CHECK(d.owns(pb));
  CHECK(!d.owns(pa));

  // A moved-from tree is reusable.
  const Pos fresh = a.addRoot(42);
  CHECK(a.owns(fresh));
  CHECK(!a.owns(pa));
  CHECK(a.size() == 1);
}

void testCopySemantics() {
  Tree a = makeSample();
  Tree b(a);
  CHECK(b.size() == 9);
  CHECK(values(b.positions()) == values(a.positions()));
  CHECK(values(b.positions(Traversal::Inorder)) ==
        values(a.positions(Traversal::Inorder)));

  *find(b, 1) = 100; // the copy is deep
  CHECK(*a.root() == 1);
  CHECK(*b.root() == 100);
  b.removeAboveExternal(find(b, 4));
  CHECK(a.size() == 9);
  CHECK(b.size() == 7);
  CHECK(wellFormed(a));
  CHECK(wellFormed(b));

  Tree c;
  c.addRoot(7);
  c = a; // assignment replaces the old contents
  CHECK(c.size() == 9);
  CHECK(values(c.positions()) == values(a.positions()));

  Tree &alias = c;
  c = alias; // self-assignment is harmless
  CHECK(c.size() == 9);
  CHECK(wellFormed(c));

  const Tree emptyTree;
  c = emptyTree;
  CHECK(c.empty());
  const Tree e(emptyTree);
  CHECK(e.empty());
}

void testMoveAndSwap() {
  Tree a = makeSample();
  Tree b;
  b.addRoot(99);
  const Pos pa = a.root();

  Tree c(std::move(a));
  CHECK(a.empty());
  CHECK(a.root().isNull());
  CHECK(c.size() == 9);
  CHECK(c.root() == pa); // the nodes themselves were not touched

  b = std::move(c); // replaces b's old content
  CHECK(c.empty());
  CHECK(b.size() == 9);
  CHECK(values(b.positions()) == (Ints{1, 2, 4, 5, 8, 9, 3, 6, 7}));

  swap(a, b); // found through ADL
  CHECK(b.empty());
  CHECK(a.size() == 9);
  a.swap(b);
  CHECK(a.empty());
  CHECK(b.size() == 9);

  b.clear();
  CHECK(b.empty());
  CHECK(b.root().isNull());
  b.addRoot(5); // reusable after clear()
  CHECK(b.size() == 1);
}

void testDeepTreeNoStackOverflow() {
  constexpr std::size_t kDepth = 100000; // a recursive version would overflow
  Tree t;
  Pos cur = t.addRoot(0);
  for (std::size_t i = 1; i <= kDepth; ++i) {
    t.expandExternal(cur, static_cast<int>(i), -1);
    cur = cur.left();
  }
  CHECK(t.size() == 2 * kDepth + 1);
  CHECK(t.height() == kDepth);
  CHECK(t.depth(cur) == kDepth);
  CHECK(t.positions(Traversal::Preorder).size() == t.size());
  CHECK(t.positions(Traversal::Inorder).size() == t.size());
  CHECK(t.positions(Traversal::Postorder).size() == t.size());
  CHECK(t.positions(Traversal::LevelOrder).size() == t.size());

  const Tree copy(t);
  CHECK(copy.size() == t.size());
  CHECK(copy.height() == kDepth);
  // Both deep trees are also destroyed iteratively when they leave scope.
}

// -------------------------------------- non-trivial element types ---------

/// Not default-constructible; counts live instances; its copy constructor
/// can be told to throw after a given number of successful copies.
struct Probe {
  static int live;
  static int copiesBeforeThrow; ///< -1: never throw

  int value;

  explicit Probe(int v) : value(v) { ++live; }
  Probe(const Probe &other) : value(other.value) {
    if (copiesBeforeThrow == 0) {
      throw std::runtime_error("Probe: copy failed");
    }
    if (copiesBeforeThrow > 0) {
      --copiesBeforeThrow;
    }
    ++live;
  }
  Probe(Probe &&other) noexcept : value(other.value) { ++live; }
  Probe &operator=(const Probe &) = default;
  ~Probe() { --live; }
};

int Probe::live = 0;
int Probe::copiesBeforeThrow = -1;

using ProbeTree = adt::LinkedBinaryTree<Probe>;

/// Seven nodes: 0 with children 1, 2; 1 with children 3, 4; 2 with children
/// 5, 6.
ProbeTree makeProbeTree() {
  ProbeTree t;
  t.addRoot(Probe(0));
  t.expandExternal(t.root(), Probe(1), Probe(2));
  t.expandExternal(t.root().left(), Probe(3), Probe(4));
  t.expandExternal(t.root().right(), Probe(5), Probe(6));
  return t;
}

void testCopyConstructorIsExceptionSafe() {
  const int baseline = Probe::live;
  {
    ProbeTree t = makeProbeTree();
    CHECK(Probe::live == baseline + 7);

    for (int failAt = 0; failAt < 7; ++failAt) {
      Probe::copiesBeforeThrow = failAt;
      bool threw = false;
      try {
        ProbeTree copy(t);
      } catch (const std::runtime_error &) {
        threw = true;
      }
      Probe::copiesBeforeThrow = -1;
      CHECK(threw);
      CHECK(Probe::live == baseline + 7); // the partial copy was released
      CHECK(t.size() == 7);
    }

    const ProbeTree ok(t); // copying works when nothing throws
    CHECK(ok.size() == 7);
    CHECK(Probe::live == baseline + 14);
  }
  CHECK(Probe::live == baseline);
}

void testCopyAssignmentIsExceptionSafe() {
  const int baseline = Probe::live;
  {
    ProbeTree source = makeProbeTree();
    ProbeTree target;
    target.addRoot(Probe(100));
    target.expandExternal(target.root(), Probe(101), Probe(102));

    Probe::copiesBeforeThrow = 2;
    bool threw = false;
    try {
      target = source;
    } catch (const std::runtime_error &) {
      threw = true;
    }
    Probe::copiesBeforeThrow = -1;

    CHECK(threw);
    CHECK(target.size() == 3); // strong guarantee: target is unchanged
    CHECK(target.root()->value == 100);
    CHECK(Probe::live == baseline + 7 + 3); // nothing leaked, nothing lost

    target = source; // and it works when nothing throws
    CHECK(target.size() == 7);
    CHECK(Probe::live == baseline + 14);
  }
  CHECK(Probe::live == baseline);
}

void testMoveOnlyElements() {
  using UniqueTree = adt::LinkedBinaryTree<std::unique_ptr<int>>;
  UniqueTree t;
  t.addRoot(std::make_unique<int>(1));
  t.expandExternal(t.root(), std::make_unique<int>(2),
                   std::make_unique<int>(3));
  CHECK(t.size() == 3);
  CHECK(**t.root() == 1);
  CHECK(**t.root().left() == 2);

  UniqueTree moved(std::move(t));
  CHECK(**moved.root().right() == 3);

  moved.removeAboveExternal(moved.root().left()); // frees two nodes
  CHECK(moved.size() == 1);
  CHECK(**moved.root() == 3);
}

// ------------------------------------------------- randomized test ---------

/// True if the tree has exactly the nodes and parent links of @p parentOf
/// (element -> parent element, -1 for the root).
bool matchesModel(const Tree &t, const std::map<int, int> &parentOf) {
  const std::vector<CPos> ps = t.positions();
  if (ps.size() != parentOf.size()) {
    return false;
  }
  for (const CPos &p : ps) {
    const auto it = parentOf.find(*p);
    if (it == parentOf.end()) {
      return false;
    }
    if (p.isRoot()) {
      if (it->second != -1) {
        return false;
      }
    } else if (it->second != *p.parent()) {
      return false;
    }
  }
  return true;
}

void testRandomOperationsAgainstModel() {
  for (std::uint32_t seed = 1; seed <= 30; ++seed) {
    std::mt19937 rng(seed);
    Tree t;
    std::map<int, int> parentOf; // element -> parent element (-1: root)
    int next = 0;
    t.addRoot(next);
    parentOf[next++] = -1;

    bool ok = true;
    for (int step = 0; step < 150 && ok; ++step) {
      std::vector<Pos> leaves;
      for (const Pos &p : t.positions()) {
        if (p.isExternal()) {
          leaves.push_back(p);
        }
      }
      const bool expand = t.size() == 1 || rng() % 3 != 0;

      if (expand) {
        const Pos leaf = leaves[rng() % leaves.size()];
        const int parentElt = *leaf;
        const int a = next++;
        const int b = next++;
        t.expandExternal(leaf, a, b);
        parentOf[a] = parentElt;
        parentOf[b] = parentElt;
      } else {
        std::vector<Pos> removable;
        for (const Pos &p : leaves) {
          if (!p.isRoot()) {
            removable.push_back(p);
          }
        }
        const Pos leaf = removable[rng() % removable.size()];
        const int leafElt = *leaf;
        const int parentElt = *leaf.parent();
        const int siblingElt = *leaf.sibling();
        const int grandElt = parentOf[parentElt];

        const Pos promoted = t.removeAboveExternal(leaf);
        ok = ok && (*promoted == siblingElt);
        parentOf.erase(leafElt);
        parentOf.erase(parentElt);
        parentOf[siblingElt] = grandElt;
      }
      ok = ok && (t.size() == parentOf.size()) && matchesModel(t, parentOf) &&
           wellFormed(t);
    }
    CHECK(ok);
  }
}

struct TestCase {
  const char *name;
  void (*fn)();
};

} // namespace

int main() {
  const TestCase tests[] = {
      {"empty tree", testEmptyTree},
      {"addRoot and accessors", testAddRootAndAccessors},
      {"default elements", testDefaultElements},
      {"const correctness", testConstCorrectness},
      {"boundary violations", testBoundaryViolations},
      {"traversals, height and depth", testTraversalsHeightAndDepth},
      {"removeAboveExternal", testRemoveAboveExternal},
      {"invalid operations", testInvalidOperations},
      {"foreign positions", testForeignPositions},
      {"copy semantics", testCopySemantics},
      {"move and swap", testMoveAndSwap},
      {"deep tree (no stack overflow)", testDeepTreeNoStackOverflow},
      {"copy constructor exception safety", testCopyConstructorIsExceptionSafe},
      {"copy assignment exception safety", testCopyAssignmentIsExceptionSafe},
      {"move-only elements", testMoveOnlyElements},
      {"random operations vs. model", testRandomOperationsAgainstModel},
  };

  int failedTests = 0;
  for (const auto &tc : tests) {
    const int before = g_failures;
    tc.fn();
    const bool passed = (g_failures == before);
    std::cout << (passed ? "[ PASS ] " : "[ FAIL ] ") << tc.name << '\n';
    if (!passed) {
      ++failedTests;
    }
  }

  std::cout << (failedTests == 0 ? "\nAll tests passed.\n"
                                 : "\nSome tests FAILED.\n");
  return failedTests == 0 ? 0 : 1;
}