// File : C-12.2.cpp

/*
    C-12.2 Describe an efficient greedy algorithm for making change for a specified
    value using a minimum number of coins, assuming there are four denominations of coins, 
    called quarters, dimes, nickels, and pennies, with values
    25, 10, 5, and 1, respectively. Argue why your algorithm is correct.

    --------------------------------------------------------------------------------
    MATHEMATICAL ARGUMENT & PROOF OF CORRECTNESS:
    --------------------------------------------------------------------------------
    Let the denominations be c1 = 25, c2 = 10, c3 = 5, c4 = 1.
    The greedy algorithm chooses the maximum possible number of quarters, then dimes,
    then nickels, and finally pennies:
        q = floor(V / 25), remainder V1 = V mod 25
        d = floor(V1 / 10), remainder V2 = V1 mod 10
        n = floor(V2 / 5),  remainder V3 = V2 mod 5
        p = V3

    Proof that this greedy strategy yields the strictly optimal (minimum) number of coins:
    Consider any optimal coin combination O that makes value V with the minimum number of coins.

    1. Pennies Bound (p <= 4):
       If O contains >= 5 pennies, replacing 5 pennies with 1 nickel preserves the total value
       (5 * 1 = 5) while strictly reducing the coin count (1 < 5), contradicting optimality.
       Hence, in any optimal solution, p in {0, 1, 2, 3, 4}.

    2. Nickels Bound (n <= 1):
       If O contains >= 2 nickels, replacing 2 nickels with 1 dime preserves the total value
       (2 * 5 = 10) while strictly reducing the coin count (1 < 2), contradicting optimality.
       Hence, in any optimal solution, n in {0, 1}.

    3. Nickels + Dimes Bound (cannot have n = 1 and d >= 2):
       If O contains >= 1 nickel and >= 2 dimes, replacing 1 nickel and 2 dimes with 1 quarter
       preserves the total value (5 + 2 * 10 = 25) while reducing the coin count (1 < 3),
       contradicting optimality.

    4. Dimes Bound (d <= 2):
       If O contains >= 3 dimes, replacing 3 dimes with 1 quarter and 1 nickel preserves the total
       value (3 * 10 = 30 = 25 + 5) while strictly reducing the coin count (2 < 3),
       contradicting optimality. Hence, d in {0, 1, 2}.
       Furthermore, if d = 2, from (3) we must have n = 0.

    5. Maximum Value of Non-Quarter Coins:
       From (1)-(4), the maximum value that dimes and nickels can contribute is:
           max(2 * 10 + 0 * 5, 1 * 10 + 1 * 5, 0 * 10 + 1 * 5) = max(20, 15, 5) = 20 cents.
       Adding the maximum pennies contribution (4 cents), the maximum total value contributed
       by all non-quarter coins in ANY optimal solution is:
           20 + 4 = 24 cents.

    6. Optimality of Quarters:
       Because non-quarter coins can account for at most 24 cents, any value V >= 25 cannot be
       formed optimally without quarters. Specifically, any optimal solution MUST use at least
       floor(V / 25) quarters; using fewer would leave a remainder of at least 25 cents to be made
       by non-quarter coins, which by (5) is impossible without violating optimality.
       Thus, the greedy choice of q = floor(V / 25) quarters is optimal.

    7. Inductive Reduction for Remaining Coins:
       - After taking q quarters, the remaining amount V1 < 25.
         In any optimal solution, nickels and pennies can contribute at most 1 * 5 + 4 * 1 = 9 cents.
         Therefore, if V1 >= 10, an optimal solution MUST use floor(V1 / 10) dimes.
       - After taking dimes, remaining amount V2 < 10.
         Pennies can contribute at most 4 cents. Therefore, if V2 >= 5, an optimal solution MUST
         use floor(V2 / 5) = 1 nickel.
       - Finally, remaining amount V3 < 5 is made with V3 pennies, which is unique and optimal.

    Therefore, the greedy algorithm is guaranteed to be correct and optimal for the US coin system.
*/

#include <iostream>
#include <cassert>
#include <chrono>
#include <random>
#include <vector>
#include "coin_change_greedy.hpp"

void test_edge_cases() {
    std::cout << "[*] Running edge case tests...\n";

    // Value 0
    {
        auto res = dsa::compute_greedy_change(0);
        assert(res.total_coins() == 0);
        assert(res.quarters == 0 && res.dimes == 0 && res.nickels == 0 && res.pennies == 0);
        assert(res.total_cents() == 0);
    }

    // Single cent
    {
        auto res = dsa::compute_greedy_change(1);
        assert(res.quarters == 0 && res.dimes == 0 && res.nickels == 0 && res.pennies == 1);
        assert(res.total_coins() == 1);
        assert(res.total_cents() == 1);
    }

    // Exact denomination boundaries
    {
        auto n5 = dsa::compute_greedy_change(5);
        assert(n5.nickels == 1 && n5.total_coins() == 1);

        auto d10 = dsa::compute_greedy_change(10);
        assert(d10.dimes == 1 && d10.total_coins() == 1);

        auto q25 = dsa::compute_greedy_change(25);
        assert(q25.quarters == 1 && q25.total_coins() == 1);
    }

    std::cout << "[+] Edge case tests passed!\n";
}

