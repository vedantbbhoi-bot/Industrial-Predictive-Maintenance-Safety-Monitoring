#pragma once
#include <math.h>
#include <stdint.h>
#include "config.h"
enum State : uint8_t { NORMAL = 0, WARNING = 1, CRITICAL = 2 };
enum { CH_VIB = 0, CH_TEMP = 1, CH_CUR = 2 };

struct Inputs {            // NaN = invalid / disconnected sensor
  bool  vibHigh;
  float temp, hum, cur;
};
struct Snapshot {          // what gets logged and sent to the dashboard
  uint32_t t; bool vib; float temp, hum, cur;
  State state; bool silenced; uint8_t lv[3];
  bool errTemp, errCur;
};

// One sensor channel: climbs one level at a time after a confirmation time,
// drops to NORMAL only after the value stays below its exit level for EXIT_MS (hysteresis).
class Channel {
public:
  uint8_t lv = 0;
  void step(uint32_t now, uint8_t raw, bool below, uint32_t warnMs, uint32_t critMs);
  void reset() { lv = 0; up_ = dn_ = false; }
private:
  bool up_ = false, dn_ = false; uint32_t upT_ = 0, dnT_ = 0; uint8_t tgt_ = 0;
};

class Fsm {
public:
  void  update(uint32_t now, const Inputs& in);   // call every loop (1-2 Hz is enough)
  void  acknowledge();                            // debounced button press
  State state() const { return st_; }
  bool  silenced() const { return sil_; }
  bool  buzzerOn() const { return st_ != NORMAL && !sil_; }  // SILENCED affects the buzzer only
  State ledPattern() const { return st_; }                   // LED follows state only
  bool  errTemp() const { return errT_; }
  bool  errCur() const { return errC_; }
  Snapshot snapshot(uint32_t now, const Inputs& in) const;
private:
  Channel ch_[3];
  State st_ = NORMAL; bool sil_ = false, errT_ = false, errC_ = false;
  bool prevHigh_ = false, highOn_ = false, actOn_ = false;
  uint32_t highT_ = 0, actT_ = 0, pulses_[8]; uint8_t pn_ = 0, pi_ = 0;
};
