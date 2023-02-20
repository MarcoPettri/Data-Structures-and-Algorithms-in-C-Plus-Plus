// Exercise Reinforcement: R-7.9

/*
	Write a C++ function printArray(A, m, n) that prints an m × n two-
	 dimensional array A of integers, declared to be "int** A," to the standard
	 output. Each of the m rows should appear on a separate line.
*/

#include<iostream>
#include<cstddef>

void printArray(int**, std::size_t, std::size_t);

int main() {

	const std::size_t ROWS{ 4 };
	const std::size_t COLUMNS{ 5 };
	const int SIZE{ ROWS * COLUMNS };
	int arr[ROWS][COLUMNS];

	for (int row_column{ 0 }; row_column < SIZE; row_column++) {

		std::size_t row = row_column / COLUMNS;
		std::size_t column = row_column % COLUMNS;
		
		arr[row][column] = row_column + 1;
	}	


	int** arrayPtr = new int* [SIZE];
	for (int row{ 0 }; row < ROWS; row++) { arrayPtr[row] = arr[row]; }
	

	printArray(arrayPtr, ROWS, COLUMNS);

	delete [] arrayPtr;

	return 0;
}

void printArray(int** array, std::size_t m, std::size_t n) {

	int size = m * n;

	for (std::size_t row_column{ 0 }; row_column < size; row_column++) {

		std::size_t row = row_column / n;
		std::size_t column = row_column % n;

		std::cout << "\t" <<  array[row][column]
			<< ( (column + 1) % n == 0 ? "\n" : ", ");

	}
}