// Exercise Creativity: 1.1
/*
			Write a pseudo-code description of a method that reverses an array of n
			 integers, so that the numbers are listed in the opposite order than they were
			 before, and compare this method to an equivalent C++ method for doing
			 the same thing.
*/

// Answer: see file C-1.1.md

#include<iostream>
#include<cstddef>

void reverseArray(int[], const std::size_t);

void displayArray(const int[], const std::size_t);

int main() 
{
	const std::size_t SIZE{ 6 };
	int array[SIZE]{ 1,2,3,4,5,6, };


	displayArray(array, SIZE);
	
	reverseArray(array, SIZE);

	std::cout << std::endl;

	displayArray(array, SIZE);
	
	return 0;
}

void reverseArray(int arr[], std::size_t size)
{
	size--;
	for (std::size_t start{ 0 }; start < size; start++, size--)
	{
		// the values to be swapped
		int a = arr[start];
		int b = arr[size]; 

		//Swaps the values a and b
		arr[start] = b;
		arr[size] = a;	

	}
}
void displayArray(const int arr[], const std::size_t size)
{
	for (std::size_t i{ 0 }; i < size; i++)
	{
		std::cout << arr[i] << " ";
	}
}