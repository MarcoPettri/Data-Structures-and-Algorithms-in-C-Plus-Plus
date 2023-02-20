// Exercise Reinforcement: 1.19

/*
		Write a short C++ function that takes an integer n and returns the sum of
		 all the odd integers smaller than n
*/

#include<iostream>

int sumOdd(int n) { return (n / 2) * (n / 2); }

int main() {
	int test{ 0 };

	for (int odd{ 1 }; odd < 15; odd += 2) { test += odd; }

	std::cout << sumOdd(15) << std::endl;
	std::cout << test << std::endl;
	return 0;
}