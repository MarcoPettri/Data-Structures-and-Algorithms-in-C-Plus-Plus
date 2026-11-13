// File : C-12.4.cpp

/*
    C-12.4 In the art gallery guarding problem we are given a line L that repre-
    sents a long hallway in an art gallery. We are also given a set X =
    {x_0, x_1, . . . , x_{n-1}} of real numbers that specify the positions of paintings
    in this hallway. Suppose that a single guard can protect all the paintings
    within distance at most 1 of his or her position (on both sides). Design
    an algorithm for finding a placement of guards that uses the minimum
    number of guards to guard all the paintings with positions in X .
*/

#include <iostream>
#include <vector>
#include <cassert>
#include <chrono>
#include <random>
#include <algorithm>
#include "art_gallery_guarding.hpp"

void test_edge_cases() {
    std::cout << "[*] Running edge case tests...\n";

    // Edge Case 1: Empty hallway
    {
        auto res = dsa::solve_art_gallery_guarding(std::vector<double>{});
        assert(res.guard_count() == 0);
        assert(res.guard_positions.empty());
    }

    // Edge Case 2: Single painting
    {
        std::vector<double> paintings = {5.0};
        auto res = dsa::solve_art_gallery_guarding(paintings);
        assert(res.guard_count() == 1);
        assert(res.guard_positions[0] == 6.0);
        assert(res.covers_all(paintings));
    }

    // Edge Case 3: Multiple identical paintings at the same coordinate
    {
        std::vector<double> paintings = {3.5, 3.5, 3.5, 3.5};
        auto res = dsa::solve_art_gallery_guarding(paintings);
        assert(res.guard_count() == 1);
        assert(res.guard_positions[0] == 4.5);
        assert(res.covers_all(paintings));
    }

    // Edge Case 4: Invalid non-positive radius
    {
        bool caught = false;
        try {
            [[maybe_unused]] auto res = dsa::solve_art_gallery_guarding(std::vector<double>{1.0, 2.0}, -1.0);
        } catch (const std::invalid_argument&) {
            caught = true;
        }
        assert(caught);
    }

    std::cout << "[+] Edge case tests passed!\n";
}

void test_boundary_conditions() {
    std::cout << "[*] Running boundary condition tests...\n";

    // Distance exactly 2.0 (covered by 1 guard placed at 1.0)
    {
        std::vector<double> paintings = {0.0, 2.0};
        auto res = dsa::solve_art_gallery_guarding(paintings, 1.0);
        assert(res.guard_count() == 1);
        assert(res.guard_positions[0] == 1.0);
        assert(res.covers_all(paintings));
    }

    // Distance slightly greater than 2.0 (requires 2 guards)
    {
        std::vector<double> paintings = {0.0, 2.001};
        auto res = dsa::solve_art_gallery_guarding(paintings, 1.0);
        assert(res.guard_count() == 2);
        assert(res.covers_all(paintings));
    }

    // Multiple equidistant points {0.0, 1.0, 2.0, 3.0, 4.0}
    // Guard 1 at 1.0 covers [0, 2]
    // Guard 2 at 4.0 covers [3, 5]
    {
        std::vector<double> paintings = {0.0, 1.0, 2.0, 3.0, 4.0};
        auto res = dsa::solve_art_gallery_guarding(paintings, 1.0);
        assert(res.guard_count() == 2);
        assert(res.guard_positions[0] == 1.0);
        assert(res.guard_positions[1] == 4.0);
        assert(res.covers_all(paintings));
    }

    std::cout << "[+] Boundary condition tests passed!\n";
}

void test_negative_and_unsorted_coordinates() {
    std::cout << "[*] Running negative and unsorted coordinate tests...\n";

    {
        std::vector<double> paintings = {1.5, -4.0, 0.0, -3.0, 5.0, -5.0, 10.0};
        auto res = dsa::solve_art_gallery_guarding(paintings, 1.0);

        // Sorted: {-5.0, -4.0, -3.0, 0.0, 1.5, 5.0, 10.0}
        // Guard 1: -5.0 + 1 = -4.0, covers [-5.0, -3.0]
        // Guard 2:  0.0 + 1 =  1.0, covers [0.0, 2.0]
        // Guard 3:  5.0 + 1 =  6.0, covers [5.0, 7.0]
        // Guard 4: 10.0 + 1 = 11.0, covers [10.0, 12.0]
        assert(res.guard_count() == 4);
        assert(res.guard_positions[0] == -4.0);
        assert(res.guard_positions[1] == 1.0);
        assert(res.guard_positions[2] == 6.0);
        assert(res.guard_positions[3] == 11.0);
        assert(res.covers_all(paintings));
    }

    std::cout << "[+] Negative and unsorted coordinate tests passed!\n";
}

void test_generic_radius() {
    std::cout << "[*] Running generic radius tests...\n";

    {
        std::vector<double> paintings = {1.0, 3.0, 5.5, 6.0, 12.0};
        auto res = dsa::solve_art_gallery_guarding(paintings, 2.5);

        assert(res.guard_count() == 2);
        assert(res.covers_all(paintings, 2.5));
    }

    std::cout << "[+] Generic radius tests passed!\n";
}

void run_stress_test() {
    std::cout << "[*] Running performance stress test (N = 1,000,000 paintings)...\n";

    constexpr std::size_t N = 1'000'000;
    std::vector<double> paintings(N);

    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> dist(-500'000.0, 500'000.0);

    for (std::size_t i = 0; i < N; ++i) {
        paintings[i] = dist(rng);
    }

    const auto start = std::chrono::high_resolution_clock::now();
    auto res = dsa::solve_art_gallery_guarding(paintings, 1.0);
    const auto end = std::chrono::high_resolution_clock::now();

    const std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "[+] Placed " << res.guard_count() << " guards for " << N
              << " paintings in " << elapsed.count() << " ms.\n";

    // Verify coverage in linear time over sorted coordinates
    std::sort(paintings.begin(), paintings.end());
    std::size_t g_idx = 0;
    for (std::size_t i = 0; i < N; ++i) {
        while (g_idx < res.guard_positions.size() &&
               res.guard_positions[g_idx] + 1.0 + 1e-9 < paintings[i]) {
            ++g_idx;
        }
        assert(g_idx < res.guard_positions.size());
        assert(std::abs(paintings[i] - res.guard_positions[g_idx]) <= 1.0 + 1e-9);
    }

    std::cout << "[+] Verification successful: all 1,000,000 paintings fully guarded!\n";
}

int main() {
    test_edge_cases();
    test_boundary_conditions();
    test_negative_and_unsorted_coordinates();
    test_generic_radius();
    run_stress_test();

    std::cout << "========================================================\n";
    std::cout << "          All tests passed successfully!                \n";
    std::cout << "========================================================\n";

    return 0;
}
