/**
 * @file leprechaun_simulation.hpp
 * @brief Interface of the "Jumping Leprechauns" n-body simulation.
 *
 * Every leprechaun owns a gold value and a position on the horizon. The
 * positions are kept in an ordered map (`std::multimap`) so that the nearest
 * neighbour on each side of a leprechaun can be found in O(1) once its map
 * node is known, and a jump costs O(log n) with no memory allocation.
 */
#ifndef LEPRECHAUN_SIMULATION_HPP
#define LEPRECHAUN_SIMULATION_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <random>
#include <vector>

namespace leprechaun {

/**
 * @brief Simulation of n leprechauns jumping along a one-dimensional horizon.
 *
 * Rules of one iteration (see step()): leprechauns are processed in index
 * order. Processing leprechaun @c i
 *  1. moves it: @f$x_i \leftarrow x_i + r\,g_i@f$ with @c r drawn uniformly
 *     from [-1, 1);
 *  2. steals half of the gold of the nearest leprechaun on its left and
 *     half of the gold of the nearest leprechaun on its right (if they
 *     exist) and adds it to @f$g_i@f$.
 *
 * Positions are stored in a `std::multimap<double, Index>` (several
 * leprechauns may share a position). Per-leprechaun iterators into that map
 * are cached so each jump is a node re-key (C++17 node handles), i.e.
 * O(log n) time and zero allocations.
 *
 * Leprechauns are identified by zero-based indices 0 .. n-1 (the problem
 * statement's leprechaun k corresponds to index k-1).
 *
 * @note The class is not thread-safe. It is move-only because the cached
 *       iterators refer to nodes owned by the object.
 */
class Simulation {
public:
    /// Identifier of a leprechaun (zero based).
    using Index = std::size_t;
    /// Pseudo random engine used for jump lengths.
    using Engine = std::mt19937_64;
    /// Ordered container mapping horizon position -> leprechaun.
    using PositionMap = std::multimap<double, Index>;

    /// Gold every leprechaun owns at the start (one million dollars).
    static constexpr double kInitialGold = 1'000'000.0;

    /**
     * @brief Creates @p n leprechauns at positions 0, 1, ..., n-1.
     * @param n    Number of leprechauns (must be >= 1).
     * @param seed Seed of the random engine (default: engine's default seed,
     *             which makes runs reproducible).
     * @throws std::invalid_argument if @p n is zero.
     */
    explicit Simulation(std::size_t n, std::uint64_t seed = Engine::default_seed);

    /**
     * @brief Creates one leprechaun per entry of @p positions.
     * @param positions Initial horizon positions (non-empty, finite values).
     * @param seed      Seed of the random engine.
     * @throws std::invalid_argument if @p positions is empty or contains a
     *         non-finite value.
     */
    explicit Simulation(const std::vector<double>& positions,
                        std::uint64_t seed = Engine::default_seed);

    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation(Simulation&&) noexcept = default;
    Simulation& operator=(Simulation&&) noexcept = default;
    ~Simulation() = default;

    /**
     * @brief Runs one full iteration: every leprechaun is processed once,
     *        in index order, with a freshly drawn random factor.
     */
    void step();

    /**
     * @brief Runs @p iterations consecutive iterations.
     * @param iterations Number of iterations to perform.
     */
    void run(std::size_t iterations);

    /**
     * @brief Processes a single leprechaun with an explicit random factor.
     *
     * Exposed so the rules can be tested deterministically; step() calls it
     * with random values.
     *
     * @param i Leprechaun to process.
     * @param r Random factor, normally in [-1, 1).
     * @throws std::out_of_range if @p i >= size().
     */
    void jump(Index i, double r);

    /// @return Number of leprechauns.
    [[nodiscard]] std::size_t size() const noexcept { return gold_.size(); }

    /// @return Number of completed calls to step() (via step() or run()).
    [[nodiscard]] std::size_t iterations() const noexcept { return iterations_; }

    /**
     * @brief Gold owned by leprechaun @p i.
     * @throws std::out_of_range if @p i >= size().
     */
    [[nodiscard]] double gold(Index i) const;

    /**
     * @brief Horizon position of leprechaun @p i.
     * @throws std::out_of_range if @p i >= size().
     */
    [[nodiscard]] double position(Index i) const;

    /// @return Sum of all gold values (conserved by the rules, up to rounding).
    [[nodiscard]] double totalGold() const noexcept;

    /// @return Leprechaun indices ordered by increasing horizon position.
    [[nodiscard]] std::vector<Index> orderedIndices() const;

private:
    /// Moves half of leprechaun @p victim's gold out and returns that amount.
    double stealHalf(Index victim) noexcept;

    /// Validates an index, throwing std::out_of_range on failure.
    void checkIndex(Index i) const;

    /// Builds map_ and handles_ from the given positions.
    void initialise(const std::vector<double>& positions);

    PositionMap map_;                           ///< position -> leprechaun
    std::vector<PositionMap::iterator> nodes_;  ///< leprechaun -> its map node
    std::vector<double> gold_;                  ///< leprechaun -> gold
    Engine engine_;                             ///< random engine
    std::uniform_real_distribution<double> dist_{-1.0, 1.0};  ///< jump factor
    std::size_t iterations_{0};                 ///< completed iterations
};

}  // namespace leprechaun

#endif  // LEPRECHAUN_SIMULATION_HPP