// File : P-11.9.cpp

/*
    P-11.9 Implement an extended set ADT that includes the functions union(B),
    intersect(B), subtract(B), size(), empty(), plus the functions equals(B),
    contains(e), insert(e), and remove(e) with obvious meaning.
*/

#include "test_extended_set.hpp"

using namespace test_extended_set;

int main() {
  run("empty set", test_empty_set);
  run("insert / contains / size", test_insert_contains_size);
  run("insert in ascending, descending, mixed order", test_insert_orders);
  run("insert lvalue and rvalue", test_insert_rvalue_and_lvalue);
  run("remove", test_remove);
  run("clear and swap", test_clear_and_swap);
  run("construction", test_construction);
  run("custom comparator", test_custom_comparator);
  run("unite", test_unite);
  run("intersect", test_intersect);
  run("subtract", test_subtract);
  run("self-aliasing operations", test_self_aliasing);
  run("equals and operators", test_equals_and_operators);
  run("algebraic identities", test_algebraic_identities);
  run("string elements", test_strings);
  run("randomised vs std::set", test_randomised_against_std_set);
  run("large sets", test_large_sets);

  std::cout << "\n"
            << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures == 0 ? 0 : 1;
}