// Exercise Creativity: C-2.7
/*
    Write a program that consists of three classes, A, B, and C, such that B is
   a subclass of A and C is a subclass of B. Each class should define a member
    variable named “x” (that is, each has its own variable named x). Describe
    a way for a member function in C to access and set A’s version of x to a
    given value, without changing B or C’s version.
*/

#include <iostream>

class A {
public:
  A(int val) : x(val) {}
  int get_x() { return x; }
  void set_x(int val) { x = val; }

private:
  int x;
};

class B : public A {
public:
  B(int val) : A(val), x(val) {}
  int get_x() { return x; }

private:
  int x;
};

class C : public B {
public:
  C(int val) : B(val), x(val) {}
  int get_x() { return x; }

  int get_A_x() { return A::get_x(); }
  void set_A_x(int val) { A::set_x(val); }

private:
  int x;
};

int main() {
  A a(1);
  B b(2);
  C c(3);
  std::cout << a.get_x() << std::endl;
  std::cout << b.get_x() << std::endl;
  std::cout << c.get_x() << std::endl;
  std::cout << "----------------" << std::endl;
  std::cout << c.get_A_x() << std::endl;
  c.set_A_x(10);
  std::cout << c.get_A_x() << std::endl;
  std::cout << "----------------" << std::endl;
  std::cout << a.get_x() << std::endl;
  std::cout << b.get_x() << std::endl;
  std::cout << c.get_x() << std::endl;
  return 0;
}