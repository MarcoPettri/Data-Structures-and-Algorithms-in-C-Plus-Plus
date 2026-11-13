// File : R-12.2.cpp

/*
    R-12.2 Design an efficient algorithm for the matrix chain multiplication problem
    that outputs a fully parenthesized expression for how to multiply the matrices in the
    chain using the minimum number of operations.
*/

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <chrono>
#include <random>
#include <stack>
#include <stdexcept>
#include "matrix_chain_multiplication.hpp"

/**
 * @brief Helper function to verify that parentheses in an expression are well-formed and balanced.
 */
[[nodiscard]] bool is_balanced_parentheses(const std::string& expr) {
    int balance = 0;
    for (char ch : expr) {
        if (ch == '(') {
            ++balance;
        } else if (ch == ')') {
            --balance;
            if (balance < 0) return false;
        }
    }
    return balance == 0;
}

/**
 * @brief Helper function to independently recalculate the cost from a split table
 *        to verify consistency with the DP table minimum.
 */
[[nodiscard]] std::uint64_t evaluate_split_cost(
    std::size_t i, std::size_t j,
    const std::vector<std::size_t>& dimensions,
    const std::vector<std::vector<std::size_t>>& split_table) {
    if (i == j) {
        return 0;
    }
    const std::size_t k = split_table[i][j];
    const std::uint64_t left_cost = evaluate_split_cost(i, k, dimensions, split_table);
    const std::uint64_t right_cost = evaluate_split_cost(k + 1, j, dimensions, split_table);
    const std::uint64_t mult_cost = static_cast<std::uint64_t>(dimensions[i]) *
                                    static_cast<std::uint64_t>(dimensions[k + 1]) *
                                    static_cast<std::uint64_t>(dimensions[j + 1]);
    return left_cost + right_cost + mult_cost;
}

void test_edge_cases() {
    std::cout << "[*] Running edge case tests...\n";

    // Degenerate Case 1: Empty dimensions
    {
        auto res = dsa::optimize_matrix_chain({});
        assert(res.min_operations == 0);
        assert(res.parenthesized_expression.empty());
    }

    // Degenerate Case 2: Single dimension (cannot form any matrix)
    {
        auto res = dsa::optimize_matrix_chain({10});
        assert(res.min_operations == 0);
        assert(res.parenthesized_expression.empty());
    }

    // Degenerate Case 3: Exactly 1 matrix (dimensions {10, 20} -> A1 of size 10x20)
    {
        auto res = dsa::optimize_matrix_chain({10, 20});
        assert(res.min_operations == 0);
        assert(res.parenthesized_expression == "A1");
    }

    // Exception test: invalid custom names count
    {
        bool caught = false;
        try {
            [[maybe_unused]] auto res = dsa::optimize_matrix_chain({10, 20, 30}, {"OnlyOneName"});
        } catch (const std::invalid_argument&) {
            caught = true;
        }
        assert(caught);
    }

    std::cout << "[+] Edge case tests passed!\n";
}

void test_two_and_three_matrices() {
    std::cout << "[*] Running two and three matrix chain tests...\n";

    // Two matrices: A1 (10x20), A2 (20x30)
    // Only one multiplication choice: 10 * 20 * 30 = 6000
    {
        std::vector<std::size_t> dims = {10, 20, 30};
        auto res = dsa::optimize_matrix_chain(dims);
        assert(res.min_operations == 6000);
        assert(res.parenthesized_expression == "(A1 * A2)");
        assert(is_balanced_parentheses(res.parenthesized_expression));
        assert(evaluate_split_cost(0, 1, dims, res.split_table) == 6000);
    }

    // Three matrices: A1 (10x100), A2 (100x5), A3 (5x50)
    // Option 1: ((A1 * A2) * A3) -> (10*100*5) + (10*5*50) = 5000 + 2500 = 7500
    // Option 2: (A1 * (A2 * A3)) -> (100*5*50) + (10*100*50) = 25000 + 50000 = 75000
    // Optimal is Option 1
    {
        std::vector<std::size_t> dims = {10, 100, 5, 50};
        auto res = dsa::optimize_matrix_chain(dims);
        assert(res.min_operations == 7500);
        assert(res.parenthesized_expression == "((A1 * A2) * A3)");
        assert(is_balanced_parentheses(res.parenthesized_expression));
        assert(evaluate_split_cost(0, 2, dims, res.split_table) == 7500);
    }

    // Three matrices with alternate optimal parenthesization
    // A1 (10x20), A2 (20x30), A3 (30x40)
    // Option 1: ((A1 * A2) * A3) -> 10*20*30 + 10*30*40 = 6000 + 12000 = 18000
    // Option 2: (A1 * (A2 * A3)) -> 20*30*40 + 10*20*40 = 24000 + 8000 = 32000
    {
        std::vector<std::size_t> dims = {10, 20, 30, 40};
        auto res = dsa::optimize_matrix_chain(dims);
        assert(res.min_operations == 18000);
        assert(res.parenthesized_expression == "((A1 * A2) * A3)");
        assert(evaluate_split_cost(0, 2, dims, res.split_table) == 18000);
    }

    std::cout << "[+] Two and three matrix chain tests passed!\n";
}

