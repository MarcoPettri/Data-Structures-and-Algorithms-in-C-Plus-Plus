/**
 * @file top_down_splay_tree.hpp
 * @brief Top-down splay tree implementing search and splaying in a single
 *        downward pass, as described in C-10.23.
 *
 * @details
 * The standard (bottom-up) splay tree requires two passes:
 *   1. A downward search pass to locate the target node.
 *   2. An upward pass to splay (rotate) the node to the root.
 *
 * ### Algorithm Overview
 * Three sub-trees are maintained during the descent:
 *   - **L** (Left assembly)  : nodes whose keys are < key of future root.
 *   - **R** (Right assembly) : nodes whose keys are > key of future root.
 *   - **t** (current node)   : the node currently being examined.
 *
 * At each step we look ahead TWO levels and apply:
 *   - **Zig-zig** : both t and its relevant child lean the same way  →
 *                   rotate the parent first, then link to L/R assembly.
 *   - **Zig-zag** : t and its child lean opposite ways  →
 *                   link t to L/R assembly, then descend.
 *   - **Zig**     : only one more level exists (last step)  →
 *                   simple single link to L/R assembly.
 *
 * At the end the three sub-trees are assembled:
 *   L-max's right  ← t's left  (or null)
 *   R-min's left   ← t's right (or null)
 *   t->left = L root , t->right = R root
 *
 * ### Complexity
 * | Operation  | Amortised  |
 * |------------|-----------|
 * | find       | O(log n)  |
 * | insert     | O(log n)  |
 * | erase      | O(log n)  |
 *
 * @tparam Key   Comparable key type (must support operator<).
 * @tparam Value Mapped value type.

 */

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

// ─────────────────────────────────────────────────────────────────────────────
// Forward declaration
// ─────────────────────────────────────────────────────────────────────────────

template <typename Key, typename Value, typename Compare = std::less<Key>>
class TopDownSplayTree;

// ─────────────────────────────────────────────────────────────────────────────
// TopDownSplayTree interface
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class TopDownSplayTree
 * @brief A self-adjusting binary search tree that performs search and splay
 *        in a **single downward pass** (C-10.23).
 *
 * @tparam Key     Key type. Must be totally ordered via @p Compare.
 * @tparam Value   Mapped value type.
 * @tparam Compare Binary predicate implementing strict weak ordering on Key.
 *                 Defaults to @c std::less<Key>.
 */
template <typename Key, typename Value, typename Compare>
class TopDownSplayTree {
public:
    // ── Public types ──────────────────────────────────────────────────────────

    using key_type    = Key;
    using mapped_type = Value;
    using value_type  = std::pair<const Key, Value>;
    using size_type   = std::size_t;
    using key_compare = Compare;

    // ── Node (public so callers can inspect the tree structure in tests) ──────

    /**
     * @struct Node
     * @brief Internal BST node holding a key-value pair and child pointers.
     */
    struct Node {
        Key    key;           ///< Search key.
        Value  value;         ///< Mapped value.
        Node  *left  = nullptr; ///< Left child (keys < this->key).
        Node  *right = nullptr; ///< Right child (keys > this->key).

        /// @brief Constructs a leaf node.
        Node(Key k, Value v) : key(std::move(k)), value(std::move(v)) {}
    };

    // ── Constructor / Destructor ──────────────────────────────────────────────

    /// @brief Default-constructs an empty splay tree.
    TopDownSplayTree() = default;

    /// @brief Destructor — frees all nodes.
    ~TopDownSplayTree() noexcept { clear(root_); }

    // Disable copy (tree ownership is unique)
    TopDownSplayTree(const TopDownSplayTree &)            = delete;
    TopDownSplayTree &operator=(const TopDownSplayTree &) = delete;

    // Enable move semantics
    TopDownSplayTree(TopDownSplayTree &&other) noexcept;
    TopDownSplayTree &operator=(TopDownSplayTree &&other) noexcept;

    // ── Capacity ──────────────────────────────────────────────────────────────

