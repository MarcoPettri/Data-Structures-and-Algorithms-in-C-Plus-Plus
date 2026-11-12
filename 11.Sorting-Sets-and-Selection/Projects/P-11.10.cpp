// File : P-11.10.cpp

/*
    P-11.10 Implement the tree-based union/find partition data structure with both the
    union-by-size and path-compression heuristics.
*/


#include "test_union_find.hpp"

using namespace test_union_find;
int main() {
    run("empty structure and singletons", test_empty_and_singletons);
    run("unite and connected", test_unite_and_connected);
    run("makeSet growth", test_makeSet_growth);
    run("out-of-range ids", test_out_of_range);
    run("union by size: small under large", test_union_by_size_attaches_small_under_large);
    run("union by size: height <= log2 n", test_union_by_size_bounds_height);
    run("path compression flattens the path", test_path_compression_flattens_path);
    run("randomised vs naive labelling", test_randomised_against_naive_labels);
    run("1e6-element chains", test_large_chain_no_deep_recursion);
    run("Partition<std::string>", test_partition_strings);
    run("Partition<int>, unknown elements", test_partition_unknown_element_and_integers);
    run("Partition singleton representative", test_partition_singleton_representative);

    std::cout << "\n" << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}