// Exercise Creativity: C-1.4
/*

		Write a C++ function that takes an STL vector of int values and prints all
		 the odd values in the vector
*/

#include<iostream>
#include<vector>

void displaysOdd(const std::vector<int>&);

int main()
{
	std::vector<int> vect{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

	displaysOdd(vect);
	return 0;
}

void displaysOdd(const std::vector<int>& vect)
{
	for (const int& value : vect)
	{
		if (value & 1)
		{
			std::cout << value << " ";
		}
	}
}