    /// @brief Returns the number of key-value pairs stored.
    [[nodiscard]] size_type size()  const noexcept { return size_; }

    /// @brief Returns true iff the tree contains no entries.
    [[nodiscard]] bool      empty() const noexcept { return size_ == 0; }

    // ── Lookup ───────────────────────────────────────────────────────────────

    /**
     * @brief Searches for @p key, splaying the nearest node to the root.
     * @details Performed in **one** downward pass (zig-zig / zig-zag / zig).
     * @param  key  The key to search for.
     * @return Pointer to the mapped value if found, nullptr otherwise.
     * @complexity Amortised O(log n).
     */
    Value *find(const Key &key);

    /// @brief Const overload of find().
    const Value *find(const Key &key) const;

    // ── Modifiers ────────────────────────────────────────────────────────────

    /**
     * @brief Inserts (or updates) a key-value pair.
     * @details Splays the tree so the insertion point is at the root, then
     *          creates a new root node connecting the left/right sub-trees.
     * @param key   Key to insert.
     * @param value Associated value (copied / moved into the node).
     * @complexity Amortised O(log n).
     */
    void insert(Key key, Value value);

    /**
     * @brief Removes the node with @p key if it exists.
     * @details Splays the target to the root, then joins left/right sub-trees
     *          by splaying the maximum of the left sub-tree.
     * @param  key  Key to remove.
     * @return true if the key was found and removed, false otherwise.
     * @complexity Amortised O(log n).
     */
    bool erase(const Key &key);

    // ── Accessors for root (useful for unit tests) ────────────────────────────

    /// @brief Returns a pointer to the current root node (may be nullptr).
    [[nodiscard]] Node *root() const noexcept { return root_; }

private:
    // ── Internal state ────────────────────────────────────────────────────────

    Node       *root_ = nullptr; ///< Root of the splay tree.
    size_type   size_ = 0;       ///< Number of stored entries.
    key_compare cmp_{};          ///< Key comparator.

    // ── Core algorithm ────────────────────────────────────────────────────────

    /**
     * @brief Splays the node with @p key to the root using a **single**
     *        downward pass.
     *
     * @details
     * Uses three "assembly" pointers:
     *   - @c t      : current node being examined.
     *   - @c lMax   : rightmost node of the Left assembly (keys < splayed key).
     *   - @c rMin   : leftmost  node of the Right assembly (keys > splayed key).
     *
     * ### Step logic (two levels at a time):
     *
     * **Zig-zig left** (key < t->key && key < t->left->key):
     * @code
     *       t                 y
     *      / \     →         / \
     *     y   C    rotate   x   t
     *    / \         right     / \
     *   x   B               B   C
     *  / \
     * ...
     * @endcode
     * Rotate t right (y becomes new t), then link old t to Right assembly.
     *
     * **Zig-zig right** (key > t->key && key > t->right->key):
     * Mirror of zig-zig left.
     *
     * **Zig-zag left** (key < t->key && key > t->left->key):
     * Link t to Right assembly, descend to t->left (no rotation needed).
     *
     * **Zig-zag right** (key > t->key && key < t->right->key):
     * Link t to Left assembly, descend to t->right (no rotation needed).
     *
     * **Zig** (only one level left or key == t->key):
     * Link t to L or R assembly depending on comparison, then stop.
     *
     * @param key Target key.
     */
    void splay(const Key &key) noexcept;

    // ── Rotation helpers ─────────────────────────────────────────────────────

    /**
     * @brief Rotates @p t right: left child becomes new local root.
     * @param t Reference to the node pointer being rotated.
     */
    static void rotateRight(Node *&t) noexcept;

    /**
     * @brief Rotates @p t left: right child becomes new local root.
     * @param t Reference to the node pointer being rotated.
     */
    static void rotateLeft(Node *&t) noexcept;

    // ── Memory management ─────────────────────────────────────────────────────

    /// @brief Recursively deallocates all nodes in the sub-tree rooted at @p n.
    static void clear(Node *n) noexcept;
};

