// File : C-5.8.cpp

/*
    C-5.8 Postfix notation is an unambiguous way of writing an arithmetic
   expres- sion without parentheses. It is defined so that if “(exp1) ◦ (exp2)”
   is a normal fully parenthesized expression whose operation is “◦”, then the
    postfix version of this is “pexp1 pexp2◦”, where pexp1 is the postfix ver-
    sion of exp1 and pexp2 is the postfix version of exp2. The postfix version
    of a single number or variable is just that number or variable. So, for ex-
    ample, the postfix version of “((5 + 2) ∗ (8 − 3))/4” is “5 2 + 8 3 − ∗
    4 /”. Describe a nonrecursive way of evaluating an expression in postfix
    notation.
*/

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using Vars = std::map<std::string, double>;

namespace {

bool isOperator(const std::string &t) {
  return t.size() == 1 && std::string("+-*/^").find(t[0]) != std::string::npos;
}

// Accepts forms like 5, 3.14, .5, -2, +7.5, 1e3. Rejects "inf", "nan",
// "0x10"-style surprises by requiring the token to start with a digit or '.'
// (after an optional sign).
bool parseNumber(const std::string &t, double &out) {
  if (t.empty())
    return false;
  std::size_t k = (t[0] == '+' || t[0] == '-') ? 1 : 0;
  if (k >= t.size())
    return false;
  if (!std::isdigit(static_cast<unsigned char>(t[k])) && t[k] != '.')
    return false;
  if (t.find_first_of("xXnNiI") != std::string::npos)
    return false;

  char *end = nullptr;
  out = std::strtod(t.c_str(), &end);
  return end == t.c_str() + t.size();
}

double apply(char op, double a, double b) {
  switch (op) {
  case '+':
    return a + b;
  case '-':
    return a - b;
  case '*':
    return a * b;
  case '/':
    if (b == 0.0)
      throw std::runtime_error("division by zero");
    return a / b;
  case '^':
    return std::pow(a, b);
  }
  throw std::runtime_error("unknown operator"); // unreachable
}

} // namespace

// Evaluates a whitespace-separated postfix expression using an explicit stack.
// Throws std::runtime_error on any malformed input.
double evaluatePostfix(const std::string &expr, const Vars &vars = {}) {
  std::vector<double> stack; // top of stack = back()
  std::istringstream in(expr);
  std::string tok;

  while (in >> tok) {
    double value;
    if (isOperator(tok)) {
      if (stack.size() < 2)
        throw std::runtime_error("stack underflow at operator '" + tok + "'");
      double b = stack.back();
      stack.pop_back(); // right operand (top)
      double a = stack.back();
      stack.pop_back(); // left operand
      stack.push_back(apply(tok[0], a, b));
    } else if (parseNumber(tok, value)) {
      stack.push_back(value);
    } else {
      auto it = vars.find(tok);
      if (it == vars.end())
        throw std::runtime_error("unknown token or variable '" + tok + "'");
      stack.push_back(it->second);
    }
  }

  if (stack.empty())
    throw std::runtime_error("empty expression");
  if (stack.size() > 1)
    throw std::runtime_error("too many operands");
  return stack.back();
}

// ------------------------- Tests -------------------------
void testBasics();
void testBookExample();
void testOperandOrder();
void testNegativeNumbersAndWhitespace();
void testVariables();
void testErrors();
void testDeepExpression();

int main() {
  testBasics();
  testBookExample();
  testOperandOrder();
  testNegativeNumbersAndWhitespace();
  testVariables();
  testErrors();
  testDeepExpression();
  std::cout << "All tests passed.\n";
  std::cout << "5 2 + 8 3 - * 4 /  =  " << evaluatePostfix("5 2 + 8 3 - * 4 /")
            << '\n';
  return 0;
}

// ------------------------- Tests -------------------------

bool close(double a, double b) { return std::fabs(a - b) < 1e-9; }

template <typename F> bool throwsError(F &&f) {
  try {
    f();
  } catch (const std::runtime_error &) {
    return true;
  }
  return false;
}

void testBasics() {
  assert(close(evaluatePostfix("42"), 42));
  assert(close(evaluatePostfix("5 2 +"), 7));
  assert(close(evaluatePostfix("1.5 2 *"), 3));
  assert(close(evaluatePostfix("2 3 ^"), 8));
}

void testBookExample() {
  // ((5 + 2) * (8 - 3)) / 4 = 8.75
  assert(close(evaluatePostfix("5 2 + 8 3 - * 4 /"), 8.75));
}

void testOperandOrder() {
  // Non-commutative operators reveal whether a and b are popped in the right
  // order.
  assert(close(evaluatePostfix("3 4 -"), -1));
  assert(close(evaluatePostfix("8 2 /"), 4));
  assert(close(evaluatePostfix("2 3 ^"), 8)); // 2^3, not 3^2
}

void testNegativeNumbersAndWhitespace() {
  assert(close(evaluatePostfix("-3 4 *"), -12));
  assert(close(evaluatePostfix("5 -2 -"), 7));       // 5 - (-2)
  assert(close(evaluatePostfix("  1   2\t+\n"), 3)); // odd whitespace
}

void testVariables() {
  Vars v = {{"x", 3}, {"y", 4}};
  assert(close(evaluatePostfix("x y + 2 *", v), 14));
  assert(close(evaluatePostfix("x x *", v), 9));
}

void testErrors() {
  assert(throwsError([] { evaluatePostfix(""); }));      // empty
  assert(throwsError([] { evaluatePostfix("   "); }));   // only whitespace
  assert(throwsError([] { evaluatePostfix("+"); }));     // underflow
  assert(throwsError([] { evaluatePostfix("1 +"); }));   // underflow
  assert(throwsError([] { evaluatePostfix("1 2"); }));   // leftover operand
  assert(throwsError([] { evaluatePostfix("1 0 /"); })); // divide by zero
  assert(throwsError([] { evaluatePostfix("1 2 &"); })); // unknown token
  assert(throwsError([] { evaluatePostfix("x 1 +"); })); // undefined variable
  assert(throwsError([] { evaluatePostfix("1.2.3 1 +"); })); // malformed number
  assert(throwsError(
      [] { evaluatePostfix("nan 1 +"); })); // not a variable or number
}

void testDeepExpression() {
  // "1 1 + 1 + 1 + ..." with 100000 additions: stays iterative, no recursion
  // depth issue.
  std::string e = "1";
  for (int i = 0; i < 100000; ++i)
    e += " 1 +";
  assert(close(evaluatePostfix(e), 100001));

  // Right-nested: "1 1 1 ... 1 + + ... +" holds all operands at once (O(n)
  // stack).
  std::string r(" 1");
  std::string deep = "1";
  for (int i = 0; i < 50000; ++i)
    deep += r;
  for (int i = 0; i < 50000; ++i)
    deep += " +";
  assert(close(evaluatePostfix(deep), 50001));
}