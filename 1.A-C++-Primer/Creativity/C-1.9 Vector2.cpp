// Vector2.cpp
/*
	Contains the definitions of the member functions of the Vector2 class
*/

#include"C-1.9 Vector2.h"


// Constructors 
Vector2::Vector2(double x_, double y_) // Constructor
	: x(x_), y(y_) { }

Vector2::Vector2() // Default constructors
	: x(0), y(0) { }

Vector2::Vector2(const Vector2& other) // Copy constructor
{
	x = other.x;
	y = other.y;
}

// Operators
Vector2 Vector2::operator+(const Vector2& other) // vector addition
{
	return Vector2(x + other.x, y + other.y);
}

Vector2 Vector2::operator*(double scalar) // scalar-vector multiplication
{
	return Vector2(scalar * x, scalar * y);
}

double Vector2::operator*(const Vector2 & other) // dot product
{
	return x * other.x + y * other.y;
}


std::ostream& operator<<(std::ostream& out, const Vector2& self)
{
	out << "(" << self._x() << ", " << self._y() << ")";

	return out;
}

// Setters 
void Vector2::setX(double x_)
{
	x = x_;
}

void Vector2::setY(double y_)
{
	y = y_;
}

void Vector2::set(double x_, double y_)
{
	x = x_;
	y = y_;
}

