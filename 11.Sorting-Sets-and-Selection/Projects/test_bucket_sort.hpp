/**
 * @file test_bucket_sort.hpp
 * @brief Self-contained unit tests for algo::bucket_sort (no external framework).
 *
 */

#include "bucket_sort.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_bucket_sort{

int g_checks = 0;
int g_failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(cond)) {                                                         \
            ++g_failures;                                                      \
            std::cerr << "  FAIL " << __FILE__ << ':' << __LINE__ << "  "      \
                      << #cond << '\n';                                        \
        }                                                                      \
    } while (false)

#define CHECK_THROWS_AS(expr, ExType)                                          \
    do {                                                                       \
        ++g_checks;                                                            \
        bool caught_ = false;                                                  \
        try {                                                                  \
            expr;                                                              \
        } catch (const ExType&) {                                              \
            caught_ = true;                                                    \
        } catch (...) {                                                        \
        }                                                                      \
        if (!caught_) {                                                        \
            ++g_failures;                                                      \
            std::cerr << "  FAIL " << __FILE__ << ':' << __LINE__              \
                      << "  expected " #ExType " from " #expr "\n";            \
        }                                                                      \
    } while (false)

using IntList = std::list<int>;

/// Runs one test function and reports it.
void run(const char* name, void (*fn)()) {
    const int before = g_failures;
    fn();
    std::cout << (g_failures == before ? "[ OK ] " : "[FAIL] ") << name << '\n';
}

bool is_sorted_list(const IntList& l) { return std::is_sorted(l.begin(), l.end()); }

IntList random_list(std::size_t n, int N, std::uint32_t seed) {
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, N - 1);
    IntList l;
    for (std::size_t i = 0; i < n; ++i) l.push_back(dist(gen));
    return l;
}

// ---------------------------------------------------------------------------
// Basic cases
// ---------------------------------------------------------------------------

void test_empty_and_single() {
    IntList empty;
    algo::bucket_sort(empty, 10);
    CHECK(empty.empty());

    IntList one{7};
    algo::bucket_sort(one, 10);
    CHECK(one == IntList{7});
}

void test_small_known_input() {
    IntList l{5, 3, 9, 0, 3, 7, 1, 9, 0};
    algo::bucket_sort(l, 10);
    CHECK((l == IntList{0, 0, 1, 3, 3, 5, 7, 9, 9}));
}

void test_already_sorted_and_reversed() {
    IntList asc{0, 1, 2, 3, 4, 5};
    algo::bucket_sort(asc, 6);
    CHECK((asc == IntList{0, 1, 2, 3, 4, 5}));

    IntList desc{5, 4, 3, 2, 1, 0};
    algo::bucket_sort(desc, 6);
    CHECK((desc == IntList{0, 1, 2, 3, 4, 5}));
}

void test_all_equal() {
    IntList l(100, 3);
    algo::bucket_sort(l, 4);
    CHECK(l.size() == 100);
    CHECK(std::all_of(l.begin(), l.end(), [](int v) { return v == 3; }));
}

void test_minimum_universe_N2() {
    IntList l{1, 0, 1, 1, 0, 0, 1};
    algo::bucket_sort(l, 2);
    CHECK((l == IntList{0, 0, 0, 1, 1, 1, 1}));
}

void test_boundary_keys() {
    IntList l{99, 0, 99, 0, 50};
    algo::bucket_sort(l, 100);
    CHECK((l == IntList{0, 0, 50, 99, 99}));
}

// ---------------------------------------------------------------------------
// Randomised comparison against std::sort
// ---------------------------------------------------------------------------

void test_random_against_std_sort() {
    struct Case { std::size_t n; int N; };
    const Case cases[] = {{10, 2}, {100, 5}, {1000, 1000}, {5000, 3}, {2000, 100000}};
    std::uint32_t seed = 1;
    for (const Case& c : cases) {
        IntList l = random_list(c.n, c.N, seed++);
        std::vector<int> ref(l.begin(), l.end());
        std::sort(ref.begin(), ref.end());

        algo::bucket_sort(l, static_cast<std::size_t>(c.N));

        CHECK(l.size() == c.n);
        CHECK(std::equal(l.begin(), l.end(), ref.begin(), ref.end()));
    }
}

void test_large_input() {
    IntList l = random_list(1'000'000, 1024, 42);
    algo::bucket_sort(l, 1024);
    CHECK(l.size() == 1'000'000);
    CHECK(is_sorted_list(l));
}

// ---------------------------------------------------------------------------
// Generic / key-extractor behaviour
// ---------------------------------------------------------------------------

