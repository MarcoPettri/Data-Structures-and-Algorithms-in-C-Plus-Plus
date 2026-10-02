// Exercise Reinforcement: 1.11

/*
	Write a C++ class, Flower, that has three member variables of type string,
	 int, and float, which respectively represent the name of the flower, its
	 number of pedals, and price. Your class must include a constructor method
	 that initializes each variable to an appropriate value, and your class should
	 include functions for setting the value of each type, and getting the value
	 of each type.

*/

#include "R-1.11 Flower.h"

int main() {

	Flower flower1 = Flower("Lotus", 20, 20.78);

	std::cout << flower1;

	
	//-------Test Members Functions-------//
	flower1.setName("Iris");
	flower1.setPrice(23.44);
	flower1.setPetals(3);
	std::cout << "\nName: " << flower1.name() << std::endl;
	std::cout << "Number of Petals: " << flower1.petals() << std::endl;
	std::cout << "Price: " << flower1.price() << std::endl;

	return 0;
}