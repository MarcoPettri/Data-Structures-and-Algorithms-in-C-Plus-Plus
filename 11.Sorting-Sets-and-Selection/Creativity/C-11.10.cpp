// File : C-11.10.cpp

/*
    C-11.10 Describe a nonrecursive, in-place version of the quick-sort algorithm. The
    algorithm should still be based on the same divide-and-conquer approach,
    but use an explicit stack to process subproblems.
*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <random>
#include <chrono>
#include "iterative_quicksort.hpp"

void run_tests() {
    std::cout << "[*] Running functional unit tests for Iterative QuickSort...\n";

    // Test 1: Standard Unsorted Vector
    {
        std::vector<int> data = {9, 1, 5, 2, 8, 3, 7, 4, 6, 0};
        dsa::iterative_quicksort(data.begin(), data.end());
        assert(std::is_sorted(data.begin(), data.end()));
    }

    // Test 2: Vector with Duplicate Entries
    {
        std::vector<int> data = {5, 2, 2, 8, 5, 1, 9, 2};
        dsa::iterative_quicksort(data.begin(), data.end());
        assert(std::is_sorted(data.begin(), data.end()));
    }

    // Test 3: Already Sorted Sequence
    {
        std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        dsa::iterative_quicksort(data.begin(), data.end());
        assert(std::is_sorted(data.begin(), data.end()));
    }

    // Test 4: Reverse Sorted Sequence
    {
        std::vector<int> data = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
        dsa::iterative_quicksort(data.begin(), data.end());
        assert(std::is_sorted(data.begin(), data.end()));
    }

    // Test 5: Single Element & Empty Sequences
    {
        std::vector<int> single = {42};
        dsa::iterative_quicksort(single.begin(), single.end());
        assert(single == (std::vector<int>{42}));

        std::vector<int> empty = {};
        dsa::iterative_quicksort(empty.begin(), empty.end());
        assert(empty.empty());
    }

    // Test 6: Custom Comparator (Descending Order)
    {
        std::vector<int> data = {1, 5, 3, 9, 2};
        dsa::iterative_quicksort(data.begin(), data.end(), std::greater<int>{});
        assert(std::is_sorted(data.begin(), data.end(), std::greater<int>{}));
    }

    std::cout << "[+] All functional unit tests passed successfully!\n";
}

void run_stress_test() {
    std::cout << "[*] Running performance stress test (N = 1,000,000)...\n";

    constexpr size_t N = 1'000'000;
    std::vector<int> data(N);

    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist(-10'000'000, 10'000'000);
    for (size_t i = 0; i < N; ++i) {
        data[i] = dist(rng);
    }

    auto start = std::chrono::high_resolution_clock::now();
    dsa::iterative_quicksort(data.begin(), data.end());
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "[+] Iterative QuickSort completed N = 1,000,000 in " << elapsed.count() << " ms.\n";

    assert(std::is_sorted(data.begin(), data.end()));
    std::cout << "[+] Output correctness verified (fully sorted).\n";
}

int main() {
    run_tests();
    run_stress_test();
    std::cout << "[SUCCESS] All Iterative QuickSort verifications passed without errors.\n";
    return 0;
}