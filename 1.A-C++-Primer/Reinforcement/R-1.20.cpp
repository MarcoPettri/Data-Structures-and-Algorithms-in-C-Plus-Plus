// Exercise Reinforcement: 1.20

/*
		Write a short C++ function that takes a positive double value x and returns
		 the number of times we can divide x by 2 before we get a number less
		 than 2
*/

#include<iostream>

int descom(int);

int main() 
{
	std::cout << descom(25) << std::endl;
	std::cout << descom(152) << std::endl;
	std::cout << descom(2345) << std::endl;


	return 0;
}
int descom(int x) 
{
	int times{ 0 };

	for (; x >= 2; x >>= 1, times++);
	return --times;
}