// ─────────────────────────────────────────────────────────────────────────────
// Implementation
// ─────────────────────────────────────────────────────────────────────────────

// ── Move constructor ──────────────────────────────────────────────────────────

template <typename Key, typename Value, typename Compare>
TopDownSplayTree<Key, Value, Compare>::TopDownSplayTree(
        TopDownSplayTree &&other) noexcept
    : root_(other.root_), size_(other.size_) {
    other.root_ = nullptr;
    other.size_ = 0;
}

// ── Move assignment ───────────────────────────────────────────────────────────

template <typename Key, typename Value, typename Compare>
TopDownSplayTree<Key, Value, Compare> &
TopDownSplayTree<Key, Value, Compare>::operator=(
        TopDownSplayTree &&other) noexcept {
    if (this != &other) {
        clear(root_);
        root_       = other.root_;
        size_       = other.size_;
        other.root_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

// ── rotateRight ───────────────────────────────────────────────────────────────

/**
 * @brief Right rotation around node @p t.
 *
 * @details
 * Before:          After:
 * @code
 *     t               y
 *    / \             / \
 *   y   C    →      A   t
 *  / \                 / \
 * A   B               B   C
 * @endcode
 * @p t is updated to point to y.
 */
template <typename Key, typename Value, typename Compare>
void TopDownSplayTree<Key, Value, Compare>::rotateRight(Node *&t) noexcept {
    Node *y = t->left;
    t->left = y->right;
    y->right = t;
    t = y;
}

// ── rotateLeft ────────────────────────────────────────────────────────────────

/**
 * @brief Left rotation around node @p t.
 *
 * @details
 * Before:          After:
 * @code
 *   t                 y
 *  / \               / \
 * A   y     →       t   C
 *    / \           / \
 *   B   C         A   B
 * @endcode
 * @p t is updated to point to y.
 */
template <typename Key, typename Value, typename Compare>
void TopDownSplayTree<Key, Value, Compare>::rotateLeft(Node *&t) noexcept {
    Node *y = t->right;
    t->right = y->left;
    y->left = t;
    t = y;
}

// ── clear ─────────────────────────────────────────────────────────────────────

template <typename Key, typename Value, typename Compare>
void TopDownSplayTree<Key, Value, Compare>::clear(Node *n) noexcept {
    if (!n) return;
    clear(n->left);
    clear(n->right);
    delete n;
}

// ── splay (core single-pass algorithm) ───────────────────────────────────────

/**
 * @brief Single-pass top-down splay.
 *
 * Uses a "header" sentinel node to simplify L/R assembly pointer management.
 * After the loop:
 *   - lMax->right is set to t->left   (joins Left tree with t's left sub-tree)
 *   - rMin->left  is set to t->right  (joins Right tree with t's right sub-tree)
 *   - t->left  = header.right  (Left assembly root)
 *   - t->right = header.left   (Right assembly root)
 */
template <typename Key, typename Value, typename Compare>
void TopDownSplayTree<Key, Value, Compare>::splay(const Key &key) noexcept {
    if (!root_) return;

    // Sentinel header: its right child = Left assembly root,
    //                  its left  child = Right assembly root.
    Node header(key, Value{});
    Node *lMax = &header; // rightmost node in Left  assembly
    Node *rMin = &header; // leftmost  node in Right assembly

    Node *t = root_;

    for (;;) {
        if (cmp_(key, t->key)) {
            // ── key < t->key : go LEFT ─────────────────────────────────────
            if (!t->left) break; // t is the closest node; stop.

            if (cmp_(key, t->left->key)) {
                // ── Zig-zig LEFT ───────────────────────────────────────────
                // Rotate t right BEFORE linking, so parent and grandparent
                // move together — the defining feature of zig-zig.
                rotateRight(t);
                if (!t->left) break; // after rotation, may have no left child
            }
            // ── Zig or Zig-zag LEFT: link t to Right assembly ─────────────
            rMin->left = t;
            rMin       = t;
            t          = t->left;

        } else if (cmp_(t->key, key)) {
            // ── key > t->key : go RIGHT ────────────────────────────────────
            if (!t->right) break;

            if (cmp_(t->right->key, key)) {
                // ── Zig-zig RIGHT ──────────────────────────────────────────
                rotateLeft(t);
                if (!t->right) break;
            }
            // ── Zig or Zig-zag RIGHT: link t to Left assembly ────────────
            lMax->right = t;
            lMax        = t;
            t           = t->right;

        } else {
            // ── key == t->key : found ──────────────────────────────────────
            break;
        }
    }

    // ── Assemble the three parts ──────────────────────────────────────────────
    lMax->right = t->left;
    rMin->left  = t->right;
    t->left     = header.right; // root of Left  assembly
    t->right    = header.left;  // root of Right assembly

    root_ = t;
}

// ── find ──────────────────────────────────────────────────────────────────────

template <typename Key, typename Value, typename Compare>
Value *TopDownSplayTree<Key, Value, Compare>::find(const Key &key) {
    if (!root_) return nullptr;
    splay(key);
    // After splay, root_ is the node with key (or its nearest neighbour).
    if (!cmp_(root_->key, key) && !cmp_(key, root_->key))
        return &root_->value;
    return nullptr;
}

template <typename Key, typename Value, typename Compare>
const Value *
TopDownSplayTree<Key, Value, Compare>::find(const Key &key) const {
    // Cast away const temporarily; splay is logically const (amortised).
    return const_cast<TopDownSplayTree *>(this)->find(key);
}

// ── insert ────────────────────────────────────────────────────────────────────

/**
 * @brief Inserts or updates a node.
 *
 * @details
 * 1. Splay the tree with the target key.
 * 2. If the root equals the key, update value in-place.
 * 3. Otherwise, create a new root whose left/right sub-trees are split from
 *    the old root by comparison.
 */
template <typename Key, typename Value, typename Compare>
void TopDownSplayTree<Key, Value, Compare>::insert(Key key, Value value) {
    if (!root_) {
        root_ = new Node(std::move(key), std::move(value));
        ++size_;
        return;
    }

    splay(key);

    // If key already exists — update and return.
    if (!cmp_(root_->key, key) && !cmp_(key, root_->key)) {
        root_->value = std::move(value);
        return;
    }

    // Key not found: build new root connecting left and right sub-trees.
    Node *newNode = new Node(std::move(key), std::move(value));

    if (cmp_(newNode->key, root_->key)) {
        // new key < root key → new node takes root's left sub-tree
        newNode->left  = root_->left;
        newNode->right = root_;
        root_->left    = nullptr;
    } else {
        // new key > root key → new node takes root's right sub-tree
        newNode->right = root_->right;
        newNode->left  = root_;
        root_->right   = nullptr;
    }

    root_ = newNode;
    ++size_;
}

// ── erase ─────────────────────────────────────────────────────────────────────

/**
 * @brief Removes a node.
 *
 * @details
 * 1. Splay key to root. If not found, return false.
 * 2. Remove root_.
 * 3. If left sub-tree is empty, root_ = right sub-tree.
 * 4. Otherwise, splay the maximum of the left sub-tree (so it has no right
 *    child), then attach the right sub-tree as its new right child.
 */
template <typename Key, typename Value, typename Compare>
bool TopDownSplayTree<Key, Value, Compare>::erase(const Key &key) {
    if (!root_) return false;

    splay(key);

    // key must equal root_->key for the node to exist.
    if (cmp_(root_->key, key) || cmp_(key, root_->key))
        return false;

    Node *toDelete = root_;
    Node *leftSub  = root_->left;
    Node *rightSub = root_->right;

    if (!leftSub) {
        root_ = rightSub;
    } else {
        root_ = leftSub;
        // Splay the maximum of the left sub-tree: use a key guaranteed larger.
        splay(key); // splays closest-to-key, which is the max of leftSub
        // After splay, root_->right is nullptr (it was the max of leftSub).
        root_->right = rightSub;
    }

    delete toDelete;
    --size_;
    return true;
}

