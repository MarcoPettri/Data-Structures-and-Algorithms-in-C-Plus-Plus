// Exercise Creativity: 1.9

/*
		Write a C++ class Vector2, that stores the (x, y) coordinates of a two-
		 dimensional vector, where x and y are of type double. Show how to
		 override various C++ operators in order to implement the addition of two
		 vectors (producing a vector result), the multiplication of a scalar times
		 a vector (producing a vector result), and the dot product of two vectors
		 (producing a double result).

*/

#include"C-1.9 Vector2.h"

int main()
{
	// Test Constructors 
	Vector2 vect1{ 23.2, 34.44 };
	Vector2 vect2;
	Vector2 vect3 = vect1;

	// Display vectors
	std::cout << "Vector 1\n" << vect1 << std::endl;
	std::cout << "\nVector 2\n" << vect2 << std::endl;
	std::cout << "\nVector 3\n" << vect3 << std::endl;
	
	// Testing setter function 
	vect1.setX(14.87);

	// Testing operator
	std::cout << "\nVector 1 * Vector3\n"
		<< vect1 << " * " << vect3 << " = "
		<< vect1 * vect3 << std::endl;

	// Testing setter function 
	vect2.set(8.3, 2);

	// Testing operator
	std::cout << "\nVector 2 * scalar\n"
		<< vect2 << " * 4 = "
		<< vect2 * 4 << std::endl;

	std::cout << "\nVector 3 + Vector 2\n"
		<< vect3 << " + " << vect2 << " = "
		<< vect3 + vect2 << std::endl;

	return 0;
}