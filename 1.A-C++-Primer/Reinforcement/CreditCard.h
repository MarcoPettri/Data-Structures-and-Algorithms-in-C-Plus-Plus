
// CreditCard.h

/* 
        CreditCard class for Exercise Reinforcement: R-1.12 to R-1.16 
        
*/

#ifndef CREDIT_CARD_H                           
#define CREDIT_CARD_H

#include <string>				
#include <iostream>				

class CreditCard {
public:
    // constructor
    CreditCard(const std::string& no, const std::string& nm, 
        int lim, double bal = 0);

    // accessors functions
    std::string   getNumber() const { return number; }
    std::string   getName() const { return name; }
    double   	getBalance() const { return balance; }
    int      	getLimit() const { return limit; }

    // Setters functions 
    void setName(const std::string& nm) { name = nm; }

    void setNumber(const std::string& no) { number = no; }

    void setLimit(int lim);

    void setBalance(double bal);

    bool chargeIt(double price); // make a charge 
                                           
    void makePayment(double payment, double interest,
        double late_fee = 0); // make a payment

private:                                
    std::string   number;				// credit card number
    std::string   name;				// card owner's name
    int       	limit;				// credit limit
    double    	balance;			// credit card balance
};

// print card information
std::ostream& operator<<(std::ostream& out, const CreditCard& c);

#endif