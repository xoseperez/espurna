# Tuya CB3S (Beken BK7231N) on ESPurna

ESPurna on Beken BK72xx chips, the ones inside Tuya CB2S / CB3S / WB2S modules that replaced the ESP8266 in most Tuya
devices. The port is built with [LibreTiny](https://docs.libretiny.eu/) (PlatformIO platform `libretiny`, Arduino API on top
of the Beken BDK 3.0.33) and runs next to the ESP8266 and ESP32 builds from the same source. Beken-specific code is under
[`code/espurna/arch/beken`](../code/espurna/arch/beken), the builds are in
[`code/platformio_beken.ini`](../code/platformio_beken.ini).

It was brought up on a Tuya 1-channel 5 V relay board with a 433 MHz receiver (CB3S module, BK7231N). Tested on that board:
relay, both buttons, LED, the 433 MHz remote, Wi-Fi, web UI (direct and behind a reverse proxy), OTA over Wi-Fi, MQTT, HTTP
API, NTP, serial and telnet terminal, settings kept across reboots and updates.

<p>
  <img src="img/cb3s-board.webp" width="49%" alt="Tuya 1-channel 5 V relay board with a CB3S module and a 433 MHz receiver">
  <img src="img/cb3s-wired.webp" width="49%" alt="The same board wired to a CP2102 USB-UART adapter, powered from micro-USB">
</p>

## 1. Builds

| Build | Board |
|-------|-------|
| `cb3s-generic` | any CB3S board (Tuya flash layout); no relays, buttons or LEDs pre-assigned, configure them in the web UI |
| `tuya-cb3s-relay-rf` | **Tuya 1-channel 5 V relay + 433 MHz receiver**: relay P24, button P14, second ("total") button P6, LED P8 (inverted, Wi-Fi status), 433 MHz receiver data on P26 |

Enabled modules: web UI, API, MQTT, terminal, Home Assistant; the RF bridge (rc-switch) on `tuya-cb3s-relay-rf`.

Pin numbers are GPIO numbers (`P24` is GPIO 24, set it as `24` in the web UI). Only the pins that the module brings out
can be used.

## 2. What you need

