// File : P-14.3.cpp

/*
    Implement the B-tree data structure, assuming a block size of 1.024 and
    integer keys. Test the number of "disk transfers" needed to process a
    sequence of map operations.
*/

#include "b_tree_map.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

void test_block_geometry() {
  using Map1024 = ds::BTreeMap<int, int, 1024>;
  static_assert(Map1024::block_size() == 1024);
  static_assert(Map1024::order() == 84);
  static_assert(Map1024::max_keys() == 83);
  static_assert(Map1024::min_keys() == 41);
  assert(Map1024::block_size() == 1024);
  assert(Map1024::order() == 84);
}

void test_empty_and_single_element() {
  ds::BTreeMap<int, int, 1024> map;
  assert(map.empty());
  assert(map.size() == 0);
  assert(map.height() == 0);
  assert(map.live_blocks() == 0);
  assert(!map.find(10).has_value());
  assert(!map.contains(10));
  assert(!map.first_entry().has_value());
  assert(!map.last_entry().has_value());
  assert(!map.ceiling_entry(10).has_value());
  assert(!map.floor_entry(10).has_value());
  assert(!map.higher_entry(10).has_value());
  assert(!map.lower_entry(10).has_value());
  assert(map.sub_map(0, 100).empty());
  assert(!map.erase(10));
  assert(map.validate());

  bool threw = false;
  try {
    (void)map.at(10);
  } catch (const std::out_of_range &) {
    threw = true;
  }
  assert(threw);

  const auto ins1 = map.insert(42, 420);
  assert(ins1.second && ins1.first.first == 42 && ins1.first.second == 420);
  assert(!map.empty() && map.size() == 1 && map.height() == 1);
  assert(map.live_blocks() == 1);
  assert(map.contains(42) && map.at(42) == 420);
  assert(map.first_entry()->first == 42 && map.last_entry()->first == 42);
  assert(map.ceiling_entry(42)->first == 42);
  assert(map.floor_entry(42)->first == 42);
  assert(!map.higher_entry(42).has_value());
  assert(!map.lower_entry(42).has_value());

  // Duplicate insert preserves existing value; put overwrites it.
  const auto dup = map.insert(42, 999);
  assert(!dup.second && dup.first.second == 420);
  const auto upd = map.put(42, 840);
  assert(!upd.second && upd.first.second == 840);
  assert(map.at(42) == 840);

  assert(map.erase(42));
  assert(map.empty() && map.size() == 0 && map.height() == 0);
  assert(map.live_blocks() == 0);
  assert(map.validate());
}

void test_ordered_navigation_and_custom_comparator() {
  ds::BTreeMap<int, int, 1024> map;
  for (int k : {10, 20, 30, 40, 50, 60, 70}) {
    assert(map.put(k, k * 10).second);
  }
  assert(map.first_entry()->first == 10);
  assert(map.last_entry()->first == 70);
  assert(map.ceiling_entry(35)->first == 40);
  assert(map.ceiling_entry(40)->first == 40);
  assert(map.floor_entry(35)->first == 30);
  assert(map.floor_entry(40)->first == 40);
  assert(map.higher_entry(40)->first == 50);
  assert(map.lower_entry(40)->first == 30);
  assert(!map.lower_entry(10).has_value());
  assert(!map.higher_entry(70).has_value());

  const auto range = map.sub_map(20, 55);
  assert(range.size() == 4);
  assert(range[0].first == 20 && range[1].first == 30 && range[2].first == 40 &&
         range[3].first == 50);
  assert(map.sub_map(50, 20).empty());
  assert(map.validate());

  ds::BTreeMap<int, int, 1024, std::greater<int>> desc;
  for (int k = 1; k <= 200; ++k) {
    desc.put(k, k);
  }
  assert(desc.first_entry()->first == 200);
  assert(desc.last_entry()->first == 1);
  assert(desc.validate());
}

void test_splits_borrows_merges_and_block_reuse() {
  // Small block size (128 bytes -> order 10) exercises deep splits & merges.
  ds::BTreeMap<int, int, 128> map;
  constexpr int n = 500;
  for (int i = 0; i < n; ++i) {
    assert(map.put(i, i * 2).second);
  }
  assert(map.size() == n);
  assert(map.height() >= 3);
  assert(map.validate());

  for (int i = 0; i < n; i += 2) {
    assert(map.erase(i));
    assert(!map.erase(i));
  }
  assert(map.size() == n / 2);
  assert(map.validate());

  // Re-inserting should reuse freed blocks rather than growing unboundedly.
  const std::size_t allocated_before = map.allocated_blocks();
  for (int i = 0; i < n; i += 2) {
    assert(map.put(i, i * 3).second);
  }
  assert(map.size() == n);
  assert(map.allocated_blocks() == allocated_before);
  assert(map.validate());

  for (int i = 0; i < n; ++i) {
    assert(map.erase(i));
  }
  assert(map.empty() && map.live_blocks() == 0 && map.validate());
}

