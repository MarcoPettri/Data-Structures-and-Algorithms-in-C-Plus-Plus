/**
 * @file b_tree_map.hpp
 * @brief Ordered map ADT implemented with a block-based B-tree (interface).
 *
 * Models a disk-resident B-tree where every node occupies a single fixed-size
 * block (default 1024 bytes) managed by an instrumented @ref ds::BlockDevice.
 * The branching order @c M is derived at compile time from @c BlockSize,
 * @c sizeof(Key), @c sizeof(T), and block-pointer size.
 *
 */

#ifndef DS_B_TREE_MAP_HPP
#define DS_B_TREE_MAP_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace ds {

/**
 * @brief Counters for simulated disk block transfers.
 */
struct DiskStats {
  std::size_t reads = 0;  ///< Number of block read transfers.
  std::size_t writes = 0; ///< Number of block write transfers.

  /**
   * @brief Total number of disk transfers (reads + writes).
   * @return Combined read and write count.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] constexpr std::size_t total() const noexcept {
    return reads + writes;
  }

  /**
   * @brief Reset all transfer counters to zero.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  void reset() noexcept {
    reads = 0;
    writes = 0;
  }
};

/**
 * @brief Simulated block-oriented disk device with transfer instrumentation.
 *
 * Stores fixed-size byte blocks indexed by a 32-bit block identifier and
 * recycles freed blocks via an internal free-list. Every @ref read_block and
 * @ref write_block call increments the @ref DiskStats counters.
 *
 * @tparam BlockSize Size of each disk block in bytes (default: 1024).
 */
template <std::size_t BlockSize = 1024> class BlockDevice {
public:
  using block_id_type = std::uint32_t;
  using block_type = std::array<std::byte, BlockSize>;

  static constexpr block_id_type kInvalidBlock =
      (std::numeric_limits<block_id_type>::max)();
  static constexpr std::size_t kBlockSize = BlockSize;

  static_assert(BlockSize >= 64, "BlockSize must be at least 64 bytes.");

  /**
   * @brief Allocate a zero-initialized block (reusing freed blocks first).
   * @return Identifier of the newly allocated block.
   * @throws std::length_error if the maximum number of blocks is exceeded.
   * @section complexity Time: $O(1)$ amortized. Auxiliary space: $O(1)$
   * amortized (plus $O(B)$ for a new block when the free-list is empty, where
   * $B$ is @c BlockSize).
   */
  block_id_type allocate_block() {
    if (!free_list_.empty()) {
      const block_id_type id = free_list_.back();
      free_list_.pop_back();
      blocks_[id].fill(std::byte{0});
      return id;
    }
    if (blocks_.size() >= kInvalidBlock) {
      throw std::length_error("BlockDevice capacity exhausted");
    }
    const auto id = static_cast<block_id_type>(blocks_.size());
    blocks_.emplace_back();
    blocks_.back().fill(std::byte{0});
    return id;
  }

  /**
   * @brief Return a block to the free-list for reuse.
   * @param id Identifier of the block to free.
   * @throws std::out_of_range if @p id is out of range.
   * @section complexity Time: $O(1)$ amortized. Auxiliary space: $O(1)$ amortized.
   */
  void free_block(block_id_type id) {
    if (id >= blocks_.size()) {
      throw std::out_of_range("BlockDevice::free_block: invalid block id");
    }
    free_list_.push_back(id);
  }

  /**
   * @brief Read a block from the simulated disk, counting one read transfer.
   * @param id Identifier of the block to read.
   * @return Const reference to the block bytes.
   * @throws std::out_of_range if @p id is out of range.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const block_type &read_block(block_id_type id) const {
    if (id >= blocks_.size()) {
      throw std::out_of_range("BlockDevice::read_block: invalid block id");
    }
    ++stats_.reads;
    return blocks_[id];
  }

  /**
   * @brief Inspect a block without incrementing disk transfer counters.
   * @param id Identifier of the block to inspect.
   * @return Const reference to the block bytes.
   * @throws std::out_of_range if @p id is out of range.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const block_type &peek_block(block_id_type id) const {
    if (id >= blocks_.size()) {
      throw std::out_of_range("BlockDevice::peek_block: invalid block id");
    }
    return blocks_[id];
  }

  /**
   * @brief Write a block to the simulated disk, counting one write transfer.
   * @param id Identifier of the block to write.
   * @param data Block payload to store.
   * @throws std::out_of_range if @p id is out of range.
   * @section complexity Time: $O(B)$ where $B$ is @c BlockSize. Auxiliary space: $O(1)$.
   */
  void write_block(block_id_type id, const block_type &data) {
    if (id >= blocks_.size()) {
      throw std::out_of_range("BlockDevice::write_block: invalid block id");
    }
    ++stats_.writes;
    blocks_[id] = data;
  }

  /**
   * @brief Release all blocks and reset transfer statistics.
   * @section complexity Time: $O(N)$ where $N$ is total allocated blocks. Auxiliary space: $O(1)$.
   */
  void clear() noexcept {
    blocks_.clear();
    free_list_.clear();
    stats_.reset();
  }

  /**
   * @brief Current disk transfer statistics.
   * @return Const reference to @ref DiskStats.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const DiskStats &stats() const noexcept { return stats_; }

  /**
   * @brief Reset disk transfer counters to zero.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  void reset_stats() noexcept { stats_.reset(); }

  /**
   * @brief Total number of blocks ever allocated in the device image.
   * @return Block count including freed blocks.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t allocated_blocks() const noexcept {
    return blocks_.size();
  }

  /**
   * @brief Number of currently live (non-freed) blocks.
   * @return Live block count.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::size_t live_blocks() const noexcept {
    return blocks_.size() - free_list_.size();
  }

private:
  std::vector<block_type> blocks_;
  std::vector<block_id_type> free_list_;
  mutable DiskStats stats_{};
};

namespace detail {

template <typename Key, typename T, typename BlockId, std::size_t BlockSize>
constexpr std::size_t compute_btree_order() noexcept {
  // Block layout: [uint32_t key_count][uint32_t is_leaf]
  //               [(M-1) * Key][(M-1) * T][M * BlockId]
  constexpr std::size_t header_bytes = sizeof(std::uint32_t) * 2;
  constexpr std::size_t entry_bytes = sizeof(Key) + sizeof(T);
  constexpr std::size_t child_bytes = sizeof(BlockId);
  if (BlockSize < header_bytes + entry_bytes + 2 * child_bytes) {
    return 0;
  }
  std::size_t order =
      (BlockSize - header_bytes + entry_bytes) / (entry_bytes + child_bytes);
  // Ensure even order (M = 2t) so proactive splitting of (2t - 1) keys
  // yields two nodes of (t - 1) == kMinKeys keys each.
  if (order % 2 != 0) {
    --order;
  }
  return order;
}

} // namespace detail

/**
 * @brief Ordered map backed by a disk-simulated B-tree of fixed block size.
 *
 * Every node is serialized into a single @c BlockSize byte block in an internal
 * @ref BlockDevice. With @c BlockSize = 1024 and 4-byte @c int keys and values,
 * the computed order is @c M = 84 (up to 83 keys and 84 children per node,
 * minimum 41 keys per non-root node).
 *
 * @tparam Key Trivially copyable key type ordered by @p Compare.
 * @tparam T Trivially copyable mapped value type.
 * @tparam BlockSize Size of each simulated disk block in bytes (default: 1024).
 * @tparam Compare Strict weak ordering for keys (default: @c std::less<Key>).
 */
template <typename Key = int, typename T = int, std::size_t BlockSize = 1024,
          typename Compare = std::less<Key>>
class BTreeMap {
public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<Key, T>;
  using size_type = std::size_t;
  using block_id_type = typename BlockDevice<BlockSize>::block_id_type;

  static_assert(std::is_trivially_copyable_v<Key>,
                "BTreeMap requires trivially copyable Key for block storage.");
  static_assert(std::is_trivially_copyable_v<T>,
                "BTreeMap requires trivially copyable T for block storage.");

  static constexpr size_type kBlockSize = BlockSize;
  static constexpr size_type kOrder =
      detail::compute_btree_order<Key, T, block_id_type, BlockSize>();
  static_assert(kOrder >= 4,
                "BlockSize is too small to hold a B-tree node of order >= 4.");

  static constexpr size_type kMaxKeys = kOrder - 1;
  static constexpr size_type kMinChildren = (kOrder + 1) / 2;
  static constexpr size_type kMinKeys = kMinChildren - 1;

  /**
   * @brief Construct an empty B-tree map.
   * @param compare Strict weak ordering comparator for keys.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$. Disk transfers: $0$.
   */
  explicit BTreeMap(const Compare &compare = Compare());

  /**
   * @brief Number of stored key-value entries.
   * @return Entry count @f$n@f$.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$. Disk transfers: $0$.
   */
  [[nodiscard]] size_type size() const noexcept { return size_; }

  /**
   * @brief Check whether the map is empty.
   * @return True iff @c size() == 0.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$. Disk transfers: $0$.
   */
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

  /**
   * @brief Maximum number of children per node (@c M).
   * @return B-tree order @c kOrder.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] static constexpr size_type order() noexcept { return kOrder; }

  /**
   * @brief Minimum number of keys in a non-root node.
   * @return @c kMinKeys.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] static constexpr size_type min_keys() noexcept {
    return kMinKeys;
  }

  /**
   * @brief Maximum number of keys in any node.
   * @return @c kMaxKeys.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] static constexpr size_type max_keys() noexcept {
    return kMaxKeys;
  }

  /**
   * @brief Block size in bytes.
   * @return @c BlockSize.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] static constexpr size_type block_size() noexcept {
    return kBlockSize;
  }

  /**
   * @brief Current height of the tree in nodes (0 when empty).
   * @return Tree height @f$h@f$.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$. Disk transfers: $0$.
   */
  [[nodiscard]] size_type height() const noexcept { return height_; }

  /**
   * @brief Current simulated disk transfer statistics.
   * @return Const reference to @ref DiskStats.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] const DiskStats &disk_stats() const noexcept {
    return disk_.stats();
  }

  /**
   * @brief Reset simulated disk transfer counters to zero.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  void reset_disk_stats() noexcept { disk_.reset_stats(); }

  /**
   * @brief Number of live (non-freed) blocks in the underlying device.
   * @return Live block count.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] size_type live_blocks() const noexcept {
    return disk_.live_blocks();
  }

  /**
   * @brief Total number of blocks allocated in the underlying device.
   * @return Allocated block count.
   * @section complexity Time: $O(1)$. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] size_type allocated_blocks() const noexcept {
    return disk_.allocated_blocks();
  }

  /**
   * @brief Remove all entries and free all blocks.
   * @section complexity Time: $O(N)$ where $N$ is allocated blocks. Auxiliary space: $O(1)$.
   */
  void clear() noexcept;

  /**
   * @brief Find a key and return a copy of its entry, or @c std::nullopt.
   * @param key Key to search for.
   * @return Matching @c (key, value) pair if present.
   * @section complexity Time: $O(M \log_M n)$ comparisons, $O(\log_M n)$ disk
   * reads where $n$ is entry count and $M$ is @c kOrder. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::optional<value_type> find(const Key &key) const;

  /**
   * @brief Test whether a key is present in the map.
   * @param key Key to test.
   * @return True iff @p key exists.
   * @section complexity Time: $O(M \log_M n)$ comparisons, $O(\log_M n)$ disk
   * reads. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] bool contains(const Key &key) const;

  /**
   * @brief Retrieve the mapped value for an existing key.
   * @param key Key to look up.
   * @return Copy of the mapped value.
   * @throws std::out_of_range if @p key is not present.
   * @section complexity Time: $O(M \log_M n)$ comparisons, $O(\log_M n)$ disk
   * reads. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] T at(const Key &key) const;

  /**
   * @brief Insert a new entry or overwrite the mapped value if @p key exists.
   * @param key Key to insert or update.
   * @param value Mapped value to store.
   * @return Stored entry and @c true iff a new key was inserted.
   * @section complexity Time: $O(M \log_M n)$ comparisons, $O(\log_M n)$ disk
   * transfers. Auxiliary space: $O(1)$.
   */
  std::pair<value_type, bool> put(const Key &key, const T &value);

  /**
   * @brief Insert a new entry only if @p key is not already present.
   * @param key Key to insert.
   * @param value Mapped value to store if absent.
   * @return Resulting entry in the map and @c true iff insertion occurred.
   * @section complexity Time: $O(M \log_M n)$ comparisons, $O(\log_M n)$ disk
   * transfers. Auxiliary space: $O(1)$.
   */
  std::pair<value_type, bool> insert(const Key &key, const T &value);

  /**
   * @brief Erase the entry with the given key, if present.
   * @param key Key to remove.
   * @return True iff an entry was found and removed.
   * @section complexity Time: $O(M \log_M n)$ comparisons, $O(\log_M n)$ disk
   * transfers. Auxiliary space: $O(\log_M n)$ recursion stack.
   */
  bool erase(const Key &key);

  /**
   * @brief Return the entry with the smallest key.
   * @return Minimum entry, or @c std::nullopt if empty.
   * @section complexity Time: $O(\log_M n)$ disk reads. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::optional<value_type> first_entry() const;

  /**
   * @brief Return the entry with the largest key.
   * @return Maximum entry, or @c std::nullopt if empty.
   * @section complexity Time: $O(\log_M n)$ disk reads. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::optional<value_type> last_entry() const;

  /**
   * @brief Return the entry with the smallest key @f$\ge@f$ @p key.
   * @param key Lower bound key.
   * @return Ceiling entry, or @c std::nullopt if none exists.
   * @section complexity Time: $O(M \log_M n)$ comparisons, $O(\log_M n)$ disk
   * reads. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::optional<value_type> ceiling_entry(const Key &key) const;

  /**
   * @brief Return the entry with the largest key @f$\le@f$ @p key.
   * @param key Upper bound key.
   * @return Floor entry, or @c std::nullopt if none exists.
   * @section complexity Time: $O(M \log_M n)$ comparisons, $O(\log_M n)$ disk
   * reads. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::optional<value_type> floor_entry(const Key &key) const;

  /**
   * @brief Return the entry with the smallest key strictly greater than @p key.
   * @param key Strict lower bound key.
   * @return Higher entry, or @c std::nullopt if none exists.
   * @section complexity Time: $O(M \log_M n)$ comparisons, $O(\log_M n)$ disk
   * reads. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::optional<value_type> higher_entry(const Key &key) const;

  /**
   * @brief Return the entry with the largest key strictly less than @p key.
   * @param key Strict upper bound key.
   * @return Lower entry, or @c std::nullopt if none exists.
   * @section complexity Time: $O(M \log_M n)$ comparisons, $O(\log_M n)$ disk
   * reads. Auxiliary space: $O(1)$.
   */
  [[nodiscard]] std::optional<value_type> lower_entry(const Key &key) const;

  /**
   * @brief Collect all entries with keys in the half-open range
   *        @f$[\text{from}, \text{to})@f$ in sorted order.
   * @param from Inclusive lower bound.
   * @param to Exclusive upper bound.
   * @return Sorted vector of matching entries.
   * @section complexity Time: $O(\log_M n + k)$ disk reads and $O(k)$ space
   * for @f$k@f$ returned entries.
   */
  [[nodiscard]] std::vector<value_type> sub_map(const Key &from,
                                                const Key &to) const;

  /**
   * @brief Collect all entries in ascending key order.
   * @return Sorted vector of all entries.
   * @section complexity Time: $O(n)$ where $n$ is entry count. Auxiliary space: $O(n)$.
   */
  [[nodiscard]] std::vector<value_type> entries() const;

  /**
   * @brief Verify all B-tree structural and ordering invariants without
   *        affecting disk transfer counters.
   * @return True iff the tree is valid.
   * @section complexity Time: $O(n)$. Auxiliary space: $O(\log_M n)$.
   */
  [[nodiscard]] bool validate() const;

private:
  static constexpr block_id_type kInvalidBlock =
      BlockDevice<BlockSize>::kInvalidBlock;

  struct Node {
    std::uint32_t key_count = 0;
    bool is_leaf = true;
    std::array<Key, kMaxKeys> keys{};
    std::array<T, kMaxKeys> values{};
    std::array<block_id_type, kOrder> children{};
  };

  BlockDevice<BlockSize> disk_;
  block_id_type root_ = kInvalidBlock;
  size_type size_ = 0;
  size_type height_ = 0;
  Compare comp_;

  [[nodiscard]] Node read_node(block_id_type id) const;
  [[nodiscard]] Node peek_node(block_id_type id) const;
  void write_node(block_id_type id, const Node &node);

  static Node
  deserialize(const typename BlockDevice<BlockSize>::block_type &buf);
  static typename BlockDevice<BlockSize>::block_type
  serialize(const Node &node);

  [[nodiscard]] size_type lower_bound_index(const Node &node,
                                            const Key &key) const;

  std::pair<value_type, bool> insert_impl(const Key &key, const T &value,
                                          bool overwrite);
  void split_child(block_id_type parent_id, Node &parent, size_type child_idx);
  std::pair<value_type, bool> insert_nonfull(block_id_type node_id,
                                             const Key &key, const T &value,
                                             bool overwrite);

  bool erase_from(block_id_type node_id, const Key &key);
  void ensure_min_plus_one(block_id_type parent_id, Node &parent,
                           size_type child_idx);
  void borrow_from_prev(block_id_type parent_id, Node &parent,
                        size_type child_idx);
  void borrow_from_next(block_id_type parent_id, Node &parent,
                        size_type child_idx);
  void merge_children(block_id_type parent_id, Node &parent,
                      size_type left_idx);
  [[nodiscard]] value_type rightmost_entry(block_id_type node_id) const;
  [[nodiscard]] value_type leftmost_entry(block_id_type node_id) const;

  void collect_range(block_id_type node_id, const Key &from, const Key &to,
                     std::vector<value_type> &out) const;
  void collect_all(block_id_type node_id, std::vector<value_type> &out) const;

  bool validate_node(block_id_type node_id, const Key *min_bound,
                     const Key *max_bound, size_type depth,
                     std::optional<size_type> &leaf_depth,
                     size_type &counted_keys, size_type &counted_blocks) const;
};

} // namespace ds

#include "b_tree_map.tpp"

#endif // DS_B_TREE_MAP_HPP
