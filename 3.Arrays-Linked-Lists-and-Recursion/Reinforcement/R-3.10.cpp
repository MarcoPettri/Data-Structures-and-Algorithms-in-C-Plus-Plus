// File: R-3.10.cpp
/*

R-3.10 Describe a nonrecursive function for finding, by link hopping, the middle
node of a doubly linked list with header and trailer sentinels. (Note: This
function must only use link hopping; it cannot use a counter.) What is the
running time of this function?

Answer:
  Use two iterators, `fwd` starting at the first real node (after the header
  sentinel) and `bwd` starting at the last real node (before the trailer
  sentinel).  At each step hop `fwd` one step forward and `bwd` one step
  backward.  Stop as soon as they point to the same node (odd-length list) or
  `fwd` has passed `bwd` (even-length list).  At that point `fwd` is the
  middle node (for even lengths, the second of the two middle nodes).

  Running time: O(n) — each iterator performs at most n/2 hops, so the total
  number of link hops is n, giving linear time in the number of elements.

*/
#include "DoublyLinkedList.hpp"
#include <iostream>

// Returns an iterator to the middle node of `list`.
// Uses only link hopping (no counter).  For an even-size list the iterator
// points to the second of the two middle nodes.
// Precondition: list is non-empty.
template <typename T>
typename DoublyLinkedList<T>::iterator
findMiddle(DoublyLinkedList<T> &list) {
    auto fwd = list.begin();          // hops forward  (→)
    auto bwd = list.end(); --bwd;     // hops backward (←), starts at last node

    // Advance both pointers until they meet or cross.
    while (fwd != bwd) {
        ++fwd;
        if (fwd == bwd) break;  // they just met → stop before overshooting
        --bwd;
    }
    return fwd;
}

int main() {
    // ── odd-length list: middle of {1,2,3,4,5} is 3 ──────────────────────
    DoublyLinkedList<int> odd = {1, 2, 3, 4, 5};
    std::cout << "Odd list  {1,2,3,4,5}  → middle = "
              << *findMiddle(odd) << "\n";   // expected: 3

    // ── even-length list: two middles of {1,2,3,4} are 2 and 3 ──────────
    DoublyLinkedList<int> even = {1, 2, 3, 4};
    std::cout << "Even list {1,2,3,4}    → middle = "
              << *findMiddle(even) << "\n";  // expected: 3 (second middle)

    // ── single-element list ───────────────────────────────────────────────
    DoublyLinkedList<int> single = {42};
    std::cout << "Single    {42}         → middle = "
              << *findMiddle(single) << "\n"; // expected: 42

    return 0;
}