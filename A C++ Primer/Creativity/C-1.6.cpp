// Exercise Creativity: 1.6
/*
		Write a short C++ program that outputs all possible strings formed by
		 using each of the characters 'a', 'b', 'c', 'd', 'e', and 'f' exactly
		 once.
*/

#include<iostream>
#include<utility>
#include<cstddef>

void permutations(char[], const std::size_t&, std::size_t i = 0);
void display(const char*);

int main()
{
	char string[]{ "abcdef" };

	std::size_t SIZE{ sizeof string / sizeof(char) };

	--SIZE;
	permutations(string, SIZE);

	return 0;
}

void permutations(char str[], const std::size_t& size, std::size_t i)
{
	if( i == size - 1)
	{
		display(str);
		return;
	}
	
	for (std::size_t j{ i }; j < size; j++)
	{
		std::swap(str[i], str[j]);

		permutations(str, size, i + 1);

		std::swap(str[i], str[j]);
	}
}

void display(const char* str)
{	
	std::cout << "(";
	for (; *str != '\0'; str++)
	{
		std::cout << *str
			<< (*(str + 1) != '\0' ? ", " : ")");
	}
	std::cout << std::endl;
}
