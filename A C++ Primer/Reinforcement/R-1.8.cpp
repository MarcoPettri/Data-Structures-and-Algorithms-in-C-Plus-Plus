// Exercise Reinforcement : R-1.8

/*
	Write a short C++ function, isMultiple, that takes two positive long values,
	 n and m, and returns true if and only if n is a multiple of m, that is, n = mi
	 for some integer i.

*/

#include<iostream>

bool isMultiple(unsigned long n, unsigned long m) { return  n % m == 0; }

int main() {

	unsigned long number1;
	unsigned long number2;

	std::cout << "Enter two positive long numbers separated by spaces: ",
		std:: cin >> number1 >> number2;

	std::cout << number1 
		<< (isMultiple(number1, number2) ? " is " : " isn't ")
		<< "a multiple of " << number2;

	return 0;
}
