// File: abs_dff_prog.hpp
#ifndef ABS_DIFF_PROG_HPP
#define ABS_DIFF_PROG_HPP
#include "progression.hpp"
#include <cmath>

class AbsDiffProg : public Progression {
public:
  AbsDiffProg(long f = 2, long s = 200) : Progression(f), second(s), next(s) {}

protected:
  long firstValue() override {
    cur = first;
    next = second;
    return cur;
  }

  long nextValue() override {
    long newCur = next;
    next = std::abs(next - cur);
    cur = newCur;
    return cur;
  }

private:
  long second;
  long next;
};

#endif