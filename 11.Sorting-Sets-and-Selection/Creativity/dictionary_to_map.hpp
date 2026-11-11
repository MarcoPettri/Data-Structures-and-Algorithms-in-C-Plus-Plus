/**
 * @file dictionary_to_map.hpp
 * @brief Converts a linked-list dictionary (with duplicate keys) into a 
 *        linked-list map (with unique keys) in O(n) time, preserving relative order.
 */

#ifndef DICTIONARY_TO_MAP_HPP
#define DICTIONARY_TO_MAP_HPP

#include <iostream>
#include <unordered_set>
#include <memory>
#include <utility>

namespace dsa {

/**
 * @brief Represents a node in a singly linked list storing a key-value pair.
 * 
 * @tparam Key Key type.
 * @tparam Value Value type.
 */
template <typename Key, typename Value>
struct LinkedListNode {
    Key key;
    Value value;
    std::unique_ptr<LinkedListNode<Key, Value>> next;

    LinkedListNode(const Key& k, const Value& v) : key(k), value(v), next(nullptr) {}
    LinkedListNode(Key&& k, Value&& v) : key(std::move(k)), value(std::move(v)), next(nullptr) {}
};

/**
 * @brief Converts a dictionary implemented as a linked list into a map implemented 
 *        as a linked list with unique keys, preserving the relative order of the first 
 *        occurrence of each key.
 * 
 * @tparam Key Key type.
 * @tparam Value Value type.
 * @param head Pointer to the head of the dictionary linked list.
 * @return [[nodiscard]] std::unique_ptr<LinkedListNode<Key, Value>> Head of the new map linked list.
 * 
 * @section complexity
 * - **Time Complexity:** $\mathcal{O}(n)$, where $n$ is the number of entries in the dictionary linked list. 
 *   The algorithm traverses the linked list of size $n$ exactly once. Each hash set lookup and insertion takes $\mathcal{O}(1)$ 
 *   expected time.
 * - **Space Complexity:** $\mathcal{O}(n)$ auxiliary space. The hash set stores up to $n$ unique keys, and the 
 *   resulting map linked list allocates nodes for each unique key-value pair.
 */
template <typename Key, typename Value>
[[nodiscard]] std::unique_ptr<LinkedListNode<Key, Value>> convert_dictionary_to_map(
    const LinkedListNode<Key, Value>* head) {
    
    std::unique_ptr<LinkedListNode<Key, Value>> map_head = nullptr;
    LinkedListNode<Key, Value>* map_tail = nullptr;
    
    // Hash set to track keys already added to M in O(1) expected time
    std::unordered_set<Key> seen_keys;

    const auto* current = head;
    while (current != nullptr) {
        // If key is not yet in the map, insert it (preserving the first occurrence)
        if (seen_keys.find(current->key) == seen_keys.end()) {
            seen_keys.insert(current->key);

            auto new_node = std::make_unique<LinkedListNode<Key, Value>>(current->key, current->value);
            
            if (!map_head) {
                map_head = std::move(new_node);
                map_tail = map_head.get();
            } else {
                map_tail->next = std::move(new_node);
                map_tail = map_tail->next.get();
            }
        }
        current = current->next.get();
    }

    return map_head;
}

} // namespace dsa

#endif // DICTIONARY_TO_MAP_HPP