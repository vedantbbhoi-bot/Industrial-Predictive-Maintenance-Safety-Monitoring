// Machine Health Node: glue between the tested core (lib/core) and the hardware + network.
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "Fsm.h"
#include "Filter.h"
#include "Debounce.h"
#include "Sensors.h"
#include "Json.h"
#include "pins.h"
#include "net_config.h"
#ifdef SIM_MODE
#include "sim.h"
#endif

static AsyncWebServer server(80);
static AsyncEventSource events("/events");
static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
static DHT dht(PIN_DHT, DHT22);          // DHT11 users: change to DHT11 and lower TEMP_CRIT in config.h
static Fsm fsm;
static Debounce dbVib(cfg::DEBOUNCE_MS), dbAck(cfg::DEBOUNCE_MS);
static Ema emaCur(cfg::EMA_ALPHA);
static const char* NAMES[] = {"NORMAL", "WARNING", "CRITICAL"};

static bool gOled = false, dhtFirst = true;
static float gTemp = NAN, gHum = NAN, gCur = NAN;
static int dhtFails = 0;
static Inputs gIn{false, NAN, NAN, NAN};
static uint32_t tFsm = 0, tCur = 0, tDht = 0, tOut = 0;

#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif
static void startWatchdog() {            // FR9: a hung loop resets the chip after 8 s
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t c;
  c.timeout_ms = 8000; c.idle_core_mask = 0; c.trigger_panic = true;
  if (esp_task_wdt_reconfigure(&c) != ESP_OK) esp_task_wdt_init(&c);
#else
  esp_task_wdt_init(8, true);            // Arduino core 2.x API: seconds, panic (reset) on timeout
#endif
  esp_task_wdt_add(NULL);
}

static void showBoot() {
  if (!gOled) return;
  oled.clearDisplay(); oled.setTextColor(SSD1306_WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("Machine Health");
  oled.print("WiFi: "); oled.println(AP_SSID);
  oled.print("Pass: "); oled.println(AP_PASS);
  oled.print("IP:   "); oled.println(WiFi.softAPIP().toString().c_str());
  oled.display();
}

static void showLive(const Snapshot& s) {
  if (!gOled) return;
  char l[28];
  oled.clearDisplay();
  oled.fillRect(0, 0, 128, 16, SSD1306_WHITE);              // state shown as an inverted block
  oled.setTextColor(SSD1306_BLACK); oled.setTextSize(2);
  oled.setCursor(4, 1); oled.print(NAMES[s.state]);
  oled.setTextColor(SSD1306_WHITE); oled.setTextSize(1);
  if (s.errTemp) snprintf(l, sizeof l, "Temp  DHT22 ERR"); else snprintf(l, sizeof l, "Temp  %.1f C", s.temp);
  oled.setCursor(0, 20); oled.print(l);
  if (isnan(s.hum)) snprintf(l, sizeof l, "Hum   --"); else snprintf(l, sizeof l, "Hum   %.1f %%", s.hum);
  oled.setCursor(0, 30); oled.print(l);
  if (s.errCur) snprintf(l, sizeof l, "Curr  ACS712 ERR"); else snprintf(l, sizeof l, "Curr  %.2f A", s.cur);
  oled.setCursor(0, 40); oled.print(l);
  snprintf(l, sizeof l, "Vib   %s%s", s.vib ? "HIGH" : "low", s.silenced ? "  MUTED" : "");
  oled.setCursor(0, 50); oled.print(l);
  oled.display();
}

// LED follows state only; buzzer follows state + SILENCED (FR3, FR5)
static void driveOutputs(uint32_t now) {
  State st = fsm.ledPattern();
  digitalWrite(PIN_LED_G, st == NORMAL);
  digitalWrite(PIN_LED_Y, st == WARNING);
  digitalWrite(PIN_LED_R, st == CRITICAL);
  bool on = false;
  if (fsm.buzzerOn()) on = (st == CRITICAL) ? true : (now % 1000) < 200;   // WARNING: short beep each second
  digitalWrite(PIN_BUZZER, on);
}

static void readCurrent() {
  float mv = analogReadMilliVolts(PIN_ACS);
  // open circuit / unpowered module pulls the divider to ~0 V; a healthy ACS712 stays well inside this window
  float raw = (mv < 200.0f || mv > 3100.0f) ? NAN : mvToCurrent(mv, cfg::ACS_ZERO_V);
  gCur = emaCur.update(raw);
}

static void readDht() {
  if (dhtFirst) { dhtFirst = false; dht.readTemperature(); return; }   // discard first reading after boot
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (isnan(t) || isnan(h)) { if (++dhtFails >= 2) { gTemp = NAN; gHum = NAN; } }  // FR10 after 2 failures in a row
  else { dhtFails = 0; gTemp = t; gHum = h; }
}

static void emit(uint32_t now) {
  Snapshot s = fsm.snapshot(now, gIn);
  char b[256];
  formatJson(b, sizeof b, s); events.send(b);
  formatCsv(b, sizeof b, s);  Serial.println(b);
  showLive(s);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_VIB, INPUT); pinMode(PIN_ACK, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT); pinMode(PIN_LED_R, OUTPUT); pinMode(PIN_LED_G, OUTPUT); pinMode(PIN_LED_Y, OUTPUT);
  analogSetPinAttenuation(PIN_ACS, ADC_11db);
  dht.begin();
  Wire.begin(PIN_SDA, PIN_SCL);
  gOled = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!gOled) Serial.println("OLED not found (continuing without it)");
  if (!LittleFS.begin(true)) Serial.println("LittleFS mount failed: upload the filesystem image");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 4);                 // WPA2, channel 1, max 4 clients (FR6)
  server.addHandler(&events);
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.begin();
  Serial.printf("WiFi %s  pass %s  http://%s/\n", AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str());
  Serial.println("timestamp_ms,vibration,temp_C,humidity_pct,current_A,state,silenced");
  showBoot();
  delay(4000);                                            // leave the password on screen at boot
  startWatchdog();
}

void loop() {
  esp_task_wdt_reset();
  uint32_t now = millis();
#ifdef SIM_MODE
  Inputs sim = simInputs(now);
  bool vibRaw = sim.vibHigh; gTemp = sim.temp; gHum = sim.hum; gCur = sim.cur;
#else
  bool vibRaw = digitalRead(PIN_VIB) == HIGH;
  if (now - tCur >= 500)  { tCur = now; readCurrent(); }    // 2 Hz
  if (now - tDht >= 2500) { tDht = now; readDht(); }        // >= 2.5 s (DHT22 needs >= 2 s)
#endif
  bool vib = dbVib.update(now, vibRaw);
  dbAck.update(now, digitalRead(PIN_ACK) == LOW);
  if (dbAck.rose()) fsm.acknowledge();
  if (now - tFsm >= 100) { tFsm = now; gIn = Inputs{vib, gTemp, gHum, gCur}; fsm.update(now, gIn); }
  driveOutputs(now);
  if (now - tOut >= 1000) { tOut = now; emit(now); }
#ifdef DEBUG_HANG
  if (Serial.available() && Serial.read() == 'h') { Serial.println("hang test: watchdog should reset in ~8 s"); while (true) {} }
#endif
}
