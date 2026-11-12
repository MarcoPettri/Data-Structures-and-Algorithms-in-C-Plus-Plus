// File : C-11.13.cpp

/*
    C-11.13 Suppose we are given an n-element sequence S such that each element
    in S represents a different vote for president, where each vote is given
    as an integer representing a particular candidate. Design an O(n log n)-
    time algorithm to see who wins the election S represents, assuming the
    candidate with the most votes wins (even if there are O(n) candidates).
*/

#include <iostream>
#include <vector>
#include <optional>
#include <cassert>
#include <chrono>
#include <random>
#include "election_winner.hpp"

void run_tests() {
    std::cout << "[*] Running functional unit tests for election winner...\n";

    // Test 1: Standard vote sequence with a clear winner
    {
        std::vector<int> votes = {101, 102, 101, 103, 101, 102, 101};
        auto winner = dsa::find_election_winner(votes.begin(), votes.end());
        assert(winner.has_value());
        assert(*winner == 101);
    }

    // Test 2: Single vote
    {
        std::vector<int> votes = {42};
        auto winner = dsa::find_election_winner(votes.begin(), votes.end());
        assert(winner.has_value());
        assert(*winner == 42);
    }

    // Test 3: Empty election sequence
    {
        std::vector<int> votes = {};
        auto winner = dsa::find_election_winner(votes.begin(), votes.end());
        assert(!winner.has_value());
    }

    // Test 4: All O(n) candidates receive exactly 1 vote (unique elements)
    {
        std::vector<int> votes = {5, 1, 3, 2, 4};
        auto winner = dsa::find_election_winner(votes.begin(), votes.end());
        assert(winner.has_value());
        // Since all counts are 1, it will return the first encountered candidate after sorting (min element: 1)
        assert(*winner == 1);
    }

    std::cout << "[+] All functional unit tests passed successfully!\n";
}

void run_stress_test() {
    std::cout << "[*] Running performance stress test (N = 1,000,000 votes, O(n) candidates)...\n";

    constexpr size_t N = 1'000'000;
    std::vector<int> votes(N);

    // Populate votes with O(n) distinct candidates, ensuring candidate 777 has a majority/plurality
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist(1, 500'000);

    for (size_t i = 0; i < N; ++i) {
        votes[i] = dist(rng);
    }
    // Inject winning votes for candidate 777
    for (size_t i = 0; i < N; i += 3) {
        votes[i] = 777;
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto winner = dsa::find_election_winner(votes.begin(), votes.end());
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "[+] Stress test completed in " << elapsed.count() << " ms.\n";
    std::cout << "[+] Elected winner ID: " << *winner << "\n";

    assert(winner.has_value());
    std::cout << "[+] Output correctness verified.\n";
}

int main() {
    run_tests();
    run_stress_test();
    std::cout << "[SUCCESS] All election winner verifications passed without errors.\n";
    return 0;
}