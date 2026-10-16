// File : C-5.7.cpp

/*
    C-5.7 Describe a nonrecursive algorithm for enumerating all permutations of
   the numbers {1, 2, . . . , n}.
*/

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <set>
#include <vector>

using Perm = std::vector<int>;

// Transforms `a` into its lexicographic successor.
// Returns false (leaving `a` untouched) if `a` is the last permutation.
bool nextPermutation(Perm &a) {
  const int n = static_cast<int>(a.size());
  if (n < 2)
    return false;

  // 1. Find the pivot: rightmost i with a[i] < a[i+1].
  int i = n - 2;
  while (i >= 0 && a[i] >= a[i + 1])
    --i;
  if (i < 0)
    return false; // fully descending: no successor

  // 2. Find rightmost element greater than the pivot.
  int j = n - 1;
  while (a[j] <= a[i])
    --j;

  // 3. Swap, then 4. reverse the (descending) suffix to make it ascending.
  std::swap(a[i], a[j]);
  std::reverse(a.begin() + i + 1, a.end());
  return true;
}

// Enumerates all permutations of {1..n} in lexicographic order.
std::vector<Perm> allPermutations(int n) {
  std::vector<Perm> result;
  if (n < 0)
    return result;

  Perm a(n);
  std::iota(a.begin(), a.end(), 1); // 1, 2, ..., n

  do {
    result.push_back(a);
  } while (nextPermutation(a));
  return result;
}
// ------------------------- Tests -------------------------
void testSmallExplicit();
void testEdgeCases();
void testCountsUniquenessOrder();
void testAgainstStd();

int main() {
  testSmallExplicit();
  testEdgeCases();
  testCountsUniquenessOrder();
  testAgainstStd();
  std::cout << "All tests passed.\n\n";

  std::cout << "Permutations of {1,2,3,4}:\n";
  for (const auto &p : allPermutations(4)) {
    for (int x : p)
      std::cout << x << ' ';
    std::cout << '\n';
  }
  return 0;
}
// ------------------------- Tests -------------------------

unsigned long long factorial(int n) {
  unsigned long long f = 1;
  for (int k = 2; k <= n; ++k)
    f *= k;
  return f;
}

void testSmallExplicit() {
  // n = 3 must produce exactly this sequence.
  std::vector<Perm> expected = {{1, 2, 3}, {1, 3, 2}, {2, 1, 3},
                                {2, 3, 1}, {3, 1, 2}, {3, 2, 1}};
  assert(allPermutations(3) == expected);
}

void testEdgeCases() {
  // n = 0: one permutation (the empty one); n = 1: just {1}.
  assert(allPermutations(0).size() == 1);
  assert(allPermutations(1) == std::vector<Perm>({{1}}));
  assert(allPermutations(2) == std::vector<Perm>({{1, 2}, {2, 1}}));

  // Last permutation has no successor and stays untouched.
  Perm last = {3, 2, 1};
  assert(!nextPermutation(last));
  assert((last == Perm{3, 2, 1}));
}

void testCountsUniquenessOrder() {
  for (int n = 1; n <= 8; ++n) {
    auto perms = allPermutations(n);

    // Count equals n!
    assert(perms.size() == factorial(n));

    // All distinct
    std::set<Perm> unique(perms.begin(), perms.end());
    assert(unique.size() == perms.size());

    // Strictly increasing lexicographic order
    assert(std::is_sorted(perms.begin(), perms.end()));

    // Each one is genuinely a permutation of 1..n
    Perm sorted(n);
    std::iota(sorted.begin(), sorted.end(), 1);
    for (const auto &p : perms)
      assert(std::is_permutation(p.begin(), p.end(), sorted.begin()));
  }
}

void testAgainstStd() {
  // Cross-check with std::next_permutation for n = 7.
  Perm a(7), b(7);
  std::iota(a.begin(), a.end(), 1);
  b = a;
  bool ra, rb;
  do {
    assert(a == b);
    ra = nextPermutation(a);
    rb = std::next_permutation(b.begin(), b.end());
    assert(ra == rb);
  } while (ra);
}
