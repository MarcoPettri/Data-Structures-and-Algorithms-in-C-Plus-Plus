/**
 * @file    array_vector.hpp
 * @brief   Interface of the generic, allocator-aware ArrayVector container.
 *
 * @details
 * ArrayVector is a contiguous, dynamically resizable sequence container
 * (the "vector ADT"). Elements are stored in a single heap block obtained
 * from an Allocator; the block grows geometrically so that appending is
 * amortised O(1).
 */

#ifndef ADT_ARRAY_VECTOR_HPP
#define ADT_ARRAY_VECTOR_HPP

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>

/// @brief Namespace for abstract-data-type containers.
namespace adt {

/// @cond INTERNAL
namespace detail {
/// SFINAE helper: enabled only if @p It is at least an input iterator.
/// Prevents the range overloads from hijacking calls such as (5, 3).
template <typename It>
using require_input_iterator = std::enable_if_t<
    std::is_convertible_v<typename std::iterator_traits<It>::iterator_category,
                          std::input_iterator_tag>>;
} // namespace detail
/// @endcond

/**
 * @brief   Contiguous, growable array with allocator support.
 *
 * @tparam  T          Element type. Must be Erasable; operations additionally
 *                     require MoveInsertable / CopyInsertable /
 *                     MoveAssignable as documented per member. T does **not**
 *                     need to be default-constructible.
 * @tparam  Allocator  Allocator type satisfying the C++ Allocator
 *                     requirements. Fancy (non-raw) pointers are not
 *                     supported.
 *
 * @par Iterator invalidation
 * Any operation that reallocates (growth, reserve(), shrink_to_fit())
 * invalidates all iterators, pointers and references. insert()/erase()
 * invalidate everything at or after the modification point.
 *
 * @par Exception safety
 * - push_back()/emplace_back(), reserve(), shrink_to_fit(): strong guarantee
 *   (if @c T is copy-insertable or nothrow-move-constructible).
 * - insert()/emplace()/erase(): basic guarantee.
 * - Destructor, clear(), pop_back(), swap(), move construction: @c noexcept.
 */
template <typename T, typename Allocator = std::allocator<T>>
class ArrayVector {
  static_assert(std::is_object_v<T>, "ArrayVector requires an object type");
  static_assert(std::is_same_v<typename Allocator::value_type, T>,
                "Allocator::value_type must be T");

  using traits_ = std::allocator_traits<Allocator>;
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
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  /// @}

  /// @name Construction, destruction, assignment
  /// @{

  /// @brief Constructs an empty vector with a default-constructed allocator.
  ArrayVector() noexcept(std::is_nothrow_default_constructible_v<Allocator>)
      : ArrayVector(Allocator()) {}

  /// @brief Constructs an empty vector using @p alloc.
  explicit ArrayVector(const allocator_type &alloc) noexcept : alloc_(alloc) {}

  /**
   * @brief Constructs @p count value-initialised elements.
   * @param count Number of elements.
   * @param alloc Allocator to use.
   * @throws std::length_error if @p count exceeds max_size().
   * @pre   T is DefaultInsertable.
   */
  explicit ArrayVector(size_type count,
                       const allocator_type &alloc = allocator_type());

  /**
   * @brief Constructs @p count copies of @p value.
   * @pre   T is CopyInsertable.
   */
  ArrayVector(size_type count, const T &value,
              const allocator_type &alloc = allocator_type());

  /**
   * @brief Constructs from the range [@p first, @p last).
   * @tparam InputIt Input iterator whose value type is convertible to T.
   */
  template <typename InputIt,
            typename = detail::require_input_iterator<InputIt>>
  ArrayVector(InputIt first, InputIt last,
              const allocator_type &alloc = allocator_type());

  /// @brief Constructs from an initializer list.
  ArrayVector(std::initializer_list<T> init,
              const allocator_type &alloc = allocator_type());

  /// @brief Copy constructor (allocator obtained via
  /// select_on_container_copy_construction).
  ArrayVector(const ArrayVector &other);

