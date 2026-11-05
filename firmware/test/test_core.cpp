// Native tests: run on a PC, no ESP32 needed. Test IDs match PRD section 15.
#include <stdio.h>
#include <string.h>
#include "Fsm.h"
#include "Filter.h"
#include "Debounce.h"
#include "Sensors.h"
#include "Json.h"

static int fails = 0, total = 0;
#define CHECK(name, cond) do { total++; if (!(cond)) { fails++; printf("FAIL  %s\n", name); } else printf("ok    %s\n", name); } while (0)

struct Sim {
  Fsm f; uint32_t now = 1000; Inputs in{false, 25.0f, 50.0f, 0.2f};
  void run(uint32_t ms) { for (uint32_t e = 0; e < ms; e += 100) { now += 100; f.update(now, in); } }
  void pulse(int n) { for (int i = 0; i < n; i++) { in.vibHigh = true; run(100); in.vibHigh = false; run(300); } }
};
static bool near(float a, float b, float tol) { return fabsf(a - b) <= tol; }

int main() {
  { Sim s; s.run(10000);
    CHECK("T1  idle stays NORMAL, no errors", s.f.state() == NORMAL && !s.f.errTemp() && !s.f.errCur() && !s.f.buzzerOn()); }

  { Sim s; s.in.vibHigh = true; s.run(1500);
    CHECK("T2a vibration HIGH 1.5 s: still NORMAL", s.f.state() == NORMAL);
    s.run(1000);
    CHECK("T2b vibration HIGH > 2 s: WARNING + buzzer", s.f.state() == WARNING && s.f.buzzerOn());
    s.run(500);
    CHECK("T3a still WARNING before extra 2 s", s.f.state() == WARNING);
    s.run(2000);
    CHECK("T3b sustained vibration: CRITICAL", s.f.state() == CRITICAL);
    s.in.vibHigh = false; s.run(4000);
    CHECK("T8v quiet 4 s: not yet NORMAL", s.f.state() == CRITICAL);
    s.run(1500);
    CHECK("T8v quiet > 5 s: NORMAL", s.f.state() == NORMAL); }

  { Sim s; s.pulse(3); s.run(3000);
    CHECK("T2c 3 pulses in 2 s: NORMAL", s.f.state() == NORMAL); }
  { Sim s; s.pulse(4);
    CHECK("T2d 4 pulses in 2 s: WARNING", s.f.state() == WARNING); }

  { Sim s; s.in.temp = 50; s.run(2500);
    CHECK("T4a temp 50 C for 2.5 s: NORMAL", s.f.state() == NORMAL);
    s.run(1000);
    CHECK("T4b temp 50 C for 3.5 s: WARNING", s.f.state() == WARNING);
    s.in.temp = 60; s.run(1500);
    CHECK("T4c temp 60 C for 1.5 s: still WARNING", s.f.state() == WARNING);
    s.run(1000);
    CHECK("T4d temp 60 C for 2.5 s: CRITICAL", s.f.state() == CRITICAL); }

  { Sim s; s.in.cur = 1.8f; s.run(2500);
    CHECK("T5a current 1.8 A: WARNING", s.f.state() == WARNING);
    s.in.cur = 3.0f; s.run(2500);
    CHECK("T5b current 3.0 A: CRITICAL", s.f.state() == CRITICAL); }

  { Sim s; s.in.temp = 50; s.run(3500);
    s.f.acknowledge();
    CHECK("T6  ack in WARNING: silenced, buzzer off, LED still WARNING",
          s.f.silenced() && !s.f.buzzerOn() && s.f.ledPattern() == WARNING && s.f.state() == WARNING);
    s.in.temp = 60; s.run(2500);
    CHECK("T7  escalation clears SILENCED, buzzer back, LED CRITICAL",
          s.f.state() == CRITICAL && !s.f.silenced() && s.f.buzzerOn() && s.f.ledPattern() == CRITICAL);
    s.f.acknowledge();
    CHECK("T14 ack in CRITICAL silences buzzer", s.f.silenced() && !s.f.buzzerOn());
    s.in.temp = 30; s.run(4500);
    CHECK("T8a below exit 4.5 s: still CRITICAL, still silenced", s.f.state() == CRITICAL && s.f.silenced());
    s.run(1000);
    CHECK("T8b below exit > 5 s: NORMAL, SILENCED cleared", s.f.state() == NORMAL && !s.f.silenced()); }

  { Sim s; s.f.acknowledge();
    CHECK("ack while NORMAL does nothing", !s.f.silenced()); }

  { Sim s; s.in.temp = 50; s.run(3500);
    s.in.temp = 44; s.run(10000);
    CHECK("hysteresis: 44 C (between 42 and 45) holds WARNING", s.f.state() == WARNING);
    s.in.temp = 41; s.run(4000);
    CHECK("hysteresis: 41 C for 4 s still WARNING", s.f.state() == WARNING);
    s.run(1500);
    CHECK("hysteresis: 41 C for > 5 s NORMAL", s.f.state() == NORMAL); }

  { Sim s; s.in.temp = NAN; s.in.hum = NAN; s.in.cur = 1.8f; s.run(3500);
    CHECK("T9b DHT22 invalid: error flagged", s.f.errTemp());
    CHECK("T13 other sensors continue (current -> WARNING)", s.f.state() == WARNING && !s.f.errCur());
    s.in.temp = 120; s.run(200);
    CHECK("FR10 out-of-range temp flagged", s.f.errTemp());
    s.in.temp = 25; s.in.cur = NAN; s.run(200);
    CHECK("FR10 invalid current flagged, temp error cleared", s.f.errCur() && !s.f.errTemp()); }

  CHECK("T10 3.0 V at ADC pin = 5.0 V at divider input", near(mvToDividerInput(3000), 5.0f, 0.001f));
  CHECK("T15 zero calibration: no load reads 0 A", near(mvToCurrent(1500, 2.5f), 0.0f, 0.001f));
  CHECK("ACS712 1.0 A reads ~1.0 A", near(mvToCurrent(1611, 2.5f), 1.0f, 0.02f));

  { Ema e(0.3f); e.update(10); float v = 0; for (int i = 0; i < 50; i++) v = e.update(20);
    CHECK("EMA converges to input", near(v, 20.0f, 0.01f));
    CHECK("EMA ignores NaN and keeps state", isnan(e.update(NAN)) && near(e.update(20), 20.0f, 0.01f)); }

  { Debounce d(75); bool a = d.update(1000, true); d.update(1030, false); d.update(1060, true);
    CHECK("debounce: 30 ms glitch ignored", !a && !d.update(1100, true));
    Debounce e(75); e.update(1000, true); e.update(1080, true);
    bool first = e.rose(); e.update(1100, true);
    CHECK("debounce: rose() true once then false", first && !e.rose()); }

  { char b[200]; Snapshot s{}; s.t = 123456; s.vib = false; s.temp = 28.4f; s.hum = 55.1f; s.cur = 0.85f;
    s.state = NORMAL; formatJson(b, sizeof b, s);
    CHECK("JSON matches dashboard contract",
          !strcmp(b, "{\"t\":123456,\"vib\":0,\"temp\":28.4,\"hum\":55.1,\"cur\":0.85,\"state\":\"NORMAL\",\"silenced\":0,\"lv\":{\"vib\":0,\"temp\":0,\"cur\":0}}"));
    s.temp = NAN; s.state = WARNING; s.vib = true; s.silenced = true; s.lv[2] = 1; formatJson(b, sizeof b, s);
    CHECK("JSON sends null for invalid sensor", strstr(b, "\"temp\":null") && strstr(b, "\"silenced\":1"));
    s.temp = 28.4f; formatCsv(b, sizeof b, s);
    CHECK("CSV matches FR7 format", !strcmp(b, "123456,1,28.4,55.1,0.85,WARNING,1")); }

  printf("\n%d/%d passed\n", total - fails, total);
  return fails ? 1 : 0;
}