struct Item {
    int key;
    int seq;  // original position, used to verify stability
};

void test_stability_with_key_extractor() {
    constexpr int N = 8;
    std::mt19937 gen(7);
    std::uniform_int_distribution<int> dist(0, N - 1);

    std::list<Item> l;
    for (int i = 0; i < 2000; ++i) l.push_back({dist(gen), i});

    algo::bucket_sort(l, N, [](const Item& it) { return it.key; });

    CHECK(l.size() == 2000);
    bool ordered = true;
    for (auto a = l.begin(), b = std::next(a); b != l.end(); ++a, ++b) {
        if (a->key > b->key || (a->key == b->key && a->seq > b->seq)) {
            ordered = false;
            break;
        }
    }
    CHECK(ordered);
}

void test_various_key_types() {
    std::list<std::string> words{"ccc", "a", "bb", "", "dddd", "bb", "a"};
    // key = length (std::size_t), universe [0, 4]
    algo::bucket_sort(words, 5, [](const std::string& s) { return s.size(); });
    CHECK((words == std::list<std::string>{"", "a", "a", "bb", "bb", "ccc", "dddd"}));

    std::list<unsigned char> bytes{200, 3, 255, 0, 3};
    algo::bucket_sort(bytes, 256, [](unsigned char b) { return b; });
    CHECK((bytes == std::list<unsigned char>{0, 3, 3, 200, 255}));
}

// ---------------------------------------------------------------------------
// Node-relinking guarantees (no copies / moves, pointers stay valid)
// ---------------------------------------------------------------------------

void test_nodes_are_relinked_not_copied() {
    IntList l = random_list(500, 20, 99);

    std::vector<const int*> before;
    for (const int& v : l) before.push_back(&v);

    algo::bucket_sort(l, 20);

    std::vector<const int*> after;
    for (const int& v : l) after.push_back(&v);

    std::sort(before.begin(), before.end(), std::less<const int*>());
    std::sort(after.begin(), after.end(), std::less<const int*>());
    CHECK(before == after);  // exactly the same node addresses
}

struct NoCopy {
    int key;
    NoCopy(int k) : key(k) {}
    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
    NoCopy(NoCopy&&) = delete;
    NoCopy& operator=(NoCopy&&) = delete;
};

void test_works_with_non_copyable_non_movable_type() {
    std::list<NoCopy> l;
    for (int k : {3, 1, 2, 0, 3, 1}) l.emplace_back(k);

    algo::bucket_sort(l, 4, [](const NoCopy& x) { return x.key; });

    std::vector<int> keys;
    for (const NoCopy& x : l) keys.push_back(x.key);
    CHECK((keys == std::vector<int>{0, 1, 1, 2, 3, 3}));
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

void test_invalid_N() {
    IntList l{0, 1};
    CHECK_THROWS_AS(algo::bucket_sort(l, 0), std::invalid_argument);
    CHECK_THROWS_AS(algo::bucket_sort(l, 1), std::invalid_argument);
    CHECK((l == IntList{0, 1}));  // untouched

    IntList empty;
    CHECK_THROWS_AS(algo::bucket_sort(empty, 1), std::invalid_argument);
}

void test_out_of_range_keys() {
    IntList too_big{1, 2, 10, 0};
    CHECK_THROWS_AS(algo::bucket_sort(too_big, 10), std::out_of_range);

    IntList negative{1, -1, 0};
    CHECK_THROWS_AS(algo::bucket_sort(negative, 10), std::out_of_range);
}

void test_list_intact_after_invalid_key() {
    IntList l{4, 1, 7, 100, 2, 9, 3};  // 100 is invalid for N = 10
    std::vector<int> expected(l.begin(), l.end());
    std::sort(expected.begin(), expected.end());

    CHECK_THROWS_AS(algo::bucket_sort(l, 10), std::out_of_range);

    std::vector<int> got(l.begin(), l.end());
    std::sort(got.begin(), got.end());
    CHECK(got == expected);  // same multiset: nothing lost or duplicated
}

void test_list_intact_after_throwing_extractor() {
    IntList l = random_list(100, 10, 5);
    std::vector<int> expected(l.begin(), l.end());
    std::sort(expected.begin(), expected.end());

    int calls = 0;
    auto key = [&calls](int v) -> int {
        if (++calls == 37) throw std::runtime_error("boom");
        return v;
    };
    CHECK_THROWS_AS(algo::bucket_sort(l, 10, key), std::runtime_error);

    std::vector<int> got(l.begin(), l.end());
    std::sort(got.begin(), got.end());
    CHECK(got == expected);
}

}  // namespace test_bucket_sort