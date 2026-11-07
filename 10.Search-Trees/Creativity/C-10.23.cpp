// File : C-10.23.cpp

/*
    C-10.23 The standard splaying step requires two passes, one downward pass to
    find the node x to splay, followed by an upward pass to splay the node
    x. Describe a method for splaying and searching for x in one
    downward pass. Each substep now requires that you consider the next two nodes
    in the path down to x, with a possible zig substep performed at the end.
    Describe how to perform the zig-zig, zig-zag, and zig steps.

*/

#include "top_down_splay_tree.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Helper: in-order traversal to verify BST order
// ─────────────────────────────────────────────────────────────────────────────

template <typename Key, typename Value>
void inOrder(typename TopDownSplayTree<Key, Value>::Node *node,
             std::vector<Key> &out) {
    if (!node) return;
    inOrder<Key, Value>(node->left,  out);
    out.push_back(node->key);
    inOrder<Key, Value>(node->right, out);
}

template <typename Key, typename Value>
bool isBST(TopDownSplayTree<Key, Value> &tree) {
    std::vector<Key> keys;
    inOrder<Key, Value>(tree.root(), keys);
    for (std::size_t i = 1; i < keys.size(); ++i)
        if (!(keys[i - 1] < keys[i])) return false;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite
// ─────────────────────────────────────────────────────────────────────────────

static void test_basic_insert_find() {
    std::cout << "[Test] Basic insert & find\n";
    TopDownSplayTree<int, std::string> tree;

    tree.insert(10, "ten");
    tree.insert(20, "twenty");
    tree.insert(5,  "five");
    tree.insert(15, "fifteen");

    assert(tree.size() == 4);
    assert(tree.find(10)  != nullptr && *tree.find(10)  == "ten");
    assert(tree.find(20)  != nullptr && *tree.find(20)  == "twenty");
    assert(tree.find(5)   != nullptr && *tree.find(5)   == "five");
    assert(tree.find(15)  != nullptr && *tree.find(15)  == "fifteen");
    assert(tree.find(99)  == nullptr);  // not present

    // After every find the accessed node must be at the root (splay property).
    tree.find(5);
    assert(tree.root() && tree.root()->key == 5);

    tree.find(20);
    assert(tree.root() && tree.root()->key == 20);

    std::cout << "  PASSED\n";
}

static void test_update_existing_key() {
    std::cout << "[Test] Update existing key via insert\n";
    TopDownSplayTree<int, std::string> tree;

    tree.insert(7, "seven");
    tree.insert(7, "SEVEN");   // should update, not create a new node

    assert(tree.size() == 1);
    assert(tree.find(7) != nullptr && *tree.find(7) == "SEVEN");

    std::cout << "  PASSED\n";
}

static void test_erase() {
    std::cout << "[Test] Erase\n";
    TopDownSplayTree<int, std::string> tree;

    for (int k : {30, 10, 50, 5, 20, 40, 60})
        tree.insert(k, std::to_string(k));

    assert(tree.size() == 7);

    // Erase a leaf
    assert(tree.erase(5));
    assert(tree.size() == 6);
    assert(tree.find(5) == nullptr);

    // Erase an internal node
    assert(tree.erase(10));
    assert(tree.size() == 5);
    assert(tree.find(10) == nullptr);

    // Erase root
    tree.find(30); // splay 30 to root
    assert(tree.erase(30));
    assert(tree.size() == 4);
    assert(tree.find(30) == nullptr);

    // Erase non-existent key
    assert(!tree.erase(999));
    assert(tree.size() == 4);

    std::cout << "  PASSED\n";
}

static void test_bst_invariant() {
    std::cout << "[Test] BST invariant maintained after mixed operations\n";
    TopDownSplayTree<int, std::string> tree;

    std::vector<int> keys = {15, 10, 20, 8, 12, 18, 25, 3, 9, 11, 13};
    for (int k : keys)
        tree.insert(k, std::to_string(k));

    assert(isBST<int, std::string>(tree));

    // A series of finds (each splays)
    for (int k : {8, 18, 11, 25, 3})
        tree.find(k);

    assert(isBST<int, std::string>(tree));

    // A series of erases
    for (int k : {10, 20, 15})
        tree.erase(k);

    assert(isBST<int, std::string>(tree));

    std::cout << "  PASSED\n";
}

static void test_zig_zig_sequence() {
    std::cout << "[Test] Zig-zig triggered by sequential insertions\n";
    // Inserting 1..7 in order builds a right-leaning chain.
    // find(1) must zig-zig its way to the root in one pass.
    TopDownSplayTree<int, int> tree;
    for (int k = 1; k <= 7; ++k)
        tree.insert(k, k * 10);

    tree.find(1); // triggers repeated zig-zig lefts
    assert(tree.root() && tree.root()->key == 1);
    assert(isBST<int, int>(tree));

    std::cout << "  PASSED\n";
}

static void test_zig_zag_sequence() {
    std::cout << "[Test] Zig-zag triggered by alternating insertions\n";
    // Insert in a pattern that forces zig-zag: 10, 5, 8, 3, 7
    TopDownSplayTree<int, int> tree;
    for (int k : {10, 5, 8, 3, 7})
        tree.insert(k, k);

    tree.find(7); // likely zig-zag path
    assert(tree.root() && tree.root()->key == 7);
    assert(isBST<int, int>(tree));

    std::cout << "  PASSED\n";
}

static void test_move_semantics() {
    std::cout << "[Test] Move constructor and move assignment\n";
    TopDownSplayTree<int, int> tree;
    tree.insert(1, 100);
    tree.insert(2, 200);

    TopDownSplayTree<int, int> moved(std::move(tree));
    assert(tree.empty());
    assert(moved.size() == 2);
    assert(*moved.find(1) == 100);
    assert(*moved.find(2) == 200);

    TopDownSplayTree<int, int> assigned;
    assigned = std::move(moved);
    assert(moved.empty());
    assert(assigned.size() == 2);

    std::cout << "  PASSED\n";
}

static void test_single_element() {
    std::cout << "[Test] Single-element edge cases\n";
    TopDownSplayTree<int, int> tree;

    tree.insert(42, 1);
    assert(tree.size() == 1);
    assert(!tree.empty());
    assert(tree.find(42) != nullptr);
    assert(tree.find(0)  == nullptr);

    assert(tree.erase(42));
    assert(tree.empty());
    assert(tree.find(42) == nullptr);

    std::cout << "  PASSED\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== C-10.23: Top-Down Single-Pass Splay Tree ===\n\n";

    test_basic_insert_find();
    test_update_existing_key();
    test_erase();
    test_bst_invariant();
    test_zig_zig_sequence();
    test_zig_zag_sequence();
    test_move_semantics();
    test_single_element();

    std::cout << "\nAll tests passed!\n";
    return 0;
}
