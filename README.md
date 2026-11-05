# Industrial Predictive Maintenance & Safety Monitoring Node

ESP32 machine-health monitor (vibration, temperature/humidity, current) with a
NORMAL / WARNING / CRITICAL state machine, SILENCED buzzer flag, SoftAP + live dashboard.
Spec: `docs/PRD_v6.0.pdf`.

## Parts (build in this order)

| Part | Folder | What | Done when |
|---|---|---|---|
| 1 Hardware bring-up | `docs/` | Wiring (PRD s7-8), 3.3 V vs 5 V rules, ACS712 divider, calibration (s14) | T10 and T15 pass: divider reads ~3.0 V, no-load current ~0 A |
| 2 Firmware core | `firmware/` | Sensor scheduler, filters, debounce, FSM + SILENCED, FR10 invalid detection | T1-T9b pass over Serial |
| 3 Firmware I/O | `firmware/` | OLED, LED patterns, buzzer, ack button (GPIO 27), watchdog, Serial CSV | T6, T7, T8, T12, T14 pass |
| 4 Network | `firmware/` | SoftAP (WPA2), `/events` SSE stream, LittleFS serves dashboard | 3-4 laptops update, latency < 1.5 s (T11) |
| 5 Dashboard app | `dashboard/` | Apple-style live dashboard. **Done.** | Matches FSM rules in demo mode |
| 6 Test and demo | `docs/` | Test results T1-T15, calibration record, demo script, report | Success criteria (PRD s18) all ticked |

## Dashboard (Part 5)

- Single file `dashboard/index.html`, no internet needed. Preview it now: open the file in a browser (demo mode, with a simulator panel). `?demo=1` forces demo mode.
- On the ESP32, copy it to `firmware/data/index.html` and upload with PlatformIO "Upload Filesystem Image".
- Data contract (firmware must send this on `GET /events`, one JSON per message, about 1 per second):

```
{"t":123456,"vib":0,"temp":28.4,"hum":55.1,"cur":0.85,
 "state":"NORMAL","silenced":0,"lv":{"vib":0,"temp":0,"cur":0}}
```
`null` for a value = sensor invalid (FR10). `lv` = per-sensor level 0/1/2.

- Acknowledge is the physical button on GPIO 27 (PRD FR3); the dashboard only shows the silenced state.
- Demo-mode FSM simplification: CRITICAL returns straight to NORMAL after the 5 s exit window; the PRD does not define a CRITICAL to WARNING step. Pulse counting (more than 3 pulses in 2 s) is not simulated.

## Push to GitHub

```
git init && git add . && git commit -m "Part 5: dashboard app + project plan"
git branch -M main
git remote add origin https://github.com/vedantbbhoi-bot/Industrial-Predictive-Maintenance-Safety-Monitoring.git
git push -u origin main
```
