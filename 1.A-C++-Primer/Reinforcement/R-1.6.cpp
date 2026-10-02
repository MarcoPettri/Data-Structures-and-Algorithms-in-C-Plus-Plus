// Exercise  Reinforcement: R-1.6 

/*
		Consider the following attempt to allocate a 10-element array of pointers
		to doubles and initialize the associated double values to 0.0. Rewrite the
		following (incorrect) code to do this correctly. (Hint: Storage for the
		doubles needs to be allocated.)


		double* dp[10]
		for (int i = 0; i < 10; i++) dp[i] = 0.0;

*/

#include<iostream>
#include<iomanip>

int main() {

	double* dp[10];
	std::cout << std::fixed << std::setprecision(1);

	for (int i{ 0 }; i < 10; i++) {

		double j = 0.0;
		dp[i] = &j;
	}

	for (auto val : dp) { std::cout << *val << " "; }
	return 0;
}