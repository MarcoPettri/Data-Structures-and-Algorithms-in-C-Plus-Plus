//Exercise Creativity: 1.8

/*

		Write a short C++ program that takes two arguments of type STL vec-
		 tor<double>, a and b, and returns the element-by-element product of a
		 and b. That is, it returns a vector c of the same length such that c[i] =
		 a[i] · b[i]
*/

#include<iostream>
#include<vector>

std::vector<double> product(const std::vector<double>&, const std::vector<double>&);

void print(const std::vector<double>);


int main()
{
	std::vector<double> a{
		163.70318562, 137.18213891,  71.82280202,  25.24440848,
		88.21119776,  29.95234437, 104.74153387,  63.56880433
	};

	std::vector<double> b{
		62.04613578, 173.58500902,  22.35042079,  64.56389312,
	   107.45497082,  64.73432271,  39.26988218,  29.04163382
	};	

	std::vector<double> result = product(a, b);

	std::cout << std::fixed;
	std::cout << "\nVector A:\n\n";
	print(a);

	std::cout << "\nVector B:\n\n";
	print(b);	
	
	std::cout << "\nA * B:\n\n";
	print(result);

	

	return 0;
}

std::vector<double> product(const std::vector<double>& a, const std::vector<double>& b)
{
	std::vector<double> c;

	if (a.size() == b.size())
	{
		auto itA = a.begin();
		auto itB = b.begin();

		for (; itA != a.end() || itB != b.end(); itA++, itB++)
		{
			c.push_back(*itA * *itB);
		}
		return c;
	}
	else
	{
		std::cout << "dimensions must agree.";
		return c;
	}
}

void print(const std::vector<double> vect)
{
	std::cout << "< ";
	for (auto value : vect)
	{
		std::cout << value
			<< (value != vect.back() ? ", " : " >");
	}
	std::cout << std::endl;
}