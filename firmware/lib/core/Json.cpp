#include "Json.h"
#include <stdio.h>
static const char* NAMES[] = {"NORMAL", "WARNING", "CRITICAL"};
static void num(char* o, size_t n, float v, int d, const char* nul) {
  if (isnan(v)) snprintf(o, n, "%s", nul); else snprintf(o, n, "%.*f", d, v);
}
int formatJson(char* b, size_t n, const Snapshot& s) {
  char t[16], h[16], c[16];
  num(t, 16, s.temp, 1, "null"); num(h, 16, s.hum, 1, "null"); num(c, 16, s.cur, 2, "null");
  return snprintf(b, n,
    "{\"t\":%lu,\"vib\":%d,\"temp\":%s,\"hum\":%s,\"cur\":%s,\"state\":\"%s\",\"silenced\":%d,"
    "\"lv\":{\"vib\":%d,\"temp\":%d,\"cur\":%d}}",
    (unsigned long)s.t, s.vib ? 1 : 0, t, h, c, NAMES[s.state], s.silenced ? 1 : 0,
    s.lv[0], s.lv[1], s.lv[2]);
}
int formatCsv(char* b, size_t n, const Snapshot& s) {
  char t[16], h[16], c[16];
  num(t, 16, s.temp, 1, "nan"); num(h, 16, s.hum, 1, "nan"); num(c, 16, s.cur, 2, "nan");
  return snprintf(b, n, "%lu,%d,%s,%s,%s,%s,%d", (unsigned long)s.t, s.vib ? 1 : 0, t, h, c,
                  NAMES[s.state], s.silenced ? 1 : 0);
}
