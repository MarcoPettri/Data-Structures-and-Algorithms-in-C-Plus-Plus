// File : P-8.3.cpp

/*
P-8.3 Develop a C++ implementation of a priority queue that is based on a heap
and supports the locator-based functions
*/
#include "adaptable_priority_queue_test.cpp"

using namespace adaptable_priority_queue_test;

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