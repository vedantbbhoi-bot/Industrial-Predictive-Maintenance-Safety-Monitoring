#pragma once
#include "config.h"
// ADC pin millivolts (use analogReadMilliVolts on ESP32) -> voltage at the divider input.
inline float mvToDividerInput(float pinMv) { return pinMv / 1000.0f * cfg::DIV_RATIO; }
// zeroV = divider-input voltage measured with NO load (expected near 2.5 V). Do not mix domains.
inline float mvToCurrent(float pinMv, float zeroV) {
  return (mvToDividerInput(pinMv) - zeroV) / cfg::ACS_SENS;
}
