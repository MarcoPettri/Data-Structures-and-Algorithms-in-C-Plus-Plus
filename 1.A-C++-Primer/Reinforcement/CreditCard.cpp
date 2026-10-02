
// CreditCard.cpp
/*
     Contains the definition of the member functions of the CreditCard class 
*/

#include "CreditCard.h"				

			
// standard constructor
CreditCard::CreditCard(const std::string& no, const std::string& nm,
                        int lim, double bal) {
    number = no;
    name = nm;
    balance = bal;
    limit = lim;
}


void CreditCard::setLimit(int lim) {
    if (lim < 0) {
        std::cout << "Limit must be greater than 0" << std::endl;
        return;
    }
    limit = lim;
}

void CreditCard::setBalance(double bal) {
    if (bal < 0) {
        std::cout << "Balance must be greater than 0" << std::endl;
        return;
    }
    balance = bal;
}

// make a charge 
bool CreditCard::chargeIt(double price) {

    if (price < 0 || price + balance > double(limit)) { 

        std::cout << "The Price must be greater than 0 and not exceed the limit." << std::endl;
        return false; // over limit or Price Negative
    }

    balance += price;
    return true;      // the charge goes through
}

// make a payment
void CreditCard::makePayment(double payment,
    double interest, double late_fee) {

    if (payment < 0 || interest < 0 ) {
        
        std::cout << ( payment < 0 ? "Payment must be greater than 0" : 
            "Interest must be greater than 0" ) << std::endl;

        return;
    }
    balance -= payment  + late_fee + (payment * (interest / 100));
}
// print card information
std::ostream& operator<<(std::ostream& out, const CreditCard& c) {
    out << "Number = " << c.getNumber() << "\n"
        << "Name = " << c.getName() << "\n"
        << "Balance = " << c.getBalance() << "\n"
        << "Limit = " << c.getLimit() << "\n";
    return out;
}