void test_clrs_classic_example() {
    std::cout << "[*] Running classic textbook 6-matrix example...\n";

    // Dimensions: 30x35, 35x15, 15x5, 5x10, 10x20, 20x25
    // Known optimal cost: 15,125
    // Optimal parenthesization: ((A1 * (A2 * A3)) * ((A4 * A5) * A6))
    const std::vector<std::size_t> dims = {30, 35, 15, 5, 10, 20, 25};
    dsa::MatrixChainOptimizer optimizer(dims);
    const auto res = optimizer.solve();

    std::cout << "    Computed minimum operations: " << res.min_operations << "\n";
    std::cout << "    Optimal parenthesization:    " << res.parenthesized_expression << "\n";

    assert(res.min_operations == 15125);
    assert(res.parenthesized_expression == "((A1 * (A2 * A3)) * ((A4 * A5) * A6))");
    assert(is_balanced_parentheses(res.parenthesized_expression));
    assert(evaluate_split_cost(0, dims.size() - 2, dims, res.split_table) == 15125);

    std::cout << "[+] Classic textbook 6-matrix example passed!\n";
}

void test_custom_identifiers() {
    std::cout << "[*] Running custom matrix identifiers test...\n";

    const std::vector<std::size_t> dims = {5, 10, 3, 12, 5};
    const std::vector<std::string> names = {"Alpha", "Beta", "Gamma", "Delta"};

    auto res = dsa::optimize_matrix_chain(dims, names);
    assert(!res.parenthesized_expression.empty());
    assert(is_balanced_parentheses(res.parenthesized_expression));
    // Verify custom names are actually present in the output
    for (const auto& name : names) {
        assert(res.parenthesized_expression.find(name) != std::string::npos);
    }

    assert(evaluate_split_cost(0, names.size() - 1, dims, res.split_table) == res.min_operations);
    std::cout << "    Parenthesized with custom names: " << res.parenthesized_expression << "\n";
    std::cout << "[+] Custom matrix identifiers test passed!\n";
}

void run_stress_test() {
    std::cout << "[*] Running performance stress test (N = 100 matrices)...\n";

    constexpr std::size_t N = 100;
    std::vector<std::size_t> dims(N + 1);

    std::mt19937_64 rng(42);
    std::uniform_int_distribution<std::size_t> dist(10, 1000);

    for (std::size_t i = 0; i <= N; ++i) {
        dims[i] = dist(rng);
    }

    const auto start = std::chrono::high_resolution_clock::now();
    auto res = dsa::optimize_matrix_chain(dims);
    const auto end = std::chrono::high_resolution_clock::now();

    const std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "[+] Solved chain of " << N << " matrices in " << elapsed.count() << " ms.\n";
    std::cout << "[+] Minimum operations: " << res.min_operations << "\n";
    std::cout << "[+] Expression length: " << res.parenthesized_expression.length() << " chars.\n";

    assert(is_balanced_parentheses(res.parenthesized_expression));
    assert(evaluate_split_cost(0, N - 1, dims, res.split_table) == res.min_operations);
    std::cout << "[+] Stress test verified and passed successfully!\n";
}

int main() {
    test_edge_cases();
    test_two_and_three_matrices();
    test_clrs_classic_example();
    test_custom_identifiers();
    run_stress_test();

    std::cout << "========================================================\n";
    std::cout << "    All tests passed successfully!                      \n";
    std::cout << "========================================================\n";

    return 0;
}
