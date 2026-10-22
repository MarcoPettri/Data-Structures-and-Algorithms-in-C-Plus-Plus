/**
 * @file tree_exceptions.hpp
 * @brief Exceptions thrown by the tree ADTs when a precondition is violated.
 *
 * All of them derive from TreeError, which derives from std::logic_error:
 * they signal misuse by the caller, not a runtime failure of the environment.
 */
#ifndef C7_TREE_EXCEPTIONS_HPP
#define C7_TREE_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

namespace adt {

/** @brief Base class of every tree-related precondition violation. */
class TreeError : public std::logic_error {
public:
  explicit TreeError(const std::string &what) : std::logic_error(what) {}
  explicit TreeError(const char *what) : std::logic_error(what) {}
};

/** @brief A position is null or does not belong to the tree it was used with.
 */
class InvalidPosition : public TreeError {
public:
  using TreeError::TreeError;
};

/** @brief Navigation past the boundary of the tree (parent of the root,
 *         child of an external node, ...). */
class BoundaryViolation : public TreeError {
public:
  using TreeError::TreeError;
};

/** @brief The operation is not allowed in the current state of the tree. */
class InvalidOperation : public TreeError {
public:
  using TreeError::TreeError;
};

} // namespace adt

#endif // C7_TREE_EXCEPTIONS_HPP