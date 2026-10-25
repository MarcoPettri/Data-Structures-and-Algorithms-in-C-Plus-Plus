// File : P-9.6.cpp

/*
    P-9.6 Implement the ordered map ADT using a skip list.
*/

#include "skip_list_map.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace test {

struct State {
  int checks = 0;
  int failures = 0;
};

inline State &state() noexcept {
  static State instance;
  return instance;
}

using TestFn = void (*)();

struct Case {
  const char *name;
  TestFn fn;
};

inline std::vector<Case> &cases() {
  static std::vector<Case> registry;
  return registry;
}

struct Registrar {
  Registrar(const char *name, TestFn fn) { cases().push_back({name, fn}); }
};

} // namespace test

#define TEST_CASE(fn_name)                                                     \
  static void fn_name();                                                       \
  static const ::test::Registrar registrar_##fn_name{#fn_name, &fn_name};      \
  static void fn_name()

#define CHECK(...)                                                             \
  do {                                                                         \
    ++::test::state().checks;                                                  \
    if (!(__VA_ARGS__)) {                                                      \
      ++::test::state().failures;                                              \
      std::cerr << __FILE__ << ':' << __LINE__                                 \
                << ": CHECK failed: " << #__VA_ARGS__ << '\n';                 \
    }                                                                          \
  } while (false)

#define CHECK_THROWS_AS(ExType, ...)                                           \
  do {                                                                         \
    ++::test::state().checks;                                                  \
    bool caught_ = false;                                                      \
    try {                                                                      \
      (void)(__VA_ARGS__);                                                     \
    } catch (const ExType &) {                                                 \
      caught_ = true;                                                          \
    } catch (...) {                                                            \
    }                                                                          \
    if (!caught_) {                                                            \
      ++::test::state().failures;                                              \
      std::cerr << __FILE__ << ':' << __LINE__ << ": expected " #ExType        \
                << " from: " << #__VA_ARGS__ << '\n';                          \
    }                                                                          \
  } while (false)

namespace {

using adt::SkipListMap;
using IntMap = SkipListMap<int, int>;
using StrMap = SkipListMap<std::string, int>;

template <typename Map>
std::vector<typename Map::key_type> keysOf(const Map &map) {
  std::vector<typename Map::key_type> keys;
  keys.reserve(map.size());
  for (const auto &entry : map) {
    keys.push_back(entry.first);
  }
  return keys;
}

struct Descending {
  bool operator()(int lhs, int rhs) const noexcept { return lhs > rhs; }
};

struct NonDefaultKey {
  explicit NonDefaultKey(int value) : value(value) {}
  int value;

  friend bool operator<(const NonDefaultKey &lhs,
                        const NonDefaultKey &rhs) noexcept {
    return lhs.value < rhs.value;
  }
};

static_assert(
    std::is_same_v<std::iterator_traits<IntMap::iterator>::iterator_category,
                   std::forward_iterator_tag>);
static_assert(std::is_convertible_v<IntMap::iterator, IntMap::const_iterator>);
static_assert(
    !std::is_convertible_v<IntMap::const_iterator, IntMap::iterator>);
static_assert(std::is_const_v<std::remove_reference_t<
                  decltype(std::declval<IntMap::value_type &>().first)>>);

} // namespace

TEST_CASE(default_construction_and_configuration_validation) {
  const IntMap map;
  CHECK(map.empty());
  CHECK(map.size() == 0);
  CHECK(map.begin() == map.end());
  CHECK(!map.min().has_value());
  CHECK(!map.max().has_value());

  CHECK_THROWS_AS(std::invalid_argument, IntMap(0));
  CHECK_THROWS_AS(std::invalid_argument, IntMap(4, 0.0));
  CHECK_THROWS_AS(std::invalid_argument, IntMap(4, 1.0));
}

TEST_CASE(insert_and_update_keep_unique_ordered_keys) {
  IntMap map(12, 0.5, std::less<int>{}, 7);
  CHECK(map.try_emplace(3, 30).second);
  CHECK(map.try_emplace(1, 10).second);
  CHECK(map.try_emplace(2, 20).second);

  auto duplicate = map.try_emplace(2, 200);
  CHECK(!duplicate.second);
  CHECK(duplicate.first->second == 20);

  auto updated = map.insert_or_assign(2, 22);
  CHECK(!updated.second);
  CHECK(updated.first->second == 22);
  CHECK(map.size() == 3);
  CHECK((keysOf(map) == std::vector<int>{1, 2, 3}));

  CHECK(map.insert({4, 40}).second);
  CHECK(!map.insert({4, 400}).second);
  CHECK(map.at(4) == 40);
}

TEST_CASE(bounds_neighbors_and_checked_lookup) {
  const IntMap map{{2, 20}, {5, 50}, {8, 80}};
  CHECK(map.find(5)->second == 50);
  CHECK(map.find(6) == map.end());
  CHECK(map.contains(8));
  CHECK(!map.contains(9));

  CHECK(map.lower_bound(5)->first == 5);
  CHECK(map.lower_bound(6)->first == 8);
  CHECK(map.lower_bound(9) == map.end());
  CHECK(map.upper_bound(5)->first == 8);
  CHECK(map.upper_bound(6)->first == 8);
  CHECK(map.upper_bound(8) == map.end());

  CHECK(map.predecessor(2) == map.end());
  CHECK(map.predecessor(5)->first == 2);
  CHECK(map.predecessor(6)->first == 5);
  CHECK(map.successor(2)->first == 5);
  CHECK(map.successor(5)->first == 8);
  CHECK(map.successor(8) == map.end());
  CHECK(map.at(5) == 50);
  CHECK_THROWS_AS(std::out_of_range, map.at(42));
}

