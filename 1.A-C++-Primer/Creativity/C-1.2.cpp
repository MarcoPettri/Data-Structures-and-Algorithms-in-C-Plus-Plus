// Exercise Creativity: C-1.2
/*
		Write a short C++ function that takes an array of int values and determines
		 if there is a pair of numbers in the array whose product is even

*/

#include<iostream>
#include<cstddef>

bool productEven(const int[], const std::size_t& size);

int main()
{	
	const std::size_t SIZE_1{ 12 }, SIZE_2{ 13 };

	int array[SIZE_1]{ 0, 1, 3, 3, 7, 5, 3, 7, 3, 3, 13, 11 };
	int array2[SIZE_2]{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

	std::cout << std::boolalpha << productEven(array, SIZE_1) << std::endl;
	std::cout << productEven(array2, SIZE_2) << std::endl;


	return 0;

}

bool productEven(const int arr[], const std::size_t& size)
{
	
	for (std::size_t left{ 0 }; left < size; left++){

		for (std::size_t right{ left + 1 }; right < size; right++)
		{
			
			if ((arr[left] && arr[right]) &&
				(arr[left] * arr[right] % 2 == 0))
					return true;
		} 
		
	}
	return false;

}