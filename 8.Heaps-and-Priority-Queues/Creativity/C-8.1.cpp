// File : C-8.1.cpp

/*
    C-8.1 An online computer system for trading stock needs to process orders of
    the form “buy 100 shares at $x each” or “sell 100 shares at $y each.” A
    buy order for $x can only be processed if there is an existing sell order
    with price $y such that y ≤ x. Likewise, a sell order for $y can only be
    processed if there is an existing buy order with price $x such that x ≥ y.
    If a buy or sell order is entered but cannot be processed, it must wait
    for a future order that allows it to be processed. Describe a scheme that
    allows for buy and sell orders to be entered in O(log n) time, independent
    of whether or not they can be immediately processed.

    C-8.2 Extend a solution to the previous problem so that users are allowed to
    update the prices for their buy or sell orders that have yet to be
   processed.
*/

#include "order_book_test.cpp"

using namespace order_book_tests;

int main() {
  int failedTests = 0;
  for (const TestCase &test : registry()) {
    const int before = g_failures;
    try {
      test.fn();
    } catch (const std::exception &e) {
      ++g_failures;
      std::cerr << test.name << ": unexpected exception: " << e.what() << '\n';
    } catch (...) {
      ++g_failures;
      std::cerr << test.name << ": unexpected non-standard exception\n";
    }
    const bool ok = g_failures == before;
    failedTests += ok ? 0 : 1;
    std::cout << (ok ? "[ PASS ] " : "[ FAIL ] ") << test.name << '\n';
  }
  std::cout << registry().size() - static_cast<std::size_t>(failedTests) << '/'
            << registry().size() << " tests passed, " << g_checks << " checks, "
            << g_failures << " failures\n";
  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}