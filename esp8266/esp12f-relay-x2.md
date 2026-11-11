# LC-Tech ESP-12F relay x2 (ESP8266) on ESPurna

A 2-relay board with an ESP-12F (4 MB flash), 5 V / 7-30 V supply.

![LC-Tech ESP-12F relay x2](https://raw.githubusercontent.com/blakadder/templates/master/assets/device_images/ESP12F_Relay_X2.webp)

*Photo: [Tasmota Device Templates](https://templates.blakadder.com/ESP12F_Relay_X2.html).*

## Image and pins

`esp8266-4m` with the *LC-Tech ESP12F relay x2* device template:

| Function | GPIO |
|----------|------|
| relay 1 | 4 |
| relay 2 | 5 |

The board has no buttons of its own; external inputs go on free pins as per-relay buttons, e.g. `relayBtnGpio0 12` and
`relayBtnGpio1 2`.

## Button modes

One board in a wall drives a heating circulation pump and a lamp:

- relay 1 with a *follow* button: the relay follows the input level (a contact that another controller closes), while
  the WebUI and MQTT may switch it in between; the next change of the input wins;
- relay 2 with a *switch* button (a wall switch), and its state mirrored with a relay of another board over MQTT.

The button type list on the RELAYS panel: *button*, *switch*, *follow*, *AC mains switch*.

## Tested

Both relays, *follow* and *switch* inputs, the MQTT mirror both ways. It runs a board build of this fork
(`esp12f-relay-x2`), not yet the generic `esp8266-4m`.
