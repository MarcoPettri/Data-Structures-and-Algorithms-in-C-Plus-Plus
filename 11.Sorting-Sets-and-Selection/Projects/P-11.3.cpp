// File : P-11.3.cpp

/*
    P-11.3 Design and implement a version of the bucket-sort algorithm for sorting
    a linked list of n entries (for instance, a list of type std::list<int>) with
    integer keys taken from the range [0, N − 1], for N ≥ 2. The algorithm
    should run in O(n + N) time.
*/

#include "test_bucket_sort.hpp"
using namespace test_bucket_sort;

int main() {
  run("empty and single element", test_empty_and_single);
  run("small known input", test_small_known_input);
  run("already sorted / reversed", test_already_sorted_and_reversed);
  run("all keys equal", test_all_equal);
  run("minimum universe N = 2", test_minimum_universe_N2);
  run("boundary keys 0 and N-1", test_boundary_keys);
  run("random vs std::sort", test_random_against_std_sort);
  run("large input (1e6)", test_large_input);
  run("stability with key extractor", test_stability_with_key_extractor);
  run("other key / element types", test_various_key_types);
  run("nodes relinked, not copied", test_nodes_are_relinked_not_copied);
  run("non-copyable non-movable elements",
      test_works_with_non_copyable_non_movable_type);
  run("invalid N", test_invalid_N);
  run("out-of-range keys", test_out_of_range_keys);
  run("list intact after invalid key", test_list_intact_after_invalid_key);
  run("list intact after throwing extractor",
      test_list_intact_after_throwing_extractor);

  std::cout << "\n"
            << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures == 0 ? 0 : 1;
}