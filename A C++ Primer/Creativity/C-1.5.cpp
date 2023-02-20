// Exercise Creativity: 1.5
/*
		Write a C++ function that takes an array containing the set of all integers
		 in the range 1 to 52 and shuffles it into random order. Use the built-in func-
		 tion rand, which returns a pseudo-random integer each time it is called.
		 Your function should output each possible order with equal probability.
*/

#include<iostream>
#include<iomanip>
#include<cstdlib>
#include<ctime>

void shuffle(int[], std::size_t);
void display(const int*, const std::size_t&);

int main()
{
	const std::size_t SIZE{ 52 };
	int arr[SIZE]{
		 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11, 12,
		13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
		23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
		43, 44, 45, 46, 47, 48, 49, 50, 51, 52
	};
	
	std::cout << "\t before" << std::endl;
	display(arr, SIZE);

	shuffle(arr, SIZE);

	std::cout << "\n\t After" << std::endl;
	display(arr, SIZE);

	return 0;
}
void shuffle(int array[], std::size_t size)
{

	std::srand(static_cast<unsigned>(std::time(nullptr)));
	while (size > 1)
	{
		size--;
		std::size_t j{ size - 1 > 0 ? std::rand() % (size - 1) : 0 };

		int a{ array[size] };
		int b{ array[j] };

		array[size] = b, array[j] = a;
	}
}
void display(const int* array, const std::size_t& size)
{
	for (std::size_t i{ 0 }; i < size; i++)
	{
		std::cout << std::setw(2) << array[i];
		std::cout << ((i + 1) % 13 == 0 ? '\n' : ' ');
	}

}