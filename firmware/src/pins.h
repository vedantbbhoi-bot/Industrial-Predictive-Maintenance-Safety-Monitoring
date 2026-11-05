#pragma once
// PRD section 8. Never use GPIO 0, 2, 12, 15 for sensors/LEDs (strapping pins).
constexpr int PIN_DHT = 4, PIN_VIB = 25, PIN_ACS = 34, PIN_SDA = 21, PIN_SCL = 22, PIN_BUZZER = 13;
constexpr int PIN_LED_R = 26, PIN_LED_G = 32, PIN_LED_Y = 33;   // three discrete LEDs
#ifdef SIM_MODE
constexpr int PIN_ACK = 0;    // BOOT button on the dev board: the simulator needs no wiring
#else
constexpr int PIN_ACK = 27;   // acknowledge button, active LOW (internal pull-up)
#endif
