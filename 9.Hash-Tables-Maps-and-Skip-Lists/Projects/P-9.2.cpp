// File : P-9.2.cpp

/*
    P-9.2 Write an implementation of the dictionary ADT using a linked list.
*/

#include "linked_list_dictionary.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
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

/** @brief Global counters shared by all checks. */
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

/** @brief Registers a test case at static-initialisation time. */
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

// ===========================================================================
// Test fixtures and helpers
// ===========================================================================
namespace {

using adt::LinkedListDictionary;
using IntDict = LinkedListDictionary<int, int>;
using StrDict = LinkedListDictionary<std::string, int>;

/** @brief Collects keys in iteration order. */
template <typename Dict>
std::vector<typename Dict::key_type> keysOf(const Dict &dict) {
  std::vector<typename Dict::key_type> keys;
  keys.reserve(dict.size());
  for (const auto &entry : dict) {
    keys.push_back(entry.first);
  }
  return keys;
}

/** @brief Value type that counts live instances and can fail on demand when
 * copied. */
struct Tracked {
  static inline int live = 0;
  static inline int copiesBeforeThrow = -1; // -1: never throw

  int value;

  explicit Tracked(int v = 0) : value(v) { ++live; }
  Tracked(const Tracked &other) : value(other.value) {
    if (copiesBeforeThrow == 0) {
      throw std::runtime_error("injected copy failure");
    }
    if (copiesBeforeThrow > 0) {
      --copiesBeforeThrow;
    }
    ++live;
  }
  Tracked(Tracked &&other) noexcept : value(other.value) { ++live; }
  Tracked &operator=(const Tracked &) = default;
  Tracked &operator=(Tracked &&) noexcept = default;
  ~Tracked() { --live; }

  friend bool operator==(const Tracked &a, const Tracked &b) {
    return a.value == b.value;
  }
};

using TrackedDict = LinkedListDictionary<int, Tracked>;

/** @brief Case-insensitive ASCII key comparison for the custom-predicate test.
 */
struct CaseInsensitiveEqual {
  bool operator()(const std::string &a, const std::string &b) const noexcept {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
             return std::tolower(static_cast<unsigned char>(x)) ==
                    std::tolower(static_cast<unsigned char>(y));
           });
  }
};

// ---------------------------------------------------------------------------
// Compile-time properties
// ---------------------------------------------------------------------------
static_assert(std::is_nothrow_default_constructible_v<IntDict>);
static_assert(std::is_nothrow_move_constructible_v<IntDict>);
static_assert(std::is_nothrow_move_assignable_v<IntDict>);
static_assert(std::is_nothrow_swappable_v<IntDict>);
static_assert(
    std::is_same_v<std::iterator_traits<IntDict::iterator>::iterator_category,
                   std::forward_iterator_tag>);
static_assert(
    std::is_convertible_v<IntDict::iterator, IntDict::const_iterator>);
static_assert(
    !std::is_convertible_v<IntDict::const_iterator, IntDict::iterator>);

} // namespace

// ===========================================================================
// Test cases
// ===========================================================================

TEST_CASE(default_constructed_is_empty) {
  const IntDict dict;
  CHECK(dict.empty());
  CHECK(dict.size() == 0);
  CHECK(dict.begin() == dict.end());
  CHECK(!dict.contains(1));
  CHECK(dict.find(1) == dict.end());
}

TEST_CASE(try_emplace_inserts_new_and_keeps_existing) {
  IntDict dict;
  const auto first = dict.try_emplace(1, 100);
  CHECK(first.second);
  CHECK(first.first->first == 1);
  CHECK(first.first->second == 100);

  const auto second = dict.try_emplace(1, 999);
  CHECK(!second.second);
  CHECK(second.first == first.first);
  CHECK(dict.at(1) == 100);
  CHECK(dict.size() == 1);
}

TEST_CASE(insert_never_overwrites) {
  StrDict dict;
  CHECK(dict.insert({"one", 1}).second);
  CHECK(!dict.insert({"one", 11}).second);

  const StrDict::value_type entry{"two", 2};
  CHECK(dict.insert(entry).second);

  CHECK(dict.size() == 2);
  CHECK(dict.at("one") == 1);
  CHECK(dict.at("two") == 2);
}

TEST_CASE(insert_or_assign_overwrites) {
  StrDict dict;
  auto inserted = dict.insert_or_assign("k", 1);
  CHECK(inserted.second);
  auto assigned = dict.insert_or_assign("k", 2);
  CHECK(!assigned.second);
  CHECK(assigned.first == inserted.first);
  CHECK(dict.at("k") == 2);
  CHECK(dict.size() == 1);

  std::string key = "movable";
  dict.insert_or_assign(std::move(key), 5);
  CHECK(dict.at("movable") == 5);
}

TEST_CASE(subscript_operator_inserts_default_and_allows_update) {
  StrDict dict;
  CHECK(dict["a"] == 0); // value-initialised
  dict["a"] = 7;
  dict["b"] += 3;
  CHECK(dict.at("a") == 7);
  CHECK(dict.at("b") == 3);
  CHECK(dict.size() == 2);
}

