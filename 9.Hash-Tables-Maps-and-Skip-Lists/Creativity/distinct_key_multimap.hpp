/**
 * @file distinct_key_multimap.hpp
 * @brief Ordered dictionary for n elements with k distinct keys supporting
 * O(log k + s) operations.
 */

#pragma once

#include <cstddef>
#include <list>
#include <map>
#include <utility>
#include <vector>

/**
 * @class DistinctKeyMultimap
 * @brief Implements an ordered dictionary storing n elements mapped across k <
 * n distinct keys.
 *
 * Uses a balanced search tree (Red-Black Tree) for the k distinct keys, where
 * each node maintains a linked list of elements sharing that key.
 *
 * @tparam Key Total-ordered key type.
 * @tparam Element Element/value type stored.
 */
template <typename Key, typename Element> class DistinctKeyMultimap {

public:
  DistinctKeyMultimap() = default;
  ~DistinctKeyMultimap() = default;

  /**
   * @brief Inserts an element with the given key.
   *
   * @param key The key.
   * @param element The element to store.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $O(\log k)$ expected/worst-case, where $k$ is the
   * number of distinct keys.
   */
  void insert(const Key &key, const Element &element) {
    tree_[key].push_back(element);
    total_elements_++;
  }

  /**
   * @brief Finds and returns all elements associated with the given key.
   *
   * @param key The target key.
   * @return std::vector<Element> All elements matching the key.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $O(\log k + s)$, where $k$ is distinct keys and $s$
   * is the number of elements returned.
   */
  [[nodiscard]] std::vector<Element> findAll(const Key &key) const {
    auto it = tree_.find(key);
    if (it == tree_.end()) {
      return {};
    }
    std::vector<Element> result;
    result.reserve(it->second.size());
    for (const auto &elem : it->second) {
      result.push_back(elem);
    }
    return result;
  }

  /**
   * @brief Performs a range query returning all elements whose keys fall within
   * [low_key, high_key].
   *
   * @param low_key Lower bound key (inclusive).
   * @param high_key Upper bound key (inclusive).
   * @return std::vector<Element> All elements in the specified key range.
   *
   * @section complexity Complexity Analysis
   * - **Time Complexity:** $O(\log k + s)$, where $k$ is distinct keys and $s$
   * is the number of elements returned.
   */
  [[nodiscard]] std::vector<Element> rangeQuery(const Key &low_key,
                                                const Key &high_key) const {
    std::vector<Element> result;
    auto it = tree_.lower_bound(low_key);
    auto end_it = tree_.upper_bound(high_key);

    for (; it != end_it; ++it) {
      for (const auto &elem : it->second) {
        result.push_back(elem);
      }
    }
    return result;
  }

  /**
   * @brief Removes a specific element associated with a key.
   *
   * @param key The key.
   * @param element The element to remove.
   * @return true if found and removed, false otherwise.
   */
  bool remove(const Key &key, const Element &element) {
    auto it = tree_.find(key);
    if (it == tree_.end()) {
      return false;
    }
    auto &lst = it->second;
    for (auto list_it = lst.begin(); list_it != lst.end(); ++list_it) {
      if (*list_it == element) {
        lst.erase(list_it);
        total_elements_--;
        if (lst.empty()) {
          tree_.erase(it);
        }
        return true;
      }
    }
    return false;
  }

  /** @brief Returns total number of elements n. */
  [[nodiscard]] size_t size() const { return total_elements_; }

  /** @brief Returns number of distinct keys k. */
  [[nodiscard]] size_t distinctKeysCount() const { return tree_.size(); }

  /** @brief Checks if the structure is empty. */
  [[nodiscard]] bool empty() const { return total_elements_ == 0; }

private:
  std::map<Key, std::list<Element>> tree_;
  size_t total_elements_ = 0;
};