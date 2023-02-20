// Exercise Reinforcement: 1.18
/*
		Write a short C++ function, isTwoPower, that takes an int i and returns
		 true if and only if i is a power of 2. Do not use multiplication or division,
		 however.

*/

#include<iostream>

bool isTwoPower(int i) { return (i & (i -1)) == 0 ? true : false; }


int main() {

	std::cout << std::boolalpha;

	for (int number{ 1 }; number < 36; number++)
	{
		if(isTwoPower(number))
		{
			std::cout << number << std::endl;
		}
	}

	return 0;
}