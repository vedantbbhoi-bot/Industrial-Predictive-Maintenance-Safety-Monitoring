# Industrial Predictive Maintenance & Safety Monitoring Node

ESP32 machine-health monitor (vibration, temperature/humidity, current) with a
NORMAL / WARNING / CRITICAL state machine, SILENCED buzzer flag, SoftAP + live dashboard.
Spec: `docs/PRD_v6.0.pdf`.

## Parts (build in this order)

| Part | Folder | What | Done when |
|---|---|---|---|
| 1 Hardware bring-up | `docs/` | Wiring (PRD s7-8), 3.3 V vs 5 V rules, ACS712 divider, calibration (s14) | T10 and T15 pass: divider reads ~3.0 V, no-load current ~0 A |
| 2 Firmware core | `firmware/lib/core` | Filters, debounce, FSM + SILENCED, FR10 invalid detection, ADC math, JSON/CSV output. **Done.** | 38 PC tests pass (`make test`) |
| 3 Firmware I/O | `firmware/src` | OLED, LEDs, buzzer, ack button (GPIO 27), watchdog, Serial CSV. **Written, not yet run on a board.** | T6, T7, T8, T12, T14 pass on the ESP32 |
| 4 Network | `firmware/src` | SoftAP (WPA2), `/events` SSE stream, dashboard served from the ESP32. **Written, not yet run on a board.** | 3-4 laptops update, latency < 1.5 s (T11) |
| 5 Dashboard app | `dashboard/` | Apple-style live dashboard. **Done.** | Matches FSM rules in demo mode |
| 6 Test and demo | `docs/` | Test results T1-T15, calibration record, demo script, report | Success criteria (PRD s18) all ticked |

## Order of work

Software first, hardware and hardware testing last. The firmware core has no ESP32 code in it, so it is tested on a PC.

## Firmware core (Part 2)

Run the tests: `cd firmware && make test` (needs g++). Or just push: GitHub Actions runs them and shows a green tick under the **Actions** tab.
All thresholds and timings are in `firmware/lib/core/config.h`.

## Run on the ESP32 (Parts 3 and 4)

1. Install VS Code, then the **PlatformIO IDE** extension. (The project uses the official `espressif32 @ 6.12.0` platform; the newer pioarduino platform failed to install on one Windows PC.) Open the `firmware` folder in VS Code.
2. **No sensors yet? Use simulation:** plug in the bare ESP32, then in the PlatformIO sidebar pick the `esp32-sim` environment: *Upload*, then *Upload Filesystem Image*, then *Monitor*.
   It plays a looping script (temperature, current, vibration, unplugged DHT22). The BOOT button is the acknowledge button.
3. On a laptop, join Wi-Fi `MachineHealth` (password in `firmware/src/net_config.h`, also on the OLED at boot) and open http://192.168.4.1/
4. Real hardware later: same steps with the `esp32` environment. Pins are in `firmware/src/pins.h`. Calibrate `ACS_ZERO_V` in `config.h`.
5. Watchdog test (T12): in `esp32-sim`, type `h` in the Serial Monitor. The board should reboot itself after about 8 seconds.

Status: the firmware passes a syntax check against stand-in Arduino headers and the core logic passes its tests, but nobody has built it with the real libraries or run it on a board yet. Expect to fix a few small compile errors on the first build, and send them to me.

## Dashboard (Part 5)

- Single file `dashboard/index.html`, no internet needed. Preview it now: open the file in a browser (demo mode, with a simulator panel). `?demo=1` forces demo mode.
- On the ESP32 it is uploaded automatically: `platformio.ini` points the filesystem image at the `dashboard/` folder.
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
