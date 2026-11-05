#pragma once
// SIM_MODE only: a looping scripted scenario, so the whole chain can be tested with no sensors.
#include <math.h>
#include "Fsm.h"
struct Seg { uint32_t ms; bool vib; float temp, cur; };
static const Seg SEGS[] = {
  { 8000, false, 28.0f, 0.3f},   // normal
  {12000, false, 50.0f, 0.3f},   // temperature -> WARNING
  {10000, false, 60.0f, 0.3f},   // temperature -> CRITICAL (press BOOT to silence, watch it clear on escalation)
  {12000, false, 30.0f, 0.3f},   // recover to NORMAL
  { 8000, false, 28.0f, 1.8f},   // current -> WARNING
  {10000, false, 28.0f, 0.3f},   // recover
  { 6000, true,  28.0f, 0.3f},   // vibration -> WARNING then CRITICAL
  {10000, false, 28.0f, 0.3f},   // recover
  { 6000, false, NAN,   0.3f},   // DHT22 "unplugged" (FR10)
};
inline Inputs simInputs(uint32_t now) {
  uint32_t total = 0;
  for (const Seg& s : SEGS) total += s.ms;
  uint32_t t = now % total;
  for (const Seg& s : SEGS) {
    if (t < s.ms) return Inputs{s.vib, s.temp, isnan(s.temp) ? NAN : 55.0f, s.cur};
    t -= s.ms;
  }
  return Inputs{false, 28.0f, 55.0f, 0.3f};
}
