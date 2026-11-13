/**
 * @file election_winner.hpp
 * @brief Algorithm for finding the election winner 
 *        in O(n log n) time using sorting and linear scan aggregation.
 */

#ifndef ELECTION_WINNER_HPP
#define ELECTION_WINNER_HPP

#include <algorithm>
#include <iterator>
#include <optional>
#include <vector>

namespace dsa {

/**
 * @brief Determines the candidate with the most votes in O(n log n) time.
 * 
 * @tparam InputIt Input iterator type.
 * @param first Iterator to the beginning of the vote sequence.
 * @param last Iterator to the end of the sequence.
 * @return [[nodiscard]] std::optional<typename std::iterator_traits<InputId>::value_type> 
 *         The winning candidate ID, or std::nullopt if the vote sequence is empty.
 * 
 * @section complexity
 * - **Time Complexity:** $\mathcal{O}(n \log n)$, where $n = \text{distance}(first, last)$. 
 *   Sorting the copied vote sequence takes $\mathcal{O}(n \log n)$ time, and the subsequent 
 *   linear frequency scan takes $\mathcal{O}(n)$ time.
 * - **Space Complexity:** $\mathcal{O}(n)$ auxiliary space to store the copied and sorted sequence of votes.
 */
template <typename InputIt>
[[nodiscard]] auto find_election_winner(InputIt first, InputIt last)
    -> std::optional<typename std::iterator_traits<InputIt>::value_type> {

  using candidate_type = typename std::iterator_traits<InputIt>::value_type;

  if (first == last) {
    return std::nullopt;
  }

  // Copy elements into a local vector to preserve input immutability
  std::vector<candidate_type> votes(first, last);

  // Sort votes to group identical candidate IDs together - O(n log n)
  std::sort(votes.begin(), votes.end());

  candidate_type winner = votes[0];
  size_t max_count = 0;
  size_t current_count = 0;
  candidate_type current_candidate = votes[0];

  // Linear frequency scan - O(n)
  for (size_t i = 0; i < votes.size(); ++i) {
    if (votes[i] == current_candidate) {
      ++current_count;
    } else {
      if (current_count > max_count) {
        max_count = current_count;
        winner = current_candidate;
      }
      current_candidate = votes[i];
      current_count = 1;
    }
  }

  // Final check for the last candidate group
  if (current_count > max_count) {
    max_count = current_count;
    winner = current_candidate;
  }

  return winner;
}

} // namespace dsa

#endif // ELECTION_WINNER_HPP