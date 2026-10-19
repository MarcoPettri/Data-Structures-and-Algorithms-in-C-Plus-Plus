/**
 * @file    array_list.hpp
 * @brief   Interface of the generic array-based List ADT.

 *
 * @details
 * ArrayList implements the classic *list ADT* (an indexed sequence with
 * insertion and removal at any rank) on top of a contiguous array obtained
 * from an Allocator. Capacity grows geometrically, so appending is
 * amortised O(1); inserting or removing at rank @c i costs O(size() - i).
 */

#ifndef ADT_ARRAY_LIST_HPP
#define ADT_ARRAY_LIST_HPP

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

/// @brief Namespace for abstract-data-type containers.
namespace adt {

/// @cond INTERNAL
namespace detail {
/// SFINAE helper: enabled only if @p It is at least an input iterator.
/// Stops the range constructor from hijacking calls such as (3, 7).
template <typename It>
using list_input_iterator = std::enable_if_t<
    std::is_convertible_v<typename std::iterator_traits<It>::iterator_category,
                          std::input_iterator_tag>>;
} // namespace detail
/// @endcond

/**
 * @brief   Array-based list ADT with allocator support.
 *
 * @tparam  T          Element type (object type). Need not be
 *                     default-constructible. Operations require
 *                     MoveInsertable / MoveAssignable (and CopyInsertable
 *                     where a `const T&` is accepted), as documented below.
 * @tparam  Allocator  Allocator satisfying the C++ Allocator requirements.
 *                     Fancy (non-raw) pointers are not supported.
 *
 * @par Positions
 * Positions are zero-based ranks. Methods taking an index throw
 * @c std::out_of_range when it is invalid, except operator[], front(),
 * back(), pop_front() and pop_back(), which only assert in debug builds.
 *
 * @par Iterator invalidation
 * Anything that reallocates invalidates all iterators, pointers and
 * references. insert/remove invalidate everything at or after the
 * modification point.
 *
 * @par Exception safety
 * - Insertion that reallocates: strong guarantee if T is copy-insertable or
 *   nothrow-move-constructible.
 * - Insertion without reallocation, removal: basic guarantee.
 * - Destructor, clear(), pop_back(), swap(), move construction: @c noexcept.
 */
template <typename T, typename Allocator = std::allocator<T>> class ArrayList {
  using traits_ = std::allocator_traits<Allocator>;

  static_assert(std::is_object_v<T>, "ArrayList requires an object type");
  static_assert(std::is_same_v<typename Allocator::value_type, T>,
                "Allocator::value_type must be T");
  static_assert(std::is_same_v<typename traits_::pointer, T *>,
                "Fancy pointers are not supported");

public:
  /// @name Member types
  /// @{
  using value_type = T;
  using allocator_type = Allocator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T &;
  using const_reference = const T &;
  using pointer = T *;
  using const_pointer = const T *;
  using iterator = T *;             ///< Random-access (contiguous) iterator.
  using const_iterator = const T *; ///< Constant random-access iterator.
  /// @}

  /// @brief Value returned by index_of() when the element is not found.
  static constexpr size_type npos = static_cast<size_type>(-1);

  /// @name Construction, destruction, assignment
  /// @{

  /// @brief Constructs an empty list.
  ArrayList() noexcept(std::is_nothrow_default_constructible_v<Allocator>)
      : ArrayList(Allocator()) {}

  /// @brief Constructs an empty list using @p alloc.
  explicit ArrayList(const allocator_type &alloc) noexcept : alloc_(alloc) {}

  /**
   * @brief Constructs @p count copies of @p value.
   * @throws std::length_error if @p count exceeds max_size().
   */
  ArrayList(size_type count, const T &value,
            const allocator_type &alloc = allocator_type());

  /// @brief Constructs from the range [@p first, @p last) (input iterators or
  /// better).
  template <typename InputIt, typename = detail::list_input_iterator<InputIt>>
  ArrayList(InputIt first, InputIt last,
            const allocator_type &alloc = allocator_type());

  /// @brief Constructs from an initializer list.
  ArrayList(std::initializer_list<T> init,
            const allocator_type &alloc = allocator_type());

  /// @brief Copy constructor.
  ArrayList(const ArrayList &other);

  /// @brief Move constructor; steals the buffer in O(1), leaving @p other
  /// empty.
  ArrayList(ArrayList &&other) noexcept;

  /// @brief Destroys all elements and releases the storage.
  ~ArrayList();

  /// @brief Copy assignment (honours allocator propagation traits).
  ArrayList &operator=(const ArrayList &other);

  /// @brief Move assignment (honours allocator propagation traits).
  ArrayList &operator=(ArrayList &&other) noexcept(
      traits_::propagate_on_container_move_assignment::value ||
      traits_::is_always_equal::value);

  /// @brief Replaces the contents with @p init.
  ArrayList &operator=(std::initializer_list<T> init);

  /// @brief Returns a copy of the allocator.
  allocator_type get_allocator() const noexcept { return alloc_; }
  /// @}

  /// @name Element access
  /// @{

  /// @brief Bounds-checked access. @throws std::out_of_range if @p index >=
  /// size().
  reference at(size_type index);
  /// @copydoc at(size_type)
  const_reference at(size_type index) const;

  /// @brief Unchecked access. @pre index < size().
  reference operator[](size_type index) noexcept;
  /// @copydoc operator[](size_type)
  const_reference operator[](size_type index) const noexcept;

  /// @brief First element. @pre !empty().
  reference front() noexcept;
  /// @copydoc front()
  const_reference front() const noexcept;

  /// @brief Last element. @pre !empty().
  reference back() noexcept;
  /// @copydoc back()
  const_reference back() const noexcept;

  /// @brief Pointer to the contiguous storage (@c nullptr when no storage is
  /// held).
  pointer data() noexcept { return data_; }
  /// @copydoc data()
  const_pointer data() const noexcept { return data_; }

  /// @brief Replaces the element at @p index with a copy of @p value.
  /// @throws std::out_of_range if @p index >= size().
  void set(size_type index, const T &value) { at(index) = value; }
  /// @brief Replaces the element at @p index by moving @p value in.
  /// @throws std::out_of_range if @p index >= size().
  void set(size_type index, T &&value) { at(index) = std::move(value); }
  /// @}

  /// @name Iterators
  /// @{
  iterator begin() noexcept { return data_; }
  const_iterator begin() const noexcept { return data_; }
  const_iterator cbegin() const noexcept { return data_; }
  iterator end() noexcept { return data_ + size_; }
  const_iterator end() const noexcept { return data_ + size_; }
  const_iterator cend() const noexcept { return data_ + size_; }
  /// @}

  /// @name Capacity
  /// @{

  /// @brief Number of elements.
  size_type size() const noexcept { return size_; }
  /// @brief @c true if the list holds no elements.
  bool empty() const noexcept { return size_ == 0; }
  /// @brief Number of elements that fit without reallocating.
  size_type capacity() const noexcept { return cap_; }
  /// @brief Largest possible number of elements.
  size_type max_size() const noexcept { return traits_::max_size(alloc_); }

  /// @brief Ensures capacity() >= @p new_cap. @throws std::length_error if too
  /// large.
  void reserve(size_type new_cap);

  /// @brief Reduces capacity() to exactly size() (releases storage when empty).
  void shrink_to_fit();
  /// @}

  /// @name Insertion
  /// @{

  /**
   * @brief Constructs an element in place at rank @p index.
   *
   * Elements previously at ranks >= @p index shift one place right.
   * Arguments may alias elements of this list.
   *
   * @return Reference to the new element.
   * @throws std::out_of_range if @p index > size().
   * @par Complexity O(size() - index), plus reallocation when full.
   */
  template <typename... Args>
  reference emplace(size_type index, Args &&...args);

  /// @brief Inserts a copy of @p value at rank @p index. @see emplace()
  reference insert(size_type index, const T &value) {
    return emplace(index, value);
  }
  /// @brief Inserts @p value (by move) at rank @p index. @see emplace()
  reference insert(size_type index, T &&value) {
    return emplace(index, std::move(value));
  }

  /// @brief Constructs an element at the end. Amortised O(1).
  template <typename... Args> reference emplace_back(Args &&...args) {
    return emplace(size_, std::forward<Args>(args)...);
  }
  /// @brief Appends a copy of @p value.
  void push_back(const T &value) { emplace_back(value); }
  /// @brief Appends @p value by move.
  void push_back(T &&value) { emplace_back(std::move(value)); }

  /// @brief Constructs an element at the front. O(size()).
  template <typename... Args> reference emplace_front(Args &&...args) {
    return emplace(0, std::forward<Args>(args)...);
  }
  /// @brief Prepends a copy of @p value.
  void push_front(const T &value) { emplace_front(value); }
  /// @brief Prepends @p value by move.
  void push_front(T &&value) { emplace_front(std::move(value)); }
  /// @}

  /// @name Removal
  /// @{

  /**
   * @brief Removes and returns the element at rank @p index.
   * @throws std::out_of_range if @p index >= size().
   * @par Complexity O(size() - index).
   */
  T remove_at(size_type index);

  /// @brief Removes the last element. @pre !empty(). O(1).
  void pop_back() noexcept;

  /// @brief Removes the first element. @pre !empty(). O(size()).
  void pop_front();

  /**
   * @brief Removes the first element equal to @p value.
   * @return @c true if an element was removed.
   * @pre T is EqualityComparable.
   */
  bool remove(const T &value);

  /// @brief Destroys all elements; capacity is unchanged.
  void clear() noexcept;
  /// @}

  /// @name Searching
  /// @{

  /// @brief Rank of the first element equal to @p value, or #npos. @pre T is
  /// EqualityComparable.
  size_type index_of(const T &value) const;
  /// @brief @c true if an element equal to @p value exists.
  bool contains(const T &value) const { return index_of(value) != npos; }
  /// @}

  /// @brief Exchanges contents with @p other in O(1).
  void
  swap(ArrayList &other) noexcept(traits_::propagate_on_container_swap::value ||
                                  traits_::is_always_equal::value);

private:
  /// @cond INTERNAL
  pointer allocate_(size_type n);
  void deallocate_(pointer p, size_type n) noexcept;
  void destroy_range_(pointer first, pointer last) noexcept;
  void release_() noexcept;
  void relocate_(pointer src, size_type n, pointer dst);
  void reallocate_(size_type new_cap);
  size_type grow_capacity_(size_type required) const;
  void check_index_(size_type index, size_type limit) const;
  void erase_at_(size_type index);
  void steal_(ArrayList &other) noexcept;
  template <typename It> void append_(It first, It last);
  /// @endcond

  allocator_type alloc_;   ///< Allocator instance.
  pointer data_ = nullptr; ///< Start of storage.
  size_type size_ = 0;     ///< Number of live elements.
  size_type cap_ = 0;      ///< Allocated slots.
};

/// @name Non-member functions
/// @{
/// @brief Element-wise equality. @pre T is EqualityComparable.
template <typename T, typename A>
bool operator==(const ArrayList<T, A> &l, const ArrayList<T, A> &r);
/// @brief Negation of operator==.
template <typename T, typename A>
bool operator!=(const ArrayList<T, A> &l, const ArrayList<T, A> &r);
/// @brief ADL-friendly swap; calls l.swap(r).
template <typename T, typename A>
void swap(ArrayList<T, A> &l, ArrayList<T, A> &r) noexcept(noexcept(l.swap(r)));
/// @}

} // namespace adt

#include "array_list.tpp"

#endif // ADT_ARRAY_LIST_HPP