- a USB-UART adapter with 3.3 V logic (CP2102, CH340, FT232; the photo shows a CP2102);
- four wires soldered to the module pads, or three if you can reach CEN with a wire end by hand;
- [ltchiptool](https://github.com/libretiny-eu/ltchiptool) for the first flash. It comes with the LibreTiny platform
  (`%USERPROFILE%\.platformio\penv\.libretiny\Scripts\ltchiptool.exe` after the first build) or `pip install ltchiptool`;
- optional: [bk7231tools](https://github.com/tuya-cloudcutter/bk7231tools) to read the stock pin map out of the backup.

The flashing is done once. After that every update goes over Wi-Fi.

## 3. Wiring

![CB3S pads: TX1, RX1, GND at the top row, CEN and 3.3V at the bottom row](img/cb3s-pads.webp)

| CB3S pad | Connect to |
|----------|------------|
| 16, TX1 (P11) | adapter RX |
| 15, RX1 (P10) | adapter TX |
| 9, GND | adapter GND |
| 1, CEN | a loose wire: touch it to GND to reset the chip |

- **Power the board from its micro-USB socket.** This socket is 5 V power only: the board has no USB-UART chip, and the
  BK7231N has no USB. Leave the adapter's 3.3 V unconnected then. Without USB power, feed pad 8 (3.3 V) from a 3.3 V supply
  strong enough for Wi-Fi; the 3.3 V pin of most adapters is not.
- Check the pads with a meter before you solder. Pad 9 (GND) beeps to the minus of the electrolytic capacitor and to the USB
  shell, pad 8 reads 3.3 V with the board powered. If it is the other way round, the module is rotated.
- The full CB3S pad order. Top row from the antenna end: 16 TX1 (P11), 15 RX1 (P10), 14 P7, 13 P8, 12 P21, 11 TX2 (P0),
  10 P9, 9 GND. Bottom row: 1 CEN, 2 P23 (ADC), 3 EN, 4 P14, 5 P26, 6 P24, 7 P6, 8 3.3 V.
- CEN does not need to stay connected. The chip enters its bootloader for a short moment after every reset, and ltchiptool
  catches it there. Touch CEN to GND briefly while ltchiptool shows `Connecting...`. Unplugging and plugging the USB power
  does the same.
- UART1 (P10 / P11) is also where the firmware logs, at 115200 baud, so the same wires give you the boot log and the
  terminal later.

## 4. Back up the stock firmware

Do this first. It is the only way back to the Tuya firmware, and it holds the pin map of the board.

```
ltchiptool flash read -d COM3 -t 45 bk7231n cb3s-stock.bin
```

`-d` is the adapter's port (COM3, `/dev/ttyUSB0`), `-t 45` waits 45 s for the chip; touch CEN to GND when it says
`Connecting...`. A full dump is 2 MB.

On a board that is not in the table above, read the pin map out of the dump:

```
bk7231tools dissect_dump -e --storage cb3s-stock.bin
```

`user_param_key.json` in the output lists the pins. `rl1_pin` is relay 1, `bt1_pin` is button 1, `netled1_pin` is the
Wi-Fi LED, `remote_io` is the 433 MHz receiver. The `_lv` fields give the active level (`1` = active high).

## 5. Flash ESPurna

Build from `code/`:

```
py -3 -m platformio run -c platformio_beken.ini -e tuya-cb3s-relay-rf
```

The image is `code/.pio/build_beken/tuya-cb3s-relay-rf/firmware.uf2`. It uses its own build directory, so it does not touch
the ESP8266 / ESP32 builds in `.pio/build`. Then flash it:

```
ltchiptool flash write -d COM3 -t 45 code/.pio/build_beken/tuya-cb3s-relay-rf/firmware.uf2
```

Touch CEN to GND at `Connecting...` again. The chip starts ESPurna by itself at the end, no reset is needed.

Only the `app` partition (`0x11000`) is written. The Tuya bootloader and the Tuya config at `0x1EE000` stay where they were,
so the stock pin map is still on the chip.

## 6. First setup

1. Join the Wi-Fi network `ESPURNA-XXXXXX` (the last six digits of the MAC) with the password `fibonacci`.
2. Open `http://192.168.4.1`, log in as `admin` / `fibonacci` and set a new password when asked.
3. Add your network on the **Wi-Fi** tab, then reach the device at the address your router gives it.

No captive portal yet: open the address by hand.

Buttons on `tuya-cb3s-relay-rf` (ESPurna defaults):

| Button | Click | Double click | Long click | Very long click |
|--------|-------|--------------|------------|-----------------|
| P14 | toggle the relay | start the access point | reboot | factory reset |
| P6 | toggle the relay | - | - | - |

![Status page: four relays, network, firmware, BDK core version, heap](img/webui-status.webp)

The status page after setup: the real relay on GPIO 24 and three dummy relays, the Beken BDK as the core version,
about 50 KB of free heap. There is no VCC row, the module has no supply voltage measurement.

## 7. Updates over Wi-Fi

Upload `firmware.uf2` on **Admin → Upgrade** in the web UI. It takes about 30 s and keeps the settings. The `.bin` files
from the build are not accepted: Beken OTA uses the LibreTiny UF2 package.

![Admin tab, OTA: Browse and Upgrade](img/webui-ota.webp)

If the upload stops halfway (Wi-Fi drop, closed tab), the board keeps running the old firmware. Start the upload again.

## 8. The 433 MHz remote

The receiver on P26 is decoded by the chip itself (rc-switch). Every code it hears goes to MQTT `<root>/rfin` and to the
debug log, so you can use the remote buttons in Home Assistant or Node-RED without binding them on the board.

To switch relays from the remote:

1. Make as many relays as you need: one per remote button pair, or one per button. **Add Relay** at the end of the
   **Relays** tab, with the provider **Dummy**, gives a relay with no GPIO: it only has a state, which goes to MQTT and the
   API like any relay. 32 relays in total is the limit. From the terminal, `set relayDummy <N>` and a `reset` add N such
   relays after the ones with a GPIO.

   ![Relays tab: a dummy relay and the Add Relay button](img/webui-relays.webp)

2. On the **RFBRIDGE** tab every relay has an ON and an OFF row. Press **LEARN** in a row and then the remote button within a few
   seconds. The same code in both rows makes that button toggle the relay.
3. Or type the code into the field and press **Save**. The format is the one `<root>/rfin` and the debug log show:
   `C0`, protocol, pulse length (2 bytes), number of bits, then the code, all in hex. `C00100F11844D001` is protocol 1,
   241 µs, 24 bit, code `44D001`. The pulse length is not compared, so any value matches.
4. From the terminal: `rfb.learn <relay> <on|off>`, `rfb.forget <relay> [on|off]`, or `set rfbON<relay> <code>` /
   `set rfbOFF<relay> <code>`.

![RF BRIDGE tab: ON and OFF codes with LEARN and FORGET for each relay](img/webui-rfbridge.webp)

With four relays, an 8-button remote fits exactly: ON and OFF of each relay on its own button, as above. With more remote buttons,
add more relays; every new relay gets its own ON / OFF rows here.

Tested with an 8-button remote (protocol 1, 24 bit): 16 out of 16 presses received. About one press in 16 is sometimes
decoded as protocol 17 with a different code, and that press is ignored.

## 9. How this build differs from the stock LibreTiny settings

- **BLE off** (`CFG_SUPPORT_BLE = 0`). With BLE on, the SDK links the whole BLE 5.1 stack, and the OTA image no longer fits
  the 664 KiB `download` partition.
- **PMF and WPA3 off** (`CFG_IEEE80211W = 0`, `CFG_WPA3 = 0`). With them on, the SDK supplicant failed the handshake
  (`AUTH_FAIL`) on OpenWrt access points with WPA2-PSK, 802.11r and optional PMF (`ieee80211w=1`). WPA2 networks work as
  before.
- **AsyncTCP task stack 3072** (`CONFIG_ASYNC_TCP_STACK_SIZE`). FreeRTOS here counts the stack in words, not in bytes as on
  the ESP32. The library default meant 64 KiB and left 6 KiB of heap, and clients of the access point could not connect.
  About 59 KB of heap is free after boot.
- **Logs on UART1** (`LT_UART_DEFAULT_PORT=1`): LibreTiny, the SDK and ESPurna all print on the flashing pads.
- **Patched AsyncTCP** ([`code/lib_beken/AsyncTCP-espurna`](../code/lib_beken/AsyncTCP-espurna)), used instead of the
  registry version. A dropped connection could leave the web server using a freed lwIP connection, and other requests then
  stalled for 10 to 40 s.
- **Settings** are kept in 8 rotating slots in the `userdata` partition at `0x1DB000`, below the Tuya config. Every slot has
  a CRC, and the newest valid slot is loaded at boot.

## 10. Known limitations

- The custom reset reason is lost: no RAM survives a reset on the BK7231N, so **Info** always shows `SW Reboot` after a
  reboot from the web UI or the terminal.
- No mDNS, no captive portal, no Alexa, no ArduinoOTA, no OTA from a URL yet.
- `wifiTxPwr` does nothing.
- No `gpio` terminal command.
- The ADC (P23) is not used, and there is no VCC measurement, so the VCC row is hidden on the status page.
