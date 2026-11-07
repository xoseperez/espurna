# LC-Tech ESP32 relay x2 on ESPurna

A 2-relay board with an ESP32-WROOM (ESP32-D0WD-V3, 4 MB flash), 5 V / 7-30 V supply, a pin header with the UART and
GPIO0.

![LC-Tech ESP32 relay x2](https://raw.githubusercontent.com/blakadder/templates/master/assets/device_images/ESP32_Relay_X2.webp)

*Photo: [Tasmota Device Templates](https://templates.blakadder.com/ESP32_Relay_X2.html).*

## Image and pins

`esp32-matter`. The board had an image of its own before; the generic one takes the pins from the settings. Set them
before an update from such an image (they match its built-in values, nothing changes on the running board), or after the
first boot, from the terminal of the WebUI (*DEBUG*):

```
set relayGpio0 16; set relayGpio1 17; set btnGpio0 0; set btnMode0 pushbutton; set btnDefVal0 high; set btnPinMode0 pull-up; set btnRelay0 0; set ledGpio0 23; set ledInv0 1; set sysManufacturer LC_TECH; set sysDevice ESP32_RELAY_X2
```

| Function | GPIO |
|----------|------|
| relay 1 | 16 |
| relay 2 | 17 |
| button | 0 |
| LED (inverse) | 23 |

## The lab board

- **Matter**: two plugs (one per relay, a gateway with bridged plugs), in the speaker hub and python-matter-server at the
  same time.
- **Layout migration over Wi-Fi**: from the old two-slot layout (`min_spiffs`) to the safeboot layout through the web
  `/safeboot` migration, NVS kept. Before that a migrator that touched the web server too early boot-looped it, and a wire
  was needed once.
- **Update from the WebUI**: *From GitHub* → the release's safeboot (when it differs) → restart into the safeboot →
  download of the `-ota.bin` → ESPurna, 54 s, Matter fabrics kept.

## Tested

Both relays, the button, the LED, Matter with two controllers, the safeboot update from GitHub, the two-slot migration.
