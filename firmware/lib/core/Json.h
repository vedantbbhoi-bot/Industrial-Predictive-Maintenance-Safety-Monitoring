#pragma once
#include <stddef.h>
#include "Fsm.h"
// Dashboard stream message (see README data contract). Returns length written.
int formatJson(char* buf, size_t n, const Snapshot& s);
// FR7 Serial CSV line: timestamp_ms,vibration,temp_C,humidity_pct,current_A,state,silenced
int formatCsv(char* buf, size_t n, const Snapshot& s);
