// Flower.cpp
/*
	 Contains the definition of the member functions of the Flower class
*/

#include "R-1.11 Flower.h"

Flower::Flower(const std::string& flowerName, int numberPetals, float price)
	: Name{ flowerName } {

	setPetals(numberPetals);
	setPrice(price);
}

void Flower::setName(const std::string& flowerName) {
	Name = flowerName;
}
void Flower::setPetals(int nPetals) {
	if (nPetals < 0) { std::cout << "Number of Petals must be Positive"; }
	else { Petals = nPetals; }

}

void Flower::setPrice(float flowerPrice) {
	if (flowerPrice < 0) { std::cout << "Price cannot be a negative value"; }
	else { Price = flowerPrice; }
}

std::ostream& operator<<(std::ostream& out, const Flower& f) {
	out << "Name:   " << f.name() << "\n"
		<< "Petals: " << f.petals() << "\n"
		<< "Price:  " << f.price() << std::endl;
	return out;
}