// All thresholds and timings in one place (PRD section 10). Calibrate on the real hardware.
#pragma once
#include <stdint.h>
namespace cfg {
// Vibration (digital SW-420)
const uint32_t VIB_HIGH_MS      = 2000;  // continuous HIGH longer than this = active
const uint32_t VIB_WINDOW_MS    = 2000;  // pulse window
const int      VIB_PULSES       = 3;     // MORE than this many pulses in the window = active
const uint32_t VIB_CRIT_MS      = 2000;  // active for this much longer = CRITICAL
// Temperature (DHT22). If you use a DHT11, set TEMP_CRIT to 50 or lower.
const float    TEMP_WARN = 45.0f, TEMP_CRIT = 55.0f, TEMP_EXIT = 42.0f;
const uint32_t TEMP_WARN_MS = 3000, TEMP_CRIT_MS = 2000;
// Current (ACS712 5A)
const float    CUR_WARN = 1.5f, CUR_CRIT = 2.5f, CUR_EXIT = 1.2f;
const uint32_t CUR_WARN_MS = 2000, CUR_CRIT_MS = 2000;
// Return to NORMAL: value must stay below the exit level this long
const uint32_t EXIT_MS = 5000;
// ACS712 conversion (PRD section 14)
const float ACS_SENS  = 0.185f;          // V per A (5A module). 20A module = 0.100
const float DIV_RATIO = 25.0f / 15.0f;   // (10k + 15k) / 15k
// Validity ranges (FR10)
const float TEMP_MIN = -40.0f, TEMP_MAX = 80.0f, CUR_MAX_ABS = 5.5f;
// Filtering and debounce (FR2)
const float    EMA_ALPHA   = 0.3f;
const uint32_t DEBOUNCE_MS = 75;
}
