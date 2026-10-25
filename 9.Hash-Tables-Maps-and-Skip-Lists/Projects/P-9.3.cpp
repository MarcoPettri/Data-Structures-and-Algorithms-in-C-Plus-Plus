// File : P-9.3.cpp

/*
    P-9.3 Write an implementation of the map ADT using a vector.
*/

#include "vector_dictionary.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ===========================================================================
// Minimal test harness
// ===========================================================================
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

using adt::VectorDictionary;
using IntDict = VectorDictionary<int, int>;
using StrDict = VectorDictionary<std::string, int>;

template <typename Dict>
std::vector<typename Dict::key_type> keysOf(const Dict &dict) {
  std::vector<typename Dict::key_type> keys;
  keys.reserve(dict.size());
  for (const auto &entry : dict) {
    keys.push_back(entry.first);
  }
  return keys;
}

struct CaseInsensitiveEqual {
  bool operator()(const std::string &lhs,
                  const std::string &rhs) const noexcept {
    if (lhs.size() != rhs.size()) {
      return false;
    }
    for (std::size_t i = 0; i < lhs.size(); ++i) {
      const auto lower = [](unsigned char ch) {
        return ch >= 'A' && ch <= 'Z'
                   ? static_cast<unsigned char>(ch + 'a' - 'A')
                   : ch;
      };
      if (lower(static_cast<unsigned char>(lhs[i])) !=
          lower(static_cast<unsigned char>(rhs[i]))) {
        return false;
      }
    }
    return true;
  }
};

static_assert(
    std::is_same_v<std::iterator_traits<IntDict::iterator>::iterator_category,
                   std::random_access_iterator_tag>);
static_assert(std::is_const_v<std::remove_reference_t<
                  decltype(std::declval<IntDict::value_type &>().first)>>);
static_assert(
    std::is_convertible_v<IntDict::iterator, IntDict::const_iterator>);
static_assert(
    !std::is_convertible_v<IntDict::const_iterator, IntDict::iterator>);

} // namespace

TEST_CASE(default_constructed_dictionary_is_empty) {
  const IntDict dict;
  CHECK(dict.empty());
  CHECK(dict.size() == 0);
  CHECK(dict.begin() == dict.end());
  CHECK(dict.find(1) == dict.end());
  CHECK(!dict.contains(1));
}

TEST_CASE(try_emplace_inserts_once_and_preserves_order) {
  IntDict dict;
  const auto first = dict.try_emplace(2, 20);
  CHECK(first.second);
  CHECK(first.first->first == 2);
  CHECK(first.first->second == 20);

  const auto duplicate = dict.try_emplace(2, 200);
  CHECK(!duplicate.second);
  CHECK(duplicate.first == first.first);
  CHECK(dict.at(2) == 20);

  dict.try_emplace(1, 10);
  dict.try_emplace(3, 30);
  CHECK((keysOf(dict) == std::vector<int>{2, 1, 3}));
}

TEST_CASE(insert_and_insert_or_assign) {
  StrDict dict;
  CHECK(dict.insert({"one", 1}).second);
  CHECK(!dict.insert({"one", 10}).second);

  auto inserted = dict.insert_or_assign("two", 2);
  CHECK(inserted.second);
  auto assigned = dict.insert_or_assign("two", 22);
  CHECK(!assigned.second);
  CHECK(assigned.first == inserted.first);
  CHECK(dict.at("two") == 22);
  CHECK(dict.size() == 2);
}

TEST_CASE(subscript_and_at_access_values) {
  StrDict dict;
  CHECK(dict["new"] == 0);
  dict["new"] = 4;
  dict["another"] += 3;
  CHECK(dict.at("new") == 4);
  CHECK(dict.at("another") == 3);

  const StrDict &view = dict;
  CHECK(view.at("new") == 4);
  CHECK_THROWS_AS(std::out_of_range, dict.at("missing"));
  CHECK_THROWS_AS(std::out_of_range, view.at("missing"));
}

TEST_CASE(find_contains_and_mutable_mapped_value) {
  IntDict dict{{1, 10}, {2, 20}};
  CHECK(dict.contains(1));
  CHECK(!dict.contains(3));

  auto found = dict.find(2);
  CHECK(found != dict.end());
  found->second = 25;
  CHECK(dict.at(2) == 25);
  CHECK(dict.find(3) == dict.end());
}

