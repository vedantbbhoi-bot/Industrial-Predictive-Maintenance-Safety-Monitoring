#pragma once
#include <math.h>
// Exponential moving average. Invalid (NaN) input is ignored so it cannot poison the average.
class Ema {
public:
  explicit Ema(float alpha) : a_(alpha) {}
  float update(float x) {
    if (isnan(x)) return NAN;
    if (!init_) { v_ = x; init_ = true; } else v_ += a_ * (x - v_);
    return v_;
  }
  void reset() { init_ = false; }
private:
  float a_, v_ = 0; bool init_ = false;
};
