# Moes MS-104B (ESP8266) on ESPurna

An in-wall 2-channel relay module whose wall-switch inputs are AC mains through optocouplers. ESP8266, 1 MB.

![Moes MS-104B](https://user-images.githubusercontent.com/39533759/65378668-22f5b080-dcbc-11e9-9401-8d906b204914.jpg)

*Photo: [Tasmota Device Templates](https://templates.blakadder.com/moes-MS-104B.html).*

## Image and pins

`esp8266-1m` with the *Moes MS-104B* device template, or the `moes-ms-104b` build
(`pio run -e moes-ms-104b`, in `code/platformio_override.ini`):

| Function | GPIO |
|----------|------|
| relay 1 | 14 |
| relay 2 | 15 |
| wall switch 1 (AC mains) | 13 → relay 1 |
| wall switch 2 (AC mains) | 12 → relay 2 |
| button (on the board) | 2 → relay 1 |

## Flashing

The modules came with an old Tuya firmware that [tuya-convert](https://github.com/ct-Open-Source/tuya-convert) could
replace over Wi-Fi: no soldering, no opening the case. Newer Tuya firmware closed that hole; such a module needs a
serial wire for the first flash. After the first ESPurna all updates go over Wi-Fi; the four modules here stay in the
walls.

## AC mains inputs

While the wall switch is ON the GPIO sees a 50/60 Hz pulse train, not a steady level. The buttons on GPIO13 / GPIO12 are
marked *AC mains switch*: the firmware counts the edges in an interrupt and holds the switch ON while enough of them come
in one debounce window (100 ms). How it works and the settings: [README.md](README.md#ac-mains-wall-switch-inputs).

## Tested

Four modules in walls: both relays, both AC wall switches, the on-board button, MQTT, Home Assistant. They run a board
build of this fork (1.22), not yet the generic `esp8266-1m`.