TEST_CASE(erase_preserves_remaining_insertion_order) {
  IntDict dict{{1, 10}, {2, 20}, {3, 30}, {4, 40}};
  CHECK(dict.erase(1) == 1);
  CHECK(dict.erase(3) == 1);
  CHECK(dict.erase(42) == 0);
  CHECK((keysOf(dict) == std::vector<int>{2, 4}));

  CHECK(dict.erase(2) == 1);
  CHECK(dict.erase(4) == 1);
  CHECK(dict.empty());
  dict.try_emplace(5, 50);
  CHECK((keysOf(dict) == std::vector<int>{5}));

  dict.clear();
  CHECK(dict.empty());
  dict.try_emplace(6, 60);
  CHECK(dict.at(6) == 60);
}

TEST_CASE(iteration_exposes_const_keys_and_mutable_values) {
  StrDict dict{{"c", 3}, {"a", 1}, {"b", 2}};
  CHECK((keysOf(dict) == std::vector<std::string>{"c", "a", "b"}));
  for (auto &entry : dict) {
    entry.second *= 10;
  }
  CHECK(dict.at("a") == 10);
  CHECK(dict.at("b") == 20);
  CHECK(dict.at("c") == 30);

  StrDict::const_iterator converted = dict.begin();
  CHECK(converted == dict.cbegin());
  CHECK(dict.begin() == dict.cbegin());
  CHECK(std::distance(dict.cbegin(), dict.cend()) == 3);
}

TEST_CASE(initializer_list_copy_move_and_equality) {
  const StrDict initial{{"x", 1}, {"y", 2}, {"x", 99}};
  CHECK(initial.size() == 2);
  CHECK(initial.at("x") == 1);

  StrDict copy(initial);
  CHECK(copy == initial);
  copy["x"] = 100;
  copy["z"] = 3;
  CHECK(initial.at("x") == 1);
  CHECK(copy != initial);

  StrDict assigned{{"stale", 9}};
  assigned = initial;
  CHECK(assigned == initial);
  assigned = assigned;
  CHECK(assigned == initial);

  StrDict moved(std::move(copy));
  CHECK(moved.size() == 3);
  CHECK(copy.empty());
  copy.try_emplace("usable", 7);
  CHECK(copy.at("usable") == 7);

  StrDict target{{"old", 0}};
  target = std::move(moved);
  CHECK(target.size() == 3);
  CHECK(moved.empty());

  StrDict other{{"q", 8}};
  swap(target, other);
  CHECK(target.size() == 1);
  CHECK(other.size() == 3);
  CHECK((keysOf(other) == std::vector<std::string>{"x", "y", "z"}));
}

TEST_CASE(custom_key_equivalence_is_used) {
  VectorDictionary<std::string, int, CaseInsensitiveEqual> dict;
  dict.try_emplace("Hello", 1);
  dict.insert_or_assign("HELLO", 2);
  CHECK(dict.size() == 1);
  CHECK(dict.at("hello") == 2);
  CHECK(dict.contains("hElLo"));
  CHECK(dict.key_eq()("a", "A"));
  CHECK(dict.erase("HELLO") == 1);
  CHECK(dict.empty());
}

TEST_CASE(move_only_mapped_values_are_supported) {
  VectorDictionary<int, std::unique_ptr<int>> dict;
  auto first = std::make_unique<int>(7);
  CHECK(dict.try_emplace(1, std::move(first)).second);
  CHECK(first == nullptr);

  auto duplicateValue = std::make_unique<int>(8);
  CHECK(!dict.try_emplace(1, std::move(duplicateValue)).second);
  CHECK(duplicateValue != nullptr);
  CHECK(*dict.at(1) == 7);

  dict.insert_or_assign(1, std::move(duplicateValue));
  CHECK(*dict.at(1) == 8);
  CHECK(dict.erase(1) == 1);
}

TEST_CASE(stress_vector_growth_and_erasure) {
  constexpr int count = 1500;
  IntDict dict;
  for (int i = 0; i < count; ++i) {
    dict.try_emplace(i, i * 2);
  }
  CHECK(dict.size() == static_cast<std::size_t>(count));
  CHECK(dict.at(0) == 0);
  CHECK(dict.at(count - 1) == (count - 1) * 2);

  for (int i = 0; i < count; i += 2) {
    CHECK(dict.erase(i) == 1);
  }
  CHECK(dict.size() == static_cast<std::size_t>(count / 2));
  CHECK(!dict.contains(0));
  CHECK(dict.contains(1));

  long long sum = 0;
  for (const auto &entry : dict) {
    sum += entry.second;
  }
  CHECK(sum > 0);
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

    const bool passed = test::state().failures == failuresBefore;
    failedCases += passed ? 0 : 1;
    std::cout << (passed ? "[PASS] " : "[FAIL] ") << testCase.name << '\n';
  }

  std::cout << test::state().checks << " checks, " << failedCases
            << " failed test case(s)\n";
  return failedCases == 0 ? 0 : 1;
}
