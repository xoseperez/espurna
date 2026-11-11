# ESP8266 / ESP8285 on ESPurna

The architecture ESPurna started on. This fork builds it with Arduino core 3.1.2 and NONOS SDK 3.0.5 (SDK 2.2.x crashed
inside its Wi-Fi stack on the home fleet) and replaces the 216 per-board builds with a few generic images: the board is
picked after flashing, from a device template or the settings. Builds: [`code/platformio.ini`](../code/platformio.ini).

## Images: four instead of 216

Upstream ESPurna had a build per board: 216 environments in `platformio.ini`, each with the pins and drivers of one
device compiled in. This fork has **four generic images**. The pins are not in the build: the board is chosen after
flashing (*ADMIN → Device Template*, or the settings), so one image runs every board with the same kind of hardware.
The drivers are split over the images by what fits a 1 MB flash and the ESP8266 heap:

| Image | Size (`.bin` / `.bin.gz`, release 76dc912) | What is in it |
|-------|-------------------|---------------|
| `esp8266-1m` | 754 KB / 543 KB | relays, buttons, LEDs, dummy relays, **AC mains wall-switch inputs**, 433 MHz receiver (rc-switch) and the **Sonoff RF Bridge** (EFM8BB1), relay MCUs over UART (**Sonoff Dual**, STM relay boards, **LightFox**, **iFan02**, **KingArt** curtain), the MCP23S08 expander |
| `esp8266-1m-light` | 810 KB / 571 KB | relays, buttons, LEDs + **lights** (`ltProvider`: PWM dimmer up to 5 channels, MY92XX, ...), **IR** receiver / transmitter, rotary **encoder** |
| `esp8266-1m-sensor` | 772 KB / 541 KB | relays, buttons, LEDs + **power meters** (HLW8012, CSE7766, V9261F, ECH1560, PZEM-004T v1/v3, ADE7953), **sensors** (DHT, Dallas DS18B20, BMx280, analog, NTC, digital input), analog buttons |
| `esp8266-4m` | 826 KB / 589 KB | for boards installed with the 4 MB layout: relays, buttons, LEDs, RF receiver, MCP23S08 and the drivers of `-1m-sensor` plus LDR, events and pulse meter; no light, IR or relay MCUs (their static RAM left too little heap) |
| `esp8266-1m-mini` | 303 KB / 219 KB | not for running a board: the first step of the two-step update on 1 MB boards ([`code/mini-app`](../code/mini-app)), it downloads the full image itself |

All four have the WebUI, API, MQTT, Home Assistant, the terminal (serial, telnet, web), NTP, scheduler, Domoticz,
ThingSpeak, Alexa and web / ArduinoOTA. Every driver stays idle until its pins or its port are set: sensor pins
(`dhtPin`, `dallasPin`, `ntcPin` ...), `rfbProv` / `rfbRX` (RF), `irRx` / `irRxPreset` (IR), `ltProvider` (light),
`relayProv` (relay MCUs). A UART driver (power meters, RF Bridge, relay MCUs) claims the port configured with `uartTx` /
`uartRx`; the serial log and terminal stop using that port then. Unknown hardware is safe to flash: no GPIO is driven
until configured.

## Which board on which image

The [espurna-devices](https://github.com/igorufox/espurna-devices) list names the image of every board (178 ESP8266
entries from `hardware.h`); the device search and the *Apply* confirmation show it. *Apply* does not check it, the
device cannot tell which image it runs.

| The board has | Image | Boards (examples) |
|---------------|-------|-------------------|
| relays, buttons, LEDs only | `esp8266-1m` (68 boards) | Sonoff Basic, Mini, S20, 4CH / 4CH Pro, T1 1-3CH, Touch, 1CH Inching, Slampher, SV, S31 Lite; Shelly 1 / 2; Moes MS-104B; Gosund WP3 / WS1; LC-Tech ESP-12F relay x4; the ESP-01S relay; most plain Wi-Fi plugs and wall switches |
| an RF receiver, the Sonoff RF Bridge | `esp8266-1m` | Sonoff RF, Sonoff RF Bridge (with the stock EFM8BB1 or the direct mod) |
| a relay MCU on UART | `esp8266-1m` | Sonoff Dual R2, STM relay boards, Foxel LightFox, Sonoff iFan02, KingArt curtain switch |
| a light (PWM, MY92XX), IR, an encoder | `esp8266-1m-light` (37 boards) | Sonoff B1 / LED, AiLight-type bulbs, Arilux LC01-LC11, MagicHome LED controllers, H801 / H802, LSC, Mirabella Genio, GU10 / E14 / E27 bulbs, AG-L4 |
| a power meter | `esp8266-1m-sensor` (41 boards) | Sonoff Pow / Pow R2 / R3, S31, Shelly 1PM / 2.5, BlitzWolf SHP, Gosund SP111 / P1, Teckin SP20 / SP22 / SP23, Maxcio, Hykker, Homecube |
| a sensor, analog buttons | `esp8266-1m-sensor` | Sonoff TH, Sonoff Basic with DHT / DS18B20, ESP-01S DHT11 / DS18B20 shields, PZEM-004T / V9261F / ECH1560 on an ESP-01 |
| 4 MB flash and the 4 MB layout of its old build | `esp8266-4m` (32 boards) | Wemos D1 mini (+ relay shield), NodeMCU, Witty Cloud, ESPurna H, EXS relays, Electrodragon, HW-622, LC-Tech ESP-12F relay x2, the ESP-12E power meter shields |

