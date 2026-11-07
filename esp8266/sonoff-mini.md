# Sonoff Mini R1 / R2 (ESP8285) on ESPurna

An in-wall 1-channel relay, 10 A, with an S1 / S2 input for the wall switch. ESP8285 (ESP8266 with 1 MB of flash inside
the chip). R1 and R2 run the same image; they cannot be told apart over the network.

|||
|---|---|
|![Sonoff Mini (R1)](https://user-images.githubusercontent.com/5904370/63218127-3610e000-c154-11e9-9ae9-8ed36009dda4.png)|![Sonoff Mini R2](https://raw.githubusercontent.com/blakadder/templates/master/assets/device_images/sonoff_MINIR2.webp)|
|Mini (R1)|Mini R2|

*Photos: [Tasmota Device Templates](https://templates.blakadder.com/sonoff_mini.html), [Mini R2](https://templates.blakadder.com/sonoff_MINIR2.html). The boards here are in walls.*

## Image and pins

`esp8266-1m` with the *Sonoff Mini* device template:

| Function | GPIO |
|----------|------|
| relay | 12 |
| button (on the board) | 0 |
| S1 / S2 (wall switch) | 4 |
| LED (inverse) | 13 |

## A second wall switch and a mirrored lamp

Seven Minis in the walls of one house run with a second wall switch and a *dummy* relay (`relayDummy 1`, no pin) that
mirrors the lamp of another board over MQTT (both relays publish and subscribe the same topic, so either switch turns
either lamp). The second switch goes on a spare pin:

- **Mini R2**: GPIO3 (RX): `btnGpio2 3`, `btnMode2 switch`, `btnDefVal2 initial`, `btnPinMode2 pull-up`,
  `btnPress2 relay-toggle`, `btnRlse2 relay-toggle`, `btnRelay2 1`, plus `uartRx0 153` so the serial port does not take
  GPIO3;
- **Mini R1**: GPIO16 (`btnGpio2 16`), GPIO3 is not reachable there.

These keys belong to the board, not to a build: keep them across reflashes.

## Nuances

- **In walls there is no wire**: updates only over Wi-Fi. The 1 MB layout makes it two steps (the mini, then the full
  image). One Mini whose power supply was dying rebooted in the middle of step 2 and never came back. Before an update
  read `last reset reason` in `info`: a hardware or software watchdog, an exception means the board is unstable.
- **SDK 2.2.x Wi-Fi crashes** (`sta_input`, RSN IE) hit several Minis; the images are on SDK 3.0.5 since.

## Tested

Relay, button, S1/S2, the second wall switch on GPIO3 / GPIO16, the dummy relay mirror, MQTT, Home Assistant, two-step
updates over ArduinoOTA. These boards run a board build of this fork (1.22, `itead-sonoff-mini`), not yet the generic
`esp8266-1m`.
