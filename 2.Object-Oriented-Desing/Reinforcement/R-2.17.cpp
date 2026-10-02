// Exercise Reinforcement: R-2.17
/*

    Write a short program that takes as input three integers, a, b, and c, and
    determines if they can be used in a correct arithmetic formula (in the given
    order), like “a + b = c,” “a = b − c,” or “a ∗ b = c.”

*/

#include<iostream>

int main() {
    int a, b, c;
    std::cout << "Enter three integers: ";
    std::cin >> a >> b >> c;

    auto isValidFormula = [](int a, int b, int c) -> bool {
        // Use 64-bit arithmetic to avoid overflow in sums/products
        long long A = a, B = b, C = c;

        // Form 1: a (op) b = c
        if (A + B == C) return true;               // a + b = c
        if (A - B == C) return true;               // a - b = c
        if (A * B == C) return true;               // a * b = c
        if (B != 0 && A == C * B) return true;     // a / b = c  (exact division)

        // Form 2: a = b (op) c
        if (A == B + C) return true;               // a = b + c
        if (A == B - C) return true;               // a = b - c
        if (A == B * C) return true;               // a = b * c
        if (C != 0 && B == A * C) return true;     // a = b / c  (exact division)

        return false;
    };

    std::cout << std::boolalpha;
    std::cout << isValidFormula(a, b, c) << std::endl;
    return 0;
}