TEST_CASE(minimum_maximum_and_subscript) {
  StrMap map{{"pear", 3}, {"apple", 1}, {"orange", 2}};
  CHECK(map.min()->first == "apple");
  CHECK(map.min()->second == 1);
  CHECK(map.max()->first == "pear");
  CHECK(map.max()->second == 3);

  CHECK(map["banana"] == 0);
  map["banana"] = 4;
  CHECK(map.at("banana") == 4);
}

TEST_CASE(iterators_allow_mapped_mutation_and_are_convertible) {
  IntMap map{{4, 40}, {1, 10}, {3, 30}, {2, 20}};
  CHECK((keysOf(map) == std::vector<int>{1, 2, 3, 4}));

  for (auto &entry : map) {
    entry.second += 1;
  }
  CHECK(map.at(1) == 11);
  CHECK(map.at(4) == 41);

  IntMap::const_iterator converted = map.begin();
  CHECK(converted == map.cbegin());
  CHECK(std::distance(map.cbegin(), map.cend()) == 4);
}

TEST_CASE(erase_and_clear_leave_map_reusable) {
  IntMap map{{1, 10}, {2, 20}, {3, 30}, {4, 40}};
  CHECK(map.erase(1) == 1);
  CHECK(map.erase(3) == 1);
  CHECK(map.erase(42) == 0);
  CHECK((keysOf(map) == std::vector<int>{2, 4}));
  CHECK(map.erase(4) == 1);
  CHECK(map.erase(2) == 1);
  CHECK(map.empty());

  map.try_emplace(7, 70);
  map.clear();
  CHECK(map.empty());
  map.try_emplace(8, 80);
  CHECK(map.begin()->first == 8);
}

TEST_CASE(copy_move_swap_and_equality) {
  StrMap original{{"x", 1}, {"y", 2}};
  StrMap copy(original);
  CHECK(copy == original);
  copy["x"] = 100;
  CHECK(original.at("x") == 1);

  StrMap assigned{{"stale", 0}};
  assigned = original;
  CHECK(assigned == original);
  assigned = assigned;
  CHECK(assigned == original);

  StrMap moved(std::move(copy));
  CHECK(moved.at("x") == 100);
  CHECK(copy.empty());
  copy.try_emplace("usable", 3);
  CHECK(copy.at("usable") == 3);

  StrMap left{{"a", 1}};
  StrMap right{{"b", 2}, {"c", 3}};
  swap(left, right);
  CHECK((keysOf(left) == std::vector<std::string>{"b", "c"}));
  CHECK((keysOf(right) == std::vector<std::string>{"a"}));
  right = std::move(left);
  CHECK((keysOf(right) == std::vector<std::string>{"b", "c"}));
  CHECK(left.empty());
}

TEST_CASE(custom_ordering_and_non_default_constructible_keys) {
  SkipListMap<int, std::string, Descending> descending;
  descending[1] = "one";
  descending[3] = "three";
  descending[2] = "two";
  CHECK((keysOf(descending) == std::vector<int>{3, 2, 1}));
  CHECK(descending.min()->first == 3);
  CHECK(descending.max()->first == 1);

  SkipListMap<NonDefaultKey, int> nonDefault;
  nonDefault.try_emplace(NonDefaultKey{2}, 20);
  nonDefault.try_emplace(NonDefaultKey{1}, 10);
  CHECK(nonDefault.begin()->first.value == 1);
}

TEST_CASE(move_only_values_are_supported) {
  SkipListMap<int, std::unique_ptr<int>> map;
  auto value = std::make_unique<int>(7);
  CHECK(map.try_emplace(1, std::move(value)).second);
  CHECK(value == nullptr);

  auto unused = std::make_unique<int>(8);
  CHECK(!map.try_emplace(1, std::move(unused)).second);
  CHECK(unused != nullptr);
  CHECK(*map.at(1) == 7);
  map.insert_or_assign(1, std::move(unused));
  CHECK(*map.at(1) == 8);
  CHECK(map.erase(1) == 1);
}

TEST_CASE(stress_operations_match_std_map) {
  constexpr int kCount = 10000;
  IntMap map(24, 0.5, std::less<int>{}, 99);
  std::map<int, int> reference;
  for (int i = kCount - 1; i >= 0; --i) {
    map.insert_or_assign(i, i * 3);
    reference.insert_or_assign(i, i * 3);
  }
  CHECK(map.size() == reference.size());

  auto actual = map.begin();
  auto expected = reference.begin();
  for (; expected != reference.end(); ++actual, ++expected) {
    CHECK(actual->first == expected->first);
    CHECK(actual->second == expected->second);
  }
  CHECK(actual == map.end());

  for (int i = 0; i < kCount; i += 3) {
    CHECK(map.erase(i) == reference.erase(i));
  }
  CHECK(map.size() == reference.size());
  for (const auto &entry : reference) {
    CHECK(map.at(entry.first) == entry.second);
  }
}

int main() {
  int failedCases = 0;
  for (const auto &testCase : test::cases()) {
    const int failuresBefore = test::state().failures;
    try {
      testCase.fn();
    } catch (const std::exception &error) {
      ++test::state().failures;
      std::cerr << testCase.name << ": unexpected exception: " << error.what()
                << '\n';
    } catch (...) {
      ++test::state().failures;
      std::cerr << testCase.name << ": unexpected unknown exception\n";
    }
    failedCases += test::state().failures != failuresBefore;
  }

  std::cout << test::cases().size() << " test cases, " << test::state().checks
            << " checks, " << failedCases << " failures\n";
  return test::state().failures == 0 ? 0 : 1;
}