void test_typical_values() {
    std::cout << "[*] Running typical monetary values tests...\n";

    // 41 cents: 1 quarter (25), 1 dime (10), 1 nickel (5), 1 penny (1) = 4 coins
    {
        auto res = dsa::compute_greedy_change(41);
        assert(res.quarters == 1);
        assert(res.dimes == 1);
        assert(res.nickels == 1);
        assert(res.pennies == 1);
        assert(res.total_coins() == 4);
        assert(res.total_cents() == 41);
    }

    // 24 cents (maximum non-quarter boundary): 2 dimes (20), 4 pennies (4) = 6 coins
    {
        auto res = dsa::compute_greedy_change(24);
        assert(res.quarters == 0);
        assert(res.dimes == 2);
        assert(res.nickels == 0);
        assert(res.pennies == 4);
        assert(res.total_coins() == 6);
        assert(res.total_cents() == 24);
    }

    // 99 cents: 3 quarters (75), 2 dimes (20), 0 nickels (0), 4 pennies (4) = 9 coins
    {
        auto res = dsa::compute_greedy_change(99);
        assert(res.quarters == 3);
        assert(res.dimes == 2);
        assert(res.nickels == 0);
        assert(res.pennies == 4);
        assert(res.total_coins() == 9);
        assert(res.total_cents() == 99);
    }

    // 68 cents: 2 quarters (50), 1 dime (10), 1 nickel (5), 3 pennies (3) = 7 coins
    {
        auto res = dsa::compute_greedy_change(68);
        assert(res.quarters == 2);
        assert(res.dimes == 1);
        assert(res.nickels == 1);
        assert(res.pennies == 3);
        assert(res.total_coins() == 7);
        assert(res.total_cents() == 68);
    }

    std::cout << "[+] Typical monetary values tests passed!\n";
}

void test_dp_cross_verification() {
    std::cout << "[*] Running comprehensive DP cross-verification for all cents in [0, 5000]...\n";

    constexpr std::size_t MAX_CENTS = 5000;
    for (std::size_t c = 0; c <= MAX_CENTS; ++c) {
        const auto greedy_res = dsa::compute_greedy_change(c);
        const std::uint64_t dp_min = dsa::GreedyChangeMaker::make_change_dp_reference(c);

        assert(greedy_res.total_cents() == c);
        assert(greedy_res.total_coins() == dp_min);
    }

    std::cout << "[+] Verified: Greedy matches Dynamic Programming optimal for ALL 5,001 values!\n";
}

void test_non_canonical_counterexample() {
    std::cout << "[*] Demonstrating why canonical proof matters (non-canonical counterexample)...\n";

    // In a hypothetical system without nickels {25, 10, 1}:
    // For 30 cents:
    // Greedy takes: 1 quarter (25) + 5 pennies (1) = 6 coins
    // Optimal DP takes: 3 dimes (10 + 10 + 10) = 3 coins!
    const std::vector<std::size_t> non_canonical = {25, 10, 1};
    const std::uint64_t dp_coins = dsa::GreedyChangeMaker::make_change_dp_reference(30, non_canonical);
    assert(dp_coins == 3);

    // Greedy on {25, 10, 1}: 30 / 25 = 1 (rem 5), 5 / 10 = 0 (rem 5), 5 / 1 = 5 -> total 6 coins
    const std::uint64_t greedy_coins_non_canonical = 1 + 0 + 5;
    assert(greedy_coins_non_canonical == 6);
    assert(greedy_coins_non_canonical > dp_coins);

    std::cout << "    Value: 30¢ with denominations {25, 10, 1}:\n";
    std::cout << "    - Greedy gives: " << greedy_coins_non_canonical << " coins (1x25¢ + 5x1¢)\n";
    std::cout << "    - Optimal DP gives: " << dp_coins << " coins (3x10¢)\n";
    std::cout << "[+] Non-canonical counterexample successfully verified!\n";
}

void test_large_amounts_stress() {
    std::cout << "[*] Running large amounts and performance stress test (10,000,000 queries)...\n";

    // 64-bit large monetary value: $12,345,678.99 = 1,234,567,899 cents
    {
        constexpr std::uint64_t large_cents = 1'234'567'899ULL;
        auto res = dsa::compute_greedy_change(large_cents);
        assert(res.total_cents() == large_cents);
        assert(res.quarters == large_cents / 25);
        assert(res.pennies == 4);
        assert(res.dimes == 2);
        assert(res.nickels == 0);
    }

    constexpr std::size_t NUM_QUERIES = 10'000'000;
    std::mt19937_64 rng(1337);
    std::uniform_int_distribution<std::uint64_t> dist(0, 100'000'000'000ULL);

    std::uint64_t checksum = 0;
    const auto start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < NUM_QUERIES; ++i) {
        const std::uint64_t val = dist(rng);
        const auto res = dsa::compute_greedy_change(val);
        checksum += res.total_coins();
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "[+] Executed " << NUM_QUERIES << " greedy queries in " << elapsed.count() << " ms.\n";
    std::cout << "[+] Average throughput: " << (static_cast<double>(NUM_QUERIES) / elapsed.count() * 1000.0)
              << " queries/sec (Checksum: " << checksum << ")\n";
    std::cout << "[+] Large amounts stress test passed!\n";
}

int main() {

    test_edge_cases();
    std::cout.put('\n');
    test_typical_values();
    std::cout.put('\n');
    test_dp_cross_verification();
    std::cout.put('\n');
    test_non_canonical_counterexample();
    std::cout.put('\n');
    test_large_amounts_stress();

    std::cout << "========================================================\n";
    std::cout << "       All tests and verifications passed!              \n";
    std::cout << "========================================================\n";

    return 0;
}
