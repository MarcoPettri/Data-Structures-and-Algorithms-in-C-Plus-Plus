/**
 * @file coin_change_greedy.hpp
 * @brief Greedy coin change algorithm
 *        for standard US denominations {25, 10, 5, 1} with mathematical proof of correctness.
 */

#ifndef COIN_CHANGE_GREEDY_HPP
#define COIN_CHANGE_GREEDY_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <limits>
#include <algorithm>

namespace dsa {

/**
 * @struct CoinBreakdown
 * @brief Represents the exact breakdown of coins in denominations:
 *        quarters (25¢), dimes (10¢), nickels (5¢), and pennies (1¢).
 */
struct CoinBreakdown {
    std::uint64_t quarters{0}; ///< Number of quarters (25 cents each).
    std::uint64_t dimes{0};    ///< Number of dimes (10 cents each).
    std::uint64_t nickels{0};  ///< Number of nickels (5 cents each).
    std::uint64_t pennies{0};  ///< Number of pennies (1 cent each).

    /**
     * @brief Computes total number of coins in the breakdown.
     * 
     * @return [[nodiscard]] std::uint64_t Total coin count.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(1)$
     * - **Auxiliary Space:** $\mathcal{O}(1)$
     */
    [[nodiscard]] constexpr std::uint64_t total_coins() const noexcept {
        return quarters + dimes + nickels + pennies;
    }

    /**
     * @brief Computes total monetary value in cents.
     * 
     * @return [[nodiscard]] std::uint64_t Total value in cents.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(1)$
     * - **Auxiliary Space:** $\mathcal{O}(1)$
     */
    [[nodiscard]] constexpr std::uint64_t total_cents() const noexcept {
        return quarters * 25 + dimes * 10 + nickels * 5 + pennies;
    }

    /**
     * @brief Formats coin breakdown into a readable string.
     * 
     * @return [[nodiscard]] std::string Human-readable breakdown.
     */
    [[nodiscard]] std::string to_string() const {
        std::ostringstream oss;
        oss << quarters << " quarter(s), "
            << dimes << " dime(s), "
            << nickels << " nickel(s), "
            << pennies << " penny/pennies "
            << "(Total coins: " << total_coins()
            << ", Total value: " << total_cents() << "¢)";
        return oss.str();
    }

    /**
     * @brief Equality comparison operator.
     */
    bool operator==(const CoinBreakdown& other) const noexcept {
        return quarters == other.quarters &&
               dimes == other.dimes &&
               nickels == other.nickels &&
               pennies == other.pennies;
    }

