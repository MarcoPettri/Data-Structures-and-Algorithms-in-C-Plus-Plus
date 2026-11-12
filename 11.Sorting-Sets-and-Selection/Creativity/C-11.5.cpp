// File : C-11.5.cpp

/*
    C-11.5 Describe and analyze an efficient function for removing all duplicates
    from a collection A of n elements.
*/


#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <chrono>
#include <random>
#include <algorithm>
#include "remove_duplicates.hpp"

void run_tests() {
    std::cout << "[*] Running unit tests for remove_duplicates...\n";

    // Test 1: Unsorted collection with mixed duplicates, verifying order preservation
    {
        std::vector<int> data = {4, 1, 2, 1, 3, 4, 2, 5};
        auto result = dsa::remove_duplicates(data.begin(), data.end());
        std::vector<int> expected = {4, 1, 2, 3, 5};
        assert(result == expected);
    }

    // Test 2: Empty collection
    {
        std::vector<int> data = {};
        auto result = dsa::remove_duplicates(data.begin(), data.end());
        assert(result.empty());
    }

    // Test 3: All elements unique
    {
        std::vector<char> data = {'a', 'b', 'c', 'd'};
        auto result = dsa::remove_duplicates(data.begin(), data.end());
        assert(result == data);
    }

    // Test 4: All elements identical
    {
        std::vector<std::string> data = {"apple", "apple", "apple"};
        auto result = dsa::remove_duplicates(data.begin(), data.end());
        assert(result == (std::vector<std::string>{"apple"}));
    }

    std::cout << "[+] All functional unit tests passed successfully!\n";
}

void run_stress_test() {
    std::cout << "[*] Running performance stress test (N = 1,000,000 elements)...\n";

    constexpr size_t N = 1'000'000;
    std::vector<int> data(N);

    // Populate with highly repetitive data (cycling through 1,000 unique values)
    for (size_t i = 0; i < N; ++i) {
        data[i] = static_cast<int>(i % 1000);
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto result = dsa::remove_duplicates(data.begin(), data.end());
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "[+] Stress test completed in " << elapsed.count() << " ms.\n";
    std::cout << "[+] Resulting unique count: " << result.size() << " elements.\n";

    // Verify exact uniqueness and size
    assert(result.size() == 1000);
    std::unordered_set<int> unique_check(result.begin(), result.end());
    assert(unique_check.size() == 1000);

    std::cout << "[+] Stress test correctness verified.\n";
}

int main() {
    run_tests();
    run_stress_test();
    std::cout << "[SUCCESS] All remove_duplicates verifications passed without errors.\n";
    return 0;
}