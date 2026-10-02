
// Vector2.h
/*
	Vector2 Class For Exercise Creativity: C-1.9
*/

#ifndef VECTOR2_H
#define VECTOR2_H

#include<iostream>
class Vector2
{
public:

// Constructors 
	Vector2(double, double); // constructor

	Vector2(); // Default constructor

	Vector2(const Vector2&);// Copy constructor

// Operators Functions 
	Vector2 operator+(const Vector2&); // vector addition

	Vector2 operator*(double); // scalar-vector multiplication

	double operator*(const Vector2&); // dot product

// Setters Functions
	void setX(double);
	void setY(double);
	void set(double, double);

// Getters Functions
	const double& _x() const { return x; }
	const double& _y() const { return y; }
	
private:
	double x;
	double y;

};

std::ostream& operator<<(std::ostream&, const Vector2&);

#endif

