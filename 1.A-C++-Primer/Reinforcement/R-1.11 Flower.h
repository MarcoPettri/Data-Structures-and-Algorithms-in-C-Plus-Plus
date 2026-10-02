
// Flower.H
/*

        Flower Class for Exercise Reinforcement : R - 1.11
*/

#ifndef FLOWER_H
#define FlOWER_H

#include <iostream>
#include <string>

class Flower {
public:
  Flower(const std::string &flowerName, int numberPetals, float price);
  Flower(const Flower &) = default;
  Flower(Flower &&) noexcept = default;
  ~Flower() = default;
  Flower &operator=(const Flower &) = default;
  Flower &operator=(Flower &&) noexcept = default;

  // Accessors Factions
  const std::string &name() const { return Name; }
  const double &price() const { return Price; }
  const int &petals() const { return Petals; }

  // Setters Functions
  void setName(const std::string &);
  void setPetals(int);
  void setPrice(float);

private:
  std::string Name;
  int Petals{0};
  float Price{0.0};
};

std::ostream &operator<<(std::ostream &out, const Flower &f);

#endif
