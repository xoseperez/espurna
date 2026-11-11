# NSPanel Lovelace UI — serial protocol spec for an on-ESP (ESPurna) backend

Target: Sonoff NSPanel gen1 EU (ESP32). The Nextion-compatible display is on UART2: ESP TX=GPIO16 → TFT RX, ESP RX=GPIO17 ← TFT TX, 115200 8N1. Display power is GPIO4, active LOW. The panel currently runs Tasmota 13.1.0 with the joBr99 Berry driver (`autoexec.be`) and a joBr99 "NSPanel Lovelace UI" TFT, with files dated 2023-06-17.

This spec covers what the backend does: AppDaemon `luibackend` or the ioBroker `NsPanelTs.ts`, ported to run on the ESP and talk to the TFT directly over UART.

Research date: 2026-10-24. All facts come from source code at pinned commits (links below). Nothing in this spec was checked on the device. Section 9 lists what to verify there.

## 0. Pinned sources

| Alias | What | Commit / URL |
|---|---|---|
| **JUN23** | repo state on 2023-06-17 (last commit before the panel's file date) | `6b04938a8a83929dde02aebb65613287937245fe` (2023-06-14) — `https://github.com/joBr99/nspanel-lovelace-ui/blob/6b04938a8a83929dde02aebb65613287937245fe/<path>` |
| **BERRY8** | `tasmota/autoexec.be` as of 2023-06-17 (driver version 8) | `17b7696d621f40abd3c1fcf177aa5b381113a91a` — https://github.com/joBr99/nspanel-lovelace-ui/blob/17b7696d621f40abd3c1fcf177aa5b381113a91a/tasmota/autoexec.be |
| **V474** | latest release tag v4.7.4 (2025-08-02) | `a94cf0cef3f6034ff7ad61eb9c4c9c6886e2bc02` |
| **HEAD** | main at research time (2026-10-21) | `07d6672f59f806fbbc14d1c114d4fdbf4fdf9a6a` |
| **ESPH** | sairon ESPHome C++ component (a working C++ implementation of the same framing) | https://github.com/sairon/esphome-nspanel-lovelace-ui/blob/f53de340ef8584b9bc9da7d7c0a9fa0922d985bb/components/nspanel_lovelace/nspanel_lovelace.cpp |

The TFT's behaviour is quoted from `HMI/n2t-out/*.txt`, a Nextion2Text export of `HMI/nspanel.HMI` that the repo regenerates automatically on each HMI change. In JUN23 it was last regenerated on 2023-06-01 and contains `tVersion = 51`.

Line references are given as `file:line` and resolve against the alias's base URL. For example, JUN23 `apps/nspanel-lovelace-ui/luibackend/pages.py:409` is
https://github.com/joBr99/nspanel-lovelace-ui/blob/6b04938a8a83929dde02aebb65613287937245fe/apps/nspanel-lovelace-ui/luibackend/pages.py#L409

---

## 1. Which TFT and protocol version the panel has

### 1.1 Version history (from `nspanel-lovelace-ui.py`, `desired_display_firmware_version`)

| Repo tag (date) | Backend expects TFT version | TFT file the updater flashes | Berry driver expected |
|---|---|---|---|
| v4.0.0 – v4.1.0 (2023-02-25 … 2023-05-20) | **50** | `nspanel-v4.0.3.tft` | 8 |
| **v4.1.1 – v4.1.4 (2023-05-20 … 2023-05-21), and `main` until 2023-08-20** | **51** | **`nspanel-v4.1.1.tft`** | 8 |
| v4.2.0 – v4.3.0 (2023-08-20 … 2023-09-16) | 52 | `nspanel-v4.2.0.tft` | 8 |
| v4.3.1 – v4.7.4 – HEAD (2023-09-16 … 2026) | 53 | `nspanel-v4.3.1.tft` … `nspanel-v4.7.3.tft` | 8 (the script itself reports 9 since 2023-09-18) |
| ioBroker fork TFT (`NsPanelTs.ts`, HEAD) | 54 … 61 (`tft_version 'v5.1.1'` = 61) | `nspanel.de/...` | — |

Sources:
- JUN23 `apps/nspanel-lovelace-ui/nspanel-lovelace-ui.py`: `desired_display_firmware_version = 51`, `version = "v4.1.1"`. The same lines are at tag v4.1.4, `:29-31`.
- V474 the same file `:35-37`: `53` / `"v4.7.3"`.
- JUN23 `HMI/n2t-out/pageStartup.txt:142-150`: the startup page's `tVersion` text is `51`.

**Conclusion.** A June-2023 install that followed the HA/AppDaemon path is TFT **v4.1.1, display protocol version 51, model `eu`**. If the panel was set up with the **ioBroker** script of June 2023, it would be **50** (`v4.0.5`): JUN23 `ioBroker/NsPanelTs.ts:1331-1332` sets `tft_version = 'v4.0.5'` and `desired_display_firmware_version = 50`.

The only protocol-relevant difference between 50 and 51 is that 51 adds `cardGrid2`, an 8-tile grid (#898, 2023-05-20); `cardGrid2.txt` does not exist at v4.0.5. Everything in this spec works on both 50 and 51, except `cardGrid2`.

The installed Berry driver is driver version **8**. `autoexec.be` was not changed between 2022-12-27 (`17b7696d`) and 2023-09-18, and its header line ("Extended Flashing to faster speeds; improved Error Handling") is the one the user saw.

### 1.2 Differences between 51 and the latest TFT (53, v4.7.x / HEAD)

These come from `git diff JUN23 V474 -- HMI/n2t-out`.

| Feature | 51 (June 2023) | 52 | 53 (v4.3.1 … v4.7.x) |
|---|---|---|---|
| `dimmode` 5th arg `featNewSliders` (0/1) and the `popupLightNew` page | – | yes | yes |
| Grid icon font override (an icon text ending in `¬<fontId>`) | – | – | yes |
| TFT sends `event,renderCurrentPage` in every card's Preinitialize event | – | – | yes. Added 2023-11-24 (commit `349db170`) **without** bumping tVersion, so later 53 builds have it and early ones don't. |
| `screensaver`: `vis tAMPM,1` when AM/PM is present | – | – | yes |
| `popupNotify` ignores `timeout` (renamed to `timeout2`) | – | – | yes |
| Framing, CRC, `pageType`, `entityUpd` field layout for grid/entities, `weatherUpdate`, `time`/`date`/`timeout`, events `buttonPress2`/`sleepReached`/`pageOpenDetail`/`startup` | **identical** | identical | identical |

A backend written against 51 therefore also drives 52 and 53 TFTs. It must ignore an unknown `event,renderCurrentPage` (or answer it by re-rendering the current card), and must not send a 5th `dimmode` arg to 51.

---

## 2. Serial layer

### 2.1 Frame format (both directions, identical)

```
+------+------+--------+--------+---------------------+--------+--------+
| 0x55 | 0xBB | len_lo | len_hi | payload (len bytes) | crc_lo | crc_hi |
+------+------+--------+--------+---------------------+--------+--------+
```

- `len` is the **byte** length of the UTF-8 payload, uint16 little-endian. It does not include the header or the CRC.
- `crc` is **CRC-16/MODBUS** (reflected poly 0xA001, init 0xFFFF, no final XOR; check value of `"123456789"` = 0x4B37). It is computed over **header + length + payload**, i.e. bytes `[0 .. 4+len-1]`, and transmitted little-endian.
- The payload is UTF-8 text. **ESP→TFT** commands are `~`-separated (`cmd~arg1~arg2…`). **TFT→ESP** messages are `,`-separated (`event,<type>,…`).

Berry encoder (BERRY8 `tasmota/autoexec.be:46-72`):

```berry
    def crc16(data, poly)
      if !poly  poly = 0xA001 end
      # CRC-16 MODBUS HASHING ALGORITHM
      var crc = 0xFFFF
      for i:0..size(data)-1
        crc = crc ^ data[i]
        for j:0..7
          if crc & 1
            crc = (crc >> 1) ^ poly
          else
            crc = crc >> 1
          end
        end
      end
      return crc
    end

    # encode using custom protocol 55 BB [payload length] [payload length] [payload] [crc] [crc]
    def encode(payload)
      var b = bytes()
      b += self.header                    # static header = bytes('55BB')
      b.add(size(payload), 2)   # add size as 2 bytes, little endian
      b += bytes().fromstring(payload)
      var msg_crc = self.crc16(b)         # CRC over header+len+payload
      b.add(msg_crc, 2)       # crc 2 bytes, little endian
      return b
    end
```

The TFT receive side is identical on every page. JUN23 `HMI/n2t-out/pageStartup.txt:200-260` (same block in `screensaver.txt:392`, `cardGrid.txt:615`):

```
if(u[bufferPos]==187&&u[bufferPos-1]==85)         // 0xBB preceded by 0x55
  udelete bufferPos-1                              // drop garbage before header
  ucopy payloadLength,2,2,0                        // len, LE
  payloadLength+=3 ; payloadLength+=2
  if(payloadLength<usize)
    crcrest 1,0xFFFF                               // CRC type 1 = MODBUS
    crcputu 0,payloadLength-1                      // over 55 BB len payload
    ucopy recvCrc,payloadLength-1,2,0              // received CRC, LE
    if(crcval==recvCrc)
      ucopy strCommand.txt,4,payloadLength-5,0     // payload → strCommand
      spstr strCommand.txt,tInstruction.txt,"~",0  // field 0 = command
      ...
    udelete payloadLength-1 ; bufferPos=0
```

The TFT sender (every event, e.g. JUN23 `pageStartup.txt:157-172`):

```
btlen tSend.txt,sys0         // byte length
crcrest 1,0xffff
crcputh 55 bb
crcputs sys0,2
crcputs tSend.txt,0
printh 55 bb
prints sys0,2                // 2 bytes LE
prints tSend.txt,0
prints crcval,2              // 2 bytes LE
```

The ESPHome C++ implementation is a ready reference: ESPH `:125-138` for `send_custom_command`, `:60-103` for the receive state machine, `:190-205` for `crc16`.

Test vectors (computed with the algorithm above):

| Payload | Frame (hex) |
|---|---|
| `time~14:05` | `55 BB 0A 00 74 69 6D 65 7E 31 34 3A 30 35 BC 1D` |
| `timeout~20` | `55 BB 0A 00 74 69 6D 65 6F 75 74 7E 32 30 DB E5` |
| `pageType~cardGrid` | `55 BB 11 00 70 61 67 65 54 79 70 65 7E 63 61 72 64 47 72 69 64 4F D7` |
| `dimmode~10~100~6371` | `55 BB 13 00 64 69 6D 6D 6F 64 65 7E 31 30 7E 31 30 30 7E 36 33 37 31 94 2B` |
| (TFT→ESP) `event,startup,51,eu` | `55 BB 13 00 65 76 65 6E 74 2C 73 74 61 72 74 75 70 2C 35 31 2C 65 75 56 30` |
| (TFT→ESP) `event,buttonPress2,sw1,button` | `55 BB 1D 00 65 76 65 6E 74 2C 62 75 74 74 6F 6E 50 72 65 73 73 32 2C 73 77 31 2C 62 75 74 74 6F 6E CC F5` |
| `statusUpdate~<home U+E2DB>~65535~~~~` | `55 BB 1A 00 73 74 61 74 75 73 55 70 64 61 74 65 7E EE 8B 9B 7E 36 35 35 33 35 7E 7E 7E 7E C5 DF` |

### 2.2 How the Berry driver receives (and why not to copy it)

BERRY8 `autoexec.be:31-44` (`split_55`) and `:147-208` (`every_100ms`):

```berry
    def split_55(b)
      var ret = []
      var s = size(b)
      var i = s-2   # start from last-1
      while i > 0
        if b[i] == 0x55 && b[i+1] == 0xBB
          ret.push(b[i..s-1]) # push last msg to list
          b = b[(0..i-1)]   # write the rest back to b
        end
        i -= 1
      end
      ret.push(b)
      return ret
    end
...
    def every_100ms()
        if self.ser.available() > 0
            var msg = self.ser.read()
            ...
                    var msg_list = self.split_55(msg)
                    for i:0..size(msg_list)-1
                        msg = msg_list[i]
                        if size(msg) > 0
                            if msg == bytes('000000FFFFFF88FFFFFF')
                                self.screeninit()                       # sends raw "recmod=1"+FF FF FF
                            elif size(msg)>=2 && msg[0]==0x55 && msg[1]==0xBB
                                var jm = string.format("{\"CustomRecv\":\"%s\"}",msg[4..-3].asstring())
                                tasmota.publish_result(jm, "RESULT")
                            elif msg[0]==0x07 && size(msg)==1 # BELL/Buzzer
                                tasmota.cmd("buzzer 1,1")
                            else
                                var jm = string.format("{\"nextion\":\"%s\"}",str(msg[0..-4]))
                                tasmota.publish_result(jm, "RESULT")
```

- It polls every 100 ms, reads everything that is available, and splits the chunk at each `55 BB`.
- It **does not check the CRC or the length**: the payload is taken as `msg[4..-3]`. It also **does not reassemble a frame that spans two reads**.
- **Implement a proper byte state machine instead**, as ESPH `:60-103` does: wait for `0x55`, then `0xBB`, then 2 length bytes, then `len` payload bytes, then 2 CRC bytes. Verify the CRC and drop the frame on mismatch. Resync on anything unexpected. Cap `len` at about 1024.
- Besides `55 BB` frames, the TFT emits raw Nextion bytes:
  - On TFT power-up, `00 00 00 FF FF FF 88 FF FF FF` (JUN23 `HMI/n2t-out/Program.s.txt:19`). The driver answers with the raw Nextion command `recmod=1` followed by `FF FF FF` (BERRY8 `:101-104`). The pageStartup page also sets `recmod=1` itself (`pageStartup.txt:15`), so the answer is belt-and-braces. It is cheap, so do it.
  - A single `0x07` byte is a buzzer request. That one byte belongs to an older TFT; the 51 HMI does not send it.
- Raw Nextion commands (`sendnx`): ASCII command followed by `FF FF FF`.

### 2.3 ESP-side transport rules

- **Send** with the 55BB encoder. Nothing is acknowledged.
- The TFT polls its serial buffer every **50 ms** (`Timer tmSerial`, Period 50). It processes every complete frame in the buffer on each tick and keeps the buffer across page changes. `pageType` deletes only its own frame (`cardGrid.txt:842-844`), so frames sent after it are processed by the new page.
  - The ESPHome component still needed a 75 ms delay between messages to avoid missed updates (ESPH `:14-18`, sairon issue #8). **Recommendation:** queue outgoing frames and pace them at **at least 20 ms** apart. Use about **75 ms after a `pageType~`**, giving the page time to load. Keep each burst under about 1 KB, which is the Nextion serial buffer size (not verified for this TJC/Nextion variant).
- **Payload limits (bytes, UTF-8).** The payload is copied into the page's `strCommand`. Its Max Text Size is: screensaver 600, cardGrid/cardGrid2 700, cardEntities 500, popupNotify 1000, pageStartup **50**. Longer payloads get truncated.
- **Field limits.** Per-field limits (Max Text Size) in 51:
  - Grid: tile name `tEntityN` 30, icon `bEntityN` 10, `tHeading` 40, internal id `entnN`/`nentN` **80**, type `typeN` 10.
  - Screensaver: `tTime` 15, `tDate` 40, `tMainText`/`tForecastN`/`tForecastNVal` 10, `tF4Icon` 5.
  - An icon is 3 UTF-8 bytes; `°` is 2.
- **Reserved characters.** Never put `~` in any field. Never put `,` in an entity id/internal name: TFT→ESP events are split on `,` (JUN23 `luibackend/mqtt.py:28`).
- **Dedupe.** AppDaemon drops a message identical to the previous one unless `force` is set (JUN23 `luibackend/mqtt.py:67-71`). This is optional. Do not drop a `pageType~X` that is needed to re-enter a page.

---

## 3. Startup handshake

### 3.1 What the TFT does

1. On power-up it prints the raw `00 00 00 FF FF FF 88 FF FF FF` and goes to `pageStartup` (`Program.s.txt:19-20`).
2. **`pageStartup` Preinitialize** (`pageStartup.txt:13-29`): `recmod=1`, `bauds=115200`, `dim=100`.
3. **`pageStartup` Postinitialize** clicks `bSendStartup`, which sends (`pageStartup.txt:157-172`):

   ```
   event,startup,51,eu
   ```

   The fields are: `event` / `startup` / HMI version (`tVersion.txt`, 51) / model (`eu`, hard-coded in the EU build; `us-l`/`us-p` in the US builds).
4. It **re-sends** the startup event from the spinner timer every 5 spinner cycles, roughly every few seconds (`tmSpinner`, `pageStartup.txt:382-408`). After 10 cycles it shows "This is taking longer than usual, please check your backend configuration".
5. In `pageStartup` it accepts only `time`, `date`, `dimmode`, `timeout`, and `pageType` (`pageStartup.txt:249-340`).
6. Sending `pageType~pageStartup` from any page returns the TFT to this page, so it sends `event,startup,…` again. AppDaemon does exactly this when the backend (re)starts (JUN23 `luibackend/controller.py:20-21`).

### 3.2 What the backend sends

AppDaemon order (JUN23 `luibackend/mqtt.py:30-40`, `luibackend/controller.py:30-40`, `:48-88`, `luibackend/pages.py:670-758`):

```
(on module start, and whenever the ESP has (re)started)  pageType~pageStartup
-- receive: event,startup,51,eu --
time~14:05~                       update_time   (pages.py:109-113)
date~Saturday, 24. October 2026   update_date   (pages.py:115-126)
dimmode~20~100~6371               update_screensaver_brightness (controller.py:88)
pageType~screensaver              render_card → page_type (pages.py:711-715)
timeout~20                        render_card (pages.py:717-721)
weatherUpdate~…                   update_screensaver_weather (pages.py:133-149)
[color~…]                         only if a theme is configured (theme.py:24-28)
statusUpdate~…                    update_status_icons (pages.py:151-167)
```

After that:
- send `time~` every minute (`controller.py:116`, `run_minutely`);
- send `date~` hourly (`controller.py:119`);
- refresh `weatherUpdate`/`statusUpdate` when their data changes while the screensaver is the current card.

### 3.3 Version check and mismatch

AppDaemon (JUN23 `luibackend/updater.py:25-79`):
- Pre-requisite: Berry driver ≥ 2 and TFT ≥ 16, the "new button cmd format", i.e. `buttonPress2`.
- If TFT version < desired (51) or the model differs, and the update mode is `auto-notify`, it sends a popup (`updater.py:35-37`):

  ```
  pageType~popupNotify
  entityUpdateDetail~updateDisplayNoYes~Display Update available!~65535~Dismiss~65535~Yes~65535~<text>~65535~0
  ```

  - The user's answer comes back as `event,buttonPress2,updateDisplayNoYes,notifyAction,yes|no` (`popupNotify.txt:171,197`).
  - On "yes", the backend triggers the Tasmota `FlashNextion <url>` command.
  - On "no" (or `bExit`) it re-renders the current card (`controller.py:239-243`).
- If the check sent no popup, `startup()` runs normally.

**For the ESP backend:**
- Accept `ver >= 50 && model == "eu"`.
  - Log the version.
  - If `ver == 50`, never use `cardGrid2`.
  - If `ver >= 52`, you may add the `dimmode` 5th field.
- If `ver < 50` or the model is wrong, show `popupNotify` with a single "OK" (or just render anyway).
- TFT flashing from the ESP is out of scope. It is the `FlashNextion` path in BERRY8 `:106-305`; ESPH has a C++ port, `nspanel_lovelace_upload.cpp`.
- Publish the startup version on MQTT, so the version on the real panel can be read.

---

## 4. ESP → TFT commands

Field indices below are the `spstr …,"~",N` indices used by the TFT (0 = command name).

### 4.1 Global commands (accepted on every page)

| Command | Effect | Source |
|---|---|---|
| `time~HH:MM[~addText]` | Stored in global `pageIcons.vaTime`; shown on the screensaver. An AM/PM marker is appended after `?`: `time~2:05?PM`. Field 2 goes to `tTimeAdd` (max 40 bytes). | `screensaver.txt:469-483`; `pages.py:109-113` |
| `date~<text>` | Stored in `pageIcons.vaDate` (max 40 bytes in `tDate`). Free text, e.g. `date~Sat, 24 Oct 2026` or Russian (Cyrillic glyphs are present: `HMI/fonts/gen-chars.py` includes 0400-04FF). | `screensaver.txt:484-489`; `pages.py:115-126` |
| `dimmode~<sleepDim>~<activeDim>[~<bgColor565>[~<fontColor565>]]` | See §6. | `screensaver.txt:447-468`; `controller.py:88` |
| `timeout~<seconds>` | Screensaver/sleep timeout; `0` disables it. See §6. | `screensaver.txt:790-795`; `pages.py:717-721` |
| `pageType~<page>[~arg2~arg3~arg4]` | Navigate. Valid pages in 51: `pageStartup`, `screensaver`, `screensaver2`, `cardEntities`, `cardGrid`, `cardGrid2`, `popupLight`, `popupShutter`, `popupNotify`, `cardThermo`, `cardMedia`, `cardAlarm`, `cardQR`, `cardPower`, `cardChart`, `cardLChart`. In `screensaver` it also sets `dim=100`, and each card's Preinitialize sets `dim=dimValueNormal`. | `screensaver.txt:709-789`; `pages.py:128-131` |

### 4.2 Screensaver (`pageType~screensaver`)

**`weatherUpdate`** carries up to 6 "entity" groups of 6 fields each (`type~id~icon~color~name~value`) after the command. `type`, `id`, and some names are ignored. AppDaemon generates it with `mask=["type","entityId"]`, so each group starts with `~~~` (JUN23 `pages.py:133-149`, `pages.py:403-408`). TFT parsing is at `screensaver.txt:511-610`.

| Group | Fields (idx) | Used | Shown in |
|---|---|---|---|
| 1 main (today) | 1 type, 2 id, **3 icon**, **4 color**, 5 name (ignored), **6 value** | icon, color, value | `tMainIcon`, `tMainText` (e.g. `12°C`) |
| 2 forecast 1 | 7, 8, **9 icon**, **10 color**, **11 name**, **12 value** | all 4 | `tF1Icon`, `tForecast1` (e.g. weekday `Mon`), `tForecast1Val` (e.g. `14°C`) |
| 3 forecast 2 | 13, 14, **15**, **16**, **17**, **18** | | `tF2Icon` … |
| 4 forecast 3 | 19, 20, **21**, **22**, **23**, **24** | | `tF3Icon` … |
| 5 forecast 4 | 25, 26, **27**, **28**, **29**, **30** | | `tF4Icon` … |
| 6 alternative layout | 31, 32, **33 icon**, **34 color**, 35, **36 value** | If field 36 ≠ "" the alternative layout is activated: the main icon moves left, and the second main item is shown. | `tMainIconAlt2`, `tMainTextAlt2` |

- Group 6 may be omitted or sent empty (`~~~~~~`) to keep the normal layout.
- The update is ignored while a `notify~` is shown on the screensaver (`&&tNotifyHead.txt==""`).
- The HMI README documents the same table: JUN23 `HMI/README.md`, section "screensaver page".
- For an AppDaemon "weather" entity, `value` is `<temperature><unit>`, and the forecast `name` is the weekday from the `"%a"` / babel `"E"` format (`pages.py:371-387`).

Weather icon colours (RGB565 decimal) used by AppDaemon, `get_entity_color` (JUN23 `pages.py:71-97`):

| HA condition | icon | color |
|---|---|---|
| sunny | weather-sunny | 65504 |
| partlycloudy / windy | weather-partly-cloudy / weather-windy | 38066 |
| clear-night | weather-night | 38060 |
| cloudy | weather-cloudy | 31728 |
| fog | weather-fog | 38066 |
| rainy | weather-rainy | 25375 |
| pouring | weather-pouring | 12703 |
| snowy / hail | weather-snowy / weather-hail | 65535 |
| snowy-rainy | weather-snowy-rainy | 38079 |
| lightning | weather-lightning | 65120 |
| lightning-rainy | weather-lightning-rainy | 50400 |
| exceptional | alert-circle-outline | 63878 |

There is no `windy-variant` colour in practice: line 77 contains the typo `icon_color: 64495`, so `windy-variant` falls back to 17299. The condition→icon mapping is `icons.py:5-21`.

**`statusUpdate~<icon1>~<color1>~<icon2>~<color2>~<altFont1>~<altFont2>`** sets the two small status icons, `tIcon1` top-left and `tIcon2` top-right (`screensaver.txt:490-510`; `pages.py:151-167`).
- Empty slot: send empty icon and colour fields. With no icons at all, AppDaemon sends `statusUpdate~~~~~~`.
- A non-empty `altFontN` switches that icon to `font=3`, a text font, so it can show plain text instead of an MDI glyph.

**`color~…`** (optional theme; 16 RGB565 values, in this order): `background~time~timeAMPM~date~tMainText~tForecast1~tForecast2~tForecast3~tForecast4~tForecast1Val~tForecast2Val~tForecast3Val~tForecast4Val~bar~tMainTextAlt2~tTimeAdd`. Sources: `theme.py:4-28`, `screensaver.txt:611-682`. The default is `color~0~65535~65535~…`.

**`notify~<heading>~<text>[~<headColor>~<textColor>]`** (optional): shows a notification on the screensaver. A tap while it is shown clears it (`screensaver.txt:683-708`, `:827-842`).

**`wake`**: simulates a tap on the screensaver, i.e. `click tc0,1` (`screensaver.txt:443-446`). Note that this makes the TFT **send** `event,buttonPress2,screensaver,bExit,N`.

`screensaver2` is an alternative screensaver layout with more fields (`HMI/n2t-out/screensaver2.txt`). It is not needed initially.

### 4.3 Entity item format (cards)

Every entity, including the navigation buttons, is 6 `~`-separated fields:

```
type ~ internalName ~ icon ~ iconColor ~ displayName ~ optionalValue
```

AppDaemon builds this in `generate_entities_item` (JUN23 `pages.py:169-408`; the return is at `:408`).

| field | meaning |
|---|---|
| `type` | `delete` or empty means hide the slot. Others: `light`, `switch`, `fan`, `button`, `shutter`, `text`, `number`, `input_sel`, `timer`, `media_pl`. See the table below. |
| `internalName` | Echoed back verbatim in events. Max 80 bytes; no `,` or `~`. AppDaemon uses the HA entity_id (e.g. `switch.pump`), or `uuid.xxx` for lights, or `navigate.uuid.xxx` / `navUp` for navigation. |
| `icon` | One icon glyph (UTF-8, see §5), or short text. The grid shows the first 4 characters of a sensor state in the icon slot (`pages.py:312-315`). |
| `iconColor` | RGB565 decimal (§5.2). |
| `displayName` | Label: grid max 30 bytes, entities about 30 bytes. |
| `optionalValue` | Type-specific, see below. |

How `cardEntities` interprets `type` and `optionalValue` (EU: 4 rows; JUN23 `HMI/n2t-out/cardEntities.txt:1183-1820`, row 1 at `:1222-1370`):

| type | Row widget | optionalValue | Event sent on touch |
|---|---|---|---|
| `light`, `switch`, `fan` | toggle `btOnOff` | `1` = on, `0` = off | `OnOff,<0/1>`. Touching the icon area of `light`/`fan` opens `popupLight`/`popupFan`. |
| `button` | text button (colour 1374) | button caption, e.g. `PRESS`, `ACTIVATE`, `RUN` | `button` |
| `input_sel` | text button | current option | opens `popupInSel` (on icon) / `button` |
| `text`, `timer` | read-only text | text to show, e.g. `21.5°C`, `155 W` | none (`timer` icon opens `popupTimer`) |
| `shutter` | ▲ ■ ▼ buttons | `iconUp\|iconStop\|iconDown\|upState\|stopState\|downState` with `enable`/`disable`, e.g. `<arrow-up>\|<stop>\|<arrow-down>\|enable\|enable\|disable` (`pages.py:257-285`) | `up` / `stop` / `down`. The icon opens `popupShutter`. |
| `number` | slider | `value\|min\|max` | `number-set,<value>` |

On `cardGrid` / `cardGrid2` the `type` only matters for popups:
- `light`: a long press (≥ 500 ms, `tmLP`) opens `popupLight`;
- `fan`: a long press opens `popupFan`;
- `shutter` / `input_sel` / `timer`: a press opens the popup immediately.

Every tile otherwise sends `button` on release (`cardGrid.txt:240-283`, `:598-614`). `optionalValue` is not displayed on the grid, but keep the field so the positions stay aligned.

**Recommendation for v1 (no popups):** use type **`switch`** for relay tiles, **`button`** for momentary actions (gate/garage impulse), and **`text`** for read-only values. Avoid `light`/`shutter`/`fan`: on the grid a long press would open a popup that you then have to serve.

### 4.4 Card pages

```
entityUpd~<heading>~<navLeft: 6 fields>~<navRight: 6 fields>~<entity1: 6 fields>~…~<entityN: 6 fields>
```

| Field idx | Content |
|---|---|
| 1 | heading (`tHeading`, max 40 bytes) |
| 2-7 | left nav button (`bPrev`): type, **internalName** (idx 3 → `nent1`), **icon** (idx 4), **color** (idx 5), name, value. Type `delete` or empty hides it. |
| 8-13 | right nav button (`bNext`): type, internalName (idx 9 → `nent2`), icon (10), color (11), name, value |
| 14-19 | entity 1 (type 14, internalName 15, icon 16, color 17, name 18, value 19) |
| 20-25, 26-31, … | entity 2, 3, … (stride 6) |

Card capacity: `cardEntities` 4 entities; `cardGrid` **6** (last at idx 44-49); `cardGrid2` 8 (last at 56-61, TFT ≥ 51 only). The TFT source is the comment `command format: entityUpd,heading,navigation,[,type,internalName,iconId,iconColor,displayName,optionalValue]x6` at `cardGrid.txt:666-830`. AppDaemon switches to `cardGrid2` automatically when there are more than 6 entities (`pages.py:713-714`).

**Navigation fields as AppDaemon generates them** (`pages.py:679-709`):
- No neighbour card: `delete~~~~~` (6 fields).
- Previous/next card: `button~navigate.<uuid>~<arrow-left-bold U+E730 | arrow-right-bold U+E733>~65535~<title>~<PRESS text>`.
- Hidden sub-card (left = up arrow): `x~navUp~<arrow-up-bold U+E736>~65535~~`.
- The pressed nav button sends `event,buttonPress2,<internalName>,button`. The backend owns all navigation state: the TFT does nothing on its own except send the event.
- Swipes: `mSwipeNext`/`mSwipePrev`/`mSwipeUp` hotspots "click" `bNext`/`bPrev` (`cardGrid.txt:568-597`). A hidden `bPrev` still sends its (empty) `nent1`; ignore an empty internal name.

**Re-render on state change:** send `entityUpd~…` again with the new colours/values. **No `pageType~` is needed** when the TFT is already on the page; AppDaemon uses `render_card(send_page_type=False)` from `state_change_callback` (`controller.py:155-169`). It also re-sends `timeout~` each render, which is harmless. On a full navigation, send `pageType~X`, then `timeout~N`, then `entityUpd~…`.

### 4.5 Popup notify (optional; also used for the version prompt)

```
pageType~popupNotify
entityUpdateDetail~<id>~<heading>~<headColor>~<btn1Text>~<btn1Color>~<btn2Text>~<btn2Color>~<text>~<textColor>~<timeout s, 0=none>
```

Sources: `pages.py:982-984`; `updater.py:35-37`. The answers are `event,buttonPress2,<id>,notifyAction,no|yes` and `event,buttonPress2,popupNotify,bExit` (`popupNotify.txt:145,171,197`). After an answer, re-render the previous card.

---

## 5. Icons and colours

### 5.1 Icon encoding

An icon is **one Unicode character from the Private Use Area**, rendered by the TFT's built-in Material Design Icons font. It is sent as **UTF-8 (3 bytes)** inside the field.
- Mapping: `TFT codepoint = MDI codepoint − 0xF0001 + 0xE000`, i.e. MDI `F0335` → `U+E334`. In other words, the MDI codepoint minus 0x0E2001 when the MDI codepoint is above `F0000`.
- The authoritative table is `HMI/code_gen/icons/icons.json` (name → hex), which generates `luibackend/icon_mapping.py` (JUN23 `HMI/code_gen/icons/icons.py`). It holds 6896 icons and was last regenerated in 2023-04 (`icon_mapping.py` commits `5f97824e`…`94295495`).
- Unknown names fall back to `alert-circle-outline` (`icon_mapping.py`, `get_icon_id`).
- The HEAD mapping gives the same codepoints for every icon in the table below.
- UTF-8 for U+E000…U+FFFF is 3 bytes: `0xE0|(cp>>12)`, `0x80|((cp>>6)&0x3F)`, `0x80|(cp&0x3F)`. Precompute and store the 3 bytes per icon. Example: `U+E334` → `EE 8C B4`.

| Icon (MDI name) | TFT codepoint | UTF-8 bytes | Notes |
|---|---|---|---|
| lightbulb | U+E334 | EE 8C B4 | AppDaemon default for `light` |
| lightbulb-outline | U+E335 | EE 8C B5 | |
| lightbulb-off | U+EE4E | EE B9 8E | |
| lightbulb-off-outline | U+EE4F | EE B9 8F | |
| lightbulb-on | U+E6E7 | EE 9B A7 | |
| lightbulb-group | U+F252 | EF 89 92 | |
| ceiling-light | U+E768 | EE 9D A8 | |
| ceiling-light-outline | U+F7C6 | EF 9F 86 | |
| outdoor-lamp | U+F053 | EF 81 93 | |
| wall-sconce | U+E91B | EE A4 9B | |
| wall-sconce-flat | U+E91C | EE A4 9C | |
| wall-sconce-round | U+E747 | EE 9D 87 | |
| wall-sconce-round-variant | U+E91D | EE A4 9D | |
| coach-lamp | U+F01F | EF 80 9F | |
| string-lights | U+F2B9 | EF 8A B9 | |
| string-lights-off | U+F2BA | EF 8A BA | |
| led-strip-variant | U+F050 | EF 81 90 | |
| lamp / floor-lamp | U+E6B4 / U+E8DC | EE 9A B4 / EE A3 9C | |
| water-pump | U+E58E | EE 96 8E | |
| water-pump-off | U+EF92 | EE BE 92 | |
| gate | U+E298 | EE 8A 98 | |
| gate-open | U+F169 | EF 85 A9 | |
| garage | U+E6D8 | EE 9B 98 | |
| garage-open | U+E6D9 | EE 9B 99 | |
| garage-variant | U+F2D2 | EF 8B 92 | |
| power | U+E424 | EE 90 A4 | |
| power-plug / power-plug-off | U+E6A4 / U+E6A5 | EE 9A A4 / EE 9A A5 | |
| power-socket-eu | U+E7E6 | EE 9F A6 | |
| light-switch | U+E97D | EE A5 BD | AppDaemon default for `switch` |
| toggle-switch / toggle-switch-off | U+E520 / U+E521 | EE 94 A0 / EE 94 A1 | |
| fan | U+E20F | EE 88 8F | |
| home | U+E2DB | EE 8B 9B | |
| home-outline | U+E6A0 | EE 9A A0 | |
| weather-sunny | U+E598 | EE 96 98 | |
| weather-partly-cloudy | U+E594 | EE 96 94 | |
| weather-cloudy | U+E58F | EE 96 8F | |
| weather-rainy | U+E596 | EE 96 96 | |
| weather-pouring | U+E595 | EE 96 95 | |
| weather-snowy | U+E597 | EE 96 97 | |
| weather-snowy-rainy | U+E67E | EE 99 BE | |
| weather-fog | U+E590 | EE 96 90 | |
| weather-hail | U+E591 | EE 96 91 | |
| weather-lightning | U+E592 | EE 96 92 | |
| weather-lightning-rainy | U+E67D | EE 99 BD | |
| weather-windy / -variant | U+E59C / U+E59D | EE 96 9C / EE 96 9D | |
| weather-night | U+E593 | EE 96 93 | clear-night |
| weather-night-partly-cloudy | U+EF30 | EE BC B0 | |
| thermometer | U+E50E | EE 94 8E | |
| water-percent | U+E58D | EE 96 8D | humidity |
| temperature-celsius | U+E503 | EE 94 83 | |
| alert-circle-outline | U+E5D5 | EE 97 95 | fallback |
| arrow-left-bold / arrow-right-bold / arrow-up-bold | U+E730 / U+E733 / U+E736 | EE 9C B0 / EE 9C B3 / EE 9C B6 | navigation |
| arrow-up / stop / arrow-down | U+E05C / U+E4DA / U+E044 | EE 81 9C / EE 93 9A / EE 81 84 | shutter buttons |
| window-shutter / window-shutter-open | U+F11B / U+F11D | EF 84 9B / EF 84 9D | |
| gesture-tap-button | U+F2A7 | EF 8A A7 | AppDaemon default for `button` |
| wifi / wifi-off | U+E5A8 / U+E5A9 | EE 96 A8 / EE 96 A9 | status icon |

(Codepoints were extracted from JUN23 `luibackend/icon_mapping.py` and cross-checked against HEAD `HMI/code_gen/icons/icons.json`.)

### 5.2 Colour encoding

- Colours are **RGB565 as an unsigned decimal string**: `((r>>3)<<11) | ((g>>2)<<5) | (b>>3)` (JUN23 `luibackend/helper.py:40-46`, `rgb_dec565`).
- Brightness-scaled light colour: `rgb_brightness` maps HA brightness 0-255 to 70-255 and multiplies the RGB values by it (`helper.py:31-38`; used at `pages.py:99-106`).

| Meaning | RGB | RGB565 dec | Source |
|---|---|---|---|
| **entity ON (default "yellow")** | 253,216,53 | **65222** | `pages.py:45,47` |
| **entity OFF (default "blue-grey")** | 68,115,158 | **17299** | `pages.py:46`; also the hard-coded colour for text/unknown entities |
| neutral grey (if you prefer grey for off) | 128,128,128 | 33808 | computed |
| dark grey | 64,64,64 | 16904 | computed |
| white (nav arrows, text) | 255,255,255 | 65535 | `pages.py:685` |
| black | 0,0,0 | 0 | |
| red / green / blue | | 63488 / 2016 / 31 | computed |
| orange | 255,165,0 | 64800 | computed |
| default card background `ha-dark` | ≈24,28,24 | **6371** | `controller.py:81-82`; `Program.s.txt` `defaultBcoColor=6371` |
| default font colour | | 65535 | `Program.s.txt` |
| disabled shutter button (TFT internal) | | 27501 | `cardEntities.txt` |
| button-text colour (TFT internal) | | 1374 | `cardEntities.txt` |

---

## 6. Brightness, dimming and screensaver timeout

TFT globals (JUN23 `Program.s.txt:7-16`): `sleepTimeout=20` (s), `dimValue=40` (screensaver/sleep brightness), `dimValueNormal=100` (active brightness), `defaultBcoColor=6371`, `defaultFontColor=65535`.

- **`dimmode~<sleep 0-100>~<active 0-100>[~<bg565>[~<font565>]]`** sets `dimValue`, `dimValueNormal`, and optionally the default background/font colour. The colours take effect on the next page load.
  - In 51 there is **no** 5th field. 52+ accept a 5th field, `featNewSliders` (0/1).
  - Brightness uses the Nextion `dim` scale 0-100. `0` turns the backlight off: `dimmode~0~100` gives a black screensaver.
  - **`sleep` must differ from `active`**, otherwise the sleep timer never fires. Every card's `tmSleep` only counts `if(sleepTimeout!=0&&dimValue!=dim)` (`cardGrid.txt:962-990`). AppDaemon enforces this with `if sleepBrightness==brightness: sleepBrightness -= 1` (`controller.py:73-75`).
  - On the screensaver page, `dimmode` immediately applies `dim=dimValue`. On cards it applies `dim=dimValueNormal`.
  - The AppDaemon defaults are `sleepBrightness 20`, `screenBrightness 100`, `defaultBackgroundColor ha-dark` (6371) (JUN23 `luibackend/config.py:120-137`).
- **`timeout~<s>`** sets `sleepTimeout`; `0` disables the sleep timer. AppDaemon sends it on every card render, defaulting to 20 s (`pages.py:717-721`). The HMI README says "max 65"; the variable is an int, so this is unverified (§9).
- **Card pages** (`tmSleep`, 1 s period, cardGrid/cardEntities/cardThermo/cardMedia/cardQR/cardAlarm):
  - A touch resets the idle counter (`tc0` → `sleepValue=0`).
  - After `sleepTimeout` idle seconds the TFT sets `dim=dimValue` and sends `event,sleepReached,<cardName>`. **The TFT stays on the card**, only dimmed. The backend must answer with the screensaver render (`pageType~screensaver` …), as AppDaemon does in `controller.py:233-237`.
  - `cardGrid2` reports `event,sleepReached,cardGrid` (copy-paste in the HMI, `cardGrid2.txt:1149`).
- **Screensaver page:**
  - A tap sets `dim=dimValueNormal` and sends `bExit`.
  - If no navigation follows, the screensaver re-dims after `sleepTimeout` s and resets the tap counter (`screensaver.txt:808-826`). It sends no event in this case.

---

## 7. TFT → ESP events (complete list for TFT 51)

All events are `,`-separated strings (`grep tSend.txt="event` over JUN23 `HMI/n2t-out`). AppDaemon's dispatcher is JUN23 `luibackend/mqtt.py:28-59`; its action handler is `luibackend/controller.py:203-456`.

| Event string | When | Expected backend reaction |
|---|---|---|
| `event,startup,<ver>,<model>` | pageStartup, repeated | §3 |
| `event,sleepReached,<card>` | idle timeout on a card | push the current card on the back-stack; render the screensaver |
| `event,buttonPress2,screensaver,bExit,<tapCount>` | tap on the screensaver (sent on release; `tapCount` = 1, 2, … since the last re-dim) | render the default/previous card: `pageType~cardGrid`, `timeout~`, `entityUpd~` (`controller.py:210-231`). With "double tap to unlock", act only when `tapCount >= 2`. |
| `event,buttonPress2,screensaver,swipeUp\|swipeDown\|swipeLeft\|swipeRight` | swipe on the screensaver. It **replaces** the `bExit` for that touch (`screensaver.txt:863-923`). | treat like `bExit`, or ignore |
| `event,buttonPress2,<internalName>,button` | grid tile release; cardEntities `button`/`input_sel` text button; nav buttons `bPrev`/`bNext` | nav (`navigate.*`, `navUp`, `navPrev`, `navNext`) → render the target card. Otherwise **toggle** the relay (AppDaemon toggles light/switch/input_boolean/fan, presses button/scene/script; `controller.py:281-331`). |
| `event,buttonPress2,<internalName>,OnOff,<0\|1>` | cardEntities toggle. The value is the toggle's new `val`. | 1 → on, 0 → off (`controller.py:246-250`) |
| `event,buttonPress2,<internalName>,up\|stop\|down` | cardEntities shutter row / popupShutter | cover open/stop/close (`controller.py:261-266`) |
| `event,buttonPress2,<internalName>,number-set,<v>` | cardEntities slider | set value |
| `event,buttonPress2,<internalName>,positionSlider,<v>` / `tiltOpen` / `tiltStop` / `tiltClose` / `tiltSlider,<v>` | popupShutter | |
| `event,buttonPress2,<internalName>,brightnessSlider,<0-100>` / `colorTempSlider,<0-100>` / `colorWheel,<x>\|<y>\|<wh>` | popupLight | |
| `event,buttonPress2,<id>,notifyAction,yes\|no` | popupNotify buttons | |
| `event,buttonPress2,popupLight\|popupShutter\|popupFan\|popupThermo\|popupInSel\|popupTimer\|popupNotify,bExit` | popup closed. The TFT does **not** navigate back itself. | re-render the current card with `pageType~…` + `entityUpd~…` (`controller.py:239-240`) |
| `event,pageOpenDetail,<popupType>,<internalName>` | a popup page opened (`popupLight` etc.) | send `entityUpdateDetail~…` for that popup (`controller.py:189-201`). Not needed if the UI uses no popup-capable types. |
| (TFT ≥ 53 late builds only) `event,renderCurrentPage` | each card's Preinitialize | re-send the current card's `entityUpd` / ignore |

Not from the TFT:
- The NSPanel's **two hardware buttons are ESP GPIO14 (left) and GPIO27 (right)**. They are handled entirely by the firmware.
- The ioBroker script's `event,button1` / `event,button2` are synthesised by Tasmota rules, not by the TFT (JUN23 `ioBroker/NsPanelTs.ts:2985-2990`).
- The relays are on separate GPIOs as well.
- A good use for the buttons: on the screensaver, a button press can "wake" the panel by rendering a card (`dimmode` is unchanged; the card's Preinitialize sets `dim=dimValueNormal`).

---

## 8. Worked example (exact payloads)

Icons are written as `\uXXXX` and must be sent as their 3 UTF-8 bytes. `°` is `C2 B0`. Each line below is **one 55BB frame**.

### 8.1 Boot → screensaver with time + weather

```
→ (raw) recmod=1 FF FF FF                       # only in reply to 00 00 00 FF FF FF 88 FF FF FF
→ pageType~pageStartup                          # once at ESP boot (forces a fresh startup event)
← event,startup,51,eu
→ time~14:05~
→ date~Saturday, 24. October 2026
→ dimmode~20~100~6371
→ pageType~screensaver
→ timeout~20
→ weatherUpdate~~~~65504~~12°C~~~~38066~Sun~14°C~~~~31728~Mon~11°C~~~~25375~Tue~9°C~~~~65535~Wed~1°C~~~~~~
→ statusUpdate~~65535~~17299~~
```

`weatherUpdate` above is 124 bytes and has 37 fields. Its indices: 3=main icon, 4=65504, 6=`12°C`, 9/10/11/12 = forecast-1 icon/colour/`Sun`/`14°C`, … 27/28/29/30 = forecast-4, 31-36 empty (normal layout).

### 8.2 Tap → cardGrid with 6 switches

```
← event,buttonPress2,screensaver,bExit,1
→ pageType~cardGrid
→ timeout~20
→ entityUpd~Lights~delete~~~~~~delete~~~~~~switch~r1~~65222~Kitchen~1~switch~r2~~17299~Hall~0~switch~r3~~17299~Outdoor~0~switch~r4~~65222~Garland~1~switch~r5~~17299~Pump~0~switch~r6~~17299~Gate~0
```

This is 211 bytes, under the 700 limit, with 50 fields. Index check: 1=`Lights`; 2=`delete` (prev hidden); 8=`delete` (next hidden); tile k (k=1..6) has type at `14+6(k-1)`, id at +1, icon at +2, colour at +3, name at +4, value at +5. Tile 6: 44=`switch` … 49=`0`.

With a second card, replace the left/right nav groups, e.g. `…~Lights~delete~~~~~~button~navigate.page2~~65535~Page 2~~switch~r1…`. Tapping the arrow then sends `event,buttonPress2,navigate.page2,button`.

### 8.3 Tile tap → relay toggle → update

```
← event,buttonPress2,r2,button                 # sent on touch release of tile 2
   (ESP toggles relay r2 → ON)
→ entityUpd~Lights~delete~~~~~~delete~~~~~~switch~r1~~65222~Kitchen~1~switch~r2~~65222~Hall~1~switch~r3~~17299~Outdoor~0~switch~r4~~65222~Garland~1~switch~r5~~17299~Pump~0~switch~r6~~17299~Gate~0
```

No `pageType` is needed. A relay changed by anything else (hardware button, MQTT, schedule) must produce the same `entityUpd` while the grid is the current page.

### 8.4 Idle → screensaver

```
← event,sleepReached,cardGrid                  # after 20 s idle; panel already dimmed to 20
→ pageType~screensaver
→ timeout~20
→ weatherUpdate~…                               # as in 8.1
→ statusUpdate~…
```

Minimal ESP state machine: `currentPage ∈ {startup, screensaver, card[i]}`, a back-stack for `sleepReached`/`bExit`, the last weather data, and relay states. Re-render the current page on any change. Clock: `time~` on every minute change and `date~` on day change, sent on every page (cheap).

---

## 8a. TFT upload (Nextion upload protocol v1.2, `FlashNextion`)

Sources, read 2026-10-25: HEAD `tasmota/autoexec.be` (driver version 9, "Extended Flashing to faster speeds"): `begin_nextion_flash`, `every_100ms`, `write_block`, `open_url_at`. ESPHome `esphome/components/nextion/nextion_upload_arduino.cpp` (dev): `upload_tft`, `upload_by_chunks_`. Nextion "HMI upload protocol v1.1" (nextion.tech, 2017-12-08). The v1.2 additions (`whmi-wris`, the `08` reply) are not officially documented. Both implementations above use them in the same way.

| Step | ESP → display | display → ESP |
|---|---|---|
| leave protocol reparse mode | `DRAKJHSUYDGBNCJHGJKSHBDN FF FF FF`, `recmod=0 FF FF FF` twice (Berry) | — |
| handshake, at the display's current speed (Lovelace sets `bauds=115200` in pageStartup, which is persistent) | `connect FF FF FF` (v1.1 also suggests `FF FF FF` before it and the broadcast form `FF FF connect FF FF FF`) | `comok <touch>,<reserved>,<model>,<fw>,<mcu>,<serial>,<flash size> FF FF FF`. The Berry driver waits for `comok 2`. |
| start | `whmi-wris <file size>,<baud>,res0 FF FF FF` (`whmi-wri` = protocol v1.0/1.1). Then switch the ESP UART to `<baud>`. | `05` when ready (v1.1 doc: within 500 ms; ESPHome waits 5 s) |
| data | the file in 4096-byte blocks (the last one shorter), each sent after the previous reply | `05` = next block. **v1.2:** after the first block, `08` + uint32 LE file offset. 0 means "continue"; non-zero means "continue from this offset" (unchanged parts are skipped; Berry and ESPHome re-open the URL with `Range: bytes=<offset>-`). ESPHome notes that some bootloaders put a multi-second gap between `08` and the 4 offset bytes. |
| end | — | after the last block's `05` the display installs the TFT and restarts: `00 00 00 FF FF FF 88 FF FF FF`, then `event,startup,<ver>,eu` |

Berry speeds: `FlashNextion` = v1.2 at 921600. `FlashNextionAdv 0..6` = v1.2/v1.0 at 921600/115200/256000, and 6 opens the UART at 9600 first (for a display without a valid TFT).

Once `whmi-wris` is accepted, the display treats every byte as file data until all `<size>` bytes have arrived, or until it is power-cycled. A display whose upload broke off boots into the Nextion OS without a usable TFT. It still answers `connect`, at its persistent `bauds` (115200 after Lovelace), otherwise at 9600. So the upload can be repeated from that state.

TFT files (served by the project, plain HTTP, `Accept-Ranges: bytes`; checked 2026-10-25):

| File | Size | MD5 | tVersion |
|---|---|---|---|
| `http://nspanel.pky.eu/lovelace-ui/github/nspanel-v4.7.3.tft` (= `nspanel-v4.7.4.tft`, identical bytes) | 8 373 960 | `cbcaf903f9e6d07944353498fefeba6a` | 53 (HEAD `HMI/n2t-out/pageStartup.txt`, `tVersion` = 53) |
| `http://nspanel.pky.eu/lovelace-ui/github/nspanel-v4.1.1.tft` (June 2023) | 8 261 216 | `9d8577af882754c5bbbcbce89b2a2a25` | 51 |
| `http://nspanel.pky.eu/lui.tft` (main / dev build, = `HMI/nspanel.tft` in git) | 8 550 324 | | dev |

The AppDaemon app (HEAD and V474 `nspanel-lovelace-ui.py`) uses `version = "v4.7.3"` for the EU URL, even in release v4.7.4. GitHub releases have no TFT assets. HTTPS on the same host also works, but TLS needs about 40 KB more heap on the ESP.

---

## 9. Uncertainties — verify on the real panel

1. **Actual TFT version.** Expected `51` (or `50` if the panel was installed from the ioBroker script). Verify before replacing Tasmota:
   1. In the Tasmota console, run `CustomSend pageType~pageStartup`.
   2. Watch `tele/<topic>/RESULT` (or the console) for `{"CustomRecv":"event,startup,NN,eu"}`.
   3. Also run `GetDriverVersion`; it should give `{"nlui_driver_version":"8"}`.

   `weblog 3` additionally shows `NXP: Received Raw = …`, the exact frame bytes, which gives a free CRC test vector. After the port, the ESP module should log/publish the startup string.
2. **Inter-frame pacing / buffer size.** The TFT polls every 50 ms, and the Nextion/TJC serial buffer (about 1 KB?) can overflow on bursts. ESPHome needed a 75 ms delay per message. Measure whether `pageType` followed immediately by `entityUpd` renders reliably, and tune the delay.
3. **Max Text Size in bytes vs characters for UTF-8 fonts.** Assumed bytes. Test long Cyrillic names (2 bytes per letter) on tiles (30) and the heading (40), and `date~` (40).
4. **Icon glyph presence.** Newer MDI icons (e.g. `ceiling-light-outline` U+F7C6, `string-lights` U+F2B9, `garage-variant` U+F2D2) are in the mapping, but the TFT icon font is baked into the .tft. Confirm that each chosen glyph renders; a blank or tofu glyph means it is missing from the font.
5. **`OnOff` value semantics.** Assumed to be the new toggle state (Nextion dual-state `val` after press), which is how AppDaemon treats it. Verify on cardEntities.
6. **`timeout` upper bound.** The README says "max 65 s"; the variable is a 32-bit int. Test 300 s.
7. **Startup retry period.** Inferred "every few seconds" from the spinner timer. Measure it, because ESP boot plus display power-on (GPIO4 LOW) timing decides whether the first `88` frame is caught. The backend must not depend on the raw `88` frame or on the first startup event.
8. **TFT power sequencing.** Behaviour if the ESP drives GPIO4 HIGH→LOW late, or glitches it during an ESP reboot: does the TFT reboot, and does it show pageStartup? Also check whether the TFT keeps its page across an ESP-only reboot. AppDaemon always sends `pageType~pageStartup` at backend start for this reason.
9. **`renderCurrentPage`.** Absent in 51. If the panel turns out to be a late 53 build, handle it. (ESPurna re-renders the current card. The `entityUpd` of a fresh `pageType` is usually still queued and is coalesced, so nothing is sent twice. HEAD `cardGrid.txt:14` sends it in Preinitialize.)
10. **Screensaver swipe vs tap.** A swipe sends only `swipeX`, not `bExit`. Decide whether swipes unlock.
11. **Tasmota → ESPurna UART pins.** BERRY8 opens `serial(17, 16, 115200)`, i.e. RX=17, TX=16, which matches the assumed wiring. On ESP32 Arduino, `Serial2.begin(115200, SERIAL_8N1, 17, 16)`.
12. **Model string.** EU builds send `eu`. If the panel was ever flashed with a US build it would report `us-l` or `us-p` with a different layout. Check in step 1.
