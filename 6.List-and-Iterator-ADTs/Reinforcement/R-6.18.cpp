// File : R-6.18.cpp

/*
    R-6.18 Using the Sequence interface functions, describe a recursive function
   for determining if a sequence S of n integer objects contains a given integer
   k. Your function should not contain any loops. How much space does your
    function use in addition to the space used for S?
*/
template <typename Sequence>
bool containsRecursiveHelper(const Sequence &S, int k,
                             typename Sequence::size_type index) {
  // Base Case 1: If we have checked all elements, k is not in the sequence
  if (index >= S.size()) {
    return false;
  }

  // Base Case 2: If the current element matches k, we found it
  if (S.at(index) == k) {
    return true;
  }

  // Recursive Step: Check the next index
  return containsRecursiveHelper(S, k, index + 1);
}

template <typename Sequence> bool contains(const Sequence &S, int k) {
  // Kick off recursion starting at index 0
  return containsRecursiveHelper(S, k, 0);
}

/*
How it works:

Base Cases:
    - If the index reaches or exceeds the sequence size ($n$), it returns false.
    - If the element at the current index (S.at(index)) equals $k$, it
short-circuits and returns true.

Recursive Step: If neither base case is met, it calls itself with the index
incremented by 1, moving closer to the base case.

Space Complexity Analysis:
In addition to the space used by the sequence $S$ itself, this recursive
function uses $O(n)$ auxiliary space (where $n$ is the number of elements in
$S$).

Reasoning: Each recursive call adds a new activation record (stack frame) to the
call stack. Because the recursion depth scales linearly with the size of the
sequence in the worst case (when $k$ is at the very end or not present at all),
up to $n$ stack frames will coexist simultaneously. Thus, the call stack
consumes $O(n)$ extra memory.
*/