How to choose when a board fits more than one row:

- **Light + power meter or light + RF** (Smartlife Mini Smart Socket, MagicHome ZJ-WFMN-B11): no 1 MB image has both.
  They run `esp8266-1m-light`, the other part stays off.
- **A 4 MB board** (ESP-12E/F modules, Wemos, NodeMCU) can run a 1 MB image too, but its settings are where its old build
  kept them: a board installed with the 4 MB layout stays on `esp8266-4m`, or loses its settings when it moves.
- **2 MB boards** (Shelly 1 / 2 / 1PM / 2.5, Euromate V2) run the 1 MB images, but their settings sit elsewhere in the
  2 MB layout: back up the settings (*ADMIN*, *Backup*) before the switch.
- **`esp8266-1m-light` and the two-step update**: the mini (219 KB) has to fit behind the running image, and behind the
  810 KB light image about 6 KB are missing. A light board does not update from the WebUI in two steps yet: put the
  mini on it with ArduinoOTA or over serial. The other 1 MB images leave 250-270 KB for it.

## Boards run on these images

| Board | Chip, flash | Image | Runs | Page |
|-------|-------------|-------|------|------|
| Sonoff Basic | ESP8266, 1 MB | `esp8266-1m` | one on the bench: from ESPurna 1.15 to the generic image without a wire, two-step update from the WebUI; two more on a board build of this fork | [sonoff-basic.md](sonoff-basic.md) |
| Sonoff Mini (R1 / R2) | ESP8285, 1 MB | `esp8266-1m` | seven in walls, a second wall switch on a spare pin and a dummy relay mirroring another board; on a board build of this fork | [sonoff-mini.md](sonoff-mini.md) |
| Moes MS-104B, AC mains wall switches | ESP8266, 1 MB | `esp8266-1m` | four in walls, AC mains switch inputs; on a board build of this fork | [moes-ms-104b.md](moes-ms-104b.md) |
| HW-622 (ESP-12F, 1 relay, opto input) | ESP8266, 4 MB | `esp8266-4m` | the test board of the generic 4 MB image: memory work, updates from the WebUI | [hw-622.md](hw-622.md) |
| LC-Tech ESP-12F relay x4 (4 relays, AC or 7-30 V) | ESP8266, 4 MB used as 1 MB | `esp8266-1m` | on a board build of this fork; relays through jumpers RY1-RY4 | [esp12f-relay-x4.md](esp12f-relay-x4.md) |
| LC-Tech ESP-12F relay x2 | ESP8266, 4 MB | `esp8266-4m` | one in a wall, per-relay buttons in *follow* and *switch* mode; on a board build of this fork | [esp12f-relay-x2.md](esp12f-relay-x2.md) |

