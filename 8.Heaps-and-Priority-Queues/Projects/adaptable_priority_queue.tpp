/**
 * @file adaptable_priority_queue.tpp
 * @brief Template implementation file for AdaptablePriorityQueue.
 *
 * This file is part of @c adaptable_priority_queue.hpp, which includes it after
 * the class declaration. Do not include it directly.
 */
#ifndef DS_ADAPTABLE_PRIORITY_QUEUE_TPP
#define DS_ADAPTABLE_PRIORITY_QUEUE_TPP

#include "adaptable_priority_queue.hpp"

namespace ds {

// ============================================================================
// Locator Implementation
// ============================================================================

template <typename Key, typename Value, typename Compare>
AdaptablePriorityQueue<Key, Value, Compare>::Locator::Locator(
    std::shared_ptr<Node> node) noexcept
    : node_(std::move(node)) {}

template <typename Key, typename Value, typename Compare>
typename AdaptablePriorityQueue<Key, Value, Compare>::Node &
AdaptablePriorityQueue<Key, Value, Compare>::Locator::checked() const {
  if (!node_) {
    throw std::invalid_argument(
        "AdaptablePriorityQueue::Locator: null locator");
  }
  return *node_;
}

// ============================================================================
// AdaptablePriorityQueue Implementation
// ============================================================================

template <typename Key, typename Value, typename Compare>
AdaptablePriorityQueue<Key, Value, Compare>::AdaptablePriorityQueue()
    : comp_(), tag_(std::make_unique<char>()) {}

template <typename Key, typename Value, typename Compare>
AdaptablePriorityQueue<Key, Value, Compare>::AdaptablePriorityQueue(
    Compare comp)
    : comp_(std::move(comp)), tag_(std::make_unique<char>()) {}

template <typename Key, typename Value, typename Compare>
AdaptablePriorityQueue<Key, Value, Compare> &
AdaptablePriorityQueue<Key, Value, Compare>::operator=(
    AdaptablePriorityQueue &&other) {
  if (this != &other) {
    detachAll();
    comp_ = std::move(other.comp_);
    heap_ = std::move(other.heap_);
    tag_ = std::move(other.tag_);
    other.heap_.clear();
  }
  return *this;
}

template <typename Key, typename Value, typename Compare>
AdaptablePriorityQueue<Key, Value, Compare>::~AdaptablePriorityQueue() {
  detachAll();
}

template <typename Key, typename Value, typename Compare>
typename AdaptablePriorityQueue<Key, Value, Compare>::size_type
AdaptablePriorityQueue<Key, Value, Compare>::size() const noexcept {
  return heap_.size();
}

template <typename Key, typename Value, typename Compare>
bool AdaptablePriorityQueue<Key, Value, Compare>::empty() const noexcept {
  return heap_.empty();
}

template <typename Key, typename Value, typename Compare>
typename AdaptablePriorityQueue<Key, Value, Compare>::key_compare
AdaptablePriorityQueue<Key, Value, Compare>::key_comp() const {
  return comp_;
}

template <typename Key, typename Value, typename Compare>
typename AdaptablePriorityQueue<Key, Value, Compare>::Locator
AdaptablePriorityQueue<Key, Value, Compare>::insert(Key key, Value value) {
  auto node = std::make_shared<Node>(std::move(key), std::move(value),
                                     heap_.size(), tag_.get());
  heap_.push_back(node);
  siftUp(heap_.size() - 1);
  return Locator(std::move(node));
}

template <typename Key, typename Value, typename Compare>
typename AdaptablePriorityQueue<Key, Value, Compare>::Locator
AdaptablePriorityQueue<Key, Value, Compare>::min() const {
  requireNonEmpty();
  return Locator(heap_.front());
}

template <typename Key, typename Value, typename Compare>
typename AdaptablePriorityQueue<Key, Value, Compare>::Locator
AdaptablePriorityQueue<Key, Value, Compare>::removeMin() {
  requireNonEmpty();
  return Locator(removeAt(0));
}

template <typename Key, typename Value, typename Compare>
void AdaptablePriorityQueue<Key, Value, Compare>::remove(const Locator &loc) {
  (void)removeAt(indexOf(loc));
}

template <typename Key, typename Value, typename Compare>
Key AdaptablePriorityQueue<Key, Value, Compare>::replaceKey(const Locator &loc,
                                                            Key newKey) {
  const size_type i = indexOf(loc);
  using std::swap;
  swap(heap_[i]->key, newKey);
  siftDown(siftUp(i));
  return newKey;
}

template <typename Key, typename Value, typename Compare>
Value AdaptablePriorityQueue<Key, Value, Compare>::replaceValue(
    const Locator &loc, Value newValue) {
  (void)indexOf(loc);
  using std::swap;
  swap(loc.node_->value, newValue);
  return newValue;
}

template <typename Key, typename Value, typename Compare>
bool AdaptablePriorityQueue<Key, Value, Compare>::contains(
    const Locator &loc) const noexcept {
  return loc.node_ != nullptr && tag_ != nullptr &&
         loc.node_->owner == tag_.get();
}

template <typename Key, typename Value, typename Compare>
void AdaptablePriorityQueue<Key, Value, Compare>::clear() noexcept {
  detachAll();
}

template <typename Key, typename Value, typename Compare>
bool AdaptablePriorityQueue<Key, Value, Compare>::checkInvariants() const {
  if (!heap_.empty() && tag_ == nullptr)
    return false;
  for (size_type i = 0; i < heap_.size(); ++i) {
    const NodePtr &node = heap_[i];
    if (!node || node->index != i || node->owner != tag_.get())
      return false;
    if (i > 0 && higher(heap_[i], heap_[(i - 1) / 2]))
      return false;
  }
  return true;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

template <typename Key, typename Value, typename Compare>
bool AdaptablePriorityQueue<Key, Value, Compare>::higher(
    const NodePtr &a, const NodePtr &b) const {
  return comp_(a->key, b->key);
}

template <typename Key, typename Value, typename Compare>
void AdaptablePriorityQueue<Key, Value, Compare>::requireNonEmpty() const {
  if (heap_.empty())
    throw std::out_of_range("AdaptablePriorityQueue: queue is empty");
}

template <typename Key, typename Value, typename Compare>
typename AdaptablePriorityQueue<Key, Value, Compare>::size_type
AdaptablePriorityQueue<Key, Value, Compare>::indexOf(const Locator &loc) const {
  if (!contains(loc)) {
    throw std::invalid_argument("AdaptablePriorityQueue: locator is null, "
                                "stale, or belongs to another queue");
  }
  return loc.node_->index;
}

template <typename Key, typename Value, typename Compare>
void AdaptablePriorityQueue<Key, Value, Compare>::swapNodes(size_type i,
                                                            size_type j) {
  std::swap(heap_[i], heap_[j]);
  heap_[i]->index = i;
  heap_[j]->index = j;
}

template <typename Key, typename Value, typename Compare>
typename AdaptablePriorityQueue<Key, Value, Compare>::size_type
AdaptablePriorityQueue<Key, Value, Compare>::siftUp(size_type i) {
  while (i > 0) {
    const size_type parent = (i - 1) / 2;
    if (!higher(heap_[i], heap_[parent]))
      break;
    swapNodes(i, parent);
    i = parent;
  }
  return i;
}

template <typename Key, typename Value, typename Compare>
void AdaptablePriorityQueue<Key, Value, Compare>::siftDown(size_type i) {
  const size_type n = heap_.size();
  for (;;) {
    size_type best = i;
    const size_type left = 2 * i + 1;
    const size_type right = left + 1;
    if (left < n && higher(heap_[left], heap_[best]))
      best = left;
    if (right < n && higher(heap_[right], heap_[best]))
      best = right;
    if (best == i)
      return;
    swapNodes(i, best);
    i = best;
  }
}

template <typename Key, typename Value, typename Compare>
typename AdaptablePriorityQueue<Key, Value, Compare>::NodePtr
AdaptablePriorityQueue<Key, Value, Compare>::removeAt(size_type i) {
  const size_type last = heap_.size() - 1;
  if (i != last)
    swapNodes(i, last);
  NodePtr removed = std::move(heap_.back());
  heap_.pop_back();
  removed->owner = nullptr;
  if (i != last)
    siftDown(siftUp(i));
  return removed;
}

template <typename Key, typename Value, typename Compare>
void AdaptablePriorityQueue<Key, Value, Compare>::detachAll() noexcept {
  for (NodePtr &node : heap_)
    node->owner = nullptr;
  heap_.clear();
}

} // namespace ds

#endif // DS_ADAPTABLE_PRIORITY_QUEUE_TPP