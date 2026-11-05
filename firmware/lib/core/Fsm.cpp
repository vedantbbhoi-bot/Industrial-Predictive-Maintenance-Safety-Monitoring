#include "Fsm.h"

void Channel::step(uint32_t now, uint8_t raw, bool below, uint32_t warnMs, uint32_t critMs) {
  if (raw > lv) {
    uint8_t tgt = lv + 1;                                   // never skip WARNING
    if (!up_ || tgt_ != tgt) { up_ = true; upT_ = now; tgt_ = tgt; }
    if (now - upT_ >= (tgt == 2 ? critMs : warnMs)) { lv = tgt; up_ = false; }
  } else up_ = false;
  if (below) {
    if (!dn_) { dn_ = true; dnT_ = now; }
    else if (now - dnT_ >= cfg::EXIT_MS) { lv = 0; dn_ = false; up_ = false; }
  } else dn_ = false;
}

void Fsm::update(uint32_t now, const Inputs& in) {
  // ---- Vibration: HIGH > 2 s, or more than 3 pulses in any 2 s window ----
  if (in.vibHigh) {
    if (!prevHigh_) { pulses_[pi_] = now; pi_ = (pi_ + 1) & 7; if (pn_ < 8) pn_++; }
    if (!highOn_) { highOn_ = true; highT_ = now; }
  } else highOn_ = false;
  prevHigh_ = in.vibHigh;
  int cnt = 0;
  for (uint8_t i = 0; i < pn_; i++) if (now - pulses_[i] <= cfg::VIB_WINDOW_MS) cnt++;
  bool active = (highOn_ && now - highT_ > cfg::VIB_HIGH_MS) || cnt > cfg::VIB_PULSES;
  if (active) { if (!actOn_) { actOn_ = true; actT_ = now; } } else actOn_ = false;
  uint8_t vraw = !active ? 0 : (now - actT_ >= cfg::VIB_CRIT_MS ? 2 : 1);
  ch_[CH_VIB].step(now, vraw, !in.vibHigh, 0, 0);

  // ---- Temperature (FR10: invalid reading -> flag it, channel idle, others continue) ----
  errT_ = isnan(in.temp) || in.temp < cfg::TEMP_MIN || in.temp > cfg::TEMP_MAX;
  if (errT_) ch_[CH_TEMP].reset();
  else ch_[CH_TEMP].step(now, in.temp > cfg::TEMP_CRIT ? 2 : in.temp > cfg::TEMP_WARN ? 1 : 0,
                         in.temp < cfg::TEMP_EXIT, cfg::TEMP_WARN_MS, cfg::TEMP_CRIT_MS);

  // ---- Current ----
  errC_ = isnan(in.cur) || fabsf(in.cur) > cfg::CUR_MAX_ABS;
  if (errC_) ch_[CH_CUR].reset();
  else { float a = fabsf(in.cur);
    ch_[CH_CUR].step(now, a > cfg::CUR_CRIT ? 2 : a > cfg::CUR_WARN ? 1 : 0,
                     a < cfg::CUR_EXIT, cfg::CUR_WARN_MS, cfg::CUR_CRIT_MS); }

  // ---- Overall state + SILENCED rules (FR3) ----
  State old = st_;
  uint8_t m = ch_[0].lv; if (ch_[1].lv > m) m = ch_[1].lv; if (ch_[2].lv > m) m = ch_[2].lv;
  st_ = (State)m;
  if (st_ == NORMAL) sil_ = false;                      // returning to NORMAL clears SILENCED
  else if (st_ == CRITICAL && old != CRITICAL) sil_ = false;  // escalation clears SILENCED
}

void Fsm::acknowledge() { if (st_ != NORMAL) sil_ = true; }

Snapshot Fsm::snapshot(uint32_t now, const Inputs& in) const {
  Snapshot s;
  s.t = now; s.vib = in.vibHigh;
  s.temp = errT_ ? NAN : in.temp;
  s.hum = (isnan(in.hum) || in.hum < 0 || in.hum > 100) ? NAN : in.hum;
  s.cur = errC_ ? NAN : in.cur;
  s.state = st_; s.silenced = sil_;
  s.lv[0] = ch_[0].lv; s.lv[1] = ch_[1].lv; s.lv[2] = ch_[2].lv;
  s.errTemp = errT_; s.errCur = errC_;
  return s;
}
