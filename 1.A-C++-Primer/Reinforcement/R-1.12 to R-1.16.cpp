// Exercise Reinforcement : R-1.12 to R-1.16
/*

			R-1.12 Modify the CreditCard class from Code Fragment 1.3 to check that the
					price argument passed to function chargeIt and the payment argument
					passed to function makePayment are positive.
			
			R-1.13 Modify the CreditCard class from Code Fragment 1.2 to charge interest
					on each payment.
			
			R-1.14 Modify the CreditCard class from Code Fragment 1.2 to charge a late fee
					for any payment that is past its due date.
			
			R-1.15 Modify the CreditCard class from Code Fragment 1.2 to include modifier
					functions that allow a user to modify internal variables in a CreditCard
					class in a controlled manner.
			
			R-1.16 Modify the declaration of the first for loop in the Test class in Code Frag-
					ment 1.4 so that its charges will eventually cause exactly one of the three
					credit cards to go over its credit limit. Which credit card is it?
*/



#include <vector>				
#include "CreditCard.h"				


int main() {				

	std::vector<CreditCard*> wallet(10);		// vector of 10 CreditCard pointers
	// allocate 3 new cards
	wallet[0] = new CreditCard("5391 0375 9387 5309", "John Bowman", 2500);
	wallet[1] = new CreditCard("3485 0399 3395 1954", "John Bowman", 3500);
	wallet[2] = new CreditCard("6011 4902 3294 2994", "John Bowman", 5000);

	for (int j = 1; j <= 16; j++) {			// make some charges
		wallet[0]->chargeIt(double(j));
		wallet[1]->chargeIt(2 * j);
		wallet[2]->chargeIt(double(3 * j));
	}

	std::cout << "Card payments:\n";
	for (int i = 0; i < 3; i++) {			// make more charges
		std::cout << *wallet[i];
		while (wallet[i]->getBalance() > 100.0) {
			wallet[i]->makePayment(100.0, 3.4);
			std::cout << "New balance = " << wallet[i]->getBalance() << "\n";
		}
		std::cout << "\n";
		delete wallet[i];				// deallocate storage
	}

	return 0;				
}