The other boards of [`hardware.h`](../code/espurna/config/hardware.h) (Sonoff, Shelly, BlitzWolf, ...) have device
templates in [espurna-devices](https://github.com/igorufox/espurna-devices); most of them were not run on hardware since
the move to generic images.

## First flash

```
esptool.py --chip esp8266 --baud 460800 write_flash 0x0 espurna-esp8266-1m.bin
```

GPIO0 to GND at power-up enters the ROM bootloader. Then the first setup (access point `ESPURNA-XXXXXX`, password
`fibonacci`, `http://192.168.4.1`) and *ADMIN → Device Template*.

Coming from a board build of ESPurna: the generic image reads the pins from the settings. Apply the board's template
before the update (it writes the values the build had, nothing changes on the running board) or right after the first
boot.

## Updates

- **4 MB layout**: the browser uploads the file to `/upgrade`, one restart.
- **1 MB boards**: a full image leaves about 200 KB, too little for the next one. Two steps: the browser uploads the mini,
  which downloads the full `.bin.gz` from GitHub over https, checks its SHA-256 and writes it.

Both from *ADMIN → From GitHub* by themselves. Details, the mini's own page, and the way from ESPurna 1.15:
[docs/updates.md](../docs/updates.md#esp8266-upload-or-the-mini-on-1-mb-boards).

## Nuances

- **Memory.** About 40 KB of heap region; the settings buffer (4 KB), Wi-Fi, MQTT and the web server leave 10-20 KB.
  All ESP8266 images use lwIP 'Lower Memory' (MSS 536) and create the API handlers only while the API is on: with the
  bigger lwIP buffers the WebUI came truncated and uploads dropped half-way. An upload of 825 KB takes 30-45 s.
  Measuring what each module takes at boot: `-DSETUP_HEAP_PROBE_SUPPORT=1`, terminal `HEAP.SETUP`.
- **Static RAM of idle drivers.** A driver that is compiled in costs RAM even when it is not configured; that is why
  `esp8266-4m` has no light, IR or relay MCU drivers. With all of them it had about 4 KB of heap left and crashed during
  an update.
- **In-wall boards on weak supplies.** A two-step update over ArduinoOTA bricked one Sonoff Mini whose power supply was
  dying (it rebooted in the middle of step 2). Before an update read `last reset reason` in `info`: anything but power on,
  terminal, web or OTA means the board is unstable, update it on the bench.
- **Serial pins for inputs.** The serial port (GPIO1 TX, GPIO3 RX) leaves a pin to a button, a relay or an LED set up on
  it (`btnGpio`, `relayGpio`, `relayBtnGpio`, `ledGpio`), e.g. a wall switch on RX of a Sonoff Mini R2; `uartTx0` /
  `uartRx0` set by hand win. *RELAYS → Serial port* shows the pins and which setting took one.
- **ESPurna 1.15 boards** (core 2.7.4) have about 10 KB free: a web upload drops half-way and the update stays 'in
  progress' until a restart. Their terminal takes commands as the WebSocket action `dbgcmd`.

## AC mains wall-switch inputs

Many in-wall relay modules (e.g. Moes MS-104B) connect the wall switch to a GPIO through an optocoupler, so while the
switch is ON the pin sees a 50/60 Hz pulse train instead of a steady level. A button marked as *AC mains switch* counts
these edges in a GPIO interrupt and is ON while at least *min pulses* edges were seen within one debounce window (100 ms
is recommended), optionally accepting only edges with the 50 or 60 Hz period (either full-wave or half-wave optocoupler
circuits). Such a button always works in `switch` mode: the position at boot is taken as the starting point (no toggle at
boot) and click / long-click actions are disabled, so a wall switch can never trigger reset or AP mode. Works on GPIO0..15
(and on any input-capable GPIO of the ESP32); on other pin types (GPIO16, expanders, analog) the button is disabled
instead, because a plain pin would read the pulses as constant presses and releases.

- **WebUI**: *Relays → Buttons and wall switches* has the global *AC frequency* (Any / 50 / 60 Hz) and *AC debug log*,
  plus *AC mains switch* and *Min pulses* per button. Changes other than the debug log need a reboot.
- **Tasmota templates** cannot express AC inputs (Tasmota uses `SwitchDebounce` ending with 9). Inputs imported from
  `Switch` codes become `switch` mode buttons toggling the relay with the same Tasmota number on every position change;
  mark them as *AC mains switch* on the RELAYS panel afterwards. The boards of this firmware in the device search (e.g.
  Moes MS-104B) already come with it.
- **Settings**: `btnFreq<N>` (AC detection on/off), `btnMinPulses<N>` (default 5), `btnAcFreq` (0 = any, 50, 60),
  `btnAcDbg` (pulse statistics in the debug log), together with `btnMode<N>=switch`, `btnPress<N>` /
  `btnRlse<N>=relay-toggle` and `btnDebDel<N>=100`.

## Build

```
cd code
pio run -e esp8266-1m                 # -1m-light, -1m-sensor, -4m
pio run -d mini-app                   # the mini, code/mini-app/.pio/build/esp8266-1m-mini/firmware.bin
```

`code/scripts/boards_to_devices.py` regenerates the board entries of `../espurna-devices/devices.json` from `hardware.h`
(hand-made entries are kept).
