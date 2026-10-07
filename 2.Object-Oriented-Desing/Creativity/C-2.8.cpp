// Exercise Creativity: C-2.8
/*
    Write a set of C++ classes that can simulate an Internet application, where
    one party, Alice, is periodically creating a set of packets that she wants
   to send to Bob. The Internet process is continually checking if Alice has any
    packets to send, and if so, it delivers them to Bob’s computer, and Bob is
    periodically checking if his computer has a packet from Alice, and, if so,
    he reads and deletes it.
*/

/*
internet_sim/
    ├── include/
        ├── Alice.hpp
        ├── Computer.hpp
        ├── Bob.hpp
        ├── Internet.hpp
        ├── Packet.hpp
        ├── Logger.hpp
        ├── PeriodicProcess.hpp
    ├── src/
        ├── Alice.cpp
        ├── Bob.cpp
        ├── Computer.cpp
        ├── Internet.cpp
        ├── Packet.cpp
        ├── Logger.cpp
        ├── PeriodicProcess.cpp
        ├── Internet.cpp
*/

// Runs two checks:
//   1. A single-threaded sanity test of Computer's FIFO behavior.
//   2. The full multi-threaded simulation, followed by conservation checks:
//        created   == delivered + still-in-Alice's-outbox
//        delivered == read      + still-in-Bob's-inbox
//      i.e. no packet is ever lost or duplicated.
//

#include <chrono>
#include <iostream>
#include <thread>

#include "Alice.hpp"
#include "Bob.hpp"
#include "Computer.hpp"
#include "Internet.hpp"
#include "Logger.hpp"

namespace {

int failures = 0;

void check(bool condition, const std::string &description) {
  std::cout << (condition ? "  [PASS] " : "  [FAIL] ") << description << '\n';
  if (!condition)
    ++failures;
}

// ---------------------------------------------------------------------------
void test_computer_Fifo() {
  std::cout << "\n--- Test 1: Computer queues behave FIFO ---\n";
  Computer computer("TestHost");

  computer.queue_out_going(Packet(1, "TestHost", "X", "a"));
  computer.queue_out_going(Packet(2, "TestHost", "X", "b"));
  check(computer.out_going_count() == 2, "outbox holds 2 packets");

  auto out = computer.take_all_out_going();
  check(out.size() == 2 && out[0].id() == 1 && out[1].id() == 2,
        "outbox returns packets oldest-first");
  check(computer.out_going_count() == 0,
        "outbox is empty after take_all_out_going");

  computer.deliver(Packet(3, "X", "TestHost", "c"));
  check(computer.in_coming_count() == 1, "inbox holds 1 packet");
  auto in = computer.take_all_in_coming();
  check(in.size() == 1 && in[0].id() == 3, "inbox returns the packet");
  check(computer.in_coming_count() == 0,
        "inbox is empty after take_all_in_coming");
}

// ---------------------------------------------------------------------------
void test_simulation() {
  using std::chrono::milliseconds;

  std::cout << "\n--- Test 2: full threaded simulation ---\n";

  // Tunable parameters.
  const milliseconds aliceBurstPeriod(500);   // Alice creates packets
  const milliseconds internetPollPeriod(100); // Internet checks for packets
  const milliseconds bobCheckPeriod(800);     // Bob checks his computer
  const int maxPacketsPerBurst = 4;
  const milliseconds runTime(6000);

  Computer aliceComputer("Alice");
  Computer bobComputer("Bob");

  Alice alice(aliceComputer, "Bob", aliceBurstPeriod, maxPacketsPerBurst);
  Internet internet(aliceComputer, bobComputer, internetPollPeriod);
  Bob bob(bobComputer, bobCheckPeriod);

  Logger::log("main", "starting simulation");
  bob.start();
  internet.start();
  alice.start();

  std::this_thread::sleep_for(runTime);

  // Stop in the order packets flow, so downstream parties see as much
  // traffic as possible.
  alice.stop();
  internet.stop();
  bob.stop();
  Logger::log("main", "simulation stopped");

  const int created = alice.total_created();
  const int delivered = internet.total_delivered();
  const int read = bob.total_read();
  const int unsent = static_cast<int>(aliceComputer.out_going_count());
  const int unread = static_cast<int>(bobComputer.in_coming_count());

  std::cout << "\n=== Summary ===\n"
            << "Created by Alice       : " << created << '\n'
            << "Delivered by Internet  : " << delivered << '\n'
            << "Read by Bob            : " << read << '\n'
            << "Unsent in Alice outbox : " << unsent << '\n'
            << "Unread in Bob inbox    : " << unread << "\n\n";

  check(created > 0, "Alice created at least one packet");
  check(created == delivered + unsent, "created == delivered + unsent (nothing "
                                       "lost between Alice and the network)");
  check(
      delivered == read + unread,
      "delivered == read + unread (nothing lost between the network and Bob)");
  check(read > 0, "Bob read at least one packet");
}

} // namespace

int main() {
  test_computer_Fifo();
  test_simulation();

  std::cout << '\n'
            << (failures == 0 ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED")
            << '\n';
  return failures == 0 ? 0 : 1;
}