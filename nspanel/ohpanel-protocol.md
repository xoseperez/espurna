# OHpanel TFT driven by ESPurna: protocol and porting spec

Target: Sonoff NSPanel gen1 EU (ESP32). ESPurna module `code/espurna/nspanel.cpp` (Arduino-ESP32 2.0.9). Display on UART2, ESP TX=GPIO16 to TFT RX, ESP RX=GPIO17 from TFT TX. Display power is GPIO4, LOW = on.

Goal: replace the joBr99 Lovelace TFT with the OHpanel display design (https://github.com/bruestel/ohpanel, Apache-2.0) and drive it from our module, not from OHpanel's ESP-IDF firmware. We keep our JSON model: pages of tiles bound to MQTT topics, groups and scenes, home with clock, weather and indoor temperature, hardware buttons.

Research date: 2026-10-25. Every fact here comes from the source at the pinned commits below. **Nothing was checked on the device.** Section 8 lists what to verify there.

## 0. Pinned sources

| Alias | What | Commit |
|---|---|---|
| **R092** | tag `v0.9.2` (latest stable release, 2026-10-23). Code identical to `main` HEAD `45b783e` (HEAD only changes `docs/releases/0.9.2.md`) | `ba9361982399414d1612aea85504d9707b329e75` |
| **RC1** | tag `v0.10.0-rc1` (prerelease, 2026-10-24, branch `us-variant`) | `9e28cd5c0a1ade6c88354c0348d61fbb3a7e36f9` |
| **R090** | tag `v0.9.0` (first release) | `e65101376624a697f7a0511e99552e67ec3f0eda` |

References are given as `file:line` and resolve against `https://github.com/bruestel/ohpanel/blob/<commit>/<file>#L<line>`. Without an alias they mean R092. For example, `components/nextion/src/Protocol.cpp:30` is
https://github.com/bruestel/ohpanel/blob/ba9361982399414d1612aea85504d9707b329e75/components/nextion/src/Protocol.cpp#L30

Where to read in OHpanel:

| File | What it is |
|---|---|
| `boards/nspanel/nextion/ohpanel_hmi.py` | **builds the whole TFT design from code**: every page, every object name, all Nextion event code, Program.s. This is the authoritative TFT reference |
| `boards/nspanel/nextion/layout.py` | coordinates (480x320, rotated 270°; visible width 450 px, the frame hides x>450) |
| `boards/nspanel/nextion/display-manifest.json` | generated with the TFT: page ids, picture ids per background, objects per page, text field limits (`fit`), font widths, `design` version |
| `boards/nspanel/nextion/icons.json` | 7461 MDI 7.4.47 icon names mapped to UTF-8 glyphs of the TFT icon fonts |
| `boards/nspanel/nextion/weather-icons.json` | weather code sets mapped to the Meteocons picture names |
| `components/nextion/` | UART driver, frame parser, TFT upload |
| `components/app_display/src/Display.cpp` | link state machine: find the display, identify it, detect a reboot |
| `components/app_display/src/Engine*.cpp` | the renderer and navigation (what we replace with our own code) |

---

## 1. Which TFT to pin; boot and handshake

### 1.1 Releases and the "design rebuilt in rc1" question

| Release | EU display file | Size | Notes |
|---|---|---|---|
| v0.9.0, v0.9.1, v0.9.2 | `ohpanel-nspanel-eu-display-bc629e1e.tft` | 15 821 480 B | the same design in all three (`docs/releases/0.9.2.md`: "The display design (`…bc629e1e.tft`) is unchanged") |
| v0.10.0-rc1 | `ohpanel-nspanel-eu-display-4508cd3f.tft` | 15 821 480 B | **the same EU design.** RC1 `docs/releases/0.10.0-rc1.md`: "the EU's version changes all the same (`bc629e1e` -> `4508cd3f`) with the design unchanged". The rebuild in RC1 adds the **US** variant (`…-us-display-fc4305ba.tft`, 320x480 portrait) and splits `layout.py` into `layout_eu.py` / `layout_us.py`. |

I compared the two EU files byte by byte (`cmp -l`). They differ in only 8 bytes: offsets 14680133..14680136 (the `design_ver` integer of Program.s) and the last 4 bytes (file checksum). Pages, objects and pictures are identical.

The "rebuild from Sonoff's design" was in **0.9.1**, and it was firmware only: the upload fixes for recmod/0x24 (`docs/releases/0.9.1.md`). The TFT stayed `bc629e1e`.

**Pin: `bc629e1e` from R092.**
- It is in three stable releases, and the object names in this document are taken from it.
- RC1's `4508cd3f` behaves the same. If we ever take it, only the expected `design_ver` changes (§1.3).

### 1.2 Download

GitHub release assets contain only zips (`ohpanel-0.9.2.zip`, 7 523 630 B, holds the factory bin, the OTA bin, the TFT and licences). The bare TFT is a normal git blob (not LFS), so our HTTP(S) flasher can stream it from raw.githubusercontent.com (Range requests supported):

```
https://raw.githubusercontent.com/bruestel/ohpanel/ba9361982399414d1612aea85504d9707b329e75/boards/nspanel/nextion/display/ohpanel-nspanel-eu-display-bc629e1e.tft
```
- size 15 821 480 bytes (GitHub contents API, `ref=v0.9.2`)
- git blob `0e238450618dd7d71b20fc29e918b58342c185a4`
- sha256 `bdd5d022ac925cd2f42da392dd175acae3e808e888461a01f0c415435e7b5ec3`

The file is EU landscape only: 480x320, `DIRECTION = 270` (`ohpanel_hmi.py:109`), display model `NX4832F035_011R`, which needs 16 MB flash. The tag URL `…/v0.9.2/…` also works, but the commit URL is immutable.

RC1 alternative (same design):
```
https://raw.githubusercontent.com/bruestel/ohpanel/9e28cd5c0a1ade6c88354c0348d61fbb3a7e36f9/boards/nspanel/nextion/display/ohpanel-nspanel-eu-display-4508cd3f.tft
```

How the TFT is built and versioned:
- `boards/nspanel/nextion/build.sh` runs `ohpanel_hmi.py` on an empty seed HMI, then compiles it with Nextion Editor 1.68.1 under wine.
- The result is committed to `boards/nspanel/nextion/display/` (the old file is removed) together with the manifest (`docs/development.md:64-81`).
- Version = first 8 hex digits of a sha256 over the inputs (`ohpanel_hmi.py:1304-1316`; RC1 changed it to hash the design itself).
- The version is in the file name, in the manifest (`"design": "bc629e1e"`, `display-manifest.json:16033`) and in Program.s.

### 1.3 Reading the version from the display

Program.s (`ohpanel_hmi.py:1134-1169`) declares `int design_ver=<int32>`:
- the ESP sends `get design_ver` FF FF FF;
- the TFT answers `71 <int32 LE> FF FF FF`.

| design | int32 | answer bytes |
|---|---|---|
| bc629e1e | -1134387682 | `71 1E 9E 62 BC FF FF FF` |
| 4508cd3f (RC1) | 1158204735 | `71 3F CD 08 45 FF FF FF` |

If a design has no such variable (Lovelace TFT, Sonoff's), the answer is error `1A FF FF FF` (invalid variable). OHpanel formats the answer as `%08lx` (`components/app_display/src/Display.cpp:430-436`).

`connect` FF FF FF returns the hardware identity:
- format: `comok 1,30614-0,NX4832F035_011R,163,61488,D264B8204F0E1828,16777216` FF FF FF
- fields: touch, reserved, model, firmware, MCU code, serial, flash bytes (`components/nextion/include/nextion/Protocol.h:51-61`).
- **Quirk:** on the device the display swallows the command right after `connect`. OHpanel sends `get design_ver`, then `connect`, then a throw-away `sendme` (`components/app_display/src/Display.cpp:95-110`).

### 1.4 Power-up: what the TFT does by itself

Program.s, verbatim (`ohpanel_hmi.py:1156-1169`):
```
int sys0=0,sys1=0,sys2=0
int swipex=0,swipey=0
int th_accent=19998,th_text=65535,th_dim=52857     // aurora theme (RGB565)
int design_ver=-1134387682
int bg_pic=48,bg_close=45                          // aurora background + its close icon
baud=921600
recmod=0
printh 00 00 00 ff ff ff 88 ff ff ff
lcd_dev fffb 0002 0000 0020
page blank
```

So after power-on the display:
- runs at **921600 baud 8N1 from its first byte**. `baud=`, not `bauds=`: nothing is stored, and there is no rate switch at runtime;
- is in **command mode** (`recmod=0`). No reparse mode and no unlock string are needed;
- sends `00 00 00 FF FF FF` (startup) and `88 FF FF FF` (ready). That is two frames, see §2.2;
- loads page `blank` (black). Its preinitialize code sends the `currentpage` event (§4): `92 "nspanelevent" 00 {"page": "blank", "component": "currentpage", "value": "pagechange"} 00 FF FF FF`.

**No handshake is required to start.** The TFT shows black until the ESP sends commands. There is no keepalive and no heartbeat in the TFT. The ESP owns all state: theme, page, brightness, navigation.

### 1.5 What the ESP must do at boot (OHpanel's link state machine, condensed)

Source: `components/app_display/src/Display.cpp:150-334`.

1. **UART2 at 921600** (`components/nextion/include/nextion/Display.h:34-39`; `src/main.cpp:262-267`).
2. **Search.** Send `FF FF FF sendme FF FF FF`. The leading FF FF FF ends any half command (`components/nextion/src/Display.cpp:179-185`).
   - Answer: `66 <page id> FF FF FF`. Right after boot this is `66 04 …` (blank = 4).
   - No answer at 921600: try 115200, then 9600 (an older or foreign design, e.g. Lovelace at 115200).
   - Still none: send the reparse-mode exit string and repeat the search (`components/nextion/src/Display.cpp:41-53, 110-133`):
     ```
     FF FF FF "DRAKJHSUYDGBNCJHGJKSHBDN" FF FF FF "recmod=0" FF FF FF "recmod=0" FF FF FF
     ```
   - Repeat every 2 s. After 20 s without an answer, power-cycle GPIO4 once: HIGH 800 ms, then LOW, then wait 1500 ms (`src/main.cpp:274-279`; `Display.cpp:191-201, 276-296`).
3. **Identify.** `get design_ver`, `connect`, `sendme`. Retry every 1 s for up to 3 s, then draw anyway (`Display.cpp:298-306`). If the version is not `bc629e1e`, log "wrong TFT, flash it".
4. **Theme globals** (`components/app_display/src/Engine.cpp:290-316`):
   `th_accent=<565>` `th_text=<565>` `th_dim=<565>` `bg_pic=<id>` `bg_close=<id>`. Pages copy these globals in their preinitialize code (§3.2). They must be sent **before** the first `page` command and again after every TFT reboot (variables reset to the Program.s defaults).
5. **First page.** `page 0` (home), or `page 3` (boot page) while Wi-Fi/MQTT is not up yet (`Engine.cpp:1096-1104, 381-393`).
6. **While up** (`Display.cpp:307-331`):
   - no frame from the TFT for 10 s: ping (`sendme`). Two pings unanswered: back to search;
   - 3 flow-control syncs in a row unanswered: back to search;
   - `00 00 00` or `88` arrives (TFT rebooted, e.g. brownout): wait until it has been quiet for 500 ms (at most 5 s), then search again. Theme and page are sent again.

### 1.6 Flashing the TFT with our existing flasher (`nspanel.cpp` `flash::`)

OHpanel's uploader:
- `whmi-wri <size>,<baud>,res0` then 4096-byte blocks, each acknowledged by `0x05`; only `0x05` counts (`components/nextion/src/Display.cpp:228-321`);
- before every upload it power-cycles the display (`components/app_display/src/Display.cpp:553-571`);
- upload baud is 921600 by default, `?baud=115200` as fallback (`components/app_web/src/WebDisplay.cpp:53-54`);
- the docs say it takes about 15 min over Wi-Fi (`docs/install.md:120-123`).

Our flasher uses v1.2 `whmi-wris` with offset resume. That is fine, because the display OS handles the upload, not the TFT. It needs three patches:
- `handshake()` tries only `{115200, 9600}`. **Add 921600 first**, otherwise a second flash (back to Lovelace, or a newer OHpanel) fails once OHpanel runs.
- `resume()` sends `recmod=1`. **Remove** it: it was for Lovelace and breaks OHpanel's command mode.
- `build::Baudrate` (115200) is used for the runtime UART and `powerCycle()`. Split it into `RuntimeBaud = 921600` and the flasher's rate list.

The first flash from the current Lovelace TFT runs at 115200, which is proven. Passing `921600` as upload rate is 4x faster on the UART, but Wi-Fi/TLS will be the bottleneck.

---

## 2. Serial framing

### 2.1 ESP to TFT

Plain Nextion instructions, ASCII/UTF-8, each terminated by `FF FF FF`. Nothing else: no length, no CRC, no custom frame.
- Text: `obj.txt="..."`. Inside the quotes escape `"` as `\"` and `\` as `\\`; a line break is `\r` (`components/nextion/src/Protocol.cpp:131-144`).
- Numbers: `obj.pic=N`, `obj.pco=N`, `obj.bco=N`, `obj.val=N`.
- Commands: `vis obj,0|1`, `vis 255,0|1` (all objects), `page N`, `dim=N`, `ref_stop`, `ref_star`, `get x`, `sendme`.
- Globals: `th_accent=N` etc.

**Flow control (needed).** The display reads into a ~1 KB buffer and stops reading while it draws a page. Overflow is lost and reported as `24 FF FF FF`. OHpanel's rule (`components/nextion/include/nextion/Display.h:7-13`, `components/nextion/src/Display.cpp:135-174`):
- after `page N`, `ref 0`, `ref_star` and `doevents`, and after every 512 bytes sent, send `sendme` and block until `66 xx FF FF FF` arrives (timeout 500 ms);
- before `ref_stop`, sync too, because pausing the refresh while the display still draws cuts the drawing off;
- 3 timeouts in a row: stop waiting (link lost) until the display answers again.

Our current 75 ms `TxGapMs` pacing and the 55 BB framing go away. The command queue stays, but it is drained with the sync rule above.

**Do not resend unchanged values.** Every assignment redraws the object, even with the same value, and that flickers. OHpanel caches `obj.attr → last value` per page load and clears the cache on every `currentpage` (`Engine.cpp:454-494, 1010`). The first assignment after a page load is always sent.

**Text byte limits.** Each text field has `txt_maxl` bytes. The display **cuts silently, even in the middle of a UTF-8 sequence** (`ohpanel_hmi.py:145-151`). Truncate UTF-8-safely to `txt_maxl` (table in §3) and to the pixel width (`fit` in the manifest: `[font, width_px, maxl]`). Cyrillic is 2 bytes per letter.

### 2.2 TFT to ESP

Every message ends with `FF FF FF`. Parser (`components/nextion/src/Protocol.cpp:30-108`):
- append bytes to a buffer;
- when the last 3 bytes are `FF FF FF`, strip them and dispatch the frame;
- a buffer over 1024 bytes without a terminator is dropped (noise, or a display speaking another protocol);
- payloads never contain `FF FF FF`. JSON is ASCII, and the 0x91 value is an int32, so a value with FF bytes is theoretically possible. OHpanel accepts that risk, and the only 0x91 values we would see are slider/touch numbers.

| First byte | Length (without FF FF FF) | Meaning |
|---|---|---|
| `00 00 00` | 3 | display started (Program.s `printh`) |
| `88` | 1 | ready |
| `66 pp` | 2 | answer to `sendme`: page id now shown (flow-control ack / ping) |
| `71 b0 b1 b2 b3` | 5 | answer to `get <number>` (int32 LE); used for `design_ver` |
| `70 …` | n | answer to `get <text>` (not used) |
| `"comok …"` | n | answer to `connect` |
| `65 pp cc ee` | 4 | standard Nextion touch (not used by this design; harmless) |
| `67/68 xH xL yH yL ee` | 6 | touch coordinates, only with `sendxy=1` (list pages §12.2, drawn tiles §14.3) |
| **`92 <name> 00 <JSON> 00`** | n | **design event**, name is always `nspanelevent` |
| **`91 <tag> 00 <int32 LE>`** | n | **design value report** (sliders and drags; not needed for our pages) |
| single byte | 1 | Nextion status/error: `1A` invalid variable, `24` buffer overflow, `02` invalid component, `1B` invalid operation, `1C` assignment failed, `1E` wrong parameter count |

Event frame, built by `event()` in `ohpanel_hmi.py:116-122`:
```
printh 92 / prints "nspanelevent",0 / printh 00 / prints "<json>",0 / printh 00 / printh FF FF FF
```
The JSON is always exactly `{"page": "<page>", "component": "<component>", "value": "<value>"}`, with a space after `:` and after `,`, and all values are strings. Example, bytes for a swipe left on the button page (76 bytes):
```
92 6E 73 70 61 6E 65 6C 65 76 65 6E 74 00                 0x92 "nspanelevent" 00
7B 22 70 61 67 65 22 3A 20 22 62 75 74 74 6F 6E 73 22 2C  {"page": "buttons",
20 22 63 6F 6D 70 6F 6E 65 6E 74 22 3A 20 22 73 77 69 70   "component": "swip
65 22 2C 20 22 76 61 6C 75 65 22 3A 20 22 6C 65 66 74 22  e", "value": "left"
7D 00 FF FF FF                                            } 00 FF FF FF
```
Parser rule (`Protocol.cpp:74-83`): name = bytes 1 up to the first `00`; JSON = the rest, minus a trailing `00`. A strstr-based parse is enough: the JSON has a fixed field order, but use ArduinoJson anyway.

Value frame, built by `report()` in `ohpanel_hmi.py:125-128`: `printh 91 / prints "<tag>",0 / printh 00 / prints <obj>.val,0 / printh FF FF FF`. The tag is 4 ASCII characters and the value is a 4-byte LE int32.

---

## 3. Pages and objects

Page ids, from `display-manifest.json:56-…`. Page order is set at `ohpanel_hmi.py:1329-1330`.

| id | page | swipe detect | touch event on background | used by us |
|---|---|---|---|---|
| 0 | `home` | yes | no | **yes**: home / screensaver |
| 1 | `weather` | yes | yes | **yes**: forecast days |
| 2 | `settings` | yes | no | maybe (brightness sliders, restart) |
| 3 | `boot` | no | no | optional (status while Wi-Fi/MQTT starts) |
| 4 | `blank` | no | wake only | **yes**: screen off |
| 5 | `light` | yes | no | later |
| 6 | `cover` | yes | yes | later |
| 7 | `buttons` | yes | yes | **yes**: 8-tile pages |
| 8 | `volpage` | no | no | no |
| 9 | `qr` | yes | yes | optional (Wi-Fi QR) |
| 10 | `entities` | yes | yes | optional (sensor list) |
| 11 | `entities_cards` | yes | yes | optional |
| 12 | `entities_rows` | yes | yes | optional |
| 13 | `media` | no | yes | no |
| 14 | `system` | yes | yes | optional (info page) |
| 15 | `netpage` | no | yes | no |
| 16 | `covers` | yes | yes | later |
| 17 | `thermostat` | yes | yes | later |

Every page also has `busy1..3` and `busy_tm`, except boot, blank, volpage and netpage (§3.3). Pages with swipe detection have `swipe` (TouchCap) and `swipe_tm`. Most pages have `title`, `close_hs` and `close`.

### 3.1 Fonts and icons

`boards/nspanel/nextion/fonts.py:77-88`:

| font id | what | height | digit width | used for |
|---|---|---|---|---|
| 0 | Ubuntu | 16 | 8 | tile state line, small labels |
| 1 | Ubuntu | 18 | 10 | tile name, info lines, relay labels |
| 2 | Ubuntu | 22 | 11 | today low/high, entity rows |
| 3 | Ubuntu | 26 | 13 | titles, date, value rows |
| 4 | Ubuntu | 36 | 19 | weather high (with condition) |
| 5 | Ubuntu (+13 media icons) | 48 | 25 | outdoor temp, weather high |
| 6 | Ubuntu (+13 media icons) | 72 | 37 | clock |
| 7 | MDI icons + Latin-1 (Noto) | 24 | - | status icons, dots, value-row icons |
| 8 | MDI icons + Latin-1 | 32 | - | tile icon (corner layout), shortcuts |
| 9 | MDI icons only | 48 | - | tile icon (center layout) |

Text fonts 0..6 contain every BMP character of Ubuntu Regular, so **Cyrillic is covered**. I checked the manifest `fonts` widths: `Д`, `д`, `Ж` exist in every text font, and Bulgarian is one of the 25 UI languages. Russian is not an OHpanel UI language, but our firmware writes its own texts, so that does not matter.

**Icon codepoints are identical to the Lovelace TFT:** display codepoint = MDI codepoint - 0xF0001 + 0xE000 (`fonts.py:53`, `MDI_SHIFT`). I checked all 92 entries of our `nspanel_icons.h` against OHpanel `icons.json`: 92/92 identical. So `resolveIcon()` and our raw-hex syntax (`"E334"`) work unchanged.
- The TFT holds **all 7461 MDI 7.4.47 icons** in fonts 7, 8 and 9. Any MDI name works once its codepoint is known. Generate a bigger table from `icons.json`, or let users give `"U+Fxxxx"` MDI codepoints and subtract `0xF0001 - 0xE000`.
- OHpanel's own icons follow at U+FF00 + n: `ohpanel-equalizer-1..8`, `ohpanel-equalizer-retro-1..8`, `ohpanel-speaker-wave-1..3`, `ohpanel-openhab` (`fonts.py:16-17`, `own_icons.py:173-174`).
- Useful glyphs (UTF-8): `circle-medium` `EE A7 9D` (U+E9DD, page dot "current"), `circle-small` `EE A7 9E` (U+E9DE, page dot "other"), `arrow-down-thin` `EF A6 B2`, `arrow-up-thin` `EF A6 B1`, `lightbulb` `EE 8C B4` (U+E334), `lightbulb-group` `EF 89 92` (U+F252), `home-thermometer-outline` `EE BD 94` (U+EF54), `wifi-off` (look it up in `icons.json`).

### 3.2 Themes and backgrounds

There are 5 backgrounds; each defines a theme (`display-manifest.json:3-55`). A background is a full-screen picture. Because the display has no transparency, every icon or tile picture that sits on it exists **once per background** (`ohpanel_hmi.py:37-40`). Selecting a theme means:
1. send `th_accent=… th_text=… th_dim=… bg_pic=<bg> bg_close=<close>` (globals in Program.s);
2. reload the page (`page N`). Each page's preinitialize code copies `bg_pic` into `<page>.pic` and into `picc` of every cropped text field, and `bg_close` into `close.pic` (`ohpanel_hmi.py:1108-1121`). It also colours the fixed parts from `th_*` (`ohpanel_hmi.py:1001-1027`);
3. use that background's picture ids for tiles and weather icons (table below).

| background | `bg_pic` | `bg_close` | boot logo | accent (565) | text | dim | tile slot N off/on (N=1..8) | weather chip box 1..6 | `weather_home` ids | `weather_page` ids |
|---|---|---|---|---|---|---|---|---|---|---|
| aurora (default) | 48 | 45 | 47 | #4fc3f7 = 19998 | 65535 | #c8ccc8 = 52857 | 21/22, 23/24, … 35/36 (off = 19+2N, on = 20+2N) | 67..72 | 73..108 | 109..144 |
| black | 162 | 160 | 161 | #0080f8 = 1055 | 65535 | 52857 | 157/158 for all 8 | 169 (all) | 170..205 | 206..241 |
| candy | 291 | 288 | 290 | #ff5fa8 | 65535 | #d0c4e0 | 264/265 … 278/279 | 310..315 | 316..351 | 352..387 |
| dunes | 437 | 434 | 436 | #ffb74d | #fff8ee | #d8c8b0 | 410/411 … 424/425 | 456..461 | 462..497 | 498..533 |
| forest | 584 | 580 | 583 | #8bd17c | 65535 | #c0d0c0 | 556/557 … 570/571 | 603..608 | 609..644 | 645..680 |

Weather picture id = range start + index of the name in this alphabetical list of 36 names (the same order in every background):
`clear-day, clear-night, cloudy, drizzle, dust, dust-wind, fog, fog-day, fog-night, hail, haze, hurricane, mist, not-available, overcast, overcast-day, overcast-night, partly-cloudy-day, partly-cloudy-day-drizzle, partly-cloudy-day-rain, partly-cloudy-day-sleet, partly-cloudy-day-snow, partly-cloudy-night, partly-cloudy-night-rain, partly-cloudy-night-snow, rain, sleet, smoke, snow, thunderstorms, thunderstorms-day, thunderstorms-day-rain, thunderstorms-night-rain, thunderstorms-rain, tornado, wind`
Example: aurora `partly-cloudy-day` gives home 90 and page 126.

RGB565 is `(r>>3)<<11 | (g>>2)<<5 | b>>3`. Fixed colours the renderer uses (`EngineDetail.h:18-22`, `EngineLists.cpp:37-38`): `kWhite 65535`, `kDarkGrey 10597` (#2c2c2c, the off tile), `kLightGrey 52857`, tile state line on 27469 / off 40147, bar track on 57051 / off 19017, red 63488.

Recommendation: pick **one** background in our JSON (`display.background`, default `aurora`) and embed the table above as a constexpr array. We do not need the manifest at runtime.

### 3.3 Common objects and the page-load contract

On every page load the TFT runs the page's preinitialize code:
- theme/background copy (§3.2);
- busy indicator arm: `vis busy1..3,0`, `busy_tm.tim=800`, `busy_tm.en=1` (`ohpanel_hmi.py:1043-1073`);
- `vis 255,0` (hide everything) on home, weather, buttons, entities*, cover(s), thermostat, light, media and system;
- the `currentpage` event.

The ESP must then:
1. **within 800 ms** send `busy_tm.en=0` and `vis busy1,0` / `vis busy2,0` / `vis busy3,0` (`Engine.cpp:1115-1117, 1066-1069`). Otherwise three animated dots appear in the middle of the screen ("panel not answering");
2. show what it uses. OHpanel sends `vis 255,1` on home and then hides what is not configured (`EngineHome.cpp:117-118`). On buttons it shows objects one by one (`EngineLists.cpp:119-141`). We should **show explicitly** (`vis <obj>,1`) only the objects we fill. Home has many overlapping variant fields (`outdoor_n1..7`, `low1..7`, …) that must stay hidden.
   - **Hotspots (`*_hs`) hidden by `vis 255,0` do not take touches.** Always `vis <obj>_hs,1` for every active tile or shortcut, and `vis swipe,1` (OHpanel does it explicitly on button pages, `EngineLists.cpp:121`);
3. wrap a burst in `ref_stop` … `ref_star` so the page appears in one go.

Common objects: `title` (font 3, 300 px, maxl 46), `close` (picture, `bg_close`) and `close_hs` (touch area 376,0,104x44). Pages without a close icon: home, boot, blank.

### 3.4 `home` (id 0)

Layout (`layout.py:64-149`, `ohpanel_hmi.py:342-452`). Visible area x<450, margin 12, right edge 438.

| Object(s) | Font | Box | maxl B | Content |
|---|---|---|---|---|
| `clock` | 6 (72 px) | 278,45 160x62, right-aligned | 5 | `"14:07"` |
| `ampm` | 2 | above minutes | 10 | only with a 12 h clock |
| `date` | 3 | 273,110 165x30, right-aligned | 26 | date text (the page colours it `th_dim`) |
| `weather_pic` | picture | 12,36 116x116 | - | `weather_home` id of the condition (§3.2) |
| `weather_hs` | hotspot over the icon | | | event `weather` press/release |
| `outdoor` | 5 (48 px) | 142,82 130x48 | 8 | outdoor temperature, e.g. `"12°"` (wide field) |
| `outdoor_n1..7` / `outdoor_u1..7` | 5 / 2 | number k*13 px wide + unit after it | 8 | number + small unit variant; skip, use `outdoor` |
| `low_arrow` | 7 | 138,130 | (fixed glyph ↓) | show with the low |
| `low1..7` | 2 | x=162, width k*6 | 25 | today's low; k = ceil(width_px/6), clamp 1..7 |
| `high_arrow0..7`, `high0..7` | 7 / 2 | x = 162+k*6+4 (k=0: at the low's place) | 25 | today's high after the low (`Engine.cpp:637-670`) |
| `row1_icon`, `row2_icon` | 7 | 10,162 / 10,190 (25x25) | 10 | value-row icons (`th_dim`) |
| `row1`, `row2` | 3 | 37,160 / 37,188 100x28 | 8 | value text (wide field), e.g. indoor `"23,4°"` |
| `row1_n1..12` / `row1_u1..12` (and row2) | 3 / 1 | number + unit | 8 | variant; skip |
| `info1_*`, `info2_*` (`_tag`, `_text`, `_right`, `_tagicon`, `_badge`, `_badgeico`) | 1 / 7 | y=230 / 251 | 5 / 31 / 5 | two text lines (free use: humidity, MQTT text) |
| `top1..top10` | 7 | status icons, 12+29k, y=5 | 10 | status row (Wi-Fi/MQTT warnings, alarms) |
| `top_value`, `top_icon` | 3 / 8 | top right | 16 / 10 | a value + icon in the status row |
| `wifi` | 7 | 414,5 | 5 | Wi-Fi-off icon, red 63488; empty when connected (`Engine.cpp:366-371`) |
| `shortcut1..7_icon` | 8 (32 px) | 35x35: 1..3 at y=176, x=310/357/404; 4..7 at y=224, x=263/310/357/404 | 10 | icon only, no label; pco = colour |
| `shortcut1..7_hs` | hotspot | same boxes | | event `shortcutN` **release only** |
| `relay1_text`, `relay2_text` | 1 | 47,283 / 288,283 120x20 | 26 | labels of the 2 hardware relays |
| `relay1_bar`, `relay2_bar` | solid | under the labels | | `bco` = accent when on, 10597 when off (`EngineHome.cpp:97-113`) |

Home has no `title`, `close` or `dots`.

### 3.5 `buttons` (id 7): 8 tiles

`ohpanel_hmi.py:746-777`, renderer `EngineLists.cpp:41-142`. Grid of 2 rows x 4 columns of 100x100 tiles:
- tile picture x = 10, 120, 230, 340; y = 57, 167 (`layout.py:21-48`);
- tile `i` (1..8) is numbered row by row.

| Object | Font | maxl B (px) | Use |
|---|---|---|---|
| `tileN_pic` | picture | | background slot `buttonN` off/on id (§3.2) |
| `tileN_icon` | 8 | 10 | icon, corner layout (top left) |
| `tileN_value` | 0 | 21 (82 px) | state line, e.g. `"Вкл"` / `"Выкл"` / `"45 %"` |
| `tileN_name` | 1 | **18 B (82 px)** | label |
| `tileN_bar` | progress bar | | `val` 0..100, `pco`, `bco` (percent values) |
| `tileN_center` | 9 (48 px) | 4 | icon, center layout |
| `tileN_text` | 0 | 20 (80 px) | label under the centred icon |
| `tileN_hs` | hotspot 100x100 | | events `tileN` **press** and **release** |
| `title` | 3 | 46 | page title |
| `dots` | 7 | 30 | page indicator |
| `close`, `close_hs` | | | close cross |

On load the page runs `vis 255,0`, `vis close_hs,1`, `vis tileN_center,0`.

Tile drawing (`EngineLists.cpp:65-111`):
- active (on): `tileN_pic.pic=<on id>`; `bco=65535` on icon/name/value; icon `pco=accent` (or the item colour); name `pco=10597`; value `pco=27469`;
- inactive: `pic=<off id>`; `bco=10597`; icon `pco=dim`; name `pco=52857`; value `pco=40147`;
- the text fields are solid (`bco`), so their bco must match the tile picture's flat colour;
- **order matters:** set the picture first, then bco/pco, then txt. A new picture paints over the text fields; colour assignments redraw them;
- show last: `vis tileN_hs,1`, `vis tileN_pic,1`, `vis tileN_icon,1`, `vis tileN_name,1`, `vis tileN_value,1` (corner layout), or `_hs/_pic/_center/_text` (center layout). Inside `ref_stop`/`ref_star` all tiles appear at once;
- unused tile: leave hidden. A hidden `_hs` takes no touch.

**Page dots** (`Engine.cpp:1239-1258`): `dots.txt` = one glyph per page of the swipe row (home first), `circle-medium` for the current page and `circle-small` for the others, separated by spaces. Example for 3 pages, second current: `dots.txt="\xEE\xA7\x9E \xEE\xA7\x9D \xEE\xA7\x9E"`. OHpanel shows dots only when the page is in the swipe row and the row has more than one page.

**Text width:** the name field is 82 px in font 1. A Cyrillic letter is 10..15 px, so about **6..8 Cyrillic letters**, and the byte limit 18 B allows 9 Cyrillic letters. OHpanel shortens to width with "..." using the manifest font widths (`Engine.cpp:45-104`). We need a small width table for font 1, 2 and 3 (ASCII, U+0400..U+045F, `°`). Extract it from `display-manifest.json` `fonts` (`first`, `widths`, `ranges`) at build time.

### 3.6 `weather` (id 1): one sheet per day

`ohpanel_hmi.py:455-490`, renderer `EngineInfo.cpp:37-150`. Swiping left/right changes the sheet; the ESP reloads the page (`page 1`) for each sheet (`Engine.cpp:891-911`).

| Object | Font | maxl B (px) | Use |
|---|---|---|---|
| `title` | 3 | 46 | `"Сегодня · 25 окт"` etc. |
| `weather_pic` | picture 16,46 100x100 | | `weather_page` id; set `pic`, then `vis …,1` |
| `high` | 5 | 25 (300) | high, large, e.g. `"15°"` |
| `low_arrow` + `low` | 7 / 3 | 42 (270) | low under the high |
| `high_line`, `low_arrow1..10`, `low1..10`, `condition` | 4 / 7 / 3 / 3 | 17 / 17 / 48 | variant with the condition in words (high smaller, low after it at place k, condition below) |
| `chip1..6_pic` / `_icon` / `_value` / `_text` | pic / 7 / 2 / 0 | 17 / 23 | 6 cards (3x2 at y=166 and 224): icon, value, label (wind, rain, UV …); `chipN_pic.pic` = `wchipN` box id |
| `dots` | 7 | 30 | one glyph per day |
| `close`, `close_hs` | | | |

On load the page runs `vis 255,0`, then shows `title`, `close_hs` and `close`.

### 3.7 `blank` (id 4) and `boot` (id 3)

- `blank`: black, no objects. A tap on the page sends `{"page": "blank", "component": "wake", "value": "release"}` (`ohpanel_hmi.py:846-851`).
- `boot`: `status` (font 4, 30 B), rows `firmware_*`, `device_*`, `ip_*`, `ipv6_*`, `info_*` (`_text` 18 B, `_value` 32 B; `info_value` 128 B), `qr_area` (hotspot marking where to draw a QR code with `fill`), `logo_pic` (set `pic` = boot logo id of the background, then `vis logo_pic,1`) (`ohpanel_hmi.py:810-843`, `EngineHome.cpp:24-64`). Good for "ESPurna 1.x / IP / MQTT connecting…".

### 3.8 Other pages (not needed for the first port)

Summary for later. Events are listed in §4.
- `settings`: `set1..3_text`, `_value`, `_slider` (values `set0..set2`), button `restart`.
- `light`: `icon`, `value`, `slider`, `temp_slider`, `wheel_pic`, `side1..3`, `power`.
- `cover` / `covers`: drawn by the ESP with `fill`/`cirs`.
- `thermostat`: ring drawn by the ESP.
- `media` / `volpage`, `entities` / `entities_cards` / `entities_rows` (7 or 8 rows per sheet), `system` (8 cards), `netpage`, `qr` (QR drawn by the ESP with `fill` into `qr_area` 127,40 196x196).

Several of these pages need the ESP to draw shapes (`fill`, `cirs`, `xstr`, `picq`, see `Engine.cpp:408-452`). That is more work than populating fields.

---

## 4. Events from the TFT and OHpanel's response

All events are `92 nspanelevent` JSON (§2.2). Complete list, from `ohpanel_hmi.py`:

| page | component | value | Source |
|---|---|---|---|
| any | `currentpage` | `pagechange` | page loaded (`loaded()`, `:329-331`) |
| pages with swipe detection | `swipe` | `left` / `right` / `up` / `down` | `:290-305` |
| pages with `touch=True` (weather, buttons, entities*, cover(s), thermostat, qr, media, system, netpage) | `touch` | `press` / `release` | page background touched (`:269-283`) |
| pages with close | `close` | `release` | `close_hs` or the `close` picture (`:313-320`, light `:949,987`) |
| home | `weather` | `press`, `release` | `:446-447` |
| home | `shortcut1..7` | `release` | `:448-450` |
| buttons | `tile1..8` | `press`, `release` | `:773-774` |
| blank | `wake` | `release` | `:850` |
| system / netpage | `card7` / `back` | `release` | `:603, 617` |
| settings | `restart` | `release` | `:801-807` |
| cover | `open_cover`, `stop_cover`, `close_cover`, `preset1..3` | `release` | `:651-662` |
| covers | `all`, `rowN`, `<n>_up/_stop/_down`, `preset1..3` | `release` | `:703-710` |
| thermostat | `minus`, `plus`, `mode1..4` | `release` | `:733-740` |
| light | `side1..3`, `power` | `release` | `:974-986` |
| media / volpage | `previous`, `play`, `next`, `volume`, `favorites` / `minus`, `plus`, `mute` | `release` | `:881-889, 908-912` |

Value reports (`91`): `set0..set2` (settings), `csls`/`cslu`, `ctps`/`ctpu`, `cwhl` (light), `cdrw`/`cdru` (cover drag), `tdrw`/`tdru` (thermostat ring), `vols`/`volu` (volume).
- slider tags with an `s` suffix report while moving, `u` on release (`:212-222`);
- drag values are `x*1000+y`.

### 4.1 Swipe detection (in the TFT)

`ohpanel_hmi.py:286-305`:
- `swipe` is a TouchCap over the page. On press: `swipex=tch0`, `swipey=tch1`, `swipe_tm.en=1`.
- `swipe_tm` runs every 50 ms while the finger is down and updates `swipex`, `swipey`.
- On release: `sys0=swipex-tch2`, `sys1=swipey-tch3`; `tch2` and `tch3` are where the finger came down.
- The first matching test wins: `sys0>100` → `right`; `sys0<-100` → `left`; `sys1>100` → `down`; `sys1<-100` → `up`. The value is the direction **the finger moved** (finger moves left = "left").
- No event below a 100 px movement.

**Tap vs swipe on the ESP** (`Engine.cpp:1318-1341, 1687-1690`; constants `Engine.h:404, 411`). A swipe that starts on a tile also produces that tile's `release`, in either order:
- `swipe` → remember the time, cancel any pending release, act on the swipe;
- `release` within **300 ms** after a swipe → ignore it (end of the swipe);
- any other `release` → hold it **60 ms** (`release_wait_ms_`). If a swipe arrives meanwhile, drop the release; otherwise act on it;
- `press` → no action; OHpanel only pauses background drawing for up to 800 ms.

**Long press:** the TFT has no long-press event. Only tiles (and the weather icon) send `press`, so a long press = `release` minus `press` ≥ N ms, measured on the ESP. Shortcuts send release only, so no long press there.

### 4.2 Navigation model (all on the ESP; "no page opens another", `ohpanel_hmi.py:34-36`)

- **Swipe row** = `home` + the configured `buttons` pages, in order (`Engine.cpp:982-998`).
  - `left`: next page; `right`: previous page; both wrap round (`Engine.cpp:923-933`).
  - `up` / `down`: ignored except on favourites sheets.
  - On weather and entities pages, left/right move between sheets/days without wrapping (`Engine.cpp:891-911`).
  - During a cover or ring drag (<1 s), a swipe is ignored.
- **Optional fade** (`display.page_fade`): `dim` steps down in 4 steps, `page`, wait for `currentpage`, then step back up (`Engine.cpp:935-980`).
- **Open:** a tile or shortcut with `open` pushes the current page on a history stack and shows the target (`Engine.cpp:856-871`).
- **Close / back:** `close` release pops the history; empty history → home (`Engine.cpp:873-883, 1376-1380`).
- **Home timeout:** `home_after` s (default 60) without a touch on another page → home (`Engine.cpp:1710-1719`; defaults `Format.h:89-90`).
- **Display moved by itself:** if the TFT reports `currentpage` for a page the ESP did not request, the ESP takes it only when it is home, otherwise ignores it. Before start it forces the boot page (`Engine.cpp:1038-1053`).
- **Any event** other than `currentpage` counts as "touched": it resets the idle timer (`Engine.cpp:1264-1268`).

---

## 5. Brightness, dim, sleep and periodic work

The design uses **only `dim=N`** (0..100, not stored; `dims` is never used) and the black `blank` page. It uses no `sleep`, `thsp`, `thup` or `ussp` (grep finds none in R092), so the Nextion sleep and auto-wake machinery is unused.

`Engine::screen_tick()` (`Engine.cpp:1767-1801`; defaults `Format.h:65-66`: brightness 85, idle_brightness 60, idle_after 60 s):
- **In use:** `dim=<brightness>`.
- **Idle** (no touch for `idle_after` s): `dim=<idle_brightness>`.
- **Off** (idle with idle_brightness 0, or night window/switch and idle): `dim=0` + `page 4` (blank). `asleep=true`.
- **Wake:** a tap on blank sends `wake` and marks "touched". The next tick sets `asleep=false`, opens **home** (`page 0`) and restores brightness. That tap presses nothing (`act()` returns while asleep, `Engine.cpp:1344-1345`).
- **Hardware buttons** switch relays but do **not** wake a screen that is off (`Engine.cpp:1451-1455`; release 0.9.1).
- `set_level` sends `dim=` only when the value changes (`Engine.cpp:1747-1753`). After a TFT reboot the brightness is unknown: OHpanel sets `level_=-1` in `display_away()` so the next tick resends it.

What the ESP must do periodically:

| When | What |
|---|---|
| every new minute, home shown | `clock.txt="HH:MM"` (`Engine.cpp:1857-1862`, `EngineHome.cpp:66-95`); `date.txt` when it changes |
| every loop | idle/brightness tick, home timeout, pending-release timer (60 ms) |
| 10 s with no frame from the TFT | `sendme` ping; 2 misses → re-search (`Display.cpp:316-322`) |
| on `00 00 00` / `88` | TFT rebooted: theme globals + current page again |
| state change of a bound topic | redraw only that tile, shortcut or field (cache suppresses unchanged values) |

**No TFT-side keepalive is needed.** The TFT never times out, never sleeps and never navigates by itself.

---

## 6. Mapping our JSON model to OHpanel pages

### 6.1 Today vs OHpanel

| Our Lovelace feature | OHpanel equivalent |
|---|---|
| screensaver (time, Russian date, weather + 4 days, NTC temp) | `home`: clock, date, current icon + `outdoor` + today low/high, `row1` indoor; days go to the `weather` page |
| grid card, 8 items (`pages[].items`, chunked) | `buttons` page, 8 tiles; a page with more than 8 items is split into several sheets in the swipe row |
| navPrev/navNext/navHome | swipe left/right, close cross (→ home), home timeout |
| `screensaver.timeout/brightness/dim` | `idle_after`, `brightness`, `idle_brightness` (our `dim`). `dim: 0` = blank page |
| `colors.on/off/unknown` | tile on = white tile + accent icon; off = dark tile + dim icon; unknown: off tile with value `"–"` |
| status icon (Wi-Fi/MQTT) | `wifi` icon (red), or `top1..top10` for an MQTT-down icon |
| `beep` on tap | unchanged, ESP side |
| `buttons[]` (hardware) | unchanged; `relay1/2_text` + `_bar` on home can show labels and state |

### 6.2 Proposed JSON additions (backward compatible)

```jsonc
{
  "version": 1,
  "mqtt": { "on": "1", "off": "0", "toggle": "2", "set": "/set" },
  "display": {                                    // replaces "screensaver"; "screensaver" still read as fallback
    "background": "aurora",                       // aurora|black|candy|dunes|forest
    "theme": { "accent": "#4fc3f7" },             // optional overrides (RGB hex), else the background's theme
    "brightness": 100, "idle_brightness": 10, "idle_after": 60,   // = old brightness / dim / timeout
    "night": { "from": "23:00", "to": "06:00" },  // optional: blank page at night once idle
    "home_after": 60,                             // other page → home after s without touch, 0 = never
    "page_fade": 0,
    "weather": { "lat": 52.52, "lon": 13.40, "refresh": 900 }
  },
  "home": {
    "indoor": { "icon": "home-thermometer-outline" },       // NTC → row1 (row2 free: e.g. a topic's value)
    "weather_page": "forecast",                             // tap on the weather icon
    "shortcuts": [                                          // up to 7, icon only; same item syntax as tiles
      { "icon": "lightbulb-group-off", "type": "scene", "group": "house", "action": "off" },
      { "icon": "gate", "topic": "ESPURNA-GATE/relay/0", "type": "button" },
      { "icon": "weather-partly-cloudy", "open": "forecast" }
    ],
    "relays": [ { "label": "Свет" }, { "label": "Вентилятор" } ]   // labels over the 2 hardware buttons
  },
  "swipe": [ "outside", "house" ],                // optional; default: every grid page in file order
  "pages": [
    { "id": "outside", "title": "Улица", "type": "grid", "items": [ /* as today, + "layout","color","value","open" */ ] },
    { "id": "forecast", "title": "Погода", "type": "weather", "days": 4 }
  ],
  "groups": { /* unchanged */ }, "buttons": [ /* unchanged */ ]
}
```

New item fields (all optional):
- `layout` (`corner` / `center`);
- `color` (icon colour when on);
- `value` (`on_off` / `none` / `percent`), the state line;
- `open` (page id), for a tile that opens a page;
- `long` (a second target for a long press, tiles only).

Page `id` is needed for `open` and `swipe`. Default: `p<index>`.

### 6.3 Per element

- **Home** (`page 0`):
  - clock: `clock.txt`;
  - date: our `ru::date()`, but **short**, within 165 px of font 3 and 26 bytes. Example: `"Вс, 25 окт"` = 15 B, about 130 px. Today's `"воскресенье, 25 октября"` does not fit;
  - current weather: `weather_pic.pic` from WMO code + day/night (table below); `outdoor.txt="12°"`;
  - today low/high from `days[0]`: `low<k>.txt` + `high_arrow<k>`/`high<k>`, k by pixel width (font 2: digit 11 px, `-` 6, `°` 7);
  - indoor NTC: `row1_icon.txt=<glyph>`, `row1.txt="23,4°"`;
  - up to 7 shortcuts: `shortcutN_icon.txt=<glyph>`, `.pco` = accent when on / dim when off (or hide when off, OHpanel's `show: when_on`), `vis shortcutN_hs,1`;
  - hardware relays: `relay1_text`, `relay1_bar.bco`;
  - MQTT/Wi-Fi status: `wifi` or `top1`.
- **Button pages** (`page 7`), 8 tiles:
  - Switch: tap → toggle (or on/off from the known state). Tile active = state on;
  - Scene "all off": `type: scene, action: off`. Active when any group member is on. Tap → off over the group;
  - Gate pulse button: `type: button`, tap sends `on` once. Show the tile active for ~1 s after the tap (or while the relay state topic says on). State line `"Импульс"` or none (`value: none`);
  - Text item: tile with `value` = the state text (21 B), no action;
  - Unknown state: off tile, value `"–"`;
  - State line texts (our Russian): `"Вкл"` / `"Выкл"`.
- **Weather pages** (`page 1`): one sheet per day from Open-Meteo `daily`:
  - icon: `weather_page` slot;
  - `high`/`low` (`"15°"` / `"7°"`);
  - optional chips: wind (`weather-windy`), precipitation (`weather-rainy`), UV, sunrise/sunset. These need extra Open-Meteo fields (today we fetch `weather_code`, `temperature_2m_max/min` only);
  - title: `"Сегодня · 25 окт"`, `"Завтра · 26 окт"`, then the weekday;
  - `dots` one per day.
- **WMO → picture name.** OHpanel's `wmo` set (`weather-icons.json:230-…`) has day icons only. Ours, with Open-Meteo `is_day`:

| WMO | day | night |
|---|---|---|
| 0 | clear-day | clear-night |
| 1, 2 | partly-cloudy-day | partly-cloudy-night |
| 3 | overcast-day | overcast-night |
| 45, 48 | fog-day | fog-night |
| 51, 53, 55 | drizzle | drizzle |
| 56, 57, 66, 67 | sleet | sleet |
| 61, 63, 65, 82 | rain | rain |
| 71, 73, 75, 77, 86 | snow | snow |
| 80, 81 | partly-cloudy-day-rain | partly-cloudy-night-rain |
| 85 | partly-cloudy-day-snow | partly-cloudy-night-snow |
| 95 | thunderstorms-day-rain | thunderstorms-night-rain |
| 96, 99 | thunderstorms-rain | thunderstorms-rain |
| other | not-available | not-available |

What our JSON and firmware lack today:
- page ids and the swipe row;
- shortcuts, relay labels, background/theme, idle/night and home timeout settings;
- tile layout, colour and value fields;
- a weather page definition;
- a font width table for Cyrillic fitting;
- the background picture table;
- an icon table beyond 92 names. The TFT has all 7461; raw `E334` codepoints keep working.

---

## 7. Worked example (aurora background, Russian texts)

Notation: `>` ESP to TFT (each command is followed by `FF FF FF`, shown once); `<` TFT to ESP. `{…}` stands for the event frame `92 6E 73 70 61 6E 65 6C 65 76 65 6E 74 00 <json> 00 FF FF FF`.

### 7.1 Boot to home with time and weather

```
(GPIO4 LOW, UART2 921600 8N1)
< 00 00 00 FF FF FF                      startup
< 88 FF FF FF                            ready
< {"page": "blank", "component": "currentpage", "value": "pagechange"}
> FF FF FF 73 65 6E 64 6D 65 FF FF FF    "sendme" with a leading terminator
< 66 04 FF FF FF                         page 4 (blank): found at 921600
> get design_ver                         67 65 74 20 64 65 73 69 67 6E 5F 76 65 72 FF FF FF
< 71 1E 9E 62 BC FF FF FF                bc629e1e → expected design
> connect
> sendme                                 (swallowed after connect, or answered 66 04)
< comok 1,30614-0,NX4832F035_011R,163,61488,D264B8204F0E1828,16777216 FF FF FF
> th_accent=19998
> th_text=65535
> th_dim=52857
> bg_pic=48
> bg_close=45
> page 0                                 70 61 67 65 20 30 FF FF FF
> sendme                                 flow control after a page change
< {"page": "home", "component": "currentpage", "value": "pagechange"}
< 66 00 FF FF FF
> busy_tm.en=0
> vis busy1,0
> vis busy2,0
> vis busy3,0
> sendme                                 (sync before ref_stop)
< 66 00 FF FF FF
> ref_stop
> clock.txt="14:07"
> vis clock,1
> date.txt="Вс, 25 окт"                  UTF-8: D0 92 D1 81 2C 20 32 35 20 D0 BE D0 BA D1 82
> vis date,1
> weather_pic.pic=90                     partly-cloudy-day (aurora weather_home 73 + 17)
> vis weather_pic,1
> vis weather_hs,1
> outdoor.txt="12°"                      ° = C2 B0
> vis outdoor,1
> low3.txt="7°"                          "7°" = 11+7 = 18 px → k = ceil(18/6) = 3
> vis low_arrow,1
> vis low3,1
> high3.txt="15°"
> vis high_arrow3,1
> vis high3,1
> row1_icon.txt="<EE BD 94>"             home-thermometer-outline
> vis row1_icon,1
> row1.txt="23,4°"
> vis row1,1
> shortcut1_icon.txt="<EF 89 92>"        lightbulb-group (U+F252)
> shortcut1_icon.pco=19998               a group member is on → accent
> vis shortcut1_icon,1
> vis shortcut1_hs,1
> relay1_text.txt="Свет"
> relay1_bar.bco=10597                   relay 1 off
> vis relay1_text,1
> vis relay1_bar,1
> vis swipe,1
> wifi.txt=""                            connected: empty
> dim=100
> ref_star
> sendme                                 flow control after ref_star
< 66 00 FF FF FF
(every minute)  > clock.txt="14:08"
```

### 7.2 Swipe left from home to the first button page (8 tiles)

```
< {"page": "home", "component": "swipe", "value": "left"}
  (no release follows: home has no hotspot under an empty area; a release from shortcut/weather within 300 ms is ignored)
> page 7
> sendme
< {"page": "buttons", "component": "currentpage", "value": "pagechange"}
< 66 07 FF FF FF
> busy_tm.en=0
> vis busy1,0
> vis busy2,0
> vis busy3,0
> title.txt="Дом"
> vis title,1
> vis close,1
> dots.txt="<EE A7 9E> <EE A7 9D> <EE A7 9E>"     row [home, house, outside], house current
> vis dots,1
> vis swipe,1
-- tile1 "Кухня", ceiling-light, ON
> tile1_pic.pic=22
> tile1_icon.bco=65535
> tile1_name.bco=65535
> tile1_value.bco=65535
> tile1_icon.pco=19998
> tile1_name.pco=10597
> tile1_value.pco=27469
> tile1_icon.txt="<EE 9D A8>"               ceiling-light U+E768
> tile1_value.txt="Вкл"
> tile1_name.txt="Кухня"
-- tile2 "Гостиная", OFF
> tile2_pic.pic=23
> tile2_icon.bco=10597
> tile2_name.bco=10597
> tile2_value.bco=10597
> tile2_icon.pco=52857
> tile2_name.pco=52857
> tile2_value.pco=40147
> tile2_icon.txt="<EE 9D A8>"
> tile2_value.txt="Выкл"
> tile2_name.txt="Гостиная"                 16 B ≤ 18, about 80 px: check the fit
-- tiles 3..8 the same; "Все выкл" scene: lightbulb-group-off, active if any member is on;
-- "Ворота": gate, type button, value none → tileN_value.txt=""
> sendme
< 66 07 FF FF FF
> ref_stop
> vis tile1_hs,1
> vis tile1_pic,1
> vis tile1_icon,1
> vis tile1_name,1
> vis tile1_value,1
  … same for every configured tile …
> ref_star
> sendme
< 66 07 FF FF FF
```

### 7.3 Tap on tile 3 (state change), then swipe left

```
< {"page": "buttons", "component": "tile3", "value": "press"}       t=0
< {"page": "buttons", "component": "tile3", "value": "release"}     t=120 ms
  ESP: no swipe within 300 ms before; hold the release 60 ms; no swipe → act:
  beep; publish "ESPURNA-BEDROOM/relay/0/set" = "1" (state known off → on)
  MQTT state "ESPURNA-BEDROOM/relay/0" = "1" arrives → redraw tile3 (only changed attributes):
> tile3_pic.pic=26
> tile3_icon.bco=65535
> tile3_name.bco=65535
> tile3_value.bco=65535
> tile3_icon.pco=19998
> tile3_name.pco=10597
> tile3_value.pco=27469
> tile3_value.txt="Вкл"
  (tile3_icon.txt and tile3_name.txt unchanged → not resent; the bco/pco assignments redraw them on the new picture)

-- swipe left that starts on tile 2:
< {"page": "buttons", "component": "tile2", "value": "press"}
< {"page": "buttons", "component": "swipe", "value": "left"}
< {"page": "buttons", "component": "tile2", "value": "release"}     (either order; dropped: within 300 ms of the swipe,
                                                                     or pending and cancelled by the swipe)
  ESP: next page in the swipe row ("outside"; wraps to home after the last)
> page 7                                  same physical page, new content
< {"page": "buttons", "component": "currentpage", "value": "pagechange"}
  … render as in 7.2 with the "outside" items and dots "○ ○ ●" …
```

### 7.4 Idle, blank and wake

```
(60 s without an event)   > dim=10
(idle_brightness 0, or night window)   > dim=0
                                       > page 4
< {"page": "blank", "component": "currentpage", "value": "pagechange"}
(tap)  < {"page": "blank", "component": "wake", "value": "release"}
       > page 0
       … home render …
       > dim=100
```

---

## 8. Risks, unknowns, licence

### 8.1 Verify on the real panel

1. **Hidden objects and touches.** After `vis 255,0`, do hotspots need `vis _hs,1`? (Assumed yes; OHpanel does it.) Does the TouchCap `swipe` work while hidden? OHpanel shows it explicitly on button pages and never on the weather page, so it is probably not needed there. Send `vis swipe,1` everywhere.
2. **Release semantics.** Is a hotspot's release sent when the finger slides off it? Is the order of swipe and release stable? The 60 ms / 300 ms windows come from OHpanel and are tuned there, but check them with our loop latency: `panelLoop()` runs in the ESPurna loop, not in a task.
3. **921600 baud on Arduino-ESP32 2.0.9 `HardwareSerial`** with Wi-Fi active. Watch for RX overruns and the `24` overflow status. Measure `sendme` round trips. A queue drained in `loop()` with blocking waits of up to 500 ms per sync would stall ESPurna, so use a non-blocking sync (send `sendme`, pause the queue until `66` or a timeout) or a dedicated TX task.
4. **The command swallowed after `connect`** (OHpanel's observation). Keep the dummy `sendme`.
5. **Text fit.** Cyrillic tile names in 82 px / 18 B; the date in 165 px / 26 B. Check `…` truncation with our width table.
6. **Theme after TFT reboot.** Make sure `th_*`/`bg_*` are resent before `page`; otherwise the aurora defaults show, which is wrong when another background is configured.
7. **Busy dots** appear if a page render starts later than 800 ms after `currentpage` (e.g. during MQTT reconnect or a weather fetch). Send `busy_tm.en=0` first, before anything else.
8. **Flasher.** Upload of 15.8 MB from raw.githubusercontent.com over TLS on our heap (TLS needs about 65 KB; `HeapMinTls`). Check Range resume against GitHub, the add-921600 search, and the removal of `recmod=1`. Rollback to Lovelace from OHpanel must find the display at 921600. Consider mirroring the TFT on the LAN (lion) and flashing over plain HTTP.
9. **Power-cycle timing** (GPIO4 HIGH 800 ms, then 1500 ms boot). Display boot to the `88` frame takes how long with a 15.8 MB TFT?
10. **Brightness persistence.** `dim=` is not stored: after a TFT brownout the display comes back at 100 % until our next tick.
11. **Long press** only exists on tiles (press+release timing). Shortcuts cannot have one.
12. **Weather page reload per day** (`page 1` each swipe) flashes the screen. Optionally use `page_fade`.

### 8.2 Design and maintenance risks

- **Object names are an internal contract of OHpanel** (`tools/check_names.py` checks the firmware against the manifest), not a public API. A future design may rename them. Pin the TFT by commit and check `design_ver == bc629e1e` at boot; refuse to draw (or log loudly) on a mismatch.
- OHpanel's support is EU only for this design; RC1 adds a separate US design with other coordinates.
- Pictures exist per background. Our renderer must use the table for the configured background; on `black` all 8 tile slots share one picture.

### 8.3 Licence notes

- OHpanel is **Apache-2.0** (`LICENSE`; author Jonas Brüstel). There is **no NOTICE file**. ESPurna is GPL-3.0. Apache-2.0 code may be included in a GPL-3.0 work (one-way compatible).
- **If we port code** (the frame parser in `Protocol.cpp`, the reparse-exit/upload logic in `nextion/Display.cpp`, renderer snippets, the WMO/picture tables), Apache-2.0 §4 requires:
  - a copy of the Apache-2.0 licence with the distribution;
  - a prominent notice in each modified file that we changed it;
  - keeping the original copyright and attribution notices.
  
  Suggested header for such files: `Portions derived from OHpanel (https://github.com/bruestel/ohpanel, commit ba93619), Copyright Jonas Brüstel, Apache License 2.0; modified for ESPurna.`
- Only knowing the protocol (object names, event strings) and writing our own code does not create a derivative of the firmware. Attribution in `nspanel/README.md` is still good practice.
- **The TFT file** is not only Apache-2.0 (`THIRD-PARTY-NOTICES.md`, section "Display firmware"):
  - Nextion runtime: proprietary, ITEAD;
  - Ubuntu-derived fonts: Ubuntu Font Licence;
  - Noto Sans: OFL;
  - MDI: Apache-2.0;
  - Meteocons: MIT.
  
  Flashing from the upstream URL is fine. **If we redistribute the .tft** (e.g. a mirror on our server or in our repo), ship `THIRD-PARTY-NOTICES.md` and the font licences (`boards/nspanel/nextion/assets/fonts/{UFL.txt,OFL.txt,MDI-LICENSE.txt}`, `assets/meteocons/LICENSE`) next to it, as OHpanel's release zip does.

---

## 9. Porting plan for `nspanel.cpp` (summary)

**Keep**
- boot/safeboot and crash-loop guard;
- LittleFS storage, `/nspanel.json` web endpoints and editor (its Lovelace-grid preview needs a rework);
- the config parser (extended per §6.2), entities, groups, scenes, MQTT subscribe/publish;
- relays and hardware buttons;
- the Open-Meteo task (add `is_day` per day if needed, and extras for the chips);
- NTC, buzzer, terminal commands (`nspanel.send` sends a raw Nextion command and appends FF FF FF);
- the TFT flasher (3 patches, §1.6).

**Replace**
- the 55 BB/CRC16 `Parser`, `writeFrame`, coalescing `enqueue` and the 75 ms `txLoop`. New: FF FF FF command queue with `sendme` sync, and an FF FF FF frame parser for 00/88/66/71/comok/92/91/status;
- the Lovelace renderer: `weather::sendUpdate`, `sendStatus`, `sendDimmode`, `sendTimeout`, `sendClock`, `buildCards`, `renderItem`, `renderNav*`, `showCard`, `showScreensaver`;
- `onStartup` and `onMessage` (Lovelace `event,…` CSV), `itemTap`.

**New**
- link state machine (§1.5);
- attribute cache per page load;
- home, buttons, weather and blank renderers;
- dots;
- background picture table and theme;
- swipe row and history;
- tap/swipe disambiguation and long press;
- idle/night/brightness tick;
- UTF-8 width fitting (font 0..3 width table).

Estimate:
- about 900 lines removed, about 1300..1600 lines added;
- 3..4 days of implementation and 1 day on the panel (flash the TFT, tune the timing and fit).

---

## 10. Implementation in ESPurna (2026-10-25, built, not yet run on the panel)

Code: `code/espurna/nspanel_ohpanel.h` (driver), `code/espurna/nspanel.cpp` (shared core, detection `link::`, flasher `flash::`).
User documentation: `README.md` (user guide) and `reference.md`, section "OHpanel TFT".

Implemented as specified above:
- link state machine (1.5): search with `FF FF FF sendme` + `get design_ver`, identify (`connect` + throw-away `sendme`), theme globals
  before the first `page`, ping after 10 s of silence (2 misses), 3 unanswered syncs, TFT restart (`00 00 00` / `88`) → quiet 500 ms
  (max 5 s) → search, theme and page again;
- FF FF FF frame parser (2.2), non-blocking flow control (2.1): a command queue drained in `loop()`, `sendme` after `page`, before
  `ref_stop`, after `ref_star` and every 512 bytes; the queue pauses until `66` or 500 ms. Nothing blocks the loop;
- attribute cache per page load; the busy indicator is cancelled first after every page load; hotspots and `swipe` shown explicitly;
- home (3.4): clock, date, weather picture + `outdoor`, today low/high (`low<k>`, `high_arrow<k>`, `high<k>`), `row1` indoor NTC,
  `shortcut1..7`, `relay1/2_text` + `_bar`, `wifi`, `top1` (MQTT down);
- buttons (3.5): 8 tiles, corner layout, picture → bco/pco → txt order, page dots; weather (3.6): one sheet per day with 3 chips;
- events and tap/swipe disambiguation (4.1: 300 ms guard, 60 ms hold), navigation (4.2) without wrap: home is leftmost;
- brightness (5): `dim=` only on change, idle → home dimmed, `dim: 0` → blank page, wake tap does not act;
- flasher patches (1.6): 921600 first in the handshake, no `recmod=1` at 921600, detection again after every upload.

Deviations from 6.2:
- JSON: `"display"` is the driver selection (`auto` | `ohpanel` | `lovelace`); the OHpanel options live in `"ohpanel"`
  (`background`, `accent`, `text`, `dim`, `weather_days`, `relays`); `screensaver.timeout/brightness/dim` keep their meaning
  (`idle_after`, `brightness`, `idle_brightness`); shortcuts are `screensaver.shortcuts`; there is no separate `home_after`.
- Not implemented: long press, `value` modes, night window, `page_fade`, a general history stack (close goes home; only the
  settings and thermostat pages return to where they were opened from), the boot page, `swipe` list (the row is every page in file
  order). `layout: center` and the thermostat page: section 11.
- Text widths come from approximate Ubuntu advances (slightly generous), not from `display-manifest.json`.

## 11. Tile layouts, settings, info lines and thermostat (2026-10-25, built, not yet run on the panel)

Checked against the local OHpanel checkout (`main` `45b783e`, design identical to R092): `display-manifest.json` (`fit`, `boxes`,
`fonts`, `backgrounds`), `layout.py`, `ohpanel_hmi.py` and `components/app_display/src/EngineThermostat.cpp` / `Engine.cpp`.

### 11.1 Center layout (buttons page)

| Object | Manifest | Use |
|---|---|---|
| `tileN_center` | font 9 (MDI only, 48 px), maxl 4, box = `FAV_ICON` (8, 8, 84x62) inside the tile, e.g. tile1 18,65 84x62 | icon |
| `tileN_text` | `fit` [font 0, 80 px, 20 B], box (x+10, y+71, 80x20), centred, one line, no `isbr` | name |

The page preinitialize hides `tileN_center`; we show `_hs`, `_pic`, `_center`, `_text` (not `_icon/_name/_value`). Colours as in
the corner layout (picture first, then `bco` = flat tile colour on both, `pco`, then `txt`). The name gets one line: the field is
20 B / 80 px in font 0 and sits under the icon, so the corner layout's two-line trick (first half in `tileN_value`) has no room
(the value box 12,50 82x16 overlaps the centred 48 px glyph). Longer names end in `...`. Tiles with a state line (`text`,
`thermostat`, settings) keep the corner layout: font 9 has no digits.

**Glyph coverage.** Text fonts 0..6 cover only `[32..383]`, `[536..539]`, `[900..974]`, `[1024..1119]` (manifest `fonts[*].ranges`):
no `…` (U+2026), `–` (U+2013) or `→` (U+2192). The driver uses `...`, `--` and `»` (U+00BB) instead.

### 11.2 Themes: light vs dark

All five backgrounds are dark (manifest `backgrounds[*].theme.text` is `#ffffff`, dunes `#fff8ee`), and each tile slot has exactly
two pictures, `buttonN.off` (flat `#2c2c2c` = 10597) and `buttonN.on` (flat white). A "light" background does not exist, so
**light** is a tile override on the same background: inactive = the white picture, `bco` 65535, icon `#848484` (33840), name 10597;
active = the dark picture, `bco` 10597, icon accent, name 65535. **dark** = OHpanel's renderer (`EngineLists.cpp:65-111`). The theme
globals (`th_*`, `bg_*`) do not change; switching reloads the current page.

### 11.3 Settings page

Not the TFT's `settings` page (id 2: brightness sliders, restart button), but the buttons page (id 7) with two tiles:
`tile1` Иконки (`Крупные` / `Мелкие`), `tile2` Тема (`Тёмная` / `Светлая`), corner layout, title `Настройки`, no dots unless it is a
JSON `"type": "settings"` page in the row. Reached by `"open": "settings"` (shortcut or tile; the JSON settings page if there is one)
or by swiping to the JSON page. `tileN` release toggles, stores ESPurna settings `nspIcons` = `center|corner` / `nspTheme` =
`dark|light` and reloads the page. Cross / swipe right = back.

### 11.4 Home info lines

`layout.py`: `INFO_Y = (230, 251)`, `INFO_H = 21`, `INFO_LEFT = (12, 26)`, `INFO_MAIN_X = 44`, `INFO_GAP = 8`, `INFO_RIGHT = (196, 52)`.

| Object | Font | Box | `fit` | Our use |
|---|---|---|---|---|
| `infoN_tagicon` | 7 | 12, y, 26x21, centred | 5 B | MDI icon (`screensaver.info[].icon`) |
| `infoN_text` | 1 | 44, y, 144x21, `pco` = `th_text` (page) | 144 px, 31 B | `<text>  <value>` |
| `infoN_right` | 1 | 196, y, 52x21, centred, `th_dim` | 52 px, 5 B | extra value (`82%`) |
| `infoN_tag`, `infoN_badge`, `infoN_badgeico` | 1 / 7 | as the tag | 5 B | unused (badges have a red picture) |

`infoN_right` holds 5 bytes, so `8.3° 82%` does not fit there; the main value goes into the text field.

### 11.5 Thermostat page (id 17)

Objects (`ohpanel_hmi.py` `thermostat_page`, manifest `fit` / `boxes`):

| Object | Font / type | Box | `fit` | Our use |
|---|---|---|---|---|
| `title` | 3 | | 300 px, 46 B | item name |
| `now` | 6 (72 px), centred, cropped | 140,102 170x76 | 170 px, 10 B | `" 21,9°"` (leading space centres the number, as OHpanel) |
| `target` | 2, centred, cropped, `LIGHT_GREY` | 137,182 176x26 | 176 px, 32 B | `Цель 21,5°`; `pco` accent while not confirmed |
| `state_hs` | hotspot, no events | 150,206 150x24 | | box for the state line |
| `ring_hs` + `ring_tm` | hotspot, reports `tdrw`/`tdru` (x*1000+y) | 95,30 260x260 | | **hidden**: no drag |
| `minus`, `plus` | icon font 8, preset glyph | centres 48,160 / 402,160 | | `bco` 14855, `pco` 65535, `txt` MDI minus / plus |
| `minus_hs`, `plus_hs` | hotspot, event `minus` / `plus` release; press sets `<icon>.pco=th_accent` | 20,132 56x56 / 374,132 56x56 | | shown |
| `m11_hs` .. `m44_hs` | mode pill hotspots, event `mode1..4` | y 280, h 32 | | **hidden**: no modes |
| `close`, `close_hs`, `swipe` | | | | back |

Preinitialize: `vis 255,0`, `vis title,1`, `vis close_hs,1`, `vis close,1`, busy timer, background copy (`now`, `target`, `title`
cropped from `bg_pic`). Swipe detection and background touch are on.

Drawing (after `ref_star` + sync, OHpanel `Engine.cpp` helpers):
- `cirs x,y,r,colour` (disc), `picq x,y,w,h,<bg_pic>` (restore background), `xstr x,y,w,h,font,pco,bco,xcen,ycen,sta,"text"` with
  `sta` 3 (no background);
- discs `cirs 48,160,24,14855` and `cirs 402,160,24,14855`, then `vis minus,1` / `vis plus,1` so the icons draw over them;
- ring: centre 225,160, radius 112 (= 260/2 - 18), beads of radius 7 at `a = 0.75π + 1.5π·i/(n-1)` (clockwise from lower left over
  the top, open at the bottom). OHpanel uses a 3 px pitch (177 beads); we use **89 beads (6 px pitch)**: still a closed band
  (beads overlap) at half the commands. Beads up to the knob = accent, above = track 19017; knob `cirs` radius 11, white, at
  `round(share·(n-1))`, `share = clamp((setpoint-min)/(max-min), 0, 1)`. No marker for the current temperature (as OHpanel);
- knob move: `picq` the old knob box (25x25), redraw beads from min(old,new)-4 to max(old,new)+4, draw the new knob;
- state line: `picq 150,206,150,24,<bg>`, then the icon `xstr x,206,24,24,7,<colour>,0,1,1,3,"<glyph>"` and the text
  `xstr x+28,209,w+2,18,1,<colour>,0,0,1,3,"<text>"`, the pair centred in the box: heating `Нагрев · 39%` with `fire` in #ff8a3d
  (64615), else the humidity `39%` with `water-percent` in `th_dim`. Redrawn only when text or colour change.

Events: `minus` / `plus` release → local setpoint ± `step`, snapped to the step grid above min and clamped (OHpanel's
`v = clamp(min + round((v-min)/step)·step, min, max)`); after each tap `<icon>.pco=65535` (the TFT left it in the accent colour).
OHpanel sends every tap at once; we **debounce**: the value is published 1.5 s after the last tap (`%.1f`, or the `payload`
template), shown in the accent colour until the state topic reports it (at most 6 s, OHpanel `kThermoSentMs`). Opening another
thermostat sends a pending edit immediately. `close` / swipe right, up, down → back to the card or home it was opened from.
`currentpage` `thermostat` maps to id 17 (otherwise the "display moved by itself" check would reload the page).

Data: `"type": "thermostat"` items bind up to six JSON entities on one topic (`room` = `[room, temperature|set_temperature|humidity|
active|min_temperature|max_temperature]`). JSON payloads are copied in the MQTT callback (latest per topic) and parsed in `loop()`
with ArduinoJson 5 in place. Note: ESPurna's MQTT module drops payloads of `MQTT_BUFFER_MAX_SIZE` (1024) bytes and more before any
callback; Istabai `istabai/out` (9 rooms) is 1298 bytes, so the NSPanel build needs `MQTT_BUFFER_MAX_SIZE 2048`.

Not verified on the panel: the `cirs`/`picq`/`xstr` drawing order against the page refresh (are shapes kept after `ref_star`?),
the minus/plus icon box over the 24 px disc, the 6 px bead pitch look, the light tile scheme, the centre layout fit.

## 12. List pages and 3-column grids (2026-10-25, built, not yet run on the panel)

Source: R092 `boards/nspanel/nextion/ohpanel_hmi.py:493-575` (`entities_pages()`), `layout.py:172-189` (`ENTITY_*`),
`display-manifest.json` (`fit`, `pictures`), `components/app_display/src/EngineLists.cpp:144-330` (`draw_entities`).

### 12.1 Which entities page

| page | rows | per row | name fields (font 2, manifest `fit` [font, px, bytes]) |
|---|---|---|---|
| `entities` (10) | 7, y 46 + 35·(r-1), 34 high | icon (font 8, 32 px), fields cropped from the background, a 2 px line `rowN_line` under it | `_text` 214/39, `_value` 168/31, `_longtext` 290/53, `_shortval` 92/24, `_wide` 382/69 |
| `entities_cards` (11) | 8 cards 210x58 in 2 columns | small label + value | `cardN_text` 24 B, half width: no full-width names |
| **`entities_rows` (12)** | **7, y 44 + 36·(r-1), 32 high** | card picture `rowN_pic` (`rowcard1..7`, per background), icon font 7 (24 px), fields solid `bco` 10597 | `_text` 214/39, `_value` 158/29, `_longtext` 280/51, `_shortval` 92/24, `_wide` 372/67 |

We use **`entities_rows`**: each row is a card like a tile, so it reads as something to tap, and the solid fields need no
background crop. It costs 10 px of name width against `entities`. `rowcard` picture ids: aurora 53..59, black 165 (all rows),
candy 296..302, dunes 442..448, forest 589..595. Each row also has `rowN_textscr` (a scrolling label); we keep it hidden. The
page loads with `vis 255,0`, `vis title,1`, `vis close_hs,1`, `vis close,1`; it has `dots` (125,294 200x26), `swipe` and the
background `touch` event.

Fields per row, as OHpanel chooses them for every value change: the value fits `_shortval` (92 px) → `_longtext` + `_shortval`;
else `_text` + `_value`; no value → `_wide`. The unused ones are hidden first (they would paint over). Order: `rowN_pic.pic`,
the hidden fields, `pco` + `txt` of icon, name and value, then `vis` of pic, icon, name, value, all inside `ref_stop` /
`ref_star` on a page load. Colours: icon accent (or the item colour) when on, `th_dim` when off; name `th_text` when on (or for
text and thermostat items), else `th_dim`; value like the icon, `th_text` for text and thermostats. Values: `Вкл` / `Выкл` /
`--`, scenes `Вкл: n`, thermostats `21,9° » 21,5°` (shortened to fit 158 px), text items the payload, `open` items `»`
(U+00BB, the fonts have no arrows), buttons none (the name takes `_wide`).

### 12.2 Taps on rows: `sendxy`

The design has **no hotspots on the rows** (OHpanel's entity pages only display). Touching a row's picture or text field does
not reliably reach the page's own `touch` event either, and that event has no coordinates. So the driver sends the Nextion
system variable **`sendxy=1`** before `page 12` and `sendxy=0` before any other page (only when it changes; unknown again after
a link reset). With it the TFT reports every touch as `67 xH xL yH yL ev FF FF FF` (`ev` 01 press, 00 release), in the same
coordinates as the objects. The driver keeps the press point; on the release, if the finger moved at most 40 px in x and y,
the press row is `1 + (y-44)/36` for 44 ≤ y < 296 (title, close cross and dots fall outside). The tap is then queued as the
release `row<N>` and goes through the tile rules (§4.1: ignored within 300 ms after a swipe, held 60 ms for a swipe). The
page's background `touch` press/release events are ignored (they must not replace a pending release).

### 12.3 Sheets and navigation

A list page is split into cards of 7 rows (`per_screen` 1..7 makes it fewer), titled `Title 1/2`. The cards sit in the swipe
row like button cards: swipe left/right moves between them (and to the neighbouring pages), `dots` counts the whole row (home +
cards) as on the buttons page. `currentpage` `entities_rows` maps to id 12. State changes redraw only the changed attributes
(the cache), e.g. `row3_icon.pco` and `row3_shortval.txt`.

### 12.4 Three columns (`buttons` page)

`"columns": 3` on a grid page puts its tiles into slots 1, 2, 3, 5, 6, 7 (tile n, 0-based: slot n+1 for n < 3, n+2 after);
slots 4 and 8 are never shown, so their hotspots stay hidden and take no touch. A screen holds 6 tiles; `"per_screen": N`
(1..8) limits any grid page further (min of the two and the driver's 8). The objects cannot move, so the 3 columns keep the
4-column positions (x 10, 120, 230), and the right column stays empty. Tap `tileN` maps back with the inverse (slot 4/8 → none).
Lovelace: the same splitting; a chunk of up to 6 tiles uses `cardGrid` (3x2) anyway.

Since section 13, `"columns": 3` is the same as `"layout": "3x2"`: on the stock design it is drawn as above, on the custom
design `d3731fbb` on its own page `buttons32`.

Verify on the panel: that `sendxy=1` reports `67` frames on this TFT (NX4832F035) in the rotated design coordinates, the row
mapping, that `vis rowN_pic,1` before the fields draws them on top, the `»` glyph in font 2, and the `rowcard` ids per background.

## 13. Custom design `d3731fbb`: grid layouts (2026-10-25, runs on the panel)

Our build of the OHpanel design, TFT 9 113 964 B, sha256 `a3eec0fd...4e62`: every page and object of `bc629e1e` unchanged, four
grid pages added, only the aurora background kept. Source, TFT and the full object spec are in the fork
[igorufox/ohpanel, branch `espurna-grids`](https://github.com/igorufox/ohpanel/tree/espurna-grids):
`ESPURNA.md`, `docs/espurna-custom-layouts.md`, `boards/nspanel/nextion/display/ohpanel-nspanel-eu-display-d3731fbb.tft`.

### 13.1 Design recognition

`get design_ver` answers `71 BB 1F 73 D3 FF FF FF` (int32 -747429957 = `d3731fbb`). The driver has a table of known designs
(`ohpanel::Designs`):

| design | name | grid pages 18..21 | backgrounds |
|---|---|---|---|
| `bc629e1e` | stock OHpanel v0.9.2 | no | all five |
| `4508cd3f` | stock OHpanel v0.10.0-rc1 (same design) | no | all five |
| `d3731fbb` | custom ESPurna grids | yes | aurora only |

An unknown design is drawn like the stock one and logged as unexpected (`nspanel.info`: `(UNEXPECTED)`). With a design without
the other backgrounds, `background()` is aurora (`Backgrounds[0]`) whatever `ohpanel.background` says; the TFT has no picture
>= 189, so the black/candy/dunes/forest rows would draw garbage. The theme colour overrides stay. `nspanel.info` prints
`Design: <name>: layouts ..., background ...; cards split for the grid pages | drawn grids (§14) | the 4x2 slots; background <name> (bg_pic n)`
and, on a button screen, `Card: '<title>', grid, layout 3x3 on page buttons33, 9 tiles`. `/nspanel-status` has the same text
in `"design"`. The cards are split again when the TFT is identified (`syncCards()` in `ready()`), since the design decides the
capacity.

### 13.2 `"layout"` of a grid page

`"layout": "4x2" | "3x3" | "3x2" | "2x2" | "1x2"` (default 4x2; `"columns": 3` = `"3x2"`, `"layout"` wins; `per_screen`
1..9 caps it). `nspanel.cpp gridCapacity()`:

| layout | custom design: page, tiles | stock design (§14): drawn on `buttons`, tiles | unknown design: 4x2 slots (tile n -> slot (n / cols) * 4 + n % cols + 1) | Lovelace |
|---|---|---|---|---|
| 4x2 | `buttons` (7), 8 | stock tiles, 8 (`"draw": true`: drawn) | 1..8 | min(8, TFT capacity) |
| 3x3 | `buttons33` (18), 9 | 9 | 1-3, 5-7 (6) | 6 |
| 3x2 | `buttons32` (19), 6 | 6 | 1-3, 5-7 (6) | 6 |
| 2x2 | `buttons22` (20), 4 | 4 | 1, 2, 5, 6 (4) | 4 |
| 1x2 | `buttons12` (21), 2 | 2 | 1, 5 (2) | 2 |

Settings cards and the built-in settings always use `buttons`. List pages are unchanged (`entities_rows`).

### 13.3 Drawing (`ohpanel::TilePages`)

Per page: id, event name, tile count, tile picture base (off = base + 2 (N-1), on = off + 1: 170 / 158 / 150 / 146; `buttons`
keeps the background's table, 19 + 2N on aurora), and the field limits from the manifest `fit` (bytes, px, font):

| page | `_name` | smaller name font | `_value` | `_text` (center) |
|---|---|---|---|---|
| `buttons` | 18 B, 82 px, font 1 | font 0 | 21 B, 82 px, font 0 | 20 B, 80 px, font 0 |
| `buttons33` | 25 B, 116 px, font 1 | font 0 | 21 B, 82 px, font 0 | 25 B, 116 px, font 1 |
| `buttons32` | 25 B, 116 px, font 1 | font 0 | 29 B, 116 px, font 0 | 25 B, 116 px, font 1 |
| `buttons22` | 34 B, 189 px, font 2 | font 1 | 40 B, 189 px, font 1 | 34 B, 189 px, font 2 |
| `buttons12` | 53 B, 340 px, font 3 | font 2 | 62 B, 340 px, font 2 | 63 B, 406 px, font 3 |

Digit widths for the fit estimate: fonts 0..3 = 8, 10, 11, 13 px. The order per tile is the one of `buttons`: `tileN_pic.pic`,
then `bco`/`pco`, then `font` (corner name) and `txt`, last `vis tileN_hs,1` and the shown fields inside `ref_stop`/`ref_star`;
unused tiles stay hidden. Differences on the grid pages (`states`): in corner layout a switch / scene shows its state in
`_value` (`Вкл` / `Выкл` / `Вкл: n`) and the name is one line in `_name` (page font, then the smaller one, then `...`); tiles
without a state (button, open) keep the stock rule (a long name is split, the first half in `_value`). Thermostats: `_value`
`21,9° » 21,5°` (shorter forms when it does not fit), plus ` · Нагрев` while heating where it fits; a heating thermostat has
its icon and `_value` in `#ff8a3d` (64615) on every page unless the item has a `color`. The bar (`tileN_bar`) is not used.

### 13.4 Events

`currentpage` of `buttons33` / `buttons32` / `buttons22` / `buttons12` maps to ids 18..21 (no "TFT shows another page"
correction loop). Tile, swipe, touch and close events of these pages go through the same rules as `buttons` (`tileN` up to 9).
A tile release whose page is not the page last requested is ignored (it belongs to the screen before a page change, its slot
means another item now).

Verify on the panel: the tile pictures per slot (no aurora seams at the corners), the fonts 2/3 on `buttons22`/`buttons12`,
that the `_value` / `_name` fields of `buttons33` hold `Вкл` and ~10 Cyrillic letters, the busy dots and `close` on the new
pages, and `currentpage` names.

## 14. Drawn tiles on the stock design `bc629e1e` (2026-10-25, built, not yet run on the panel)

The custom design needs its own TFT and keeps only aurora. On the stock design (`bc629e1e`, `4508cd3f`: all five backgrounds)
the driver draws the 3x3 / 3x2 / 2x2 / 1x2 grids itself with Nextion drawing commands on the stock `buttons` page (id 7).
Checked on the panel beforehand: raw `fill` / `cirs` / `xstr` work on this TFT and are fast (85 commands in one burst drew a 3x3
grid at once), `xstr` with `sta` 3 draws text without a background onto what is there, UTF-8 Cyrillic and MDI glyphs in fonts 8
(32 px) and 9 (48 px) work. A page with `"draw": true` is drawn the same way also for 4x2 and on `d3731fbb`. Settings cards, the
built-in settings and list pages are unchanged. `nspanel.info`: `Design: stock OHpanel v0.9.2: layouts 4x2 (stock tiles), 3x3 3x2
2x2 1x2 drawn, all backgrounds; cards split for drawn grids`, `Card: '<title>', grid, layout 3x3 drawn on page buttons, 9 tiles`,
`Drawn: last page <n> tiles, <c> commands, <b> bytes; last update ...; sendxy 1`. Driver: `ohpanel::drawn`, `renderDrawn()`.

### 14.1 Geometry (`drawn::tileRect()`)

The grid area (136x72 tiles in 3x3, gaps 9 / 8) is x 12..438 (12 + 3·136 + 2·9 = 438, the span of the stock tiles: the right
~30 px of the 480 px screen are under the panel's frame, so a grid centred on the screen looks pushed to the right) and y 46..278 (46 + 3·72 + 2·8 = 278: under the title and the close cross, above the page dots at
y 294). Every layout fills it: `w = (426 - (cols - 1)·9) / cols`, `h = (232 - (rows - 1)·8) / rows`, the remainder centred.
Tile n (0-based, row by row) is at `x = 27 + offset + (n % cols)·(w + 9)`, `y = 46 + (n / cols)·(h + 8)`.

| layout | tile | corner layout (boxes relative to the tile; font) | center layout | name / state field (px, digit px) |
|---|---|---|---|---|
| 4x2 | 99x112 (+3 px centred) | icon 8,8 32x32 (font 8); state 8,60 83x20 (0); name 8,82 83x24 (1, smaller 0) | icon 0,12 99x52 (9); name 4,72 91x24 (0) | 83 / 83 |
| 3x3 | 136x72 | icon 8,6 32x32 (8); state beside it 46,6 82x32 (0); name 8,44 120x22 (1 / 0) | icon 0,4 136x36 (8); name 6,44 124x22 (1) | 120 / 82 |
| 3x2 | 136x112 | icon 8,8 32x32 (8); state 8,60 120x20 (0); name 8,82 120x24 (1 / 0) | icon 0,12 136x52 (9); name 6,72 124x26 (1) | 120 / 120 |
| 2x2 | 208x112 (+1 px) | icon 10,10 48x48 (9); state beside it 66,10 132x48 (1); name 10,72 188x30 (2 / 1) | icon 0,10 208x52 (9); name 8,70 192x30 (2) | 188 / 132 |
| 1x2 | 426x112 | icon 12,10 48x48 (9); state beside it 72,10 342x48 (2); name 12,70 402x34 (3 / 2) | icon 0,8 426x52 (9); name 10,66 406x34 (3) | 402 / 342 |

The field limits feed the same text logic as the tile pages (`itemFace()` with `states` on: switches and scenes show `Вкл` /
`Выкл` / `Вкл: n` in corner layout, the name one line in the page font, then the smaller one, then `...`; tiles without a state
split a long name, the first half in the state box; thermostats `21,9° » 21,5°` plus ` · Нагрев` where it fits). `xstr` has no
`txt_maxl`, the byte limit is 96 only to keep a command short.

### 14.2 Drawing (`drawn::drawTile()`, `renderDrawn()`)

Page load: `sendxy=1` (when it changes), `page 7`, sync, the busy indicator off, **`vis 255,0`** (the page's preinitialize already
hides everything; repeated so that no stock `tileN_pic/_icon/_name/_value/_bar/_text/_center/_hs` is shown or takes a touch),
`title.txt` + `vis title,1`, `vis close,1`, `vis close_hs,1`, `dots` + `vis dots,1`, `vis swipe,1`. Then every tile:

```
fill x+14,y,w-28,h,<tile>                     two crossing bars
fill x,y+14,w,h-28,<tile>
cirs x+14,y+14,14,<tile>                      four corners (centres 14 px in from the edges)
cirs x+w-15,y+14,14,<tile>
cirs x+14,y+h-15,14,<tile>
cirs x+w-15,y+h-15,14,<tile>
xstr ix,iy,iw,ih,8|9,<icon>,<tile>,1,1,3,"<MDI glyph>"
xstr vx,vy,vw,vh,<font>,<state>,<tile>,0,1,3,"<state line>"      corner layout; center: xcen 1
xstr nx,ny,nw,nh,<font>,<name>,<tile>,0,1,3,"<name>"
```

Colours are those of the tile pages (`tileColors()`): dark theme off = 10597 tile, `th_dim` icon, 52857 name, 40147 state; on =
white tile, accent icon (the background's accent or `ohpanel.accent`, or the item `color`), 10597 name, 27469 state; light theme
the other way round (off white, on dark); a heating thermostat has its icon and state in #ff8a3d (64615). The tile is solid, so
only its corners and the gaps show the background picture: it works on all five backgrounds.

The whole page is queued at once; the transport sends it in one burst and only syncs (`sendme`) every 512 bytes, nothing waits per
command. A 3x3 page of 9 thermostats is about 95 commands / 3.0 KB (14 for the page, 9 per tile, ~310 B per tile, ~5 syncs);
a 2x2 page about 46..50 commands / 1.3..1.5 KB.

Updates: the look of each tile (colours, icon, state, name) is cached per page load (`drawnN`); a state change redraws only the
tiles whose look changed (shape over the same shape, then the texts: 9 commands, ~310 B, below one sync), never the page. A theme
or background change (`nspTheme`, config reload) reloads the page and draws everything again.

### 14.3 Touch

Drawn tiles have no hotspots. As on the list pages (§12.2) the driver sends `sendxy=1` before `page 7` of a drawn card and
`sendxy=0` before every other page (also the stock `buttons` of 4x2 and settings cards); the value is sent only when it changes and
is unknown again after a link reset. On the `67` release, when the finger moved at most 40 px from the press point, the press point
is mapped to the tile under it (`drawn::tileAt()`, the same rectangles; gaps, title, close and dots hit nothing) and queued as the
release `tile<n>`, which goes through the tile rules: ignored within 300 ms after a swipe, held 60 ms for a swipe that arrives
after it, then the item's tap (thermostats open their page). The `swipe` TouchCap and the close hotspot work as on every page; the
page's background `touch` press/release events only wake / undim the screen.

Verify on the panel: the 2x2 / 1x2 / 3x2 / 4x2 boxes and fonts (texts inside their tiles, nothing clipped), the corners on
black / candy / dunes / forest, that a tile redraw does not flicker, `sendxy` taps on every tile (corners and edges) and that a
swipe across tiles does not tap.