    /**
     * @brief Inequality comparison operator.
     */
    bool operator!=(const CoinBreakdown& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @class GreedyChangeMaker
 * @brief Solves the change-making problem using the greedy approach for canonical US coin denominations.
 * 
 * ### Mathematical Argument & Proof of Correctness:
 * Let the denominations be $c_1 = 25, c_2 = 10, c_3 = 5, c_4 = 1$.
 * The greedy strategy repeatedly chooses the largest denomination $\le$ the remaining value.
 * We prove that for any value $V \ge 0$, the greedy solution produces the minimum number of coins.
 * 
 * In any optimal solution $\mathcal{O}$:
 * 1. **Pennies ($c_4 = 1$):** Number of pennies $p \le 4$. If $p \ge 5$, five pennies ($5 \times 1 = 5$)
 *    can be replaced by 1 nickel ($1 < 5$ coins), contradicting optimality.
 * 2. **Nickels ($c_3 = 5$):** Number of nickels $n \le 1$. If $n \ge 2$, two nickels ($2 \times 5 = 10$)
 *    can be replaced by 1 dime ($1 < 2$ coins), contradicting optimality.
 * 3. **Nickels + Dimes:** We cannot have $n = 1$ and $d \ge 2$. If $n = 1$ and $d \ge 2$, one nickel
 *    and two dimes ($5 + 2 \times 10 = 25$) can be replaced by 1 quarter ($1 < 3$ coins), contradicting optimality.
 * 4. **Dimes ($c_2 = 10$):** Number of dimes $d \le 2$. If $d \ge 3$, three dimes ($3 \times 10 = 30$)
 *    can be replaced by 1 quarter and 1 nickel ($25 + 5 = 30$, using $2 < 3$ coins), contradicting optimality.
 * 
 * From (2), (3), and (4), the maximum value contributed by dimes and nickels in any optimal solution is:
 * $$\max(2 \times 10 + 0 \times 5, 1 \times 10 + 1 \times 5, 0 \times 10 + 1 \times 5) = \max(20, 15, 5) = 20\text{ cents.}$$
 * 
 * Adding the maximum contribution from pennies ($4\text{ cents}$ from (1)):
 * The maximum value contributed by all non-quarter coins in an optimal solution is:
 * $$20 + 4 = 24\text{ cents.}$$
 * 
 * **Conclusion for Quarters:**
 * Because non-quarter coins can account for at most $24\text{ cents}$, any value $V \ge 25$ *must* use
 * at least $\lfloor V / 25 \rfloor$ quarters. Using fewer quarters would require non-quarter coins to sum
 * to $\ge 25$ cents, which is impossible without violating the optimality conditions above.
 * 
 * By inductive reduction:
 * - After taking $q = \lfloor V / 25 \rfloor$ quarters, remainder $V_1 = V \pmod{25} < 25$.
 * - For $V_1 < 25$, nickels and pennies can make at most $1 \times 5 + 4 \times 1 = 9\text{ cents}$.
 *   Hence, any $V_1 \ge 10$ must use at least $\lfloor V_1 / 10 \rfloor$ dimes.
 * - After taking $d = \lfloor V_1 / 10 \rfloor$ dimes, remainder $V_2 = V_1 \pmod{10} < 10$.
 * - For $V_2 < 10$, pennies can make at most $4\text{ cents}$. Thus, any $V_2 \ge 5$ must use 1 nickel.
 * - The remaining $V_3 = V_2 \pmod 5 < 5$ is made entirely of $V_3$ pennies, which is unique and optimal.
 * 
 * Therefore, the greedy choice at each step is globally optimal.
 */
class GreedyChangeMaker {
public:
    /**
     * @brief Computes the optimal coin breakdown for a given monetary amount in cents.
     * 
     * @param cents Target non-negative value in cents.
     * @return [[nodiscard]] CoinBreakdown Struct containing coin counts for 25¢, 10¢, 5¢, and 1¢.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(1)$ — executed via four integer division/modulo operations.
     * - **Auxiliary Space:** $\mathcal{O}(1)$ — constant storage for the result struct.
     */
    [[nodiscard]] static constexpr CoinBreakdown make_change(std::uint64_t cents) noexcept {
        CoinBreakdown result;

        result.quarters = cents / 25;
        cents %= 25;

        result.dimes = cents / 10;
        cents %= 10;

        result.nickels = cents / 5;
        cents %= 5;

        result.pennies = cents;

        return result;
    }

    /**
     * @brief Computes the minimum number of coins needed to make change for a given value in cents.
     * 
     * @param cents Target non-negative value in cents.
     * @return [[nodiscard]] std::uint64_t Minimum number of coins.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(1)$
     * - **Auxiliary Space:** $\mathcal{O}(1)$
     */
    [[nodiscard]] static constexpr std::uint64_t min_coins(std::uint64_t cents) noexcept {
        return make_change(cents).total_coins();
    }

    /**
     * @brief Reference Dynamic Programming solver for minimum coin change on arbitrary coin denominations.
     *        Used for rigorous cross-verification against the greedy solution.
     * 
     * @param cents Target value in cents.
     * @param denominations Sorted coin denominations in descending or ascending order.
     * @return [[nodiscard]] std::uint64_t Minimum coins required according to dynamic programming.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(k \cdot V)$, where $k$ is the number of denominations and $V$ is cents.
     * - **Auxiliary Space:** $\mathcal{O}(V)$ for the DP memoization array.
     */
    [[nodiscard]] static std::uint64_t make_change_dp_reference(
        std::size_t cents,
        const std::vector<std::size_t>& denominations = {25, 10, 5, 1}) {
        if (cents == 0) return 0;

        constexpr std::uint64_t INF = std::numeric_limits<std::uint64_t>::max() / 2;
        std::vector<std::uint64_t> dp(cents + 1, INF);
        dp[0] = 0;

        for (std::size_t i = 1; i <= cents; ++i) {
            for (std::size_t coin : denominations) {
                if (i >= coin) {
                    dp[i] = std::min(dp[i], dp[i - coin] + 1);
                }
            }
        }

        return dp[cents];
    }
};

/**
 * @brief Free convenience function to compute greedy change for US denominations.
 * 
 * @param cents Target non-negative value in cents.
 * @return [[nodiscard]] CoinBreakdown Minimum coin breakdown.
 * 
 * @section complexity
 * - **Time Complexity:** $\mathcal{O}(1)$
 * - **Auxiliary Space:** $\mathcal{O}(1)$
 */
[[nodiscard]] inline constexpr CoinBreakdown compute_greedy_change(std::uint64_t cents) noexcept {
    return GreedyChangeMaker::make_change(cents);
}

} // namespace dsa

#endif // COIN_CHANGE_GREEDY_HPP
