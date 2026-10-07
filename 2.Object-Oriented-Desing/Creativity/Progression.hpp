// File: progression.hpp

#ifndef PROGRESSION_HPP
#define PROGRESSION_HPP

class Progression { // a generic progression
public:
  Progression(long f = 0) // constructor
      : first(f), cur(f) {}
  virtual ~Progression() = default; // destructor
  void printProgression(int n);     // print the first n values
protected:
  virtual long firstValue(); // reset
  virtual long nextValue();  // advance
protected:
  long first; // first value
  long cur;   // current value
};

#endif // PROGRESSION_HPP