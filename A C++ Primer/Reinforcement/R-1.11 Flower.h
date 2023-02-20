
//Flower.H
/*

	Flower Class for Exercise Reinforcement : R - 1.11
*/ 

#ifndef FLOWER_H
#define FlOWER_H

#include<string>
#include<iostream>

class Flower {
public:

	Flower(const std::string& flowerName, int numberPetals, float price);
	
	// Accessors Factions 
	const std::string& name() const { return Name; }
	const double& price() const { return Price; }
	const int& petals() const { return Petals; }


	// Setters Functions
	void setName(const std::string&);
	void setPetals(int);
	void setPrice(float);

private:

	std::string Name;
	int Petals{ 0 };
	float Price{ 0.0 };
};

std::ostream& operator<<(std::ostream& out, const Flower& f);



#endif