TEST_CASE(at_throws_out_of_range_for_missing_key) {
  IntDict dict{{1, 1}};
  const IntDict &view = dict;
  CHECK_THROWS_AS(std::out_of_range, dict.at(2));
  CHECK_THROWS_AS(std::out_of_range, view.at(2));
  CHECK(view.at(1) == 1);
}

TEST_CASE(find_and_contains) {
  IntDict dict{{1, 10}, {2, 20}, {3, 30}};
  CHECK(dict.contains(2));
  CHECK(!dict.contains(4));

  const auto it = dict.find(3);
  CHECK(it != dict.end());
  CHECK(it->second == 30);

  dict.find(1)->second = 11; // mutable access through non-const find
  CHECK(dict.at(1) == 11);
}

TEST_CASE(erase_head_middle_tail_and_missing) {
  IntDict dict{{1, 10}, {2, 20}, {3, 30}, {4, 40}};
  CHECK(dict.erase(1) == 1);  // head
  CHECK(dict.erase(3) == 1);  // middle
  CHECK(dict.erase(4) == 1);  // tail
  CHECK(dict.erase(42) == 0); // missing
  CHECK(dict.size() == 1);
  CHECK(keysOf(dict) == std::vector<int>{2});

  dict.try_emplace(5, 50); // must be appended after the *new* tail
  CHECK((keysOf(dict) == std::vector<int>{2, 5}));

  CHECK(dict.erase(2) == 1);
  CHECK(dict.erase(5) == 1);
  CHECK(dict.empty());
  CHECK(dict.begin() == dict.end());

  dict.try_emplace(7, 70); // list must be fully reusable after becoming empty
  CHECK(keysOf(dict) == std::vector<int>{7});
}

TEST_CASE(clear_empties_and_dictionary_stays_usable) {
  IntDict dict{{1, 1}, {2, 2}};
  dict.clear();
  CHECK(dict.empty());
  CHECK(dict.size() == 0);
  dict.try_emplace(3, 3);
  CHECK(keysOf(dict) == std::vector<int>{3});
}

TEST_CASE(iteration_follows_insertion_order_and_allows_value_mutation) {
  StrDict dict;
  dict["c"] = 3;
  dict["a"] = 1;
  dict["b"] = 2;
  CHECK((keysOf(dict) == std::vector<std::string>{"c", "a", "b"}));

  for (auto &entry : dict) {
    entry.second *= 10;
  }
  CHECK(dict.at("a") == 10);
  CHECK(dict.at("b") == 20);
  CHECK(dict.at("c") == 30);

  StrDict::const_iterator converted =
      dict.begin(); // implicit iterator -> const_iterator
  CHECK(converted == dict.cbegin());
  CHECK(dict.begin() == dict.cbegin()); // mixed comparison
  CHECK(std::distance(dict.cbegin(), dict.cend()) == 3);
}

TEST_CASE(insertion_does_not_invalidate_references) {
  IntDict dict;
  int &first = dict.try_emplace(1, 10).first->second;
  for (int i = 2; i < 200; ++i) {
    dict.try_emplace(i, i);
  }
  first = 11;
  CHECK(dict.at(1) == 11);
}

TEST_CASE(initializer_list_first_duplicate_wins) {
  const StrDict dict{{"a", 1}, {"b", 2}, {"a", 99}};
  CHECK(dict.size() == 2);
  CHECK(dict.at("a") == 1);
}

TEST_CASE(copy_is_deep_and_independent) {
  StrDict original{{"x", 1}, {"y", 2}};
  StrDict copy(original);
  CHECK(copy == original);

  copy["x"] = 100;
  copy["z"] = 3;
  CHECK(original.at("x") == 1);
  CHECK(!original.contains("z"));
  CHECK((keysOf(copy) == std::vector<std::string>{"x", "y", "z"}));

  StrDict assigned;
  assigned["stale"] = 0;
  assigned = original;
  CHECK(assigned == original);
  CHECK(!assigned.contains("stale"));

  const StrDict &alias = assigned;
  assigned = alias; // self-assignment is a no-op
  CHECK(assigned == original);
}

TEST_CASE(move_leaves_source_empty_but_usable) {
  StrDict source{{"x", 1}, {"y", 2}};
  StrDict target(std::move(source));
  CHECK(target.size() == 2);
  CHECK(source.empty()); // NOLINT: moved-from state is specified by this class

  source.try_emplace("z", 3);
  CHECK(keysOf(source) == std::vector<std::string>{"z"});

  StrDict other{{"old", 0}};
  other = std::move(target);
  CHECK((keysOf(other) == std::vector<std::string>{"x", "y"}));
  CHECK(target.empty()); // NOLINT

  other.try_emplace("w",
                    4); // tail pointer must have been transferred correctly
  CHECK((keysOf(other) == std::vector<std::string>{"x", "y", "w"}));
}