void test_randomized_against_std_map() {
  ds::BTreeMap<int, int, 128> btree;
  std::map<int, int> ref;
  std::mt19937 rng(20261124u);
  std::uniform_int_distribution<int> key_dist(-250, 250);
  std::uniform_int_distribution<int> val_dist(-10000, 10000);
  std::uniform_int_distribution<int> op_dist(0, 99);

  for (int step = 0; step < 10000; ++step) {
    const int op = op_dist(rng);
    const int k = key_dist(rng);
    const int v = val_dist(rng);

    if (op < 35) {
      const auto res = btree.put(k, v);
      const auto it = ref.find(k);
      const bool expected_inserted = (it == ref.end());
      ref[k] = v;
      assert(res.second == expected_inserted);
      assert(res.first.first == k && res.first.second == v);
    } else if (op < 55) {
      const auto res = btree.insert(k, v);
      const auto [it, expected_inserted] = ref.emplace(k, v);
      assert(res.second == expected_inserted);
      assert(res.first.first == it->first && res.first.second == it->second);
    } else if (op < 75) {
      const bool removed = btree.erase(k);
      const bool expected_removed = (ref.erase(k) > 0);
      assert(removed == expected_removed);
    } else {
      const auto found = btree.find(k);
      const auto it = ref.find(k);
      assert(found.has_value() == (it != ref.end()));
      if (found) {
        assert(found->second == it->second);
      }

      const auto ceil_e = btree.ceiling_entry(k);
      const auto ref_ceil = ref.lower_bound(k);
      assert(ceil_e.has_value() == (ref_ceil != ref.end()));
      if (ceil_e) {
        assert(ceil_e->first == ref_ceil->first);
      }

      const auto high_e = btree.higher_entry(k);
      const auto ref_high = ref.upper_bound(k);
      assert(high_e.has_value() == (ref_high != ref.end()));
      if (high_e) {
        assert(high_e->first == ref_high->first);
      }
    }

    if (step % 500 == 0) {
      assert(btree.size() == ref.size());
      assert(btree.validate());
    }
  }

  assert(btree.size() == ref.size());
  assert(btree.validate());
  const auto all = btree.entries();
  assert(all.size() == ref.size());
  std::size_t idx = 0;
  for (const auto &kv : ref) {
    assert(all[idx].first == kv.first && all[idx].second == kv.second);
    ++idx;
  }
}

void report_workload(const char *label, std::size_t ops,
                     const ds::DiskStats &stats, std::size_t height,
                     std::size_t live_blocks) {
  const double avg =
      ops == 0 ? 0.0
               : static_cast<double>(stats.total()) / static_cast<double>(ops);
  std::cout << std::left << std::setw(28) << label << " | ops=" << std::setw(7)
            << ops << " | reads=" << std::setw(8) << stats.reads
            << " | writes=" << std::setw(8) << stats.writes
            << " | total=" << std::setw(8) << stats.total()
            << " | avg/op=" << std::fixed << std::setprecision(2)
            << std::setw(6) << avg << " | h=" << height
            << " | blocks=" << live_blocks << "\n";
}

void run_disk_transfer_experiments() {
  using BTree1024 = ds::BTreeMap<int, int, 1024>;
  constexpr std::size_t kN = 50000;

  std::cout << "\n=== B-Tree Disk Transfer Experiment (BlockSize = "
            << BTree1024::block_size() << " B, Order M = " << BTree1024::order()
            << ", MaxKeys = " << BTree1024::max_keys() << ") ===\n";

  // 1. Sequential inserts
  {
    BTree1024 tree;
    for (std::size_t i = 0; i < kN; ++i) {
      tree.put(static_cast<int>(i), static_cast<int>(i));
    }
    assert(tree.validate());
    assert(tree.height() <= 3);
    report_workload("Sequential puts (50k)", kN, tree.disk_stats(),
                    tree.height(), tree.live_blocks());
  }

  // 2. Random inserts + lookups + mixed workload
  BTree1024 tree;
  std::vector<int> keys(kN);
  std::iota(keys.begin(), keys.end(), 0);
  std::mt19937 rng(42u);
  std::shuffle(keys.begin(), keys.end(), rng);

  for (int k : keys) {
    tree.put(k, k * 10);
  }
  assert(tree.validate());
  assert(tree.height() <= 3);
  report_workload("Random puts (50k)", kN, tree.disk_stats(), tree.height(),
                  tree.live_blocks());

  // 3. Random lookups (exact O(log_M n) reads, 0 writes)
  tree.reset_disk_stats();
  std::shuffle(keys.begin(), keys.end(), rng);
  for (int k : keys) {
    const auto res = tree.find(k);
    assert(res.has_value() && res->second == k * 10);
  }
  assert(tree.disk_stats().writes == 0);
  assert(tree.disk_stats().reads <= kN * tree.height());
  report_workload("Random finds (50k)", kN, tree.disk_stats(), tree.height(),
                  tree.live_blocks());

  // 4. Mixed workload: 50% find, 25% put, 25% erase
  tree.reset_disk_stats();
  std::uniform_int_distribution<int> op_dist(0, 3);
  std::uniform_int_distribution<int> key_dist(0, static_cast<int>(kN * 2));
  constexpr std::size_t kMixedOps = 50000;
  for (std::size_t i = 0; i < kMixedOps; ++i) {
    const int op = op_dist(rng);
    const int k = key_dist(rng);
    if (op < 2) {
      (void)tree.find(k);
    } else if (op == 2) {
      tree.put(k, k);
    } else {
      (void)tree.erase(k);
    }
  }
  assert(tree.validate());
  report_workload("Mixed 50/25/25 (50k)", kMixedOps, tree.disk_stats(),
                  tree.height(), tree.live_blocks());
}

} // namespace

int main() {
  test_block_geometry();
  test_empty_and_single_element();
  test_ordered_navigation_and_custom_comparator();
  test_splits_borrows_merges_and_block_reuse();
  test_randomized_against_std_map();
  run_disk_transfer_experiments();

  std::cout << "\nAll P-14.3 B-tree tests passed!\n";
  return 0;
}
