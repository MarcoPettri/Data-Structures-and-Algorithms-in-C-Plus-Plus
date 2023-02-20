// Exercise Reinforcement: 1.17
/*
		Write a C++ class, AllKinds, that has three member variables of type int,
		 long, and float, respectively. Each class must include a constructor func-
		 tion that initializes each variable to a nonzero value, and each class should
		 include functions for setting the value of each type, getting the value of
		 each type, and computing and returning the sum of each possible combi-
		 nation of types

*/

#include "R-1.17 AllKinds.h"
#include <vector>
#include <cstddef>
#include <iomanip>


int main()
{

	std::vector<AllKinds*> test(3); // vector of 3 'AllKinds' pointers
	
	// Allocate 
	test[0] = new AllKinds{ 23, 87'944L, 23.4f };
	test[1] = new AllKinds{ 43, 8'094L, 2.45f };
	test[2] = new AllKinds{ 12, 744'823L, 983.f };

// Testing getters functions
	std::cout << "\tGetting Data Members:\n"
		<< std::fixed << std::setprecision(2);

	for(std::size_t index{0}; index < test.size(); index++)
	{
		std::cout << "\nAllKinds instance: " << index + 1 << '\n'
			<< test[index]->getInteger() << '\n'
			<< test[index]->getLonger()	 << '\n'
			<< test[index]->getFloating() << std::endl;
	}

// Testing setters functions

	for (int i{ 0 }; i < 20; i++) { std::cout << '*'; }
	std::cout << "\n\nUpdating Data Members\n\n";

	test[0]->setInteger(test[1]->getInteger() * test[2]->getInteger());
	test[1]->setFloating(test[0]->getFloating() * test[2]->getFloating());
	test[2]->setLonger(test[1]->getLonger() * test[0]->getLonger());

	for (int i{ 0 }; i < 20; i++) { std::cout << '*'; }

// Testing operator overloading 
	std::cout << "\n\nAllKinds instance 1:\n"
		<< *test[0] << std::endl;	

	std::cout << "\nAllKinds instance 2:\n"
		<< *test[1] << std::endl;


	std::cout << "\nInstance 1 + Instance 2:\n"
		<< (*test[0] + *test[1]) << std::endl;

// deallocate storage
	for (std::size_t i{ 0 }; i < test.size(); i++)
	{
		delete test[i];
	}

	return 0;
}