  /// @brief Move constructor; steals the buffer, leaves @p other empty. O(1).
  ArrayVector(ArrayVector &&other) noexcept;

  /// @brief Destroys all elements and releases the storage.
  ~ArrayVector();

  /// @brief Copy assignment (honours propagate_on_container_copy_assignment).
  ArrayVector &operator=(const ArrayVector &other);

  /// @brief Move assignment (honours propagate_on_container_move_assignment).
  ArrayVector &operator=(ArrayVector &&other) noexcept(
      traits_::propagate_on_container_move_assignment::value ||
      traits_::is_always_equal::value);

  /// @brief Replaces the contents with the elements of @p init.
  ArrayVector &operator=(std::initializer_list<T> init);

  /// @brief Replaces the contents with @p count copies of @p value.
  void assign(size_type count, const T &value);

  /// @brief Replaces the contents with the range [@p first, @p last).
  template <typename InputIt,
            typename = detail::require_input_iterator<InputIt>>
  void assign(InputIt first, InputIt last);

  /// @brief Replaces the contents with the elements of @p init.
  void assign(std::initializer_list<T> init);

  /// @brief Returns a copy of the allocator.
  allocator_type get_allocator() const noexcept { return alloc_; }
  /// @}

  /// @name Element access
  /// @{

  /**
   * @brief Bounds-checked access.
   * @param i Zero-based index.
   * @throws std::out_of_range if @p i >= size().
   */
  reference at(size_type i);
  /// @copydoc at(size_type)
  const_reference at(size_type i) const;

  /// @brief Unchecked access (asserts in debug builds). @pre i < size().
  reference operator[](size_type i) noexcept;
  /// @copydoc operator[](size_type)
  const_reference operator[](size_type i) const noexcept;

  /// @brief First element. @pre !empty().
  reference front() noexcept;
  /// @copydoc front()
  const_reference front() const noexcept;

  /// @brief Last element. @pre !empty().
  reference back() noexcept;
  /// @copydoc back()
  const_reference back() const noexcept;

  /// @brief Pointer to the underlying contiguous storage (may be @c nullptr
  /// when empty).
  pointer data() noexcept { return data_; }
  /// @copydoc data()
  const_pointer data() const noexcept { return data_; }
  /// @}

  /// @name Iterators
  /// @{
  iterator begin() noexcept { return data_; }
  const_iterator begin() const noexcept { return data_; }
  const_iterator cbegin() const noexcept { return data_; }
  iterator end() noexcept { return data_ + size_; }
  const_iterator end() const noexcept { return data_ + size_; }
  const_iterator cend() const noexcept { return data_ + size_; }
  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }
  /// @}

  /// @name Capacity
  /// @{

  /// @brief Number of elements.
  size_type size() const noexcept { return size_; }
  /// @brief @c true if the vector holds no elements.
  bool empty() const noexcept { return size_ == 0; }
  /// @brief Number of elements that fit without reallocating.
  size_type capacity() const noexcept { return cap_; }
  /// @brief Largest possible number of elements.
  size_type max_size() const noexcept { return traits_::max_size(alloc_); }

  /**
   * @brief Ensures capacity() >= @p new_cap.
   * @throws std::length_error if @p new_cap > max_size().
   * @par Complexity O(size()) when it reallocates, O(1) otherwise.
   */
  void reserve(size_type new_cap);

  /// @brief Reduces capacity() to size() (non-binding in std::vector, exact
  /// here).
  void shrink_to_fit();
  /// @}

  /// @name Modifiers
  /// @{

  /// @brief Destroys all elements; capacity is unchanged. O(size()).
  void clear() noexcept;

  /**
   * @brief Appends an element constructed in place.
   * @return Reference to the new element.
   * @par Complexity Amortised O(1).
   * @note Arguments may alias elements of this vector.
   */
  template <typename... Args> reference emplace_back(Args &&...args);

  /// @brief Appends a copy of @p value. @pre T is CopyInsertable.
  void push_back(const T &value) { emplace_back(value); }
  /// @brief Appends @p value by move. @pre T is MoveInsertable.
  void push_back(T &&value) { emplace_back(std::move(value)); }

