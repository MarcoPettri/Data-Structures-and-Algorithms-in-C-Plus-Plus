// File : P-14.2.cpp

/*
    Write a C++ class that implements all the functions of the ordered map
    ADT by means of an (a, b) tree, where a and b are integer constants passed
    as parameters to a constructor.
*/

#include "ab_tree_map.hpp"

#include <cassert>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

void test_empty_single_and_ordered_operations() {
  ds::ABTreeMap<int, std::string> map(2, 4);
  assert(map.empty());
  assert(map.size() == 0);
  assert(!map.find(10));
  assert(!map.firstEntry() && !map.lastEntry());
  assert(!map.erase(10));

  assert(map.put(10, "ten").second);
  assert(map.size() == 1 && map.contains(10));
  assert(map.firstEntry()->first == 10 && map.lastEntry()->first == 10);
  assert(map.ceilingEntry(10)->second == "ten");
  assert(map.floorEntry(10)->second == "ten");
  assert(!map.higherEntry(10) && !map.lowerEntry(10));
  assert(map.erase(10));
  assert(map.empty() && map.validate());
}

void test_duplicates_ranges_and_bounds() {
  ds::ABTreeMap<int, std::string> map(2, 4);
  for (const auto &entry :
       std::vector<std::pair<int, std::string>>{{4, "four"},
                                                {2, "two"},
                                                {6, "six"},
                                                {1, "one"},
                                                {3, "three"},
                                                {5, "five"},
                                                {7, "seven"}})
    assert(map.insert(entry.first, entry.second).second);
  const auto duplicate = map.insert(4, "ignored");
  assert(!duplicate.second && duplicate.first.second == "four");
  assert(!map.put(4, "FOUR").second && map.at(4) == "FOUR");

  assert(map.firstEntry()->first == 1 && map.lastEntry()->first == 7);
  assert(map.ceilingEntry(4)->first == 4);
  assert(map.floorEntry(4)->first == 4);
  assert(map.higherEntry(4)->first == 5);
  assert(map.lowerEntry(4)->first == 3);
  assert(map.ceilingEntry(0)->first == 1);
  assert(map.floorEntry(8)->first == 7);
  assert(!map.lowerEntry(1) && !map.higherEntry(7));
  const auto range = map.subMap(3, 6);
  assert(range.size() == 3);
  assert(range[0].first == 3 && range[1].first == 4 && range[2].first == 5);
  assert(map.subMap(6, 3).empty());
  assert(map.validate());
}

void test_invalid_input_and_exceptions() {
  bool thrown = false;
  try {
    ds::ABTreeMap<int, int> invalid(2, 2);
  } catch (const std::invalid_argument &) {
    thrown = true;
  }
  assert(thrown);

  thrown = false;
  try {
    ds::ABTreeMap<int, int> invalid(static_cast<std::size_t>(-1),
                                    static_cast<std::size_t>(-1));
  } catch (const std::invalid_argument &) {
    thrown = true;
  }
  assert(thrown);

  ds::ABTreeMap<int, int> map(2, 4);
  thrown = false;
  try {
    (void)map.at(5);
  } catch (const std::out_of_range &) {
    thrown = true;
  }
  assert(thrown);
  map[5] = 25;
  assert(map.at(5) == 25);
}

void test_randomized_against_std_map() {
  constexpr int operations = 6000;
  std::mt19937 rng(1402);
  std::uniform_int_distribution<int> key_dist(-250, 250);
  std::uniform_int_distribution<int> op_dist(0, 99);
  ds::ABTreeMap<int, int> actual(3, 6);
  std::map<int, int> expected;

  for (int step = 0; step < operations; ++step) {
    const int key = key_dist(rng);
    const int op = op_dist(rng);
    if (op < 50) {
      const auto inserted = expected.emplace(key, step).second;
      if (!inserted)
        expected[key] = step;
      assert(actual.put(key, step).second == inserted);
    } else if (op < 75) {
      assert(actual.erase(key) == (expected.erase(key) != 0));
    } else {
      const auto got = actual.find(key);
      const auto found = expected.find(key);
      assert(got.has_value() == (found != expected.end()));
      if (found != expected.end())
        assert(got->second == found->second);
    }
    assert(actual.validate());
    assert(actual.size() == expected.size());
  }

  const auto entries = actual.to_vector();
  assert(entries.size() == expected.size());
  auto expected_it = expected.begin();
  for (const auto &entry : entries) {
    assert(expected_it != expected.end());
    assert(entry.first == expected_it->first &&
           entry.second == expected_it->second);
    ++expected_it;
  }
  assert(expected_it == expected.end());
}

void test_performance_sanity() {
  ds::ABTreeMap<int, int> map(8, 20);
  constexpr int count = 20000;
  for (int i = 0; i < count; ++i)
    map.put(i, i * 2);
  assert(map.size() == static_cast<std::size_t>(count));
  assert(map.validate());
  assert(map.at(count / 2) == count);
  for (int i = 0; i < count; i += 2)
    assert(map.erase(i));
  assert(map.size() == static_cast<std::size_t>(count / 2));
  assert(map.validate());
}

int main() {
  test_empty_single_and_ordered_operations();
  test_duplicates_ranges_and_bounds();
  test_invalid_input_and_exceptions();
  test_randomized_against_std_map();
  test_performance_sanity();
  std::cout << "ABTreeMap tests passed\n";
  return 0;
}