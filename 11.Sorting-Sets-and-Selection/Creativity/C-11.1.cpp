// File : C-11.1.cpp

/*
    C-11.1 Describe an efficient algorithm for converting a dictionary, D, implemented
    with a linked list, into a map, M, implemented with a linked list, so that
    each key in D has an entry in M, and the relative order of entries in M is
    the same as their relative order in D.
*/


#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <chrono>
#include <memory>
#include "dictionary_to_map.hpp"

// Helper function to append a node to a linked list for testing.
// Use the concrete node type rather than assuming the exact template signature of
// the list node implementation, which keeps the test helper compatible with the
// linked-list shape used by the project.
template <typename T, typename K, typename V>
void append_node(std::unique_ptr<T>& head, const K& key, const V& val) {
    auto new_node = std::make_unique<T>(key, val);
    if (!head) {
        head = std::move(new_node);
    } else {
        auto* curr = head.get();
        while (curr->next) {
            curr = curr->next.get();
        }
        curr->next = std::move(new_node);
    }
}

// Helper function to extract keys from linked list into a vector for verification
template <typename K, typename V>
std::vector<K> extract_keys(const dsa::LinkedListNode<K, V>* head) {
    std::vector<K> keys;
    const auto* curr = head;
    while (curr) {
        keys.push_back(curr->key);
        curr = curr->next.get();
    }
    return keys;
}

void run_tests() {
    std::cout << "[*] Running unit tests for dictionary-to-map conversion...\n";

    // Test 1: Standard case with duplicate keys, verifying relative order & first occurrence preservation
    {
        std::unique_ptr<dsa::LinkedListNode<int, std::string>> dict = nullptr;
        append_node(dict, 1, "one-a");
        append_node(dict, 2, "two-a");
        append_node(dict, 1, "one-b");
        append_node(dict, 3, "three");
        append_node(dict, 2, "two-b");

        auto map_head = dsa::convert_dictionary_to_map(dict.get());
        std::vector<int> expected_keys = {1, 2, 3};
        std::vector<int> actual_keys = extract_keys(map_head.get());

        assert(actual_keys == expected_keys);
        
        // Verify values correspond to the first occurrence
        auto* curr = map_head.get();
        assert(curr->key == 1 && curr->value == "one-a");
        curr = curr->next.get();
        assert(curr->key == 2 && curr->value == "two-a");
        curr = curr->next.get();
        assert(curr->key == 3 && curr->value == "three");
    }

    // Test 2: Empty dictionary
    {
        std::unique_ptr<dsa::LinkedListNode<int, int>> dict = nullptr;
        auto map_head = dsa::convert_dictionary_to_map(dict.get());
        assert(map_head == nullptr);
    }

    // Test 3: All unique keys
    {
        std::unique_ptr<dsa::LinkedListNode<char, int>> dict = nullptr;
        append_node(dict, 'a', 10);
        append_node(dict, 'b', 20);
        append_node(dict, 'c', 30);

        auto map_head = dsa::convert_dictionary_to_map(dict.get());
        std::vector<char> expected_keys = {'a', 'b', 'c'};
        assert(extract_keys(map_head.get()) == expected_keys);
    }

    // Test 4: All duplicate keys
    {
        std::unique_ptr<dsa::LinkedListNode<int, std::string>> dict = nullptr;
        append_node(dict, 42, "first");
        append_node(dict, 42, "second");
        append_node(dict, 42, "third");

        auto map_head = dsa::convert_dictionary_to_map(dict.get());
        std::vector<int> expected_keys = {42};
        assert(extract_keys(map_head.get()) == expected_keys);
        assert(map_head->value == "first");
    }

    std::cout << "[+] All functional unit tests passed successfully!\n";
}

void run_stress_test() {
    std::cout << "[*] Running performance stress test (N = 1,000,000 entries)...\n";

    constexpr size_t N = 1'000'000;
    std::unique_ptr<dsa::LinkedListNode<int, int>> dict = nullptr;

    // Build a large dictionary where keys loop from 0 to 999 (heavy duplication)
    for (size_t i = 0; i < N; ++i) {
        int key = static_cast<int>(i % 1000);
        append_node(dict, key, static_cast<int>(i));
    }

    auto start = std::chrono::high_resolution_clock::now();
    auto map_head = dsa::convert_dictionary_to_map(dict.get());
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "[+] Conversion completed in " << elapsed.count() << " ms.\n";

    // Verify map has exactly 1000 unique keys in the order 0, 1, ..., 999
    auto keys = extract_keys(map_head.get());
    assert(keys.size() == 1000);
    for (int i = 0; i < 1000; ++i) {
        assert(keys[i] == i);
    }
    std::cout << "[+] Stress test correctness and uniqueness verified.\n";
}

int main() {
    run_tests();
    run_stress_test();
    std::cout << "[SUCCESS] All dictionary-to-map conversions verified successfully.\n";
    return 0;
}