  /// @brief Removes the last element. @pre !empty(). O(1).
  void pop_back() noexcept;

  /**
   * @brief Constructs an element in place before @p pos.
   * @return Iterator to the inserted element.
   * @par Complexity O(end() - pos) (plus reallocation if needed).
   */
  template <typename... Args>
  iterator emplace(const_iterator pos, Args &&...args);

  /// @brief Inserts a copy of @p value before @p pos.
  iterator insert(const_iterator pos, const T &value);
  /// @brief Inserts @p value (by move) before @p pos.
  iterator insert(const_iterator pos, T &&value);
  /// @brief Inserts @p count copies of @p value before @p pos.
  iterator insert(const_iterator pos, size_type count, const T &value);
  /// @brief Inserts the range [@p first, @p last) before @p pos. The range must
  /// not refer into *this.
  template <typename InputIt,
            typename = detail::require_input_iterator<InputIt>>
  iterator insert(const_iterator pos, InputIt first, InputIt last);
  /// @brief Inserts the elements of @p init before @p pos.
  iterator insert(const_iterator pos, std::initializer_list<T> init);

  /**
   * @brief Erases the element at @p pos.
   * @return Iterator following the erased element.
   * @pre   @p pos is a valid dereferenceable iterator.
   */
  iterator erase(const_iterator pos);

  /// @brief Erases the range [@p first, @p last). @return Iterator following
  /// the last erased element.
  iterator erase(const_iterator first, const_iterator last);

  /// @brief Resizes to @p count elements, value-initialising new ones.
  void resize(size_type count);
  /// @brief Resizes to @p count elements, copying @p value into new ones.
  void resize(size_type count, const T &value);

  /**
   * @brief Exchanges contents with @p other. O(1).
   * @note  Swaps allocators only if propagate_on_container_swap is true.
   */
  void swap(ArrayVector &other) noexcept(
      traits_::propagate_on_container_swap::value ||
      traits_::is_always_equal::value);
  /// @}

private:
  /// @name Implementation helpers (see array_vector.tpp)
  /// @{
  pointer allocate_(size_type n);
  void deallocate_(pointer p, size_type n) noexcept;
  void destroy_range_(pointer first, pointer last) noexcept;
  void release_() noexcept; ///< Destroy all + free; back to empty state.
  void truncate_(
      size_type new_size) noexcept; ///< Destroy elements at index >= new_size.
  void relocate_(pointer dest);     ///< Move/copy [0,size_) into raw @p dest.
  void reallocate_(size_type new_cap); ///< Move to a new block of @p new_cap.
  size_type next_capacity_(size_type required) const;
  void check_room_(size_type extra) const; ///< Throws length_error on overflow.
  template <typename It> void append_range_(It first, It last);
  void steal_(ArrayVector &other) noexcept;
  /// @}

  allocator_type alloc_;   ///< Allocator instance.
  pointer data_ = nullptr; ///< Start of storage.
  size_type size_ = 0;     ///< Number of live elements.
  size_type cap_ = 0;      ///< Allocated slots.
};

/// @name Non-member functions
/// @{
/// @brief Lexicographical comparisons (require the corresponding operator on
/// T).
template <typename T, typename A>
bool operator==(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r);
template <typename T, typename A>
bool operator!=(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r);
template <typename T, typename A>
bool operator<(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r);
template <typename T, typename A>
bool operator<=(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r);
template <typename T, typename A>
bool operator>(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r);
template <typename T, typename A>
bool operator>=(const ArrayVector<T, A> &l, const ArrayVector<T, A> &r);

/// @brief ADL-friendly swap; calls l.swap(r).
template <typename T, typename A>
void swap(ArrayVector<T, A> &l,
          ArrayVector<T, A> &r) noexcept(noexcept(l.swap(r)));
/// @}

} // namespace adt

#include "array_vector.tpp"

#endif // ADT_ARRAY_VECTOR_HPP