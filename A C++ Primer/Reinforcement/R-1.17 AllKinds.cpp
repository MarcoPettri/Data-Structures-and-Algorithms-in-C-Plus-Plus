// AllKinds.cpp
/*
	 Contains the definition of the member functions of the AllKinds class
*/

#include "R-1.17 AllKinds.h"

AllKinds::AllKinds(int integer, long longer, float floating)
	: integer{ integer }, longer{ longer }, floating{ floating } {};

void AllKinds::setInteger(int value)
{
	integer = integer;
}

void AllKinds::setLonger(long value)
{
	longer = value;
}
void AllKinds::setFloating(float value) 
{
	floating = value;

}

AllKinds AllKinds::operator+(const AllKinds& other) const
{
	return AllKinds(integer + other.integer, longer + other.longer, floating + other.floating);
}

std::ostream& operator << (std::ostream & out, const AllKinds & insert) 
{
	out << "Int = " << insert.getInteger() << std::endl
		<< "Long = " << insert.getLonger() << std::endl
		<< "Float = " << insert.getFloating() << std::endl;
	return out;
}