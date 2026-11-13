/**
 * @file art_gallery_guarding.hpp
 * @brief Algorithm for the 1D Art Gallery Guarding problem.
 * 
 * Given the positions of paintings along a line (hallway), finds the minimum number
 * of guards required such that every painting is within distance at most 1 (or radius R)
 * of at least one guard.
 */

#ifndef ART_GALLERY_GUARDING_HPP
#define ART_GALLERY_GUARDING_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace dsa {

/**
 * @struct GuardPlacement
 * @brief Represents the result of the art gallery guarding optimization.
 * 
 * @tparam T Floating-point or arithmetic coordinate type (e.g. double, float).
 */
template <typename T = double> struct GuardPlacement {
  static_assert(std::is_arithmetic_v<T>,
                "GuardPlacement requires an arithmetic type.");

  /// Positions of the placed guards.
  std::vector<T> guard_positions;

  /**
     * @brief Returns the total number of guards placed.
     * 
     * @return [[nodiscard]] std::size_t Number of guards.
     */
  [[nodiscard]] std::size_t guard_count() const noexcept {
    return guard_positions.size();
  }

  /**
     * @brief Verifies whether every painting in the input set is protected by at least one guard.
     * 
     * @param paintings Vector of painting coordinates.
     * @param radius Maximum protection radius of a single guard (defaults to 1.0).
     * @param epsilon Tolerance for floating-point comparisons.
     * @return [[nodiscard]] bool True if all paintings are covered, false otherwise.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n + k)$ if both paintings and guards are sorted, or $\mathcal{O}(n \cdot k)$ worst-case.
     * - **Auxiliary Space:** $\mathcal{O}(1)$.
     */
  [[nodiscard]] bool covers_all(const std::vector<T> &paintings,
                                T radius = static_cast<T>(1),
                                T epsilon = static_cast<T>(1e-9)) const {
    for (const auto &p : paintings) {
      bool covered = false;
      for (const auto &g : guard_positions) {
        if (std::abs(static_cast<double>(p) - static_cast<double>(g)) <=
            static_cast<double>(radius) + static_cast<double>(epsilon)) {
          covered = true;
          break;
        }
      }
      if (!covered) {
        return false;
      }
    }
    return true;
  }
};

/**
 * @class ArtGalleryGuard
 * @brief Computes the optimal placement of guards along a 1D hallway using a greedy algorithm.
 */
class ArtGalleryGuard {
public:
  /**
     * @brief Computes the optimal guard placement to cover all paintings using the minimum number of guards.
     * 
     * @tparam T Arithmetic coordinate type (e.g. double, float).
     * @param paintings Vector of painting coordinates along the hallway line.
     * @param radius Maximum protection radius of each guard (defaults to 1.0).
     * @param epsilon Tolerance for floating-point boundary comparisons.
     * @return [[nodiscard]] GuardPlacement<T> Struct containing guard positions.
     * @throws std::invalid_argument If radius is less than or equal to zero.
     * 
     * @section complexity
     * - **Time Complexity:** $\mathcal{O}(n \log n)$, where $n$ is the number of paintings, dominated by
     *   sorting the coordinates. The subsequent greedy scan runs in $\mathcal{O}(n)$ linear time.
     * - **Auxiliary Space:** $\mathcal{O}(n)$ auxiliary storage for copying and sorting coordinates,
     *   plus $\mathcal{O}(k)$ where $k \le n$ is the number of placed guards.
     */
  template <typename T = double>
  [[nodiscard]] static GuardPlacement<T>
  place_guards(std::vector<T> paintings, T radius = static_cast<T>(1),
               T epsilon = static_cast<T>(1e-9)) {
    static_assert(std::is_arithmetic_v<T>,
                  "ArtGalleryGuard requires arithmetic coordinate types.");

    if (radius <= static_cast<T>(0)) {
      throw std::invalid_argument(
          "ArtGalleryGuard: radius must be strictly positive.");
    }

    GuardPlacement<T> result;
    if (paintings.empty()) {
      return result;
    }

    // Sort painting coordinates along the hallway line L
    std::sort(paintings.begin(), paintings.end());

    const std::size_t n = paintings.size();
    std::size_t i = 0;

    while (i < n) {
      // Place guard at the maximum forward reach covering the leftmost uncovered painting:
      const T guard_pos = paintings[i] + radius;
      result.guard_positions.push_back(guard_pos);

      const double max_reach = static_cast<double>(guard_pos) +
                               static_cast<double>(radius) +
                               static_cast<double>(epsilon);

      // Advance past all paintings covered by this guard
      ++i;
      while (i < n && static_cast<double>(paintings[i]) <= max_reach) {
        ++i;
      }
    }

    return result;
  }
};

/**
 * @brief Convenience free function to solve the art gallery guarding problem.
 * 
 * @tparam T Arithmetic coordinate type.
 * @param paintings Vector of painting coordinates.
 * @param radius Protection radius (default: 1.0).
 * @return [[nodiscard]] GuardPlacement<T> Optimal placement of guards.
 * 
 * @section complexity
 * - **Time Complexity:** $\mathcal{O}(n \log n)$
 * - **Auxiliary Space:** $\mathcal{O}(n)$
 */
template <typename T = double>
[[nodiscard]] inline GuardPlacement<T>
solve_art_gallery_guarding(std::vector<T> paintings,
                           T radius = static_cast<T>(1)) {
  return ArtGalleryGuard::place_guards<T>(std::move(paintings), radius);
}

} // namespace dsa

#endif // ART_GALLERY_GUARDING_HPP
