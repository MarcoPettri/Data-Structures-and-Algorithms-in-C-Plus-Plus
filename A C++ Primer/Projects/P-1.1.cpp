// C++ Primer: Projects: P-1.1

/* 
		A common punishment for school children is to write out the same sen-
		 tence multiple times. Write a C++ stand-alone program that will write
		 out the following sentence one hundred times: “I will always use object-
		 oriented design.” Your program should number each of the sentences and
		 it should “accidentally” make eight different random-looking typos at var-
		 ious points in the listing, so that it looks like a human typed it all by hand.

*/

#include<iostream>
#include<iomanip>
#include<utility>
#include<cstdlib>
#include<ctime>

bool isPrime(int);
void typos(char[], int);

int main()
{
	char string[]{ "I will always use object oriented design" };
	const int LENGTH{ sizeof string / sizeof(*string) };

	std::srand(static_cast<unsigned>(std::time(nullptr)));

	for (int i{ 1 }; i < 101; i++)
	{
		if (i > 60 && isPrime(i) )
		{	
			std::cout << std::setw(3) << i << ".  ";
			typos(string, LENGTH);
			std::cout << std::endl;

		}
		else {
			std::cout << std::setw(3);
			std::cout << i << ".  " << string << std::endl;
		}
	}
	
	return 0;
}

bool isPrime(int number)
{
	if ((number % 2 == 0) && (number != 2)) { 
		return false; 
	}
	else if ((number % 3 == 0) && (number != 3)) {
		return false;
	}	
	else if ((number % 5 == 0) && (number != 5)) {
		return false;
	}	
	else if ((number % 7 == 0) && (number != 7)) {
		return false;
	}

	return true;

}
void typos(char str[], int length)
{
	int index1;
	int index2;

	do{
		index1 = 1 + std::rand() % (length - 2);
		index2 = 1 + std::rand() % (length - 2);

	} while (str[index1] == ' ' || str[index2] == ' ');

	std::swap(str[index1], str[index2]);
	std::cout << str;
	std::swap(str[index1], str[index2]);
}
