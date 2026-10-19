// File : C-6.18.cpp

/*
    C-6.18 A useful operation in databases is the natural join. If we view a database
    as a list of ordered pairs of objects, then the natural join of databases A
    and B is the list of all ordered triples (x, y, z) such that the pair (x, y) is in A
    and the pair (y, z) is in B. Describe and analyze an efficient algorithm for
    computing the natural join of a list A of n pairs and a list B of m pairs
*/
/*
Algorithm: Hash-Based Natural Join
    The strategy is to index one of the lists using a hash map so we can look up matching elements in $O(1)$ average time.

Steps:
    Build Phase (Indexing $B$):Create a hash map (dictionary) where the key is the first element of a pair in $B$ ($y$), and the value is a list of all corresponding second elements ($z$).
    Why a list of values? Because multiple pairs in $B$ might share the same $y$ value (e.g., $(y, z_1)$ and $(y, z_2)$).

Probe Phase (Scanning $A$):
    - Initialize an empty result list for the triples $(x, y, z)$.
    - Iterate through each pair $(x, y)$ in list $A$.
    - Look up $y$ in the hash map.
    - If $y$ exists in the hash map, iterate through all associated $z$ values for that $y$, and for each one, 
        form the triple $(x, y, z)$ and append it to the result list.

Complexity Analysis:
    Let $k$ be the total number of resulting triples $(x, y, z)$ produced by the join.
    Time Complexity: $O(n + m + k)$ on average.
        - Building the hash table for $B$ takes $O(m)$ time.
        - Probing the hash table for each of the $n$ elements in $A$ takes $O(n)$ time.
        - Generating the $k$ output triples takes $O(k)$ time.
    Space Complexity: $O(m + k)$ auxiliary space.
        - The hash table stores the $m$ elements of list $B$, requiring $O(m)$ space.
        - The result list requires $O(k)$ space to store the output triples.
*/
#include <tuple>
#include <unordered_map>
#include <vector>

template <typename X, typename Y, typename Z>
std::vector<std::tuple<X, Y, Z>>
naturalJoin(const std::vector<std::pair<X, Y>> &A,
            const std::vector<std::pair<Y, Z>> &B) {
  // Step 1: Build phase - map y -> list of z values from B
  std::unordered_map<Y, std::vector<Z>> b_map;
  for (const auto &[y, z] : B) {
    b_map[y].push_back(z);
  }

  std::vector<std::tuple<X, Y, Z>> result;

  // Step 2: Probe phase - check elements of A against the hash map
  for (const auto &[x, y] : A) {
    auto it = b_map.find(y);
    if (it != b_map.end()) {
      // For every matching z, form the triple (x, y, z)
      for (const auto &z : it->second) {
        result.emplace_back(x, y, z);
      }
    }
  }

  return result;
}