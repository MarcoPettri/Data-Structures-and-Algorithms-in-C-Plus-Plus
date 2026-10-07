// Exercise Creativity: C-2.4
/*
    Design a class Line that implements a line, which is represented by the for-
mula y = ax + b. Your class should store a and b as double member vari-
ables. Write a member function intersect(ℓ) that returns the x coordinate
at which this line intersects line ℓ. If the two lines are parallel, then your
function should throw an exception Parallel. Write a C++ program that
creates a number of Line objects and tests each pair for intersection.
Your
    program should print an appropriate error message for parallel lines.
*/

#include <cmath>
#include <iostream>
#include <stdexcept>

class Line {
public:
  // Default constructor (sets line to 0x + 0)
  Line() = default;

  // Parameterized constructor
  Line(double a, double b) : a(a), b(b) {}

  double intersect(const Line &rhs) const {
    // Define a tiny threshold for floating-point comparison
    const double EPSILON = 1e-9;

    // Check if the absolute difference between slopes is essentially zero
    if (std::abs(a - rhs.a) < EPSILON) {
      throw std::runtime_error("Parallel lines");
    }

    return (rhs.b - b) / (a - rhs.a);
  }

private:
  double a;
  double b;
  friend std::ostream &operator<<(std::ostream &os, const Line &line) {
    return os << line.a << "x + " << line.b;
  }

  friend std::istream &operator>>(std::istream &is, Line &line) {
    is >> line.a >> line.b;
    return is;
  }
};

int main() {
  Line line1{1, 2};
  Line line2{2, 1};
  std::cout << "Intersection of " << line1 << " and " << line2
            << " is: " << line1.intersect(line2) << std::endl;

  Line line3;
  Line line4;
  try {
    std::cout << "Enter a line (a b): ";
    std::cin >> line3; // (2, 4)
    std::cout << "Enter a line (a b): ";
    std::cin >> line4; // (2, 6)
    double result = line3.intersect(line4);
    std::cout << "Intersection of " << line3 << " and " << line4
              << " is: " << result << std::endl;
  } catch (const std::runtime_error &e) {
    std::cout << e.what() << std::endl;
  } catch (...) {
    std::cout << "Unknown error" << std::endl;
  }

  return 0;
}