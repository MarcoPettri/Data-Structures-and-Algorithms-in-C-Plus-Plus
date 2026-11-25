// File : C-14.6.cpp

/*
    C-14.6 Describe an external-memory data structure to implement the queue ADT
    so that the total number of disk transfers needed to process a sequence of
    n enqueue and dequeue operations is O(n/B).

    C-14.7 Solve the previous problem for the deque ADT.
*/

#include "external_queue_deque.hpp"

#include <deque>
#include <iostream>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    ++g_checks;                                                                \
    if (!(cond)) {                                                             \
      ++g_failures;                                                            \
      std::cerr << __FILE__ << ":" << __LINE__ << " CHECK failed: " #cond      \
                << '\n';                                                       \
    }                                                                          \
  } while (0)

#define RUN(test)                                                              \
  do {                                                                         \
    const int before = g_failures;                                             \
    test();                                                                    \
    std::cout << (g_failures == before ? "[ OK ] " : "[FAIL] ") << #test       \
              << '\n';                                                         \
  } while (0)

void testQueueEmptyAndExceptions() {
  ds::ExternalQueue<int> q(4);
  CHECK(q.empty());
  CHECK(q.size() == 0);

  bool thrown = false;
  try {
    (void)q.dequeue();
  } catch (const std::underflow_error &) {
    thrown = true;
  }
  CHECK(thrown);

  thrown = false;
  try {
    ds::ExternalQueue<int> bad(0);
  } catch (const std::invalid_argument &) {
    thrown = true;
  }
  CHECK(thrown);
}

void testQueueFIFOAndIOBound() {
  constexpr std::size_t B = 8;
  constexpr int N = 1000;
  ds::ExternalQueue<int> q(B);
  ds::QueueIOStats stats;

  for (int i = 0; i < N; ++i)
    q.enqueue(i, &stats);
  CHECK(q.size() == N);

  for (int i = 0; i < N; ++i) {
    CHECK(q.front(&stats) == i);
    CHECK(q.dequeue(&stats) == i);
  }
  CHECK(q.empty());
  // 2 * N operations: total I/Os <= 2 * (N / B)
  CHECK(stats.total_ios() <= 2 * ((N + B - 1) / B));
}

void testQueueRandomizedAgainstStdQueue() {
  constexpr std::size_t B = 5;
  constexpr int OPS = 5000;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> op_dist(0, 99);

  ds::ExternalQueue<int> q(B);
  std::queue<int> ref;
  ds::QueueIOStats stats;

  for (int step = 0; step < OPS; ++step) {
    if (ref.empty() || op_dist(rng) < 55) {
      q.enqueue(step, &stats);
      ref.push(step);
    } else {
      CHECK(q.front(&stats) == ref.front());
      CHECK(q.dequeue(&stats) == ref.front());
      ref.pop();
    }
    CHECK(q.size() == ref.size());
  }

  while (!ref.empty()) {
    CHECK(q.dequeue(&stats) == ref.front());
    ref.pop();
  }
  CHECK(q.empty());
  CHECK(stats.total_ios() <= 2 * ((OPS + B - 1) / B));
}

void testDequeEmptyAndExceptions() {
  ds::ExternalDeque<int> d(4);
  CHECK(d.empty());
  CHECK(d.size() == 0);

  bool thrown = false;
  try {
    (void)d.pop_front();
  } catch (const std::underflow_error &) {
    thrown = true;
  }
  CHECK(thrown);

  thrown = false;
  try {
    (void)d.pop_back();
  } catch (const std::underflow_error &) {
    thrown = true;
  }
  CHECK(thrown);
}

void testDequeAdversarialPingPong() {
  // Fill enough to push blocks onto disk, then alternate push_front / pop_front
  // and push_back / pop_back at the boundary to verify 2B hysteresis prevents
  // O(n) disk thrashing!
  constexpr std::size_t B = 16;
  ds::ExternalDeque<int> d(B);
  std::deque<int> ref;

  for (int i = 0; i < 100; ++i) {
    d.push_back(i);
    ref.push_back(i);
  }
  CHECK(d.num_disk_blocks() > 0);

  ds::QueueIOStats stats;
  constexpr int PING_PONG = 4000;
  for (int i = 0; i < PING_PONG; ++i) {
    d.push_front(-i - 1, &stats);
    ref.push_front(-i - 1);
    CHECK(d.pop_front(&stats) == ref.front());
    ref.pop_front();

    d.push_back(i + 1000, &stats);
    ref.push_back(i + 1000);
    CHECK(d.pop_back(&stats) == ref.back());
    ref.pop_back();
  }

  // With 2B hysteresis, alternating push/pop at the same end triggers 0 I/Os!
  CHECK(stats.total_ios() == 0);
  CHECK(d.size() == ref.size());
}

void testDequeRandomizedAgainstStdDeque() {
  constexpr std::size_t B = 6;
  constexpr int OPS = 6000;
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> op_dist(0, 99);

  ds::ExternalDeque<int> d(B);
  std::deque<int> ref;
  ds::QueueIOStats stats;

  for (int step = 0; step < OPS; ++step) {
    int op = op_dist(rng);
    if (ref.empty() || op < 30) {
      d.push_front(step, &stats);
      ref.push_front(step);
    } else if (op < 60) {
      d.push_back(step, &stats);
      ref.push_back(step);
    } else if (op < 80) {
      CHECK(d.front(&stats) == ref.front());
      CHECK(d.pop_front(&stats) == ref.front());
      ref.pop_front();
    } else {
      CHECK(d.back(&stats) == ref.back());
      CHECK(d.pop_back(&stats) == ref.back());
      ref.pop_back();
    }
    CHECK(d.size() == ref.size());
  }

  while (!ref.empty()) {
    CHECK(d.pop_front(&stats) == ref.front());
    ref.pop_front();
  }
  CHECK(d.empty());
  CHECK(stats.total_ios() <= 2 * ((OPS + B - 1) / B));
}

void testPerformanceSanity() {
  constexpr std::size_t B = 64;
  constexpr int N = 200'000;

  ds::ExternalQueue<int> q(B);
  ds::QueueIOStats q_stats;
  for (int i = 0; i < N; ++i)
    q.enqueue(i, &q_stats);
  for (int i = 0; i < N; ++i)
    CHECK(q.dequeue(&q_stats) == i);
  CHECK(q_stats.total_ios() <= 2 * ((N + B - 1) / B));

  ds::ExternalDeque<int> d(B);
  ds::QueueIOStats d_stats;
  for (int i = 0; i < N; ++i) {
    if (i % 2 == 0)
      d.push_front(i, &d_stats);
    else
      d.push_back(i, &d_stats);
  }
  for (int i = 0; i < N; ++i) {
    if (i % 2 == 0)
      (void)d.pop_front(&d_stats);
    else
      (void)d.pop_back(&d_stats);
  }
  CHECK(d.empty());
  CHECK(d_stats.total_ios() <= 2 * ((N + B - 1) / B));
}

} // namespace

int main() {
  RUN(testQueueEmptyAndExceptions);
  RUN(testQueueFIFOAndIOBound);
  RUN(testQueueRandomizedAgainstStdQueue);
  RUN(testDequeEmptyAndExceptions);
  RUN(testDequeAdversarialPingPong);
  RUN(testDequeRandomizedAgainstStdDeque);
  RUN(testPerformanceSanity);

  std::cout << '\n'
            << (g_checks - g_failures) << '/' << g_checks << " checks passed\n";
  return g_failures;
}