TEST_CASE(swap_exchanges_contents) {
  IntDict a{{1, 1}, {2, 2}};
  IntDict b{{9, 9}};
  swap(a, b);
  CHECK(keysOf(a) == std::vector<int>{9});
  CHECK((keysOf(b) == std::vector<int>{1, 2}));
  a.try_emplace(10, 10);
  b.try_emplace(3, 3);
  CHECK((keysOf(a) == std::vector<int>{9, 10}));
  CHECK((keysOf(b) == std::vector<int>{1, 2, 3}));
}

TEST_CASE(equality_ignores_insertion_order) {
  IntDict a{{1, 1}, {2, 2}};
  IntDict b{{2, 2}, {1, 1}};
  CHECK(a == b);
  b[2] = 3;
  CHECK(a != b);
  b[2] = 2;
  b.erase(1);
  CHECK(a != b); // different sizes
}

TEST_CASE(custom_key_equal_is_respected) {
  LinkedListDictionary<std::string, int, CaseInsensitiveEqual> dict;
  dict["Hello"] = 1;
  dict["HELLO"] = 2; // equivalent key: updates, does not insert
  CHECK(dict.size() == 1);
  CHECK(dict.at("hello") == 2);
  CHECK(dict.contains("hElLo"));
  CHECK(dict.key_eq()("a", "A"));
  CHECK(dict.erase("HELLO") == 1);
  CHECK(dict.empty());
}

TEST_CASE(move_only_mapped_type_is_supported) {
  LinkedListDictionary<int, std::unique_ptr<int>> dict;

  auto first = std::make_unique<int>(7);
  CHECK(dict.try_emplace(1, std::move(first)).second);
  CHECK(first == nullptr);

  auto second = std::make_unique<int>(8);
  CHECK(!dict.try_emplace(1, std::move(second)).second);
  CHECK(second != nullptr); // not consumed when the key already exists
  CHECK(*dict.at(1) == 7);

  dict.insert_or_assign(1, std::move(second));
  CHECK(*dict.at(1) == 8);
  CHECK(dict.erase(1) == 1);
}

TEST_CASE(no_leaks_or_double_destruction) {
  Tracked::live = 0;
  {
    TrackedDict dict;
    for (int i = 0; i < 50; ++i) {
      dict.try_emplace(i, i);
    }
    CHECK(Tracked::live == 50);

    dict.erase(0);
    dict.erase(25);
    dict.erase(49);
    CHECK(Tracked::live == 47);

    TrackedDict copy(dict);
    CHECK(Tracked::live == 94);
    CHECK(copy == dict);

    dict.clear();
    CHECK(Tracked::live == 47);

    dict = std::move(copy);
    CHECK(Tracked::live == 47);
  }
  CHECK(Tracked::live == 0);
}

TEST_CASE(copy_constructor_is_exception_safe) {
  Tracked::live = 0;
  {
    TrackedDict dict;
    for (int i = 0; i < 5; ++i) {
      dict.try_emplace(i, i);
    }
    CHECK(Tracked::live == 5);

    Tracked::copiesBeforeThrow = 3; // the 4th element copy fails
    CHECK_THROWS_AS(std::runtime_error, TrackedDict(dict));
    Tracked::copiesBeforeThrow = -1;

    CHECK(Tracked::live == 5); // the partial copy was fully cleaned up
    CHECK(dict.size() == 5);

    TrackedDict target{{100, Tracked{100}}};
    Tracked::copiesBeforeThrow = 2;
    CHECK_THROWS_AS(std::runtime_error, target = dict);
    Tracked::copiesBeforeThrow = -1;
    CHECK(target.size() == 1); // strong guarantee: target unchanged
    CHECK(target.at(100).value == 100);
  }
  CHECK(Tracked::live == 0);
}

TEST_CASE(stress_many_entries) {
  constexpr int kCount = 20000;
  IntDict dict;
  for (int i = 0; i < kCount; ++i) {
    dict.try_emplace(i, i * 2);
  }
  CHECK(dict.size() == std::size_t{kCount});
  CHECK(dict.at(0) == 0);
  CHECK(dict.at(kCount - 1) == (kCount - 1) * 2);

  for (int i = 0; i < kCount;
       i += 2) { // always erases the current head: O(1) each
    dict.erase(i);
  }
  CHECK(dict.size() == std::size_t{kCount / 2});
  CHECK(!dict.contains(0));
  CHECK(dict.contains(1));

  long long sum = 0;
  for (const auto &entry : dict) {
    sum += entry.second;
  }
  CHECK(sum > 0);

  dict.clear();
  CHECK(dict.empty());
}

// ===========================================================================
// Entry point
// ===========================================================================
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
    std::cout << (passed ? "[ PASS ] " : "[ FAIL ] ") << testCase.name << '\n';
  }

  std::cout << '\n'
            << test::cases().size() << " test cases, " << test::state().checks
            << " checks, " << failedCases << " failed\n";
  return failedCases == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}