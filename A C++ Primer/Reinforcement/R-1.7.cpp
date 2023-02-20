// Exercise Reinforcement: R-1.7

/*
	Write a short C++ function that takes an integer n and returns the sum of
	all the integers smaller than n.
*/

#include<iostream>

int sum(int n){ return n * (n - 1) / 2; }

int main() {

	int number;
	std::cout << "Enter a integer: ",
		std::cin >> number;

	std::cout << "The sum of all the integers smaller than " << number
		<< " is " << sum(number);

	return 0;
}

