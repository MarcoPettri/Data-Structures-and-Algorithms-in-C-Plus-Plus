
// AllKinds
/*

	AllKinds Class for Exercise Reinforcement : R-1.17
*/

#ifndef ALLKINDS_H
#define AllKINDS_H

#include<iostream>

class AllKinds
{
public:
	AllKinds(int, long, float);
	
	// Getters Functions
	int getInteger() const { return integer; }
	long getLonger() const { return longer; }
	float getFloating() const { return floating; }

	// Setters Functions
	void setInteger(int value);
	void setLonger(long value);
	void setFloating(float value);

	// Calculate 
	AllKinds operator+(const AllKinds& other) const;

private:
	int integer;
	long longer;
	float floating;
};

std::ostream& operator<<(std::ostream& out, const AllKinds& insert);

#endif