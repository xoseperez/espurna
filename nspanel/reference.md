# NSPanel (gen1 EU) on ESPurna ESP32: technical reference

Every field, limit and protocol detail. For setting the panel up start with the [user guide](README.md).

Firmware: ESPurna `esp32-port` branch, release image `esp32-matter-nspanel`, env `esp32-nspanel` (`-DSONOFF_NSPANEL`, module `code/espurna/nspanel*.{cpp,h}`).
ESPurna replaces the Tasmota + Berry + AppDaemon chain and talks to the TFT directly. Two display designs are supported, chosen by
`"display"` in the JSON (default `auto` = detected):

- **OHpanel** TFT (`bc629e1e`, from OHpanel v0.9.2, all 5 backgrounds, the 3x3 / 3x2 / 2x2 / 1x2 grids drawn at runtime; or our
  custom build `d3731fbb` with those grids as pages, aurora only):
  swipe navigation, home page with clock, weather and shortcuts, button pages in grid layouts, weather days (driver
  `nspanel_ohpanel.h`, protocol `ohpanel-protocol.md`). See [OHpanel TFT](#ohpanel-tft).
- joBr99 **Lovelace UI** TFT (v4.1.1 / v4.7.3): screensaver and grid cards with arrow navigation (driver `nspanel_lovelace.h`,
  protocol `lovelace-protocol.md`). The fallback: flash it back with `nspanel.flash` (below) and the panel works as before.

Shared by both: `/nspanel.json` model, MQTT state map, groups/scenes, hardware buttons, Open-Meteo weather, room temperature, buzzer, web editor,
terminal commands and the TFT flasher (`code/espurna/nspanel.cpp`).

## Hardware mapping

| What | GPIO | Notes |
|---|---|---|
| Relay 1 / Relay 2 | 22 / 19 | active high |
| Button left / right | 14 / 27 | active low. Left: click toggles relay 1. Right: click runs `buttons[1]` from the JSON (ESPurna action `custom`) |
| Display UART2 | TX 16, RX 17 | 921600 8N1 (OHpanel) / 115200 8N1 (Lovelace) |
| Display power | 4 | driven LOW (= on) at boot |
| Buzzer | 21 | LEDC channel 15, short beep on tile tap when `"beep": true` |
| NTC | 38 (ADC1) | 11.2 kOhm pull-up to 3.3 V, NTC 10 kOhm @ 25 C, B 3950 (Tasmota `ADCParam1 2,11200,10000,3950`, ESPHome configs use the same values); read by the ESPurna `NTC` sensor (`NTC_PIN 38`, `NTC_R_UP 11200`, `NTC_R_DOWN 0` in hardware.h) |

Any hardware button can use the JSON action: set its ESPurna action to `custom` (e.g. `btnClick0` = `custom`), then `buttons[i]` applies.

## Config file `/nspanel.json`

Stored on LittleFS in the existing data/spiffs partition (Tasmota's `spiffs`, ~320 KB). If it cannot be mounted it is formatted.
Without a file, a built-in default shows the two local relays.

Edit it at `http://<panel>/nspanel` (textarea, Save, live preview of the cards, which is also a drag-and-drop editor:
order, palette of item templates, trash, double-click rename, page layout, Undo), or:

- `GET  /nspanel.json` returns the file
- `POST /nspanel.json` (body = JSON, `Content-Type: application/json`) validates, saves and reloads. The body is only collected
  by the web server; the main loop parses, saves and applies it (after a running weather fetch, up to 10 s) and the answer comes
  when that is done: 200 `OK[, warnings: ...]`, 400 with the error text when invalid (the stored file is left alone),
  413 above 16 KB, 503 `low memory ... retry` / `busy, retry` (nothing changed, send it again).
  Example: `curl --digest -u admin:<pass> -H "Content-Type: application/json" --data-binary @nspanel.json http://<panel>/nspanel.json`

All three need the normal ESPurna web authentication.

### Schema

```jsonc
{
  "version": 1,                                     // ignored, informational
  "mqtt": { "on": "1", "off": "0", "toggle": "2", "set": "/set" },
  "screensaver": {
    "timeout": 60,                                  // seconds until the card dims and the screensaver returns, 0 = never
    "brightness": 100,                              // active brightness 1..100
    "dim": 10,                                      // screensaver brightness 0..100 (0 = backlight off); must differ from brightness
    "weather": { "lat": 52.52, "lon": 13.40, "refresh": 900, "enabled": true }
  },
  "temperature": { "enabled": true, "show": true },   // room temperature on the screen; correction and MQTT interval: web UI Sensors
  "colors": { "on": 65222, "off": 17299, "unknown": 16904 },   // RGB565 decimal
  "beep": false,
  "debug": false,                                   // log every TX/RX frame
  "display": "auto",                                // auto | ohpanel | lovelace (see "Display detection")
  "language": "en",                                 // en (default) | ru: dates, built-in texts, decimal comma
  "pages": [
    {
      "title": "Outside",                           // heading, max 40 bytes
      "type": "grid",                               // grid | list (OHpanel rows) | settings (OHpanel)
      "layout": "4x2",                              // grid: 4x2 (default) | 3x3 | 3x2 | 2x2 | 1x2, see "Screens per page"
      "columns": 4,                                 // older form: 3 = "layout": "3x2" ("layout" wins)
      "per_screen": 8,                              // optional 1..9: tiles (list: rows, max 7) per screen
      "draw": false,                                // OHpanel: true = draw the tiles at runtime even for 4x2 / on d3731fbb
      "items": [
        { "name": "Porch", "icon": "outdoor-lamp", "topic": "ESPURNA-PORCH/relay/0" },
        { "name": "Gate", "icon": "gate", "topic": "ESPURNA-GATE/relay/0", "type": "button" },
        { "name": "Back yard", "icon": "tree", "page": {           // a nested page, opened by this tile
            "layout": "3x2",                        // same fields as a page; "title" defaults to the tile name
            "items": [
              { "name": "Garland", "icon": "string-lights", "topic": "ESPURNA-TERRACE1/relay/0" },
              { "name": "Sockets", "icon": "power-socket-eu", "page": { "layout": "2x2", "items": [ /* ... */ ] } }
            ]
        } },
        { "name": "Panel light", "icon": "lightbulb", "relay": 0 },
        { "name": "All lights", "icon": "lightbulb-group-off", "type": "scene", "group": "house", "action": "off" }
      ]
    }
  ],
  "groups": { "house": [ "ESPURNA-KITCHEN/relay/0", "ESPURNA-HALL/relay/1", 0 ] },
  "buttons": [ { "relay": 0 }, { "group": "outdoor", "action": "toggle" } ]
}
```

Item fields:

| Field | Meaning |
|---|---|
| `name` | tile label, max 30 bytes (Cyrillic = 2 bytes per letter, so ~15 letters) |
| `icon` | MDI name from the built-in table (`nspanel_icons.h`), or a raw TFT codepoint `"E334"` / `"U+E334"`, or a literal glyph. An unknown name shows `alert-circle-outline` and is listed in the save warnings |
| `topic` | absolute MQTT **state** topic (not under the panel's own root). Command topic = `topic + mqtt.set`, or `"command": "<topic>"` |
| `path` | JSON state payloads (zigbee2mqtt, Istabai): array of keys into the payload, e.g. `["state"]`, `["temperature"]`, `["Гостиная","set_temperature"]` (a number in the array indexes a JSON array). Without `path` the whole payload is the value. Several items may use one topic: it is subscribed once, the payload is parsed once in `loop()` |
| `on`, `off` | state values of this item, compared case-sensitively (e.g. `"ON"` / `"OFF"`); default `mqtt.on` / `mqtt.off`. They are also the command values. Toggle with an unknown state sends `on` (plain items without own `on`: `mqtt.toggle`) |
| `payload` | command template, `%s` is replaced by the value: `"{\"state\":\"%s\"}"`. Default: the plain value |
| `relay` | local relay index (0-based), instead of `topic` |
| `url`, `on_url`, `off_url` | instead of the MQTT command: an HTTP GET to `http://` or `https://` (no certificate check, max 256 characters). `url` is sent on every tap (a toggling endpoint, or a `button`); `on_url` / `off_url` switch on / off (toggle by the known state, else on). With a `topic` the state comes from MQTT as usual; without one and with two different URLs the panel keeps the state as last sent. Each request runs in a task of its own (timeouts 5 s, at most 2 at once); the log shows `URL request: HTTP 200` or the failure, never the URL. Try an address from the panel with `nspanel.url <url>` |
| `type` | `switch` (default): tap toggles. Known state → sends `on`/`off`; unknown → sends `toggle` (`"2"`, ESPurna toggle payload). `button`: tap sends `on` once (gates are pulse relays, the relay turns itself off). `scene`: `action` (`on`/`off`/`toggle`) over all members of `group`; `toggle` = any on → all off, else all on. `text`: read-only, shows the first 4 characters of the state payload in the icon slot. `thermostat`: see [Thermostat](#thermostat) |
| `group`, `action` | for `scene` |
| `page` | instead of a target: a nested page (an object with the fields of a page: `title`, `type`, `layout`, `per_screen`, `items`, which may hold further `page` tiles). The tile opens it; see [Nested pages](#nested-pages). Without `name` the tile shows the page `title`, without `title` the page shows the tile `name` |
| `open` | instead of a target: the tile opens a page. Page index (0-based), page `id`, page `title`, `"weather"`, `"home"` or `"settings"` (Lovelace: `weather`/`home` show the screensaver, `settings` does nothing) |
| `color` | OHpanel: icon colour when on, `"#rrggbb"` or RGB565 number (default: the theme accent) |
| `history` | not a thermostat (OHpanel): topic of a retained series `{"from": <unix s>, "step": <s>, "p": [<% of the step it was ON> \| null, ...]}`; a tap opens its chart instead of switching: orange bars (0 / 50 / 100 % on the left, a label every 6 h), legend `Вкл: 5,3 ч` = the hours ON, title `<name> · 24 h`, cross / swipe back to where it was opened from |
| `font` | Lovelace TFT >= 53 only: font of the icon slot, `small` / `medium-icon` / `medium` / `large` (AppDaemon names) or a font id 0..9. Sent as `<icon>¬<id>`, e.g. to show a `text` value in a text font. Ignored on 51 |

Tile colour: on = `colors.on`, off = `colors.off`, unknown = `colors.unknown`. A scene is "on" when any group member is on.

`groups`: name → list of state topics (strings) or local relay indices (numbers). Members are subscribed too, so their state is known.

`buttons[i]` (hardware button i, when its ESPurna action is `custom`): `{"relay": n}` / `{"topic": "..."}` with `"action"`
(`toggle` default, `on`, `off`), `{"group": "...", "action": ...}`, or `{"page": n}` (show the first card of page n).

Pages with more tiles than one card holds are split automatically (`Title 1/2`, `Title 2/2`). Card capacity: 8 (`cardGrid2`) when
the TFT reports version >= 51, else 6 (`cardGrid`); a chunk of up to 6 tiles always uses `cardGrid`.

Navigation is linear: the screensaver sits left of the first card (its left arrow is a home icon), the arrows step through the
cards in order, the last card has no right arrow. The Lovelace TFT has no real swipe gestures (its `mSwipe*` objects are plain tap
hotspots and the Nextion page swipe attributes are disabled, also in v4.7.x).

Pages may have an `"id"` (default `p<index>`) for `open`. `"type": "settings"` makes a page the OHpanel settings tiles (no
`items`; it sits in the swipe row like any page, the Lovelace driver skips it).

#### Nested pages

The top-level `"pages"` are exactly the swipe row (home + their sheets, counted by the page dots). A tile with `"page": {...}`
holds a nested page: it is not in the row and is shown only by its tile (nesting may go deeper, a nested page can have `page`
tiles too). On a nested page:

- OHpanel: cross and swipe right (on its first sheet) go back to where it was opened from (a history stack: home, a row page or
  another nested page, as the thermostat page); swipe left / right step through its own sheets (`Title 1/2`), on one sheet swipe
  left does nothing; the page dots show its own sheets only (none for one sheet); swipe up / down and the idle timeout go home.
- Lovelace: the left arrow of its first sheet goes back (no label), the arrows step through its own sheets only.

`"open": "<page id>"` / `"open": "<title>"` still works for top-level pages (and `weather` / `home` / `settings`); nested pages
are numbered after all top-level ones, so page indices of `"pages"` (`open`, `buttons[].page`, `nspanel.page`) are unchanged.

Screens per page: `"layout"` of a grid page (columns x rows; `"columns": 3` is `"3x2"`):

| layout | OHpanel stock TFT `bc629e1e` | OHpanel custom TFT `d3731fbb` | Lovelace |
|---|---|---|---|
| `4x2` (default) | page `buttons`, 8 stock tiles 96x96 | page `buttons`, 8 tiles 96x96 | 8 (TFT 50: 6) |
| `3x3` | drawn, 9 tiles 136x72 | page `buttons33`, 9 tiles 136x72 | 6 |
| `3x2` | drawn, 6 tiles 136x112 | page `buttons32`, 6 tiles 136x112 | 6 |
| `2x2` | drawn, 4 tiles 208x112 | page `buttons22`, 4 tiles 209x112 | 4 |
| `1x2` | drawn, 2 wide rows 426x112 | page `buttons12`, 2 wide rows 426x112 | 2 |

The stock design has no grid pages, so the driver draws those layouts itself on the stock `buttons` page (rounded tiles with
`fill` / `cirs`, icon and texts with `xstr`, all 5 backgrounds; [Drawn tiles](#drawn-tiles)). `"draw": true` on a page draws it
that way also for `4x2` and on the custom TFT. An unknown TFT design falls back to the layout's columns of the 4x2 slots.
`"per_screen": N` (1..9) allows at most N; the smallest wins and longer pages are split (`Title 1/2`). Example: 9 thermostats with
`"layout": "3x3"` = one screen. `"type": "list"` pages are rows on the OHpanel (7 a
sheet, `per_screen` up to 7); the Lovelace driver shows them as grids (`layout` / `per_screen` apply), and uses `cardGrid` (3x2)
for 6 tiles or fewer.

zigbee2mqtt switch (state `{"state":"ON",...}`, command `{"state":"OFF"}` to `<name>/set`):

```json
{ "name": "Лампа Яр", "icon": "lamp", "topic": "zigbee2mqtt/LampYarik", "path": ["state"], "on": "ON", "off": "OFF",
  "command": "zigbee2mqtt/LampYarik/set", "payload": "{\"state\":\"%s\"}" }
```

### Thermostat

`"type": "thermostat"` reads its values from one JSON state topic:

```jsonc
// Istabai: istabai/out = {"Гостиная":{"temperature":21.9,"humidity":39,"set_temperature":21.5,"active":"off",
//                                     "max_temperature":25,"min_temperature":10}, "Кабинет":{...}, ...}
{ "name": "Гостиная", "icon": "radiator", "type": "thermostat", "topic": "istabai/out", "room": "Гостиная",
  "command": "istabai/in/Гостиная", "step": 0.5 }

// generic (no "room"): explicit paths; min / max may be paths or numbers
{ "name": "Спальня", "type": "thermostat", "topic": "zigbee2mqtt/TRVZB Bedroom",
  "current": ["local_temperature"], "setpoint": ["occupied_heating_setpoint"], "active": ["running_state"],
  "min": 5, "max": 30, "step": 0.5,
  "command": "zigbee2mqtt/TRVZB Bedroom/set", "payload": "{\"occupied_heating_setpoint\":%s}" }
```

| Field | Meaning |
|---|---|
| `topic` | state topic (JSON) |
| `room` | shorthand for the paths `[room,"temperature"]`, `[room,"set_temperature"]`, `[room,"humidity"]`, `[room,"active"]`, `[room,"min_temperature"]`, `[room,"max_temperature"]` |
| `current`, `setpoint`, `humidity`, `active`, `min`, `max` | explicit paths (override `room`); `setpoint` is required without `room`; `min` / `max` may be numbers (default 5 / 30 while unknown) |
| `command` | command topic (default `topic + mqtt.set`); the payload is the setpoint with one decimal (`21.5`), or `payload` with `%s` |
| `step` | minus / plus step, default 0.5 |
| `history` | topic of a retained history series (`{"from": <unix s>, "step": <s>, "t": [<x10> \| null, ...], "s": [...], "h": [...]}`, `"h"` = humidity in % x10, optional): shows the *History* button and chart (OHpanel), see below |

Heating = `active` is known and not `off` / `false` / `0` / `idle` (any case).

OHpanel tile: corner layout always (font 9 has no digits), icon, state line `21,9° » 21,5°` (current » setpoint), name; heating =
highlighted tile with the icon and the state line in orange (`#ff8a3d`, unless the item has its own `color`); on the custom grid
pages `3x3` .. `1x2` the state line adds ` · Нагрев` where it fits (`2x2`, `1x2`), and on every page the humidity (` · 59%`) where it fits. Tap opens the thermostat page: title = item name, current temperature (large), `Цель 21,5°`, a ring of 89 beads
from min to max with a white knob at the setpoint, minus / plus, and a line with `Нагрев · 39%` (fire icon, orange) or the humidity
(`39%`, water-percent icon). Minus / plus change the setpoint at once (snapped to `step`, clamped to min..max; the target line is in
the accent colour while not confirmed) and send it 1.5 s after the last tap. The value stays until the state topic reports it (at
most 6 s). Cross, swipe right / up / down = back to the page it was opened from. Mode buttons and the ring drag are not used.
The Lovelace driver shows a thermostat as a read-only text tile with the current temperature.

History chart (OHpanel, `history` set): the hidden mode pill `m11_hs` (165,280 120x32) becomes a *History* button (drawn pill,
`chart-line` icon); its `mode1` release opens the chart on the same thermostat page with the dial objects hidden. The panel then
subscribes to the topic (unsubscribed again when the chart closes or the thermostat screen is left, so nothing is kept) and draws
the retained message when it arrives, after 4 s without one `No history yet`. The newest 96 points are drawn into a dark rounded
box over the grid area: degrees on the left (every 1 / 2 / 5 degrees by the range), a line and a label at every 6 h of local
time, heating (target above temperature, as the Istabai bridge counts it) as dim orange bands and an orange strip under the
plot, the target as a grey step line, the temperature in the accent colour, and with `"h"` the humidity as a green line on its own
% scale (labels on the right, steps of 10 / 20 %, at least 20 % high; the legend gets compact); runs of equal height are one
`line`, so a chart is ~100-350 commands. Title `<name> · 24 h`. Cross / swipe right: back to the dial; swipe up / down: back to where the thermostat
was opened from. Payloads up to 2047 bytes (MQTT buffer); `nspanel/openhab-history-rule.js` builds them in openHAB.

**MQTT buffer:** ESPurna's MQTT module drops `MQTT_MESSAGE_EVENT` payloads of `MQTT_BUFFER_MAX_SIZE` bytes or more (default 1024).
Istabai `istabai/out` with 9 rooms is ~1.3 KB, so the NSPanel build needs `#define MQTT_BUFFER_MAX_SIZE 2048` (board section
`SONOFF_NSPANEL` in `config/hardware.h`, or a build flag). Until then thermostats stay `--`; `nspanel.info` lists the warning.

**JSON null:** ArduinoJson 5 reports a JSON `null` as a string whose pointer is NULL. `json::extract()`, `jsonText()` and
`parsePath()` treat it as a missing value. Before 2026-10-30 a `null` at an entity's path crashed the panel ~2 s after MQTT
connected (the retained message), three crashes put ESPurna into its unstable mode (WiFi + telnet only, no web / MQTT): the
"ping only" state. The NSPanel build therefore sets `TELNET_STA 1` (telnet from the LAN, password required), so `reset` over
telnet gets it out without a power cycle.

## OHpanel TFT

### Flash it

```
nspanel.flash https://raw.githubusercontent.com/bruestel/ohpanel/ba9361982399414d1612aea85504d9707b329e75/boards/nspanel/nextion/display/ohpanel-nspanel-eu-display-bc629e1e.tft
```

15 821 480 bytes, sha256 `bdd5d022ac925cd2f42da392dd175acae3e808e888461a01f0c415435e7b5ec3` (OHpanel v0.9.2, EU landscape, Apache-2.0
design; fonts/icons under their own licences, `ohpanel-protocol.md` 8.3). HTTPS needs ~65 KB free heap; a copy on a LAN HTTP server
(plain `http://`, Range support) is faster and safer. At 921600 the UART does ~90 KB/s, the download is the bottleneck (several minutes).

After the upload the display installs the TFT and restarts, ESPurna detects it again (`display: auto`) and logs
`[NSPANEL] OHpanel TFT ready: design bc629e1e, model NX4832F035_011R, page 4` and `[NSPANEL] Display driver: ohpanel`.

**Custom TFT `d3731fbb`** (built from OHpanel with the grid pages `buttons33` / `buttons32` / `buttons22` / `buttons12`, only the
aurora background, 9 113 964 bytes; `ohpanel-protocol.md` section 13). Its source, the generator changes and a Windows-native build
(Nextion Editor driven by pywinauto) are in the fork [igorufox/ohpanel, branch `espurna-grids`](https://github.com/igorufox/ohpanel/tree/espurna-grids)
(`ESPURNA.md` there). Flash it straight from the fork:

```
nspanel.flash https://raw.githubusercontent.com/igorufox/ohpanel/espurna-grids/boards/nspanel/nextion/display/ohpanel-nspanel-eu-display-d3731fbb.tft
```

The driver knows both
designs (`nspanel.info`: `Design: custom ESPurna grids: layouts 4x2 3x3 3x2 2x2 1x2 (own pages), background aurora only; cards
split for the grid pages; background aurora`, or `Design: stock OHpanel v0.9.2: layouts 4x2 (stock tiles), 3x3 3x2 2x2 1x2 drawn,
all backgrounds; cards split for drawn grids; background dunes`; also `"design"` in `/nspanel-status`). With `d3731fbb`
`ohpanel.background` is always aurora (another one is logged: `Background 'candy' is not in design d3731fbb, aurora used`); the
colour overrides still apply. To go back from `d3731fbb` to the stock design (all backgrounds, drawn grids), flash the stock TFT
above (15 821 480 bytes). An unknown design is drawn as the stock one with the layouts on the 4x2 slots, and logged as unexpected.

### Roll back to Lovelace

```
nspanel.flash http://nspanel.pky.eu/lovelace-ui/github/nspanel-v4.1.1.tft
nspanel.flash http://nspanel.pky.eu/lovelace-ui/github/nspanel-v4.7.3.tft
```

(either one). The flasher finds the OHpanel display at 921600, the new TFT is detected as Lovelace after its restart.
With `"display": "ohpanel"` in the JSON, set it back to `auto` or `lovelace`.

### What it shows

Tile layout (`ohpanel.layout`, or the on-panel setting): `center` (default) = big centred icon (font 9, 48 px), the name below in one
line (font 0, 80 px / 20 bytes, `...` when longer); `corner` = icon top left and the name in font 1, a name that does not fit is
split into two lines in the small font. Tiles with a state line (`text`, `thermostat`, settings) always use the corner layout.

- **Home** (leftmost page): clock, short Russian date (`Вс, 25 окт`), current weather picture + outdoor temperature, today's low/high,
  room temperature (NTC, `23,4°`), up to 7 shortcut icons (`screensaver.shortcuts`), optional labels + state bars of the two hardware
  relays (`ohpanel.relays`), red wifi-off / MQTT-down icons.
- **Button pages**: every JSON page, 8 tiles per page (or what its `layout` holds); longer pages are split (`Title 1/2`). Tile on = white tile + accent icon, off =
  dark tile + dim icon (dark theme; light theme below). Switches, scenes and buttons have no state line, the tile colour shows the
  state (unknown = off look); a scene is on while any member is on; `button` tiles show active for 1 s after a tap (or while their
  state topic is on); `text` tiles show the state payload, thermostats `21,9° » 21,5°`. Page dots under the title.
- **List pages** (`"type": "list"`, OHpanel page `entities_rows`): 7 rows a sheet, each a card with the icon (accent when on,
  dim when off), the name and the state on the right: `Вкл` / `Выкл`, scenes `Вкл: n`, thermostats `21,9° » 21,5°`, text items
  the value, `open` items `»`; buttons have no state, their name gets the whole row. The name has 280 px (51 B) next to a short
  state (≤ 92 px), 214 px (39 B) next to a long one (≤ 158 px), 372 px (67 B) without one, `...` when longer. A tap on a row acts
  like a tap on the tile (thermostat: opens its page). Longer lists are split into sheets (`Title 1/2`) in the swipe row with
  page dots, like split button pages. The rows have no touch areas in the TFT design: the driver turns on Nextion `sendxy=1`
  on this page only and maps the touch-down point to a row (`y` 44 + 36·(row-1), 32 px high); a touch that moves more than
  40 px is not a tap.
- **Grid layouts** (`"layout"`, table in [Screens per page](#schema)): on the custom TFT each layout has its own page with bigger
  tiles and fonts; in corner layout switches and scenes there also show their state (`Вкл` / `Выкл` / `Вкл: n`) above or beside
  the name, and the name is one line in the page's font (smaller font, then `...`): `3x3` / `3x2` 116 px (font 1, ~10 Cyrillic
  letters), `2x2` 189 px (font 2, ~15), `1x2` 340 px (font 3, ~24; icon on the left, state and name beside it). Center layout:
  the icon and the label under it (font 1 / 1 / 2 / 3). On the stock TFT the same layouts are drawn, see below.
- <a id="drawn-tiles"></a>**Drawn tiles** (stock TFT: `3x3` / `3x2` / `2x2` / `1x2`; any page with `"draw": true`): the page is the
  stock `buttons` with its tile objects hidden; title, close, page dots and swipe stay. The driver draws each tile as a solid
  rounded box (two `fill` bars + 4 `cirs` corners, radius 14) in the tile colour (dark theme: off 10597, on white; light theme
  the other way round; heating thermostat on with an orange icon and state), then the icon and texts with `xstr` onto it
  (no background, any background picture shows only in the corners and gaps). The grid fills x 12..438 (like the stock tiles; the right ~30 px of the screen are under the frame), y 46..278 with gaps 9 / 8:
  `4x2` 99x112, `3x3` 136x72, `3x2` 136x112, `2x2` 208x112, `1x2` 426x112. Fonts: `4x2` / `3x3` / `3x2` name font 1 (smaller 0),
  state font 0, icon 32 px (center layout 48 px except `3x3`); `2x2` name font 2 (1), state font 1, `1x2` name font 3 (2), state
  font 2, icon 48 px. Corner layout: icon top left, the state line beside it (`3x3`, `2x2`, `1x2`) or above the name (`4x2`, `3x2`),
  a name that does not fit is split as on the stock tiles. Taps come as `sendxy` coordinates (as on list pages, `sendxy=0` again on
  every other page): the touch-down point picks the tile, gaps and the title row do nothing, a touch that moves more than 40 px is
  not a tap, swipes work as everywhere. A state change redraws only that tile (9 commands, ~310 B for a `3x3` thermostat);
  a theme / background change redraws the page. A `3x3` page with 9 tiles is one burst of ~95 commands / ~3 KB (synced every
  512 B), `2x2` ~50 commands / ~1.4 KB. `nspanel.info` shows the last page draw and update (`Drawn: ...`).
- **Weather**: one sheet per day (up to 7, `ohpanel.weather_days`): title `Сегодня · 25 окт`, picture, high/low, chips wind (m/s),
  precipitation probability, UV index.
- **Info lines** on home (`screensaver.info`, up to 2, OHpanel `info1_*` / `info2_*`): icon, `<text>  <value>` and the extra value on
  the right, e.g. `Улица  8.3°   82%`; updated on every state message.
- **Settings** (`"open": "settings"` on a shortcut or tile, or a `"type": "settings"` page): tile `Иконки` (`Крупные` = center /
  `Мелкие` = corner), tile `Тема` (`Тёмная` / `Светлая`) and tile `Фон` (the next background of the design, stock TFT only). A tap
  switches, redraws at once and stores the choice in the ESPurna settings `nspIcons` (`center` / `corner`), `nspTheme` (`dark` /
  `light`) and `nspBg` (`aurora` / `black` / `candy` / `dunes` / `forest`); they survive reboots and JSON reloads and win over
  `ohpanel.layout` / `ohpanel.theme` / `ohpanel.background` (the JSON defaults). `del nspIcons` / `del nspTheme` / `del nspBg` in
  the terminal returns to the JSON.
  Theme: every OHpanel background is dark (manifest: text `#ffffff` / `#fff8ee` on all five) and a tile has only a white and a dark
  picture, so **light** keeps the background and inverts the tiles: off = white tile, grey icon, dark text; on = dark tile, accent
  icon, white text. **dark** is OHpanel's look.
- **Thermostat** page: see [Thermostat](#thermostat).

Navigation: swipe left = next page, swipe right = previous page (home from the first button page, no wrap), swipe up/down = home
(on home: the weather days), cross = home (on a [nested page](#nested-pages): back to where it was opened from), tap on the home weather picture = weather days. On weather: left/right = next/previous
day, right on the first day = home. Tile and shortcut taps act like the Lovelace driver (switch toggles, button sends `on`,
scene applies its action, `open` navigates). A tap that ends a swipe is ignored (300 ms window, releases wait 60 ms for a swipe).

Idle: after `screensaver.timeout` s without a touch the panel goes home at `screensaver.dim` brightness; `dim: 0` = black page,
the next tap only wakes it. Hardware buttons do not wake it.

### JSON additions

```jsonc
{
  "display": "auto",
  "screensaver": {
    "timeout": 60, "brightness": 100, "dim": 10, "weather": { "lat": 52.52, "lon": 13.40 },
    "shortcuts": [                                   // up to 7, icon only
      "Кухня",                                       // name of an existing tile
      { "ref": "Все выкл" },                         // the same
      { "icon": "gate", "topic": "ESPURNA-GATE/relay/0", "type": "button" },          // a full item
      { "icon": "lightbulb-group-off", "type": "scene", "group": "house", "action": "off" },
      { "icon": "weather-partly-cloudy", "open": "weather" }
    ]
  },
  "ohpanel": {
    "background": "aurora",                          // aurora | black | candy | dunes | forest (custom TFT d3731fbb: aurora only)
    "accent": "#4fc3f7", "text": "#ffffff", "dim": "#c8ccc8",   // optional theme overrides
    "weather_days": 7,                               // 0..7 sheets on the weather page
    "relays": [ "Свет", "Вентилятор" ],              // labels over the hardware buttons on home (optional)
    "layout": "center",                              // center | corner (default center; setting nspIcons wins)
    "theme": "dark"                                  // dark | light (default dark; setting nspTheme wins)
  }
}
```

Info lines (in `screensaver`):

```jsonc
"info": [
  { "icon": "thermometer", "text": "Улица", "topic": "zigbee2mqtt/Thermometer Улица", "path": ["temperature"], "format": "%.1f°",
    "extra": { "path": ["humidity"], "format": "%.0f%%" } }       // extra: same topic unless it has its own "topic"
]
```

`format`: printf with exactly one float conversion (`%f` / `%e` / `%g` with flags, width, precision) and `%%`; anything else is
ignored with a warning and the raw value is shown. Unknown value: `--`.

The Lovelace driver ignores `ohpanel` and `shortcuts`; the OHpanel driver ignores `colors` and `font`.

### Text limits (OHpanel TFT, bytes; the TFT cuts silently, even inside a UTF-8 letter)

| Field | Limit | Practical |
|---|---|---|
| tile name | 18 B, 82 px (font 18 px) | ~7 Cyrillic letters, ~9 Latin; longer names are cut at a letter boundary with `...` (U+2026 is not in the TFT fonts) |
| tile name, center layout | 20 B, 80 px (font 0, 16 px) | one line, ~8 Cyrillic letters |
| info line text / right | 31 B, 144 px / 5 B, 52 px (font 1) | `Улица  8.3°` / `82%` |
| thermostat current / target | 10 B, 170 px (font 6) / 32 B, 176 px (font 2) | ` 21,9°` / `Цель 21,5°` |
| tile state line | 21 B, 82 px | |
| page title | 46 B, 300 px | `Title 1/2` included |
| home date | 26 B, 165 px | `Вс, 25 окт` |
| shortcut / tile icon | 10 B | one glyph |
| list row name | 51 B, 280 px / 39 B, 214 px / 67 B, 372 px (font 2) | by the state width: short (≤ 92 px), long (≤ 158 px), none |

Widths are estimated from Ubuntu font metrics (slightly generous), not from the TFT's manifest. Check long names on the panel.

## Display detection

`display: auto` (default): at boot, after every TFT upload, after `nspanel.detect`, when `display` changes and when the active
driver loses the display, ESPurna probes:

1. OHpanel: UART2 at 921600, `FF FF FF sendme`, `get design_ver` (600 ms). A `66` / `71` answer = OHpanel; then `connect` for the model,
   theme, home page.
2. Lovelace: UART2 at 115200, raw `recmod=1`, `pageType~pageStartup` (2 s). `event,startup,<ver>,eu` = Lovelace.
3. Repeat. No answer for 30 s: GPIO4 power-cycles the display once (not within 3 min after a TFT upload).

`ohpanel` / `lovelace` probe only that driver. Lost display: OHpanel pings with `sendme` after 10 s of silence (2 misses) and counts
flow-control timeouts (3 in a row); a TFT restart (`00 00 00` / `88`) re-sends theme and page. Lovelace: no startup answer for 30 s
after a TFT restart, or ≥ 32 bytes outside 55 BB frames (another TFT talking at 921600).

## What the panel shows (Lovelace TFT)

- Screensaver: time (ESPurna NTP / local time zone), date in Russian (`Суббота, 24 октября 2026`, shortened to fit 40 bytes),
  Open-Meteo weather: current temperature + icon, 4 forecast slots (today + 3 days, `Пн 12°/5°`). With the NTC enabled and
  `temperature.show`, the second main value (alternative layout) is the room temperature. Top-left status icon: wifi-off (no WiFi)
  or alert (no MQTT).
- Tap the screensaver → first card. Idle `timeout` seconds → screensaver.
- Weather: `https://api.open-meteo.com/v1/forecast` (no key), every `refresh` seconds (retry after 60 s on error), fetched in a separate
  FreeRTOS task (TLS without certificate check), so the loop and the web server are not blocked.

## MQTT

- Subscribes to every state topic of items, groups, thermostats and info lines (raw, absolute, each topic once), again on every
  reconnect, paced from `loop()` (2 per 20 ms; never from the MQTT callback).
- JSON payloads (items with `path`, thermostats, info lines): the MQTT callback only keeps the latest payload per topic (up to 8
  topics); `loop()` parses one per call with ArduinoJson in place (no copy of the strings) and updates every entity of that topic.
- Publishes commands to `<topic>/set` with `mqtt.on` / `mqtt.off` / `mqtt.toggle`, not retained.
- `<panel root>/nspanel`: the driver when it is detected (again on every MQTT connect), `{"driver":"ohpanel","version":"bc629e1e","model":"NX4832F035_011R"}`
  or `{"driver":"lovelace","version":"53","model":"eu"}`, and every TFT event as received: Lovelace `event,buttonPress2,i3,button`,
  OHpanel `{"page": "buttons", "component": "tile3", "value": "release"}`.
- `<panel root>/temperature`: room temperature, published by the ESPurna sensor module like on any ESPurna device (every
  `snsRead` x `snsReport` seconds, 6 x 10 by default; units `tmpUnits0`, °C by default; correction `tmpCorrection0`).

## WebUI screen mirror (OHpanel)

The NSPANEL section draws the display in a `<canvas>` (`code/html/src/nspanel-screen.mjs`, a small Nextion emulator) from
the commands the firmware sends to the TFT, and sends touches back as the TFT's own events.

- **Start**: WS action `nspanelScreen`. With the OHpanel driver (and no TFT upload running) the firmware
  (`nspanel_ohpanel.h`, `mirror::`) sends `#reset`, the theme (`th_*`, `bg_pic`, `bg_close`), `sendxy` and the current screen
  in full (`showCurrent()`, the wall redraws too), then every command `write()` puts on the UART. Otherwise it answers
  `#unsupported` (`#offline` when the TFT does not answer).
- **Stream**: `{"nspanelTft": "<command>\n<command>\n..."}` to every WS client, `sendme` left out. Flushed at 768 bytes or
  40 ms, one message in flight; a buffer over 4 KB, or a message the WS queue dropped (no callback within 3 s), is thrown
  away and the screen sent again in full. The mirror stops when no WS client is connected.
- **Touches**: `nspanelTouch` `{"page", "component", "value"}` goes to `ohpanel::onEvent()` like a `92 nspanelevent` frame
  (hotspot `press` / `release`, `swipe` left / right / up / down for a drag of 100 px or more, `wake` on `blank`);
  `nspanelXY` `{"x", "y", "press"}` goes to `onTouchXY()` like a `67` frame, on pages with `sendxy=1` (list rows, drawn grids).
  The firmware's own tap / swipe disambiguation applies; the buzzer beeps as for a finger.
- **Emulator**: one 480x320 canvas painted as the TFT paints: `page` loads the page's objects with what the page itself does
  on load (OHpanel `ohpanel_hmi.py`: `vis 255,0` and the title / close cross, theme colours from `th_*`), an attribute
  (`.txt` `.pco` `.bco` `.pic` `.val` `.font`) repaints that object with the background under its box first (crop / solid
  `sta`), `vis` shows or hides objects and the hotspots that have no object, `fill` `cirs` `cir` `line` `xstr` `picq` paint
  over what is there, `dim` dims the canvas. Pictures by id through the display manifest's per-background tables (tiles,
  close cross, weather icons; cards and badges as rounded boxes), converted to RGB565 like the TFT. Fonts: the TrueType
  sources of the TFT fonts (Ubuntu, Noto Sans, Material Design Icons, display code point + 0xE0001) at the em sizes and
  baselines of `fonts.py`. The web UI crops the rightmost 30 px, hidden by the panel's frame.
- **Assets**: `www/layout.json` and `boards/nspanel/nextion/display-manifest.json` of OHpanel commit `ba936198` (the stock
  `bc629e1e` TFT) and its fonts, from `cdn.jsdelivr.net/gh/bruestel/ohpanel@<commit>/` (CORS open); nothing in the firmware.
- **Not emulated**: the ring drag (`91` value reports), the TFT's own touch feedback colours, scrolling texts, the light page's
  colour wheel.

## Terminal

| Command | |
|---|---|
| `nspanel.info` | active driver + setting, TFT version/design and model, detection state, screen, card, counters (OHpanel: syncs, sync timeouts, overflows), config summary and errors, UI settings and JSON payload counters, FS, weather, room temperature, boot partitions, flasher result, last 8 events |
| `nspanel.page <index / home / weather / settings>` | show a page |
| `nspanel.detect` | detect the display again |
| `nspanel.reload` | re-read `/nspanel.json` |
| `nspanel.send <payload>` | queue a raw command for the active driver. Lovelace: `nspanel.send pageType~pageStartup`; OHpanel: a Nextion command, FF FF FF is added, e.g. `nspanel.send dim=50` |
| `nspanel.flash <url> [baud]` | upload a Nextion TFT file to the display (see below). `baud` 921600 (default), 512000, 256000, 250000, 230400 or 115200 |
| `safeboot` | set the boot partition to factory (Tasmota safeboot) and restart |

## Display (TFT) firmware update

`nspanel.flash` uploads any Nextion TFT file (Lovelace or OHpanel, see [OHpanel TFT](#ohpanel-tft) for its URL); upload protocol v1.2
with v1.1 fallback, details and sources in `lovelace-protocol.md` §8a. Latest Lovelace UI TFT for the EU panel (protocol 53,
v4.7.3 = v4.7.4, same file):

`nspanel.flash http://nspanel.pky.eu/lovelace-ui/github/nspanel-v4.7.3.tft`

8 373 960 bytes, MD5 `cbcaf903f9e6d07944353498fefeba6a`, plain HTTP with Range support. HTTPS works too but needs ~65 KB free heap
(TLS); prefer `http://`. Rollback to the June 2023 TFT (protocol 51): `.../nspanel-v4.1.1.tft` (8 261 216 bytes).

What happens:

1. A separate FreeRTOS task downloads the file (HTTPClient, redirects followed, `Content-Length` required, one 4 KB buffer).
   While it runs, the normal TX queue and RX parser leave UART2 alone, and `nspanel.send` is refused.
2. Handshake: `DRAKJHSUYDGBNCJHGJKSHBDN` (leave reparse mode), `recmod=0`, `connect`, expect `comok …`. Tries 921600 (OHpanel),
   115200 (Lovelace), then 9600. If the display does not answer, it is power-cycled through GPIO4 (up to 2 times) and asked again.
   The handshake needs only the Nextion OS, so it also works on a display left in its bootloader by a failed upload.
   After `comok`: 50 ms, a throw-away `sendme` (the display may swallow the command right after `connect`), RX drained for 250 ms.
   Every byte received during the handshake is logged in hex (`Flash: handshake RX at 115200 baud: 23 bytes received:` + hex lines).
3. RX drained again, `whmi-wris <size>,<baud>,res0`, UART switched to `<baud>`, wait up to **30 s** for `05` (the bytes received
   meanwhile are logged in hex). Then 4096-byte blocks, each answered by `05`, or by `08` + 4-byte offset (v1.2: the display skips
   data it already has; the file is re-opened with an HTTP Range request). Timeouts: 5 s per block, 10 s for the first reply /
   offset bytes, 15 s HTTP stall with 3 reconnects (Range) per block.
4. If the display does not accept or stops answering, it is power-cycled and the next variant is tried, each with a new handshake:
   `whmi-wris <size>,<baud>,res0` → `whmi-wris <size>,115200,res0` → `whmi-wri <size>,115200,res0` (v1.1, `05` per block only) →
   `whmi-wris <size>,115200,1` → `whmi-wri <size>,115200,1`. `nspanel.info` shows the last command used (`Flash: last command …`).
5. Progress goes to the log every 5 % (`[NSPANEL] Flash: 35% (2932736 / 8373960 bytes, 921600 baud, 31 s)`). `nspanel.info` shows
   phase / percent / speed while it runs, and afterwards the last result and the display's `comok` line (model, flash size).
6. Done: the display installs the TFT and restarts by itself. ESPurna detects the display again (OHpanel at 921600 or Lovelace at
   115200, see [Display detection](#display-detection)) and renders. No raw `recmod=1` is sent at 921600 (it would break OHpanel's
   command mode). Failed: the error is logged and the display is power-cycled if the upload had started. Run the same command again.

Duration at 921600: about 2-3 minutes (~92 KB/s on the wire). At 115200: about 13 minutes.

Risks:

- A broken upload leaves the display without a TFT (it shows a Nextion error screen). This is not permanent: rerun `nspanel.flash`.
  If the display never answers `comok` again, power-cycle the whole panel and retry, with `115200` as baud.
- Do not cut power and do not OTA/restart ESPurna during the upload. MQTT, the web UI and relays keep working meanwhile.
- Only EU TFTs (`nspanel-v*.tft`). The `us-l` / `us-p` files have a different layout.
- After the update the TFT reports protocol 53. Grid, screensaver, `dimmode` (4 fields; the optional 5th `featNewSliders` field
  is not needed without popups) and the events used here are unchanged from 51. `renderCurrentPage` re-renders the current card.

## Flash layout, install and update

Every ESP32 image of this fork, the panel's `esp32-matter-nspanel` included, uses the safeboot layout
(`code/esp32_4mb_safeboot.csv`, Tasmota's offsets): `nvs`, `otadata`, `safeboot` (factory, `0x10000`, 832 KB), `app0`
(ota_0, `0xE0000`, 2880 KB), `spiffs`, `coredump`. There is no second OTA slot, so ESPurna cannot update itself in place:

- ESPurna web `/upgrade` answers with an error ("...run `safeboot`...") on this layout; ArduinoOTA and the terminal `ota`
  client are disabled in the NSPanel build. The *From GitHub* release list of the WebUI needs ESPurna's `/safeboot`
  endpoints, which the NSPanel image does not have (its module keeps its own `safeboot` command and crash-loop guard).
- Updates: terminal (telnet / web debug) `safeboot` → the panel reboots into the image of the factory partition → its
  web UI → the new `espurna-esp32-matter-nspanel-ota.bin` (built: `code/.pio/build/esp32-nspanel/firmware.bin`, the app
  image only, **not** the merged `-full.bin`).

### Serial flash (recommended)

Flash the merged image (`espurna-esp32-matter-nspanel-full.bin` of the release, or `espurna-esp32-nspanel.bin` of a
build: bootloader, partition table, the ESPurna safeboot, ESPurna) once over the J1 header (3V3, ESP_TX, ESP_RX, GND, IO0;
hold IO0 low at power-up). The display is powered from the 5 V of the relay board, so it stays dark on a USB-UART adapter
alone. Example:

`esptool.py --chip esp32 --port COMx --baud 460800 write_flash --erase-all -z 0x0 espurna-esp32-matter-nspanel-full.bin`

Then configure WiFi from the ESPurna AP, set MQTT and NTP, upload `/nspanel.json`. The factory partition holds the ESPurna
safeboot (`code/safeboot-app`): after `safeboot` it joins WiFi with the ESPurna settings, its page (user `admin`, the
admin password) lists the releases and takes an upload; it boots ESPurna when done, or after 15 minutes of nothing.

### Over Tasmota (untested)

A panel with Tasmota already has the same layout with the Tasmota safeboot in the factory partition.

Install from Tasmota (first time):
1. In the Tasmota console, check the TFT version first: `CustomSend pageType~pageStartup`, expect `{"CustomRecv":"event,startup,51,eu"}`.
2. Tasmota web UI → Firmware Upgrade → upload `espurna-esp32-matter-nspanel-ota.bin`. Tasmota restarts into safeboot, writes
   the image into `app0`, boots it.
3. ESPurna starts its AP (no WiFi credentials yet): connect, configure WiFi, MQTT, NTP time zone, password.
4. Open `http://<panel>/nspanel`, paste `nspanel.json`, Save.

Update later: `safeboot` → the panel reboots into Tasmota safeboot (it uses Tasmota's own WiFi settings, still in NVS
unless NVS was erased) → its web UI → upload the new `-ota.bin`.

Crash-loop guard: when the panel boots 3 times in a row with a crash reset (panic / watchdog, or a software restart ESPurna did not
request itself) without reaching 60 s uptime, it switches the boot partition to safeboot and restarts. The counter lives in RTC memory
and is cleared by a power-on reset. Deliberate ESPurna restarts (web, terminal, MQTT, OTA...) are not counted.

## Status

Tested on one NSPanel EU, serial-flashed with the ESPurna layout:

- OHpanel driver on the stock TFT `bc629e1e` and on the custom TFT `d3731fbb`: detection, home, swipes, all grid layouts (drawn and
  own pages), list pages, nested pages, thermostats with Istabai data, settings (icons, theme, background), hardware buttons with
  groups, web OTA.
- `nspanel.flash`: Lovelace → OHpanel and OHpanel → custom TFT at 921600 (15.8 MB in about 6 minutes from GitHub over HTTPS).
- Lovelace driver: TFT v4.1.1 reports `event,startup,51,eu`, screensaver (clock, Russian date, Open-Meteo weather and forecast,
  room temperature), 8-tile cards built from the JSON, MQTT subscriptions.

Not verified yet:

- TFT versions other than 51: 50/52/53 accepted, `cardGrid2` only with >= 51, `renderCurrentPage` (late 53) re-renders the card,
  `font` (53) is untested.
- The Tasmota safeboot install path, `safeboot` and the crash-loop guard.
- NTC constants are from the Tasmota template and ESPHome configs, not measured; the sensor sits near the ESP and reads high —
  set the temperature correction in the web UI (Sensors, `tmpCorrection0`). On ESP32 the ESPurna analog sensors read the pin
  with `analogReadMilliVolts` (11 dB attenuation). `temperature.offset` and `temperature.publish` of older configs are
  ignored with a warning.
- Icon codepoints: taken from the 2023-06 `icons.json`; glyphs added to MDI after the TFT font was generated may render blank.
- Byte limits: names are cut at 30 bytes, headings and the date at 40, forecast values at 10 (assumed bytes, not characters).
  Long Cyrillic names (> 15 letters) are truncated.
- Frame pacing: 75 ms between frames, frames of the same kind are coalesced in the queue.
- Room temperature in the screensaver's alternative layout (6th weather group): layout and field size unverified.
- NVS is shared with Tasmota's leftovers (20 KB); ESPurna settings need ~4 KB.
