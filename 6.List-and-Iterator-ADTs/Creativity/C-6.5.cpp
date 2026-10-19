// File : C-6.5.cpp

/*
    C-6.5 Describe a function for performing a card shuffle of an array of 2n
   el- ements, by converting it into two lists. A card shuffle is a permutation
    where a list L is cut into two lists, L1 and L2, where L1 is the first half
   of L and L2 is the second half of L, and then these two lists are merged into
    one by taking the first element in L1, then the first element in L2,
   followed by the second element in L1, the second element in L2, and so on
*/

#include <cassert>
#include <cstddef>
#include <iostream>
#include <list>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * @brief   Performs a perfect (riffle) card shuffle in place.
 *
 * The sequence is cut into two halves L1 and L2, which are then interleaved
 * as L1[0], L2[0], L1[1], L2[1], ...
 *
 * @tparam  Array  Any container with size(), operator[] and value_type
 *                 (std::vector, adt::ArrayVector, adt::ArrayList, ...).
 * @param   a      Sequence to shuffle; its size must be even.
 * @throws  std::invalid_argument if a.size() is odd.
 * @par Complexity O(n) time, O(n) extra space.
 */
template <typename Array> void card_shuffle(Array &a) {
  using T = typename Array::value_type;

  const std::size_t total = a.size();
  if (total % 2 != 0)
    throw std::invalid_argument("card_shuffle: size must be even");
  const std::size_t n = total / 2;

  // Cut: first half -> l1, second half -> l2.
  std::list<T> l1, l2;
  for (std::size_t i = 0; i < n; ++i)
    l1.push_back(std::move(a[i]));
  for (std::size_t i = n; i < total; ++i)
    l2.push_back(std::move(a[i]));

  // Merge: alternate the fronts of l1 and l2 back into the array.
  for (std::size_t i = 0; i < total; i += 2) {
    a[i] = std::move(l1.front());
    l1.pop_front();
    a[i + 1] = std::move(l2.front());
    l2.pop_front();
  }
}

int main() {
  std::vector<int> v{1, 2, 3, 4, 5, 6};
  for (const auto &num : v) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  card_shuffle(v);
  assert((v == std::vector<int>{1, 4, 2, 5, 3, 6}));
  for (const auto &num : v) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  std::vector<int> empty;
  card_shuffle(empty); // n = 0: nothing to do
  assert(empty.empty());
  return 0;
}