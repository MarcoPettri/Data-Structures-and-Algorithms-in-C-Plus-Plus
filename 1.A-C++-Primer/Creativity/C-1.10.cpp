// Exercise Creativity : 1.10

/*
		Write an efficient C++ function that takes any integer value i and returns
		 2^i, as a long value. Your function should not multiply 2 by itself i times;
		 there are much faster ways of computing 2^i.
*/

#include<iostream>


long powerTwo(int i) { return 1 << i; }

int main() 
{

	for (int i{ 0 }; i < 11; i++) 
	{
		std::cout << "pow(2, " << i << ") = " 
			<< powerTwo(i) << std::endl;
	}

	return 0;
}