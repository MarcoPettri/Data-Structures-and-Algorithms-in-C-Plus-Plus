//Exercise Creativity: C-1.3

/*
		Write a C++ function that takes an STL vector of int values and deter-
		 mines if all the numbers are different from each other (that is, they are
		 distinct).
*/

#include<iostream>
#include<vector>


bool occurence(const std::vector<int>&);

int main()
{


	std::vector<int> arr { 9,  4,  7,  1,  2,  9,  1,  6, 11 };
	std::vector<int> arr2 { 319, 258, 232, 120, 375,  88 };

	std::cout << std::boolalpha << occurence(arr) << std::endl;
	std::cout  << occurence(arr2);
	return 0;
}

bool occurence(const std::vector<int>& vect)
{
	

	for (std::size_t target{ 0 }; target < vect.size(); target++)
	{
		for (std::size_t next{ target + 1 }; next < vect.size(); next++)
		{
			if (vect[target] == vect[next])
				return true;
		}
	}
	return false;
}