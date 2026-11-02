/*

NSPANEL MODULE - OHpanel TFT driver

Drives the OHpanel display design (EU TFT `bc629e1e`, OHpanel v0.9.2; `4508cd3f` from v0.10.0-rc1 is
the same design; `d3731fbb` is our custom build of it with the grid pages buttons33 / buttons32 /
buttons22 / buttons12 and only the aurora background, ohpanel-protocol.md section 13; on the stock
design the 3x3 / 3x2 / 2x2 / 1x2 grids are drawn at runtime on `buttons`, section 14)
with plain Nextion commands terminated by FF FF FF, 921600 8N1. The TFT answers with
FF FF FF terminated frames: 66 (sendme), 71 (get <int>), comok, 92 "nspanelevent" JSON events,
00 00 00 / 88 (TFT started). Protocol and object names: nspanel/ohpanel-protocol.md

Included by nspanel.cpp inside namespace espurna::nspanel::{anonymous}, after the shared core
(config, entities, weather, ntc, buzzer, cards). Not a standalone header.

Portions derived from OHpanel (https://github.com/bruestel/ohpanel, commit ba93619), Copyright
Jonas Brüstel, Apache License 2.0; modified for ESPurna: the background picture id table, the
object and event names of the TFT design, the weather picture order and the timing constants
(flow control, swipe/release windows, busy indicator, link supervision). The code itself is
written for ESPurna. A copy of the Apache License 2.0 is at https://www.apache.org/licenses/LICENSE-2.0

*/

#pragma once

namespace ohpanel {

constexpr uint32_t Baudrate { build::OhpanelBaud };
constexpr char Design[] = "bc629e1e";
constexpr char DesignRc1[] = "4508cd3f";
constexpr char DesignCustom[] = "d3731fbb";

// the TFT designs this driver knows (`get design_ver`), what each one has
struct DesignInfo {
    const char* id;
    const char* name;
    bool grids;        // the grid pages buttons33 / buttons32 / buttons22 / buttons12 (ids 18..21); else
                       // those layouts are drawn on `buttons` (fill / cirs / xstr, namespace drawn)
    bool backgrounds;  // all five backgrounds; else only aurora (Backgrounds[0], picture ids 0..188)
};

constexpr DesignInfo Designs[] {
    {Design, "stock OHpanel v0.9.2", false, true},
    {DesignRc1, "stock OHpanel v0.10.0-rc1", false, true},
    {DesignCustom, "custom ESPurna grids", true, false},
};

constexpr size_t FrameMax { 1024 };
constexpr size_t QueueMax { 320 };            // commands
constexpr size_t CommandMax { 1000 };         // bytes, must fit the UART TX buffer
constexpr size_t SyncBytes { 512 };           // sendme after this many bytes
constexpr size_t TxBudget { 1024 };           // bytes written per loop
constexpr uint32_t SyncTimeoutMs { 500 };
constexpr uint8_t SyncMissesMax { 3 };
constexpr uint32_t PingMs { 10000 };
constexpr uint8_t PingMissesMax { 2 };
constexpr uint32_t SearchRetryMs { 2000 };
constexpr uint32_t SearchLostMs { 10000 };    // after a TFT reboot / lost link: give up, link:: detects again
constexpr uint32_t IdentifyRetryMs { 1000 };
constexpr uint32_t IdentifyMaxMs { 3000 };
constexpr uint32_t RebootQuietMs { 500 };
constexpr uint32_t RebootMaxMs { 5000 };
constexpr uint32_t SwipeGuardMs { 300 };      // a release this soon after a swipe belongs to the swipe
constexpr uint32_t ReleaseHoldMs { 60 };      // a release waits this long for a swipe
constexpr uint32_t PulseMs { 1000 };          // `button` tiles show active after a tap
constexpr uint32_t RenderDelayMs { 100 };
constexpr uint32_t PageRetryMs { 1000 };
constexpr size_t TilesPerCard { 8 };
constexpr size_t ShortcutsMax { 7 };
constexpr size_t DotsMax { 7 };

namespace page {

constexpr int Home { 0 };
constexpr int Weather { 1 };
constexpr int Blank { 4 };
constexpr int Buttons { 7 };
constexpr int EntitiesRows { 12 };
constexpr int Thermostat { 17 };
constexpr int Buttons33 { 18 };   // custom design d3731fbb only
constexpr int Buttons32 { 19 };
constexpr int Buttons22 { 20 };
constexpr int Buttons12 { 21 };

} // namespace page

// text field limits: bytes (txt_maxl, the TFT cuts silently, even inside a UTF-8 sequence)
// and pixel width with the font's digit width (ohpanel-protocol.md 3.4 - 3.6)
struct Field {
    uint16_t bytes;
    uint8_t digit;   // px per digit of the field's font
    uint16_t width;  // px
};

constexpr Field Title { 46, 13, 300 };
constexpr Field TileName { 18, 10, 82 };
constexpr Field TileValue { 21, 8, 82 };
constexpr Field TileNameSmall { 18, 8, 82 };   // tile name field switched to font 0
constexpr Field TileCenterText { 20, 8, 80 };  // center layout: name under the icon, font 0, one line

// tile name/value fonts (ohpanel-protocol.md 3.1)
constexpr long TileNameFont { 1 };
constexpr long TileSmallFont { 0 };

// A page of tiles: the stock `buttons` (4x2) or a grid page of the custom design (igorufox/ohpanel, branch
// espurna-grids, docs/espurna-custom-layouts.md 3.3). Field limits from the manifest `fit` (font, px, bytes);
// digit widths of fonts 0..3: 8, 10, 11, 13 px
struct TilePage {
    Grid grid;
    int id;
    const char* name;      // page name in the events
    uint8_t tiles;
    uint16_t pic;          // tile N off = pic + 2 (N - 1), on = off + 1; 0: the background's tile table
    Field name_field;      // corner: the name
    Field small_field;     // corner: the name in the smaller font
    Field value_field;     // corner: the state line (or the first half of a long name)
    Field center_field;    // center: the label under the icon (_text)
    long name_font;
    long small_font;
    bool states;           // corner switch / scene tiles show their state line, names are not split
};

constexpr TilePage TilePages[] {
    {Grid::G4x2, page::Buttons, "buttons", 8, 0,
        TileName, TileNameSmall, TileValue, TileCenterText, TileNameFont, TileSmallFont, false},
    {Grid::G3x3, page::Buttons33, "buttons33", 9, 170,
        {25, 10, 116}, {25, 8, 116}, {21, 8, 82}, {25, 10, 116}, 1, 0, true},
    {Grid::G3x2, page::Buttons32, "buttons32", 6, 158,
        {25, 10, 116}, {25, 8, 116}, {29, 8, 116}, {25, 10, 116}, 1, 0, true},
    {Grid::G2x2, page::Buttons22, "buttons22", 4, 150,
        {34, 11, 189}, {34, 10, 189}, {40, 10, 189}, {34, 11, 189}, 2, 1, true},
    {Grid::G1x2, page::Buttons12, "buttons12", 2, 146,
        {53, 13, 340}, {53, 11, 340}, {62, 11, 340}, {63, 13, 406}, 3, 2, true},
};

constexpr size_t TilesMaxPage { 9 };

static_assert((TilePages[static_cast<size_t>(Grid::G3x3)].grid == Grid::G3x3)
    && (TilePages[static_cast<size_t>(Grid::G1x2)].grid == Grid::G1x2), "TilePages in Grid order");
constexpr Field Date { 26, 13, 165 };
constexpr Field Outdoor { 8, 25, 130 };
constexpr Field Row { 8, 13, 100 };
constexpr Field RelayText { 26, 10, 120 };
constexpr Field WeatherHigh { 25, 25, 300 };
constexpr Field WeatherLow { 42, 13, 270 };
constexpr Field ChipValue { 17, 11, 100 };
constexpr Field ChipText { 23, 8, 100 };
constexpr Field InfoText { 31, 10, 144 };      // home info line: x 44 .. 188, font 1
constexpr Field InfoRight { 5, 10, 52 };       // home info line right value: x 196, w 52, font 1
constexpr Field ThermoNow { 10, 37, 170 };     // thermostat current temperature, font 6
constexpr Field ThermoTarget { 32, 11, 176 };  // thermostat setpoint line, font 2
constexpr Field ThermoState { 32, 10, 122 };   // thermostat state line (xstr font 1 after the icon)

// entities_rows (id 12), font 2 (digit 11 px), manifest `fit`: a row has the label with the wide value
// column (_text + _value), the long label with the narrow value (_longtext + _shortval), or one wide field
constexpr Field RowText { 39, 11, 214 };
constexpr Field RowValue { 29, 11, 158 };
constexpr Field RowLongText { 51, 11, 280 };
constexpr Field RowShortVal { 24, 11, 92 };
constexpr Field RowWide { 67, 11, 372 };
constexpr size_t RowsPerSheet { 7 };           // ENTITY_ROWS_N
constexpr int RowsTop { 44 };                  // ENTITY_ROWS_Y: row r (1..7) is y 44 + 36 (r - 1), 32 px high
constexpr int RowsStep { 36 };

// fixed colours of the renderer (RGB565)
namespace color {

constexpr uint16_t White { 65535 };
constexpr uint16_t DarkGrey { 10597 };   // off tile
constexpr uint16_t LightGrey { 52857 };
constexpr uint16_t StateOn { 27469 };
constexpr uint16_t StateOff { 40147 };
constexpr uint16_t Red { 63488 };
constexpr uint16_t Grey { 33840 };       // #848484: inactive icon / state on light tiles
constexpr uint16_t Track { 19017 };      // thermostat ring above the setpoint
constexpr uint16_t Button { 14855 };     // thermostat minus / plus disc
constexpr uint16_t Heat { 64615 };       // #ff8a3d, heating

} // namespace color

constexpr uint16_t rgb565(uint32_t rgb) {
    return static_cast<uint16_t>((((rgb >> 19) & 0x1F) << 11) | (((rgb >> 10) & 0x3F) << 5) | ((rgb >> 3) & 0x1F));
}

// one per background (ohpanel-protocol.md 3.2); same order as BackgroundNames
struct Background {
    uint16_t pic;
    uint16_t close;
    uint16_t accent;
    uint16_t text;
    uint16_t dim;
    uint16_t tile;          // tile N (1..8) off = tile + tile_step * N, on = off + 1
    uint8_t tile_step;
    uint16_t chip;          // weather chip box n (1..6) = chip + chip_step * (n - 1)
    uint8_t chip_step;
    uint16_t weather_home;  // + weather picture index
    uint16_t weather_page;
    uint16_t rowcard;       // entities_rows card of row r (1..7) = rowcard + rowcard_step * (r - 1)
    uint8_t rowcard_step;
};

constexpr Background Backgrounds[] {
    {48, 45, rgb565(0x4fc3f7), 65535, rgb565(0xc8ccc8), 19, 2, 67, 1, 73, 109, 53, 1},                    // aurora
    {162, 160, rgb565(0x0080f8), 65535, rgb565(0xc8ccc8), 157, 0, 169, 0, 170, 206, 165, 0},              // black
    {291, 288, rgb565(0xff5fa8), 65535, rgb565(0xd0c4e0), 262, 2, 310, 1, 316, 352, 296, 1},              // candy
    {437, 434, rgb565(0xffb74d), rgb565(0xfff8ee), rgb565(0xd8c8b0), 408, 2, 456, 1, 462, 498, 442, 1},   // dunes
    {584, 580, rgb565(0x8bd17c), 65535, rgb565(0xc0d0c0), 554, 2, 603, 1, 609, 645, 589, 1},              // forest
};

static_assert((sizeof(Backgrounds) / sizeof(Backgrounds[0])) == (sizeof(BackgroundNames) / sizeof(BackgroundNames[0])),
    "one background per name");

const DesignInfo* design();

// the configured background; a design with only aurora (d3731fbb has no pictures for the others) gets aurora
size_t backgroundIndex() {
    const auto* info = design();
    if (info && !info->backgrounds) {
        return 0;
    }
    return (config.background < (sizeof(Backgrounds) / sizeof(Backgrounds[0]))) ? config.background : 0;
}

const Background& background() {
    return Backgrounds[backgroundIndex()];
}

uint16_t accent() {
    return (config.accent >= 0) ? config.accent : background().accent;
}

uint16_t textColor() {
    return (config.text_color >= 0) ? config.text_color : background().text;
}

uint16_t dimColor() {
    return (config.dim_color >= 0) ? config.dim_color : background().dim;
}

// picture of tile slot n (1..) of a page: the grid pages have their own pictures (aurora only),
// the stock `buttons` page the background's
uint16_t tilePic(const TilePage& page, size_t n, bool on) {
    if (page.pic) {
        return page.pic + 2 * (n - 1) + (on ? 1 : 0);
    }
    const auto& bg = background();
    return bg.tile + (bg.tile_step * n) + (on ? 1 : 0);
}

uint16_t rowPic(size_t n) {
    const auto& bg = background();
    return bg.rowcard + (bg.rowcard_step * (n - 1));
}

uint16_t chipPic(size_t n) {
    const auto& bg = background();
    return bg.chip + (bg.chip_step * (n - 1));
}

// index into the alphabetical list of the 36 Meteocons pictures (same order in every background):
// clear-day 0, clear-night 1, drizzle 3, fog-day 7, fog-night 8, not-available 13, overcast-day 15,
// overcast-night 16, partly-cloudy-day 17, partly-cloudy-day-rain 19, partly-cloudy-day-snow 21,
// partly-cloudy-night 22, partly-cloudy-night-rain 23, partly-cloudy-night-snow 24, rain 25, sleet 26,
// snow 28, thunderstorms-day-rain 31, thunderstorms-night-rain 32, thunderstorms-rain 33
uint8_t weatherPicture(int code, bool day) {
    switch (code) {
    case 0:
        return day ? 0 : 1;
    case 1:
    case 2:
        return day ? 17 : 22;
    case 3:
        return day ? 15 : 16;
    case 45:
    case 48:
        return day ? 7 : 8;
    case 51:
    case 53:
    case 55:
        return 3;
    case 56:
    case 57:
    case 66:
    case 67:
        return 26;
    case 61:
    case 63:
    case 65:
    case 82:
        return 25;
    case 71:
    case 73:
    case 75:
    case 77:
    case 86:
        return 28;
    case 80:
    case 81:
        return day ? 19 : 23;
    case 85:
        return day ? 21 : 24;
    case 95:
        return day ? 31 : 32;
    case 96:
    case 99:
        return 33;
    }

    return 13;
}

// -----------------------------------------------------------------------------
// Text fitting. Approximate Ubuntu Regular advances in 1/100 em; a digit is 56.
// The TFT has the exact widths in its manifest; these are a little generous (Scale)
// so that a fitted text does not get clipped.
// -----------------------------------------------------------------------------

constexpr uint8_t AsciiAdvance[95] {
    23, 25, 37, 63, 56, 85, 70, 21, 31, 31, 47, 56, 22, 33, 22, 39,   //  !"#$%&'()*+,-./
    56, 56, 56, 56, 56, 56, 56, 56, 56, 56,                           // 0-9
    22, 22, 56, 56, 56, 43, 90,                                       // :;<=>?@
    66, 65, 62, 70, 57, 54, 68, 71, 27, 47, 63, 52, 82,               // A-M
    72, 76, 61, 76, 63, 54, 55, 69, 64, 92, 62, 59, 58,               // N-Z
    33, 39, 33, 56, 50, 30,                                           // [\]^_`
    54, 59, 47, 59, 55, 36, 59, 58, 26, 26, 51, 28, 87,               // a-m
    58, 59, 59, 59, 37, 44, 39, 58, 50, 77, 50, 50, 46,               // n-z
    35, 26, 35, 56,                                                   // {|}~
};

constexpr uint8_t CyrillicAdvance[64] {
    66, 63, 65, 52, 70, 57, 92, 56, 72, 72, 63, 68, 82, 71, 76, 71,   // А-П
    61, 62, 55, 60, 82, 62, 73, 64, 95, 97, 70, 86, 62, 63, 98, 63,   // Р-Я
    54, 58, 54, 43, 60, 55, 80, 48, 59, 59, 52, 57, 70, 58, 59, 58,   // а-п
    59, 47, 47, 50, 80, 50, 60, 54, 84, 86, 60, 76, 53, 49, 81, 54,   // р-я
};

constexpr uint32_t Scale { 53 };
constexpr uint8_t EllipsisAdvance { 66 };  // "..."

uint8_t advance(uint32_t cp) {
    if ((cp >= 0x20) && (cp <= 0x7E)) {
        return AsciiAdvance[cp - 0x20];
    }
    if ((cp >= 0x410) && (cp <= 0x44F)) {
        return CyrillicAdvance[cp - 0x410];
    }
    switch (cp) {
    case 0x401: // Ё
        return 57;
    case 0x451: // ё
        return 55;
    case 0xB0:  // °
        return 38;
    case 0xB7:  // ·
        return 22;
    case 0x2013:
        return 50;
    case 0x2026:
        return EllipsisAdvance;
    }
    return 60;
}

uint32_t nextCodepoint(const String& text, size_t& pos) {
    const auto lead = static_cast<uint8_t>(text[pos++]);
    size_t extra = 0;
    uint32_t cp = lead;
    if ((lead & 0xE0) == 0xC0) {
        cp = lead & 0x1F;
        extra = 1;
    } else if ((lead & 0xF0) == 0xE0) {
        cp = lead & 0x0F;
        extra = 2;
    } else if ((lead & 0xF8) == 0xF0) {
        cp = lead & 0x07;
        extra = 3;
    }

    while (extra-- && (pos < text.length()) && ((static_cast<uint8_t>(text[pos]) & 0xC0) == 0x80)) {
        cp = (cp << 6) | (static_cast<uint8_t>(text[pos++]) & 0x3F);
    }

    return cp;
}

// cut at UTF-8 boundaries to the field's byte limit and pixel width, with "..." when shortened (no U+2026 in the TFT fonts)
String fit(const String& text, const Field& field) {
    const uint32_t limit = (static_cast<uint32_t>(field.width) * Scale) / field.digit;

    uint32_t units = 0;
    size_t pos = 0;
    while (pos < text.length()) {
        units += advance(nextCodepoint(text, pos));
    }
    if ((text.length() <= field.bytes) && (units <= limit)) {
        return text;
    }

    units = 0;
    pos = 0;
    size_t keep = 0;
    while (pos < text.length()) {
        const uint32_t width = advance(nextCodepoint(text, pos));
        if (((units + width + EllipsisAdvance) > limit) || ((pos + 3) > field.bytes)) {
            break;
        }
        units += width;
        keep = pos;
    }

    String out = text.substring(0, keep);
    while (out.endsWith(" ")) {
        out.remove(out.length() - 1);
    }
    out += "...";  // U+2026 is not in the TFT text fonts
    return out;
}

bool fits(const String& text, const Field& field) {
    return fit(text, field) == text;
}

String temperatureText(float value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld\xC2\xB0", lroundf(value));
    return buf;
}

// "23.4°" / "23,4°"
String indoorText() {
    if (!config.temperature || !config.temperature_show || std::isnan(ntc::value)) {
        return String();
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", ntc::value);
    lang::decimal(buf);
    String out(buf);
    out += "\xC2\xB0";
    return out;
}

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------

enum class Link {
    Search,     // FF FF FF sendme + get design_ver until something answers at 921600
    Identify,   // get design_ver, connect, throw-away sendme
    Ready,
    Rebooted,   // 00 00 00 / 88 seen: wait until the TFT is quiet, then search again
};

const char* linkName(Link value) {
    switch (value) {
    case Link::Search:
        return "search";
    case Link::Identify:
        return "identify";
    case Link::Ready:
        return "ready";
    case Link::Rebooted:
        return "rebooted";
    }
    return "";
}

enum class Screen {
    None,
    Home,
    Buttons,
    Weather,
    Blank,
    Settings,    // built-in settings tiles (buttons page), when no "type": "settings" page exists
    Thermostat,  // thermostat page of one item
};

const char* screenName(Screen value) {
    switch (value) {
    case Screen::None:
        return "none";
    case Screen::Home:
        return "home";
    case Screen::Buttons:
        return "buttons";
    case Screen::Weather:
        return "weather";
    case Screen::Blank:
        return "blank";
    case Screen::Settings:
        return "settings";
    case Screen::Thermostat:
        return "thermostat";
    }
    return "";
}

struct State {
    Link link { Link::Search };
    uint32_t link_since { 0 };
    uint32_t last_search { 0 };
    bool was_ready { false };
    bool lost { false };

    // identification
    bool design_known { false };
    bool design_missing { false };  // 1A: no design_ver variable, not an OHpanel TFT
    String design;
    String model;
    String comok;
    uint32_t identify_sent { 0 };
    int tft_page { -1 };            // last page id from a 66 answer
    String tft_page_name;           // last currentpage event
    bool grids { false };           // the cards were split for full grids (a known design: own pages or drawn)
    bool pages { false };           // the design has the grid pages (DesignInfo::grids)

    // RX
    uint8_t rx[FrameMax];
    size_t rx_len { 0 };
    uint32_t last_rx { 0 };
    uint32_t reboot_at { 0 };
    uint32_t reboot_last { 0 };

    // TX: commands without the terminator; an empty string is a flow-control sync
    std::deque<String> queue;
    bool waiting { false };
    uint32_t wait_since { 0 };
    size_t since_sync { 0 };
    uint8_t sync_misses { 0 };
    uint32_t last_ping { 0 };
    uint8_t ping_misses { 0 };

    // attribute cache of the current page load ("obj.attr" -> value)
    std::map<String, String> cache;

    // UI
    Screen screen { Screen::None };
    size_t card { 0 };
    size_t day { 0 };
    std::vector<Card> cards;
    int requested_page { -1 };
    uint32_t requested_at { 0 };
    bool dirty { false };
    uint32_t dirty_since { 0 };
    int home_k { 0 };               // low/high variant shown on home
    int level { -1 };               // dim= sent last
    bool dimmed { false };
    bool asleep { false };
    uint32_t last_touch { 0 };
    uint32_t last_tick { 0 };

    // touch handling
    uint32_t last_swipe { 0 };
    bool release_pending { false };
    String release_component;
    uint32_t release_at { 0 };
    std::vector<uint32_t> pulse_until; // per item, `button` tiles
    uint32_t pulse_check { 0 };

    // list rows and drawn tiles have no hotspots: sendxy=1 on their pages, 67 frames give the touch coordinates
    int8_t sendxy { -1 };           // sent last, -1 unknown
    int touch_x { -1 };             // where the finger came down (67 press), -1 none
    int touch_y { -1 };

    // settings / thermostat pages return where they were opened from
    Screen back { Screen::Home };
    size_t back_card { 0 };

    // nested pages (an item's "page"): where each was opened from, the last on top (-1 home, else a card)
    std::vector<int> history;
    size_t thermo_item { 0 };       // item shown on the thermostat page
    int knob { -1 };                // ring bead under the knob, -1 none drawn
    String thermo_state;            // state line drawn last
    bool thermo_chart { false };    // the thermostat page shows the history chart instead of the dial
    bool chart_drawn { false };     // the chart (or "no history") is on the screen
    uint32_t chart_since { 0 };     // when the chart was opened (the retained message is waited for)

    // setpoint edited on the panel: shown at once, sent after the last tap (debounce)
    int edit_item { -1 };
    float edit_value { NAN };
    uint32_t edit_tap { 0 };
    bool edit_unsent { false };
    uint32_t edit_sent { 0 };

    // counters
    uint32_t rx_frames { 0 };
    uint32_t tx_commands { 0 };
    uint32_t tx_bytes { 0 };
    uint32_t syncs { 0 };
    uint32_t sync_timeouts { 0 };
    uint32_t sync_rtt_max { 0 };
    uint32_t overflows { 0 };       // 24: TFT input buffer overflow
    uint32_t errors { 0 };          // other Nextion status codes
    uint8_t last_error { 0 };
    uint32_t dropped { 0 };
    uint32_t events { 0 };
    uint32_t queued { 0 };          // commands / bytes (with the terminator) put into the queue
    uint32_t queued_bytes { 0 };

    // drawn tiles: what the last page draw and the last tile update cost
    uint16_t draw_page_commands { 0 };
    uint16_t draw_page_bytes { 0 };
    uint8_t draw_page_tiles { 0 };
    uint16_t draw_update_commands { 0 };
    uint16_t draw_update_bytes { 0 };
    uint8_t draw_update_tiles { 0 };
};

State state;

// the identified design, nullptr while unknown or not one of Designs (drawn as the stock design)
const DesignInfo* design() {
    if (!state.design_known) {
        return nullptr;
    }
    for (const auto& info : Designs) {
        if (state.design == info.id) {
            return &info;
        }
    }
    return nullptr;
}

bool designGrids() {
    const auto* info = design();
    return info && info->grids;
}

// "custom ESPurna grids: layouts 4x2 3x3 3x2 2x2 1x2 (own pages), background aurora only"
// "stock OHpanel v0.9.2: layouts 4x2 (stock tiles), 3x3 3x2 2x2 1x2 drawn, all backgrounds"
String designText() {
    const auto* info = design();
    if (!info) {
        return state.design_known ? String(F("unknown design: stock pages assumed, layouts on the 4x2 slots")) : String();
    }

    String out(info->name);
    out += F(": layouts 4x2");
    if (!info->grids) {
        out += F(" (stock tiles),");
    }
    for (size_t index = 0; index < (sizeof(GridNames) / sizeof(GridNames[0])); ++index) {
        if (static_cast<Grid>(index) != Grid::G4x2) {
            out += ' ';
            out += GridNames[index];
        }
    }
    out += info->grids ? F(" (own pages)") : F(" drawn");
    out += info->backgrounds ? F(", all backgrounds") : F(", background aurora only");
    return out;
}

// the page of a card's grid: its own page when the design has it, else the stock 4x2 `buttons`
const TilePage& tilePage(Grid grid) {
    return state.pages ? TilePages[static_cast<size_t>(grid)] : TilePages[0];
}

// tiles a card of the grid shows: all of them on a known design (own page or drawn); on an unknown
// design the layout's columns of the 4x2 slots (3x3 / 3x2: slots 1-3, 5-7; 2x2: 1, 2, 5, 6; 1x2: 1, 5)
size_t slotsOf(Grid grid) {
    return state.grids ? gridTiles(grid) : (2u * gridCols(grid));
}

// tile slot (1..) of the n-th tile (0-based) of a card
size_t tileSlot(size_t n, Grid grid) {
    if (state.grids) {
        return n + 1;
    }
    const size_t cols = gridCols(grid);
    return (n / cols) * 4 + (n % cols) + 1;
}

// the n-th tile (0-based) in slot 1.., -1 for a slot that holds none
int slotTile(long slot, Grid grid) {
    if ((slot < 1) || (slot > static_cast<long>(state.grids ? slotsOf(grid) : tilePage(grid).tiles))) {
        return -1;
    }
    if (state.grids) {
        return slot - 1;
    }
    const long cols = gridCols(grid);
    const long row = (slot - 1) / 4;
    const long col = (slot - 1) % 4;
    return (col < cols) ? static_cast<int>(row * cols + col) : -1;
}

// a grid card whose tiles are drawn at runtime on `buttons` (ohpanel-protocol.md 14): a known design without
// the layout's own page (stock: 3x3 / 3x2 / 2x2 / 1x2), or a page with "draw": true (also 4x2)
bool drawnCard(const Card& card) {
    if (!state.grids || card.list || card.settings || (card.page >= config.pages.size())) {
        return false;
    }
    return config.pages[card.page].draw || (!state.pages && (card.grid != Grid::G4x2));
}

// -----------------------------------------------------------------------------
// Transport
// -----------------------------------------------------------------------------

// nspanel.capture: the commands sent to the TFT, kept for replaying them in the Nextion Editor simulator
// (documentation screenshots); off unless started from the terminal
namespace capture {

constexpr size_t Max { 800 };

bool on { false };
std::vector<String> lines;

} // namespace capture

void write(const String& command) {
    auto& port = uart::port();
    port.write(reinterpret_cast<const uint8_t*>(command.c_str()), command.length());
    const uint8_t end[3] {0xFF, 0xFF, 0xFF};
    port.write(end, sizeof(end));
    if (capture::on && (capture::lines.size() < capture::Max)) {
        capture::lines.push_back(command);
    }
    ++state.tx_commands;
    state.tx_bytes += command.length() + 3;
    if (config.debug) {
        DEBUG_MSG_P(PSTR("[NSPANEL] -> %s\n"), command.c_str());
    }
}

void sendSync() {
    write(F("sendme"));
    state.waiting = true;
    state.wait_since = millis();
    state.since_sync = 0;
    ++state.syncs;
}

void cmd(String command) {
    if (command.length() > CommandMax) {
        DEBUG_MSG_P(PSTR("[NSPANEL] Command too long (%u bytes), dropped\n"), command.length());
        return;
    }
    if (state.queue.size() >= QueueMax) {
        state.queue.pop_front();
        ++state.dropped;
    }
    if (command.length()) {
        ++state.queued;
        state.queued_bytes += command.length() + 3;
    }
    state.queue.push_back(std::move(command));
}

// the next commands wait until the TFT answered `sendme` (it stops reading while it draws)
void sync() {
    if (!state.queue.empty() && !state.queue.back().length()) {
        return;
    }
    cmd(String());
}

void lostLink(const char* reason);

void txLoop() {
    const auto now = millis();
    if (state.waiting) {
        if (now - state.wait_since < SyncTimeoutMs) {
            return;
        }

        state.waiting = false;
        ++state.sync_timeouts;
        if (++state.sync_misses >= SyncMissesMax) {
            lostLink("flow-control syncs unanswered");
            return;
        }
    }

    auto& port = uart::port();
    size_t budget = TxBudget;
    while (!state.queue.empty() && !state.waiting) {
        auto& front = state.queue.front();
        if (!front.length()) {
            state.queue.pop_front();
            sendSync();
            break;
        }

        const size_t need = front.length() + 3;
        if ((need > budget) || (port.availableForWrite() < static_cast<int>(need))) {
            break;
        }

        write(front);
        budget -= need;
        state.since_sync += need;
        state.queue.pop_front();

        if (state.since_sync >= SyncBytes) {
            sendSync();
        }
    }
}

String escape(const String& text) {
    String out;
    out.reserve(text.length() + 4);
    for (size_t n = 0; n < text.length(); ++n) {
        const char c = text[n];
        if ((c == '"') || (c == '\\')) {
            out += '\\';
            out += c;
        } else if (c == '\n') {
            out += F("\\r");
        } else if (c != '\r') {
            out += c;
        }
    }
    return out;
}

// true when the value is already on the TFT (assignments redraw and flicker, so they are not repeated)
bool cached(const String& key, const String& value) {
    auto it = state.cache.find(key);
    if ((it != state.cache.end()) && ((*it).second == value)) {
        return true;
    }
    state.cache[key] = value;
    return false;
}

void forget(const String& prefix) {
    for (auto it = state.cache.begin(); it != state.cache.end();) {
        if ((*it).first.startsWith(prefix)) {
            it = state.cache.erase(it);
        } else {
            ++it;
        }
    }
}

String name(const char* prefix, int n, const char* suffix) {
    String out(prefix);
    out += n;
    out += suffix;
    return out;
}

bool setNum(const String& obj, const char* attr, long value) {
    String key(obj);
    key += '.';
    key += attr;
    const String text(value);
    if (cached(key, text)) {
        return false;
    }
    cmd(key + '=' + text);
    return true;
}

bool setText(const String& obj, const String& value) {
    String key(obj);
    key += F(".txt");
    if (cached(key, value)) {
        return false;
    }
    String out(key);
    out += F("=\"");
    out += escape(value);
    out += '"';
    cmd(std::move(out));
    return true;
}

void vis(const String& obj, bool on) {
    String key(F("vis "));
    key += obj;
    const String value(on ? "1" : "0");
    if (cached(key, value)) {
        return;
    }
    cmd(key + ',' + value);
}

void setLevel(int level) {
    if (level != state.level) {
        state.level = level;
        cmd(String(F("dim=")) + level);
    }
}

// page change: the attribute cache starts over, the busy dots are cancelled first (800 ms);
// xy_touch: the page has no hotspots for its rows / tiles, touches come as sendxy coordinates (else sendxy=0)
void loadPage(int id, bool xy_touch = false) {
    state.cache.clear();
    state.home_k = 0;
    state.requested_page = id;
    state.requested_at = millis();
    state.touch_x = -1;
    state.touch_y = -1;
    const int8_t xy = xy_touch ? 1 : 0;
    if (xy != state.sendxy) {
        state.sendxy = xy;
        cmd(String(F("sendxy=")) + xy);
    }
    cmd(String(F("page ")) + id);
    sync();
    if ((id != page::Blank) && (id != 3)) {
        cmd(F("busy_tm.en=0"));
        cmd(F("vis busy1,0"));
        cmd(F("vis busy2,0"));
        cmd(F("vis busy3,0"));
    }
}

// pausing the refresh while the TFT still draws cuts the drawing off: sync first
void refStop() {
    sync();
    cmd(F("ref_stop"));
}

void refStar() {
    cmd(F("ref_star"));
    sync();
}

// filled circle (thermostat ring, drawn tile corners)
void cirs(int x, int y, int r, uint16_t colour) {
    String out(F("cirs "));
    out += x;
    out += ',';
    out += y;
    out += ',';
    out += r;
    out += ',';
    out += colour;
    cmd(std::move(out));
}

void fill(int x, int y, int w, int h, uint16_t colour) {
    String out(F("fill "));
    out += x;
    out += ',';
    out += y;
    out += ',';
    out += w;
    out += ',';
    out += h;
    out += ',';
    out += colour;
    cmd(std::move(out));
}

// xstr x,y,w,h,font,pco,bco,xcen,ycen(1: centred),sta(3: no background, the text draws onto what is there),"text"
void xstr(int x, int y, int w, int h, int font, uint16_t pco, uint16_t bco, int xcen, const String& text) {
    String out(F("xstr "));
    out += x;
    out += ',';
    out += y;
    out += ',';
    out += w;
    out += ',';
    out += h;
    out += ',';
    out += font;
    out += ',';
    out += pco;
    out += ',';
    out += bco;
    out += ',';
    out += xcen;
    out += F(",1,3,\"");
    out += escape(text);
    out += '"';
    cmd(std::move(out));
}

void sendTheme() {
    const auto& bg = background();
    cmd(String(F("th_accent=")) + accent());
    cmd(String(F("th_text=")) + textColor());
    cmd(String(F("th_dim=")) + dimColor());
    cmd(String(F("bg_pic=")) + bg.pic);
    cmd(String(F("bg_close=")) + bg.close);
}

// -----------------------------------------------------------------------------
// Rendering
// -----------------------------------------------------------------------------

void markDirty() {
    if (!state.dirty) {
        state.dirty = true;
        state.dirty_since = millis();
    }
}

// home and the cards of the swipe row (sub-page sheets are not in it)
size_t rowSize() {
    return 1 + rowCards(state.cards);
}

// one glyph per page of the row, circle-medium for the current one
String dots(size_t count, size_t current) {
    String out;
    if (count <= 1) {
        return out;
    }

    if (count > DotsMax) {
        out += String(current + 1);
        out += '/';
        out += String(count);
        return out;
    }

    for (size_t n = 0; n < count; ++n) {
        if (n) {
            out += ' ';
        }
        out += (n == current) ? "\xEE\xA7\x9D" : "\xEE\xA7\x9E";
    }
    return out;
}

void showDots(size_t count, size_t current) {
    const auto text = dots(count, current);
    if (text.length()) {
        setText(F("dots"), text);
        vis(F("dots"), true);
    } else {
        vis(F("dots"), false);
    }
}

// the swipe row, or the sheets of a sub-page (none for one sheet)
void showCardDots() {
    if ((state.card < state.cards.size()) && state.cards[state.card].sub) {
        size_t count;
        size_t current;
        pageSheets(state.cards, state.card, count, current);
        showDots(count, current);
        return;
    }
    showDots(rowSize(), state.card + 1);
}

bool timeValid(tm& out) {
    const time_t now = ::time(nullptr);
    if (now < 1609459200) { // 2021-01-01, not synced yet
        return false;
    }
    localtime_r(&now, &out);
    return true;
}

void homeClock() {
    tm t;
    if (timeValid(t)) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
        setText(F("clock"), buf);
        setText(F("date"), fit(lang::dateShort(t), Date));
        vis(F("date"), true);
    } else {
        setText(F("clock"), F("--:--"));
        vis(F("date"), false);
    }
    vis(F("clock"), true);
}

void homeStatus() {
    const bool wifi = wifiConnected();
    setText(F("wifi"), wifi ? String() : utf8(icons::WifiOff));
    setNum(F("wifi"), "pco", color::Red);
    vis(F("wifi"), true);

#if MQTT_SUPPORT
    const bool mqtt = mqttConnected();
#else
    const bool mqtt = true;
#endif
    if (wifi && !mqtt) {
        setText(F("top1"), utf8(icons::AlertCircleOutline));
        setNum(F("top1"), "pco", color::Red);
        vis(F("top1"), true);
    } else {
        vis(F("top1"), false);
    }
}

void homeIndoor() {
    const auto text = indoorText();
    if (text.length()) {
        setText(F("row1_icon"), utf8(icons::HomeThermometerOutline));
        vis(F("row1_icon"), true);
        setText(F("row1"), fit(text, Row));
        vis(F("row1"), true);
    } else {
        vis(F("row1_icon"), false);
        vis(F("row1"), false);
    }
}

void homeWeather() {
    const auto& data = weather::current;
    const auto& bg = background();
    const bool valid = config.weather && data.valid;

    if (valid) {
        setNum(F("weather_pic"), "pic", bg.weather_home + weatherPicture(data.code, data.day));
        vis(F("weather_pic"), true);
        vis(F("weather_hs"), true);
        setText(F("outdoor"), fit(temperatureText(data.temperature), Outdoor));
        vis(F("outdoor"), true);
    } else {
        vis(F("weather_pic"), false);
        vis(F("weather_hs"), false);
        vis(F("outdoor"), false);
    }

    // today's low, and the high after it: the variant k depends on the low's width (font 2, 6 px steps)
    int k = 0;
    String low;
    String high;
    if (valid && data.count) {
        low = temperatureText(data.days[0].tmin);
        high = temperatureText(data.days[0].tmax);
        // font 2 metrics from the manifest: digit 11 px, '-' 6 px, '°' 7 px
        int width = 0;
        for (size_t n = 0; n < low.length(); ++n) {
            const char c = low[n];
            width += ((c >= '0') && (c <= '9')) ? 11 : (c == '-') ? 6 : (c == '\xB0') ? 7 : 0;
        }
        k = std::max<int>(1, std::min<int>(7, (width + 5) / 6));
    }

    if (state.home_k && (state.home_k != k)) {
        vis(name("low", state.home_k, ""), false);
        vis(name("high_arrow", state.home_k, ""), false);
        vis(name("high", state.home_k, ""), false);
    }
    state.home_k = k;

    if (k) {
        vis(F("low_arrow"), true);
        setText(name("low", k, ""), low);
        vis(name("low", k, ""), true);
        setText(name("high", k, ""), high);
        vis(name("high_arrow", k, ""), true);
        vis(name("high", k, ""), true);
    } else {
        vis(F("low_arrow"), false);
    }
}

// tile / shortcut state
struct ItemView {
    bool active { false };
    String value;
};

const Thermostat* thermostatOf(const Item& item) {
    if ((item.thermostat < 0) || (static_cast<size_t>(item.thermostat) >= config.thermostats.size())) {
        return nullptr;
    }
    return &config.thermostats[item.thermostat];
}

// the setpoint edited on the panel while it is not confirmed, else the reported one
float shownSetpoint(size_t index) {
    if ((state.edit_item == static_cast<int>(index)) && !std::isnan(state.edit_value)) {
        return state.edit_value;
    }
    const auto* thermostat = thermostatOf(config.items[index]);
    return thermostat ? entityNumber(thermostat->setpoint) : NAN;
}

// "21,9° → 21,5°", shorter variants when that does not fit the field (default: the tile state line)
String thermostatTileText(size_t index, const Field& field = TileValue) {
    const auto* thermostat = thermostatOf(config.items[index]);
    if (!thermostat) {
        return String();
    }

    const float current = entityNumber(thermostat->current);
    const float setpoint = shownSetpoint(index);
    const String degree("\xC2\xB0");
    const String arrow(" \xC2\xBB ");  // U+2192 is not in the TFT fonts

    if (std::isnan(current) && std::isnan(setpoint)) {
        return F("--");
    }
    if (std::isnan(setpoint)) {
        return temperatureShort(current) + degree;
    }
    if (std::isnan(current)) {
        return arrow.substring(1) + temperatureShort(setpoint) + degree;
    }

    String out = temperatureShort(current) + degree + arrow + temperatureShort(setpoint) + degree;
    if (fits(out, field)) {
        return out;
    }
    out = temperatureShort(current) + arrow + temperatureShort(setpoint) + degree;
    return fit(out, field);
}

ItemView itemView(size_t index) {
    ItemView out;
    const auto& item = config.items[index];
    const auto now = millis();

    switch (item.type) {
    case ItemType::Switch: {
        const auto value = entityState(item.target.entity);
        out.active = (value == 1);
        out.value = (value == 1) ? lang::texts().on : (value == 0) ? lang::texts().off : "--";
        break;
    }

    case ItemType::Button:
        out.active = (entityState(item.target.entity) == 1)
            || ((index < state.pulse_until.size()) && state.pulse_until[index]
                && (static_cast<int32_t>(state.pulse_until[index] - now) > 0));
        break;

    case ItemType::Scene: {
        const auto value = groupState(item.target.group);
        out.active = (value == 1);
        if (value == 1) {
            out.value = String(lang::texts().on_count) + groupOnCount(item.target.group);
        } else {
            out.value = (value == 0) ? lang::texts().off : "--";
        }
        break;
    }

    case ItemType::Text:
        if (item.target.entity >= 0) {
            out.value = config.entities[item.target.entity].value;
        }
        break;

    case ItemType::Link:
        break;

    case ItemType::Thermostat: {
        const auto* thermostat = thermostatOf(item);
        out.active = thermostat && thermostatActive(*thermostat);
        out.value = thermostatTileText(index);
        break;
    }
    }

    return out;
}

void homeShortcuts() {
    const size_t count = std::min(config.shortcuts.size(), ShortcutsMax);
    for (size_t n = 1; n <= count; ++n) {
        const auto index = config.shortcuts[n - 1];
        const auto view = itemView(index);
        const auto icon = name("shortcut", n, "_icon");
        setText(icon, config.items[index].icon);
        const auto& item = config.items[index];
        setNum(icon, "pco", view.active ? ((item.color >= 0) ? item.color : accent()) : dimColor());
        vis(icon, true);
        vis(name("shortcut", n, "_hs"), true);
    }
}

void homeRelays() {
#if RELAY_SUPPORT
    for (size_t n = 0; n < 2; ++n) {
        if (!config.relay_labels[n].length() || (n >= relayCount())) {
            continue;
        }
        const auto text = name("relay", n + 1, "_text");
        const auto bar = name("relay", n + 1, "_bar");
        setText(text, fit(config.relay_labels[n], RelayText));
        setNum(bar, "bco", (relayStatus(n) == RelayStatus::On) ? accent() : color::DarkGrey);
        vis(text, true);
        vis(bar, true);
    }
#endif
}

// "–" while unknown; the format (one float conversion, checked by the parser) for numbers
String infoValue(int entity, const String& format) {
    if ((entity < 0) || (static_cast<size_t>(entity) >= config.entities.size())) {
        return String();
    }

    const auto& item = config.entities[entity];
    if (!item.value.length()) {
        return F("--");
    }
    if (format.length() && !std::isnan(item.number)) {
        char buf[40];
        snprintf(buf, sizeof(buf), format.c_str(), static_cast<double>(item.number));
        return buf;
    }
    return item.value;
}

// info1 / info2: icon in the tag field, "<text>  <value>" in the main field, the extra value right
void homeInfo() {
    for (size_t n = 0; n < config.info_count; ++n) {
        const auto& line = config.info[n];

        if (line.icon.length()) {
            const auto icon = name("info", n + 1, "_tagicon");
            setText(icon, line.icon);
            vis(icon, true);
        }

        String text = line.text;
        const auto value = infoValue(line.entity, line.format);
        if (value.length()) {
            if (text.length()) {
                text += F("  ");
            }
            text += value;
        }
        const auto main = name("info", n + 1, "_text");
        setText(main, fit(text, InfoText));
        vis(main, true);

        if (line.extra >= 0) {
            const auto right = name("info", n + 1, "_right");
            setText(right, fit(infoValue(line.extra, line.extra_format), InfoRight));
            vis(right, true);
        }
    }
}

void renderHome(bool full) {
    if (full) {
        loadPage(page::Home);
        refStop();
    }

    homeClock();
    homeWeather();
    homeIndoor();
    homeInfo();
    homeShortcuts();
    homeRelays();
    homeStatus();
    vis(F("swipe"), true);

    if (full) {
        refStar();
    }
}

// Corner layout without a state line: the tile colour already shows on/off. The line above the
// name holds the first half of a name that does not fit, both halves then use the small font.
struct TileText {
    String first;
    String second;
    bool small { false };
};

TileText tileText(const String& text, const TilePage& page) {
    TileText out;
    if (fits(text, page.name_field)) {
        out.second = text;
        return out;
    }

    out.small = true;
    if (fits(text, page.small_field)) {
        out.second = text;
        return out;
    }

    // last space that leaves a fitting first line
    for (int pos = text.lastIndexOf(' '); pos > 0; pos = text.lastIndexOf(' ', pos - 1)) {
        const auto first = text.substring(0, pos);
        if (fits(first, page.value_field)) {
            out.first = first;
            out.second = fit(text.substring(pos + 1), page.small_field);
            return out;
        }
    }

    // a single long word: break it where the first line is full
    auto first = fit(text, page.value_field);
    first.remove(first.length() - 3); // "..."
    if (first.length()) {
        out.first = first + '-';
        if (!fits(out.first, page.value_field)) {
            out.first = first;
        }
        out.second = fit(text.substring(first.length()), page.small_field);
        return out;
    }

    out.second = fit(text, page.small_field);
    return out;
}

// one line in the name field: its own font, else the smaller one, shortened with "..." at last
String tileName(const String& text, const TilePage& page, bool& small) {
    small = false;
    if (fits(text, page.name_field)) {
        return text;
    }
    small = true;
    return fit(text, page.small_field);
}

// What a tile shows, independent of the layout objects
struct TileFace {
    String icon;
    String value;              // corner: the line above the name
    String name;
    int32_t color { -1 };      // icon colour when active, -1 = accent
    bool active { false };
    bool corner { false };     // corner layout objects (_icon/_name/_value) instead of _center/_text
    bool state_line { false }; // value is a state (text / thermostat / settings tiles), else a name half
    bool small { false };      // corner: name in the page's smaller font
    bool heat { false };       // a heating thermostat: icon and state line in the heating colour
};

struct TileColors {
    bool pic_on;     // which tile picture: the white ("on") or the dark ("off") one
    uint16_t bco;    // flat colour of that picture, for the solid text fields
    uint16_t icon;
    uint16_t name;
    uint16_t value;
};

// Dark theme (OHpanel's): off = dark tile, dim icon; on = white tile, accent icon.
// Light theme (ours; every OHpanel background is dark, a tile has only the white and the dark
// picture): off = white tile, grey icon, dark text; on = dark tile, accent icon, white text.
// The background picture itself stays.
TileColors tileColors(const TileFace& face) {
    const uint16_t on_icon = (face.color >= 0) ? static_cast<uint16_t>(face.color)
        : face.heat ? color::Heat : accent();
    if (config.theme == Theme::Light) {
        if (face.active) {
            return {false, color::DarkGrey, on_icon, color::White,
                face.heat ? color::Heat : face.state_line ? color::LightGrey : color::White};
        }
        return {true, color::White, color::Grey, color::DarkGrey, face.state_line ? color::Grey : color::DarkGrey};
    }

    if (face.active) {
        return {true, color::White, on_icon, color::DarkGrey,
            face.heat ? color::Heat : face.state_line ? color::StateOn : color::DarkGrey};
    }
    return {false, color::DarkGrey, dimColor(), color::LightGrey, face.state_line ? color::StateOff : color::LightGrey};
}

// thermostat state line of a tile: "21.9° » 21.5°", with " · Heating" while heating and " · 59%" (the humidity) where
// they fit
String thermostatFaceText(size_t index, const TilePage& page, bool heating) {
    auto out = thermostatTileText(index, page.value_field);
    if (heating && page.states) {
        auto longer = out + F(" \xC2\xB7 ") + lang::texts().heating;
        if (fits(longer, page.value_field)) {
            out = longer;
        }
    }

    // the humidity, where it fits
    const auto* thermostat = thermostatOf(config.items[index]);
    const float humidity = thermostat ? entityNumber(thermostat->humidity) : NAN;
    if (!std::isnan(humidity)) {
        auto longer = out + F(" \xC2\xB7 ") + String(lroundf(humidity)) + '%';
        if (fits(longer, page.value_field)) {
            out = longer;
        }
    }
    return out;
}

TileFace itemFace(size_t index, const TilePage& page) {
    const auto& item = config.items[index];
    const auto view = itemView(index);

    TileFace face;
    face.icon = item.icon;
    face.active = view.active;
    face.color = item.color;

    // tiles with a state line keep the corner layout: font 9 has no digits and _text is one line
    if ((item.type == ItemType::Text) || (item.type == ItemType::Thermostat)) {
        face.corner = true;
        face.state_line = true;
        if (item.type == ItemType::Thermostat) {
            face.heat = view.active;
            face.value = thermostatFaceText(index, page, view.active);
        } else {
            face.value = fit(view.value, page.value_field);
        }
        if (page.states) {
            face.name = tileName(item.name, page, face.small);
        } else {
            face.name = fit(item.name, page.name_field);
        }
        return face;
    }

    if (config.layout == Layout::Corner) {
        face.corner = true;
        // the grid pages have room for the state ("Вкл", "Вкл: 3") and a name of one line
        if (page.states && view.value.length()) {
            face.state_line = true;
            face.value = fit(view.value, page.value_field);
            face.name = tileName(item.name, page, face.small);
            return face;
        }
        const auto text = tileText(item.name, page);
        face.value = text.first;
        face.name = text.second;
        face.small = text.small;
        return face;
    }

    // center: one line under the icon (stock: 80 px / 20 bytes in font 0)
    face.name = fit(item.name, page.center_field);
    return face;
}

// settings tile n (1..SettingsTiles)
TileFace settingsFace(size_t n) {
    TileFace face;
    face.corner = true;
    face.state_line = true;
    if (n == 1) {
        const bool center = (config.layout == Layout::Center);
        face.icon = utf8(center ? icons::ViewGrid : icons::ViewList);
        face.value = center ? lang::texts().icons_large : lang::texts().icons_small;
        face.name = lang::texts().icons;
    } else if (n == 2) {
        const bool light = (config.theme == Theme::Light);
        face.icon = utf8(icons::ThemeLightDark);
        face.value = light ? lang::texts().theme_light : lang::texts().theme_dark;
        face.name = lang::texts().theme;
    } else {
        face.icon = utf8(icons::Palette);
        face.value = lang::texts().backgrounds[backgroundIndex()];
        face.name = lang::texts().background;
    }
    return face;
}

void drawTile(const TilePage& page, size_t n, const TileFace& face, bool full) {
    const auto colors = tileColors(face);

    const auto pic = name("tile", n, "_pic");
    const auto icon = name("tile", n, face.corner ? "_icon" : "_center");
    const auto label = name("tile", n, face.corner ? "_name" : "_text");
    const auto value = name("tile", n, "_value");

    // a new picture paints over the text fields: everything after it is sent again
    if (setNum(pic, "pic", tilePic(page, n, colors.pic_on)) && !full) {
        forget(icon + '.');
        forget(label + '.');
        forget(value + '.');
    }

    setNum(icon, "bco", colors.bco);
    setNum(label, "bco", colors.bco);
    if (face.corner) {
        setNum(value, "bco", colors.bco);
    }

    setNum(icon, "pco", colors.icon);
    setNum(label, "pco", colors.name);
    if (face.corner) {
        setNum(value, "pco", colors.value);
        setNum(label, "font", face.small ? page.small_font : page.name_font);
    }

    setText(icon, face.icon);
    if (face.corner) {
        setText(value, face.value);
    }
    setText(label, face.name);
}

void showTile(size_t n, bool corner) {
    vis(name("tile", n, "_hs"), true);
    vis(name("tile", n, "_pic"), true);
    if (corner) {
        vis(name("tile", n, "_icon"), true);
        vis(name("tile", n, "_name"), true);
        vis(name("tile", n, "_value"), true);
    } else {
        vis(name("tile", n, "_center"), true);
        vis(name("tile", n, "_text"), true);
    }
}

// a buttons page (`buttons`, or `buttons33` .. `buttons12` of the custom design): a card of items,
// a "type": "settings" card, or the built-in settings (no card)
void renderTiles(const String& title, bool settings, const Page* info, size_t first, size_t count,
        bool dots, bool full, Grid grid = Grid::G4x2)
{
    const auto& page = tilePage(grid);
    if (full) {
        loadPage(page.id);
        setText(F("title"), fit(title, Title));
        vis(F("title"), true);
        vis(F("close"), true);
        vis(F("close_hs"), true);
        if (dots) {
            showCardDots();
        }
        vis(F("swipe"), true);
    }

    bool corner[TilesMaxPage] {};
    count = std::min(std::min(count, slotsOf(grid)), TilesMaxPage);
    for (size_t n = 0; n < count; ++n) {
        const auto face = settings ? settingsFace(n + 1) : itemFace(info->items[first + n], page);
        corner[n] = face.corner;
        drawTile(page, tileSlot(n, grid), face, full);
    }

    // the other slots stay hidden since the page load: no picture, no touch
    if (full) {
        refStop();
        for (size_t n = 0; n < count; ++n) {
            showTile(tileSlot(n, grid), corner[n]);
        }
        refStar();
    }
}

// -----------------------------------------------------------------------------
// List page ("type": "list") on entities_rows (id 12): a card picture per row with the icon, the
// label and the state. The design has no hotspots on the rows, so taps come as sendxy coordinates.
// -----------------------------------------------------------------------------

struct RowView {
    String value;
    bool active { false };
    bool plain { false };   // a value without on/off (text, thermostat): text colour, not dim
};

RowView rowView(size_t index) {
    const auto& item = config.items[index];
    const auto view = itemView(index);

    RowView out;
    out.active = view.active;
    switch (item.type) {
    case ItemType::Switch:
    case ItemType::Scene:
        out.value = view.value;
        break;
    case ItemType::Button:
        break;
    case ItemType::Link:
        out.value = F("\xC2\xBB");  // the fonts have no arrows
        break;
    case ItemType::Text:
        out.value = view.value;
        out.plain = true;
        break;
    case ItemType::Thermostat:
        out.value = thermostatTileText(index, RowValue);
        out.plain = true;
        break;
    }
    return out;
}

// true when the object was shown since the page load (the vis cache)
bool shown(const String& obj) {
    String key(F("vis "));
    key += obj;
    const auto it = state.cache.find(key);
    return (it != state.cache.end()) && ((*it).second == "1");
}

void drawRow(size_t n, size_t index, bool full) {
    const auto& item = config.items[index];
    const auto view = rowView(index);

    const auto pic = name("row", n, "_pic");
    const auto icon = name("row", n, "_icon");

    // the pair the value fits (as OHpanel's draw_entities): the narrow value leaves the label 280 px,
    // a wider one 214 px, no value the whole row (372 px)
    const char* label_part;
    const char* value_part;
    const Field* label_field;
    const Field* value_field;
    if (!view.value.length()) {
        label_part = "_wide";
        value_part = nullptr;
        label_field = &RowWide;
        value_field = nullptr;
    } else if (fits(view.value, RowShortVal)) {
        label_part = "_longtext";
        value_part = "_shortval";
        label_field = &RowLongText;
        value_field = &RowShortVal;
    } else {
        label_part = "_text";
        value_part = "_value";
        label_field = &RowText;
        value_field = &RowValue;
    }
    const auto label = name("row", n, label_part);

    // other fields than last time: a hidden field leaves the page background in the card, so the
    // card picture and every field of the row are sent again after the hiding
    if (!full && !shown(label)) {
        forget(name("row", n, "_"));
    }

    // the fields not used go hidden first, they would paint over the others
    static const char* const Parts[] { "_text", "_value", "_longtext", "_shortval", "_wide" };
    for (const auto* part : Parts) {
        if (strcmp(part, label_part) && (!value_part || strcmp(part, value_part))) {
            vis(name("row", n, part), false);
        }
    }

    setNum(pic, "pic", rowPic(n));

    const uint16_t on_icon = (item.color >= 0) ? static_cast<uint16_t>(item.color) : accent();
    setNum(icon, "pco", view.active ? on_icon : dimColor());
    setText(icon, item.icon);

    setNum(label, "pco", (view.active || view.plain) ? textColor() : dimColor());
    setText(label, fit(item.name, *label_field));

    String value;
    if (value_part) {
        value = name("row", n, value_part);
        setNum(value, "pco", view.active ? on_icon : (view.plain ? textColor() : dimColor()));
        setText(value, fit(view.value, *value_field));
    }

    vis(pic, true);
    vis(icon, true);
    vis(label, true);
    if (value.length()) {
        vis(value, true);
    }
}

void renderList(const Card& card, bool full) {
    if (full) {
        loadPage(page::EntitiesRows, true);
        setText(F("title"), fit(card.title, Title));
        vis(F("title"), true);
        vis(F("close"), true);
        vis(F("close_hs"), true);
        showCardDots();
        vis(F("swipe"), true);
        refStop();
    }

    // rows past the end stay hidden since the page load
    const auto& info = config.pages[card.page];
    const size_t count = std::min<size_t>(card.count, RowsPerSheet);
    for (size_t n = 1; n <= count; ++n) {
        drawRow(n, info.items[card.first + n - 1], full);
    }

    if (full) {
        refStar();
    }
}

// row 1..7 under a point of the entities_rows page, 0 for none (title, close, dots)
size_t rowAt(int x, int y) {
    if ((x < 0) || (y < RowsTop)) {
        return 0;
    }
    const size_t row = 1 + static_cast<size_t>((y - RowsTop) / RowsStep);
    return (row <= RowsPerSheet) ? row : 0;
}

// -----------------------------------------------------------------------------
// Drawn tiles (ohpanel-protocol.md 14): grid layouts the design has no page for are drawn on the stock
// `buttons` page (id 7) with fill / cirs / xstr. Its tile objects stay hidden (no pictures, no hotspots),
// touches come as sendxy coordinates. A tile is a solid rounded box, so it works on every background:
// only the corners show the background picture.
// -----------------------------------------------------------------------------

namespace drawn {

// Grid area: 136x72 tiles (3x3) with gaps 9 / 8, spans
//   x 12 .. 438: 12 + 3 * 136 + 2 * 9 = 438, the stock tiles' span: the right ~30 px of the 480 px
//     screen are under the frame, so the visible part is centred around x 225, not 240
//   y 46 .. 278: 46 + 3 * 72 + 2 * 8 = 278, under the title (and close), above the page dots (y 294).
// Every layout fills the same 426 x 232 area; the remainder of the division is centred:
//   w = (426 - (cols - 1) * 9) / cols, h = (232 - (rows - 1) * 8) / rows
//   4x2: (426 - 27) / 4 = 99 (+3 px) x (232 - 8) / 2 = 112
//   3x3: (426 - 18) / 3 = 136        x (232 - 16) / 3 = 72
//   3x2: 136                         x 112
//   2x2: (426 - 9) / 2 = 208 (+1 px) x 112
//   1x2: 426                         x 112
constexpr int AreaX { 12 };
constexpr int AreaY { 46 };
constexpr int AreaW { 426 };
constexpr int AreaH { 232 };
constexpr int GapX { 9 };
constexpr int GapY { 8 };
constexpr int Radius { 14 };           // rounded corners: 4 `cirs` of this radius
constexpr uint16_t FieldBytes { 96 };  // xstr has no txt_maxl; keeps one command short

// a box inside a tile, from its top left corner
struct Box {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

// Fonts (height, digit width): 0 Ubuntu 16 / 8 px, 1: 18 / 10, 2: 22 / 11, 3: 26 / 13; icons 8: 32 px MDI,
// 9: 48 px MDI. Small tiles (4x2, 3x3, 3x2: 99 / 136 px wide) take font 1 (smaller 0) and the 32 px icon in
// the corner layout; big tiles (2x2, 1x2: 208 / 426 px) font 2 (1) / 3 (2) and the 48 px icon. The field
// widths are the boxes minus nothing: fit() measures generously already.
struct Layout {
    TilePage page;          // field limits and fonts for itemFace(): page `buttons`, states shown (`states`)
    uint8_t value_font;     // corner: the state line, or the first half of a split name (= page.small_font)
    uint8_t center_font;    // center: the name under the icon
    uint8_t corner_icon;    // icon font of the corner layout
    uint8_t center_icon;    // icon font of the center layout
    Box icon;               // corner: icon top left; the state line beside it (short tiles) or above the name
    Box value;
    Box name;
    Box big_icon;           // center: the icon, the name under it, both centred
    Box big_name;
};

constexpr Layout Layouts[] {
    // 4x2, 99 x 112: as the stock tile (icon, state line, name); center: 48 px icon, name in font 0
    {{Grid::G4x2, page::Buttons, "buttons", 8, 0,
        {FieldBytes, 10, 83}, {FieldBytes, 8, 83}, {FieldBytes, 8, 83}, {FieldBytes, 8, 91}, 1, 0, true},
        0, 0, 8, 9, {8, 8, 32, 32}, {8, 60, 83, 20}, {8, 82, 83, 24}, {0, 12, 99, 52}, {4, 72, 91, 24}},
    // 3x3, 136 x 72: icon 6..38 with the state line beside it (46..128), name 44..66; center: 32 px icon
    {{Grid::G3x3, page::Buttons, "buttons", 9, 0,
        {FieldBytes, 10, 120}, {FieldBytes, 8, 120}, {FieldBytes, 8, 82}, {FieldBytes, 10, 124}, 1, 0, true},
        0, 1, 8, 8, {8, 6, 32, 32}, {46, 6, 82, 32}, {8, 44, 120, 22}, {0, 4, 136, 36}, {6, 44, 124, 22}},
    // 3x2, 136 x 112: icon, state line, name; center: 48 px icon 12..64, name 72..98
    {{Grid::G3x2, page::Buttons, "buttons", 6, 0,
        {FieldBytes, 10, 120}, {FieldBytes, 8, 120}, {FieldBytes, 8, 120}, {FieldBytes, 10, 124}, 1, 0, true},
        0, 1, 8, 9, {8, 8, 32, 32}, {8, 60, 120, 20}, {8, 82, 120, 24}, {0, 12, 136, 52}, {6, 72, 124, 26}},
    // 2x2, 208 x 112: 48 px icon 10..58 with the state line beside it (66..198), name 72..102 in font 2
    {{Grid::G2x2, page::Buttons, "buttons", 4, 0,
        {FieldBytes, 11, 188}, {FieldBytes, 10, 188}, {FieldBytes, 10, 132}, {FieldBytes, 11, 192}, 2, 1, true},
        1, 2, 9, 9, {10, 10, 48, 48}, {66, 10, 132, 48}, {10, 72, 188, 30}, {0, 10, 208, 52}, {8, 70, 192, 30}},
    // 1x2, 426 x 112: as 2x2, state line 72..414 in font 2, name in font 3
    {{Grid::G1x2, page::Buttons, "buttons", 2, 0,
        {FieldBytes, 13, 402}, {FieldBytes, 11, 402}, {FieldBytes, 11, 342}, {FieldBytes, 13, 406}, 3, 2, true},
        2, 3, 9, 9, {12, 10, 48, 48}, {72, 10, 342, 48}, {12, 70, 402, 34}, {0, 8, 426, 52}, {10, 66, 406, 34}},
};

static_assert((Layouts[static_cast<size_t>(Grid::G3x3)].page.grid == Grid::G3x3)
    && (Layouts[static_cast<size_t>(Grid::G1x2)].page.grid == Grid::G1x2), "Layouts in Grid order");

const Layout& layoutOf(Grid grid) {
    return Layouts[static_cast<size_t>(grid)];
}

// screen box of tile n (0-based, row by row)
void tileRect(Grid grid, size_t n, int& x, int& y, int& w, int& h) {
    const int cols = gridCols(grid);
    const int rows = gridRows(grid);
    w = (AreaW - (cols - 1) * GapX) / cols;
    h = (AreaH - (rows - 1) * GapY) / rows;
    x = AreaX + (AreaW - (cols * w + (cols - 1) * GapX)) / 2 + static_cast<int>(n % cols) * (w + GapX);
    y = AreaY + (AreaH - (rows * h + (rows - 1) * GapY)) / 2 + static_cast<int>(n / cols) * (h + GapY);
}

// tile 1..count under a point, 0 for none (gaps, title, close, dots)
size_t tileAt(Grid grid, size_t count, int px, int py) {
    for (size_t n = 0; n < count; ++n) {
        int x;
        int y;
        int w;
        int h;
        tileRect(grid, n, x, y, w, h);
        if ((px >= x) && (px < (x + w)) && (py >= y) && (py < (y + h))) {
            return n + 1;
        }
    }
    return 0;
}

// rounded box: two crossing bars and a circle in each corner
void shape(int x, int y, int w, int h, uint16_t colour) {
    fill(x + Radius, y, w - 2 * Radius, h, colour);
    fill(x, y + Radius, w, h - 2 * Radius, colour);
    cirs(x + Radius, y + Radius, Radius, colour);
    cirs(x + w - 1 - Radius, y + Radius, Radius, colour);
    cirs(x + Radius, y + h - 1 - Radius, Radius, colour);
    cirs(x + w - 1 - Radius, y + h - 1 - Radius, Radius, colour);
}

void text(int x, int y, const Box& box, int font, uint16_t pco, uint16_t bco, bool centred, const String& value) {
    if (value.length()) {
        xstr(x + box.x, y + box.y, box.w, box.h, font, pco, bco, centred ? 1 : 0, value);
    }
}

// tile n (0-based): the shape over the old one, then the icon and the texts onto it. Not drawn when it
// looks as drawn last (the page cache, cleared by a page load). True when drawn.
bool drawTile(const Layout& layout, size_t n, const TileFace& face) {
    const auto colors = tileColors(face);

    String look;
    look.reserve(face.icon.length() + face.value.length() + face.name.length() + 32);
    look += colors.bco;
    look += face.corner ? 'c' : 'm';
    look += face.small ? 's' : 'n';
    look += colors.icon;
    look += '|';
    look += face.icon;
    look += '|';
    look += colors.value;
    look += '|';
    look += face.value;
    look += '|';
    look += colors.name;
    look += '|';
    look += face.name;
    if (cached(name("drawn", n + 1, ""), look)) {
        return false;
    }

    int x;
    int y;
    int w;
    int h;
    tileRect(layout.page.grid, n, x, y, w, h);
    shape(x, y, w, h, colors.bco);

    if (face.corner) {
        text(x, y, layout.icon, layout.corner_icon, colors.icon, colors.bco, true, face.icon);
        text(x, y, layout.value, layout.value_font, colors.value, colors.bco, false, face.value);
        text(x, y, layout.name, face.small ? layout.page.small_font : layout.page.name_font,
            colors.name, colors.bco, false, face.name);
    } else {
        text(x, y, layout.big_icon, layout.center_icon, colors.icon, colors.bco, true, face.icon);
        text(x, y, layout.big_name, layout.center_font, colors.name, colors.bco, true, face.name);
    }
    return true;
}

} // namespace drawn

// A drawn grid card. Page load: `buttons` with sendxy=1, every object hidden again (the stock tileN_pic /
// _icon / _name / _value / _bar / _text / _center / _hs; the preinitialize already does it), title, close,
// dots and the swipe hotspot shown, then all tiles. An update draws only the tiles that look different.
// One burst either way: the queue syncs every SyncBytes, nothing waits per command.
void renderDrawn(const Card& card, bool full) {
    const auto& layout = drawn::layoutOf(card.grid);
    const auto commands = state.queued;
    const auto bytes = state.queued_bytes;

    if (full) {
        loadPage(page::Buttons, true);
        cmd(F("vis 255,0"));
        setText(F("title"), fit(card.title, Title));
        vis(F("title"), true);
        vis(F("close"), true);
        vis(F("close_hs"), true);
        showCardDots();
        vis(F("swipe"), true);
    }

    const auto& info = config.pages[card.page];
    const size_t count = std::min<size_t>(card.count, std::min(gridTiles(card.grid), TilesMaxPage));
    size_t tiles = 0;
    for (size_t n = 0; n < count; ++n) {
        if (drawn::drawTile(layout, n, itemFace(info.items[card.first + n], layout.page))) {
            ++tiles;
        }
    }

    if (full) {
        state.draw_page_commands = state.queued - commands;
        state.draw_page_bytes = state.queued_bytes - bytes;
        state.draw_page_tiles = tiles;
    } else if (tiles) {
        state.draw_update_commands = state.queued - commands;
        state.draw_update_bytes = state.queued_bytes - bytes;
        state.draw_update_tiles = tiles;
    }
}

void renderButtons(bool full) {
    if (state.card >= state.cards.size()) {
        return;
    }

    const auto& card = state.cards[state.card];
    if (card.list) {
        renderList(card, full);
        return;
    }
    if (drawnCard(card)) {
        renderDrawn(card, full);
        return;
    }
    renderTiles(card.title, card.settings, &config.pages[card.page], card.first, card.count, true, full, card.grid);
}

void renderSettings(bool full) {
    renderTiles(lang::texts().settings, true, nullptr, 0, SettingsTiles, false, full);
}

// -----------------------------------------------------------------------------
// Thermostat page (id 17): ring of beads drawn with `cirs` (as OHpanel EngineThermostat), current
// temperature in `now`, setpoint in `target`, heating / humidity line drawn with `xstr` into the
// state_hs box, minus and plus icons on discs. Mode pills and the ring drag are not used (their
// hotspots stay hidden, so they take no touches).
// -----------------------------------------------------------------------------

namespace thermo {

constexpr int Cx { 225 };           // centre of ring_hs (95, 30, 260x260)
constexpr int Cy { 160 };
constexpr int Radius { 112 };       // 260 / 2 - 18
constexpr int Beads { 89 };         // 6 px pitch over 270 degrees (OHpanel: 3 px, 177 beads)
constexpr int BeadR { 7 };
constexpr int KnobR { 11 };
constexpr int ButtonR { 24 };
constexpr int MinusX { 48 };        // centres of minus_hs (20, 132, 56x56) / plus_hs (374, 132, 56x56)
constexpr int PlusX { 402 };
constexpr int ButtonY { 160 };
constexpr int StateX { 150 };       // state_hs box
constexpr int StateY { 206 };
constexpr int StateW { 150 };
constexpr int StateH { 24 };
constexpr uint32_t SendDelayMs { 1500 };  // after the last minus / plus tap
constexpr uint32_t SentHoldMs { 6000 };   // the sent value is shown this long unless reported earlier

void bead(int index, int& x, int& y) {
    // clockwise from the lower left, over the top, to the lower right; open at the bottom
    const float a = 0.75f * static_cast<float>(M_PI)
        + 1.5f * static_cast<float>(M_PI) * static_cast<float>(index) / static_cast<float>(Beads - 1);
    x = static_cast<int>(lroundf(Cx + Radius * cosf(a)));
    y = static_cast<int>(lroundf(Cy + Radius * sinf(a)));
}

void circle(int x, int y, int r, uint16_t colour) {
    cirs(x, y, r, colour);
}

// the background picture over a box (OHpanel restore())
void restore(int x, int y, int w, int h) {
    String out(F("picq "));
    out += x;
    out += ',';
    out += y;
    out += ',';
    out += w;
    out += ',';
    out += h;
    out += ',';
    out += background().pic;
    cmd(std::move(out));
}

void drawBead(int index, int knob) {
    int x;
    int y;
    bead(index, x, y);
    circle(x, y, BeadR, ((knob >= 0) && (index <= knob)) ? accent() : color::Track);
}

void drawKnob(int knob) {
    if (knob >= 0) {
        int x;
        int y;
        bead(knob, x, y);
        circle(x, y, KnobR, color::White);
    }
}

// the whole ring, or only the beads around the old and the new knob
void drawRing(int knob, bool full) {
    if (full || (state.knob < 0) || (knob < 0)) {
        if (!full && (state.knob == knob)) {
            return;
        }
        if (!full && (state.knob >= 0)) {
            int x;
            int y;
            bead(state.knob, x, y);
            restore(x - KnobR - 1, y - KnobR - 1, 2 * KnobR + 3, 2 * KnobR + 3);
        }
        for (int index = 0; index < Beads; ++index) {
            drawBead(index, knob);
        }
    } else if (knob != state.knob) {
        int x;
        int y;
        bead(state.knob, x, y);
        restore(x - KnobR - 1, y - KnobR - 1, 2 * KnobR + 3, 2 * KnobR + 3);
        const int low = std::max(0, std::min(knob, state.knob) - 4);
        const int high = std::min(Beads - 1, std::max(knob, state.knob) + 4);
        for (int index = low; index <= high; ++index) {
            drawBead(index, knob);
        }
    } else {
        return;
    }

    drawKnob(knob);
    state.knob = knob;
}

int knobFor(const Thermostat& thermostat, float setpoint) {
    if (std::isnan(setpoint)) {
        return -1;
    }
    const float min = thermostatMin(thermostat);
    const float max = thermostatMax(thermostat);
    const float share = std::max(0.0f, std::min(1.0f, (setpoint - min) / (max - min)));
    return static_cast<int>(lroundf(share * (Beads - 1)));
}

uint32_t textWidth(const String& text, uint8_t digit) {
    uint32_t units = 0;
    size_t pos = 0;
    while (pos < text.length()) {
        units += advance(nextCodepoint(text, pos));
    }
    return (units * digit + 55) / 56;
}

// heating (fire, orange) and / or the humidity (water-percent, dim), centred in state_hs
void drawState(const Thermostat& thermostat) {
    const bool heating = thermostatActive(thermostat);
    const float humidity = entityNumber(thermostat.humidity);

    String text;
    if (heating) {
        text = lang::texts().heating;
    }
    if (!std::isnan(humidity)) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%ld%%", lroundf(humidity));
        if (heating) {
            text += F(" \xC2\xB7 ");
        }
        text += buf;
    }
    text = fit(text, ThermoState);

    const uint16_t colour = heating ? color::Heat : dimColor();
    String key(text);
    key += '|';
    key += colour;
    if (key == state.thermo_state) {
        return;
    }
    state.thermo_state = key;

    restore(StateX, StateY, StateW, StateH);
    if (!text.length()) {
        return;
    }

    const int width = static_cast<int>(std::min<uint32_t>(textWidth(text, ThermoState.digit), ThermoState.width));
    const int x = StateX + std::max(0, (StateW - (28 + width)) / 2);
    const auto glyph = utf8(heating ? icons::Fire : icons::WaterPercent);

    // xstr x,y,w,h,font,pco,bco,xcen,ycen,sta(3: no background),"text"
    String out(F("xstr "));
    out += x;
    out += ',';
    out += StateY;
    out += F(",24,");
    out += StateH;
    out += F(",7,");
    out += colour;
    out += F(",0,1,1,3,\"");
    out += glyph;
    out += '"';
    cmd(std::move(out));

    out = F("xstr ");
    out += x + 28;
    out += ',';
    out += StateY + 3;
    out += ',';
    out += width + 2;
    out += F(",18,1,");
    out += colour;
    out += F(",0,0,1,3,\"");
    out += escape(text);
    out += '"';
    cmd(std::move(out));
}

} // namespace thermo

// -----------------------------------------------------------------------------
// Thermostat history chart (the thermostat's "history" topic, history:: in nspanel.cpp): the thermostat page
// with its objects hidden, a dark rounded box over the grid area with the temperature (accent) and the target
// (grey, steps), heating (target above temperature, as the Istabai bridge counts it) as dim orange bands in the
// plot and an orange strip under it, degrees on the left, every 6 h of local time on the bottom. One burst of
// ~100-250 commands; the series is not kept, the chart subscribes while it is open. A switch tile with "history"
// (e.g. the heating pump) opens the same box with its own series: orange bars of the % of each step it was ON.
// -----------------------------------------------------------------------------

namespace chart {

constexpr int BoxX { drawn::AreaX };
constexpr int BoxY { drawn::AreaY };
constexpr int BoxW { drawn::AreaW };
constexpr int BoxH { drawn::AreaH };
constexpr int PlotX0 { BoxX + 42 };
constexpr int PlotX1 { BoxX + BoxW - 14 };
constexpr int PlotX1Humidity { BoxX + BoxW - 40 };  // room for the % labels on the right
constexpr int PlotY0 { BoxY + 28 };
constexpr int PlotY1 { BoxY + BoxH - 36 };
constexpr int StripY { PlotY1 + 4 };
constexpr int StripH { 5 };
constexpr int LabelY { PlotY1 + 12 };
constexpr size_t PointsMax { 96 };     // the newest points; lines per series, the queue holds 320 commands
constexpr uint32_t WaitMs { 4000 };    // no retained message by then: "no history"

constexpr uint16_t Box { color::DarkGrey };
constexpr uint16_t Grid { rgb565(0x3c4248) };
constexpr uint16_t Band { rgb565(0x4a2c18) };
constexpr uint16_t Target { color::LightGrey };
constexpr uint16_t Humidity { rgb565(0x81c784) };

// the button under the dial: the hidden mode pill m11_hs (165,280 120x32) takes the touch
constexpr int PillX { 165 };
constexpr int PillY { 280 };
constexpr int PillW { 120 };
constexpr int PillH { 32 };

void line(int x1, int y1, int x2, int y2, uint16_t colour) {
    String out(F("line "));
    out += x1;
    out += ',';
    out += y1;
    out += ',';
    out += x2;
    out += ',';
    out += y2;
    out += ',';
    out += colour;
    cmd(std::move(out));
}

void pill() {
    drawn::shape(PillX, PillY, PillW, PillH, color::Button);
    xstr(PillX + 8, PillY + 4, 24, 24, 7, color::White, 0, 1, utf8(icons::ChartLine));
    xstr(PillX + 34, PillY + 7, PillW - 40, 18, 1, color::White, 0, 0, lang::texts().history);
}

void message(const String& text) {
    drawn::shape(BoxX, BoxY, BoxW, BoxH, Box);
    xstr(BoxX, BoxY + BoxH / 2 - 12, BoxW, 24, 1, color::LightGrey, 0, 1, text);
}

// the drawn part of a series: its newest PointsMax points on a degree scale (the humidity on a % scale)
struct Scale {
    size_t first;
    size_t count;
    int low;    // whole degrees at the bottom / top of the plot
    int high;
    int right { PlotX1 };
    int hlow { 0 };    // whole % at the bottom / top
    int hhigh { 100 };

    int x(size_t index) const {
        return PlotX0 + static_cast<int>((index * static_cast<size_t>(right - PlotX0)) / (count - 1));
    }

    int y(int tenths) const {
        const float share = (tenths / 10.0f - low) / static_cast<float>(high - low);
        return PlotY1 - static_cast<int>(lroundf(share * (PlotY1 - PlotY0)));
    }

    // a humidity value (% x10) mapped onto the degree scale, so that polyline() draws it as it is
    int16_t humidityAsDegrees(int16_t tenths) const {
        const float share = (tenths / 10.0f - hlow) / static_cast<float>(hhigh - hlow);
        return static_cast<int16_t>(lroundf((low + share * (high - low)) * 10.0f));
    }

    int16_t at(const std::vector<int16_t>& values, size_t index) const {
        const size_t n = first + index;
        return (n < values.size()) ? values[n] : history::Missing;
    }
};

// a polyline through the points (a missing value breaks it); steps: the value holds until the next point.
// A run of equal heights (or a vertical step) is one line
void polyline(const std::vector<int16_t>& values, const Scale& scale, bool steps, uint16_t colour) {
    bool open = false;
    int sx = 0;     // the pending segment, straight horizontal or vertical runs grow at its end
    int sy = 0;
    int ex = 0;
    int ey = 0;

    auto flush = [&]() {
        if (open && ((sx != ex) || (sy != ey))) {
            line(sx, sy, ex, ey, colour);
        }
    };

    auto to = [&](int x, int y) {
        if (!open) {
            sx = ex = x;
            sy = ey = y;
            open = true;
        } else if ((sy == ey) && (y == ey)) {
            ex = x;
        } else if ((sx == ex) && (x == ex)) {
            ey = y;
        } else {
            flush();
            sx = ex;
            sy = ey;
            ex = x;
            ey = y;
        }
    };

    for (size_t index = 0; index < scale.count; ++index) {
        const auto value = scale.at(values, index);
        if (value == history::Missing) {
            flush();
            open = false;
            continue;
        }
        const int x = scale.x(index);
        const int y = scale.y(value);
        if (steps && open) {
            to(x, ey);
        }
        to(x, y);
    }
    flush();
}

bool heating(const history::Series& series, const Scale& scale, size_t index) {
    const auto temperature = scale.at(series.temperature, index);
    const auto setpoint = scale.at(series.setpoint, index);
    return (temperature != history::Missing) && (setpoint != history::Missing) && (setpoint > temperature);
}

// a colour bar and its name in font 0 (8 px per digit); where the next one starts. compact: four entries
// (with the humidity) fit the plot width
int legend(int x, uint16_t colour, const char* text, bool compact = false) {
    const int width = static_cast<int>(thermo::textWidth(String(text), 8)) + 4;
    const int bar = compact ? 10 : 16;
    const int gap = compact ? 16 : 22;
    fill(x, BoxY + 13, bar, 3, colour);
    xstr(x + gap, BoxY + 5, width, 18, 0, color::LightGrey, 0, 0, text);
    return x + gap + width + (compact ? 8 : 18);
}

// every 6 h of local time (the point of a full hour, whatever the step)
void hours(const history::Series& series, const Scale& scale) {
    if (series.from <= 1000000000UL) {
        return;
    }
    for (size_t index = 0; index < scale.count; ++index) {
        const time_t when = static_cast<time_t>(series.from + (scale.first + index) * series.step);
        tm local;
        localtime_r(&when, &local);
        if ((local.tm_hour % 6) || (static_cast<uint32_t>(local.tm_min * 60 + local.tm_sec) >= series.step)) {
            continue;
        }
        const int x = scale.x(index);
        line(x, PlotY0, x, PlotY1, Grid);
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:00", local.tm_hour);
        const int left = std::max(BoxX + 2, std::min(x - 24, BoxX + BoxW - 50));
        xstr(left, LabelY, 48, 18, 0, color::LightGrey, 0, 1, buf);
    }
}

// a switch: a bar per run of equal values (% of the step it was ON, 0..100 % on the left), the hours it was ON
// in the legend
void drawOn(const history::Series& series) {
    const size_t total = series.on.size();
    Scale scale;
    scale.first = (total > PointsMax) ? (total - PointsMax) : 0;
    scale.count = total - scale.first;
    scale.low = 0;
    scale.high = 100;

    bool known = false;
    uint32_t sum = 0;
    for (size_t index = 0; index < scale.count; ++index) {
        const auto value = scale.at(series.on, index);
        if (value != history::Missing) {
            known = true;
            sum += std::max<int>(0, std::min<int>(100, value));
        }
    }
    if (!known) {
        message(lang::texts().no_history);
        return;
    }

    drawn::shape(BoxX, BoxY, BoxW, BoxH, Box);

    for (int percent = 0; percent <= 100; percent += 50) {
        const int y = scale.y(percent * 10);
        line(PlotX0, y, scale.right, y, Grid);
        xstr(BoxX + 2, y - 9, PlotX0 - BoxX - 6, 18, 0, color::LightGrey, 0, 2, String(percent) + '%');
    }
    hours(series, scale);

    const int half = std::max(1, (scale.right - PlotX0) / static_cast<int>(2 * (scale.count - 1)));
    for (size_t index = 0; index < scale.count;) {
        const auto value = scale.at(series.on, index);
        size_t last = index;
        while (((last + 1) < scale.count) && (scale.at(series.on, last + 1) == value)) {
            ++last;
        }
        if ((value != history::Missing) && (value > 0)) {
            const int x1 = std::max(PlotX0, scale.x(index) - half);
            const int x2 = std::min(scale.right, scale.x(last) + half);
            const int y = scale.y(std::min<int>(100, value) * 10);
            fill(x1, y, x2 - x1 + 1, PlotY1 - y + 1, color::Heat);
        }
        index = last + 1;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", (sum / 100.0f) * series.step / 3600.0f);
    lang::decimal(buf);
    legend(PlotX0, color::Heat, (String(lang::texts().on_count) + buf + lang::texts().hours).c_str());
}

void draw(const history::Series& series) {
    if (series.temperature.empty() && !series.on.empty()) {
        drawOn(series);
        return;
    }

    const size_t total = series.temperature.size();
    Scale scale;
    scale.first = (total > PointsMax) ? (total - PointsMax) : 0;
    scale.count = total - scale.first;

    int lo = INT_MAX;
    int hi = INT_MIN;
    for (size_t index = 0; index < scale.count; ++index) {
        for (const auto* values : {&series.temperature, &series.setpoint}) {
            const auto value = scale.at(*values, index);
            if (value != history::Missing) {
                lo = std::min<int>(lo, value);
                hi = std::max<int>(hi, value);
            }
        }
    }
    if (lo > hi) {
        message(lang::texts().no_history);
        return;
    }

    scale.low = static_cast<int>(floorf(lo / 10.0f - 0.3f));
    scale.high = std::max(scale.low + 2, static_cast<int>(ceilf(hi / 10.0f + 0.3f)));
    const int range = scale.high - scale.low;
    const int every = (range > 16) ? 5 : (range > 8) ? 2 : 1;

    drawn::shape(BoxX, BoxY, BoxW, BoxH, Box);

    // heating: bands in the plot, the strip under it
    // humidity: its own % scale on the right, at least 20 % high, centred on the values (bounds in steps of 5)
    int hlo = INT_MAX;
    int hhi = INT_MIN;
    for (size_t index = 0; index < scale.count; ++index) {
        const auto value = scale.at(series.humidity, index);
        if (value != history::Missing) {
            hlo = std::min<int>(hlo, value);
            hhi = std::max<int>(hhi, value);
        }
    }
    const bool humidity = (hlo <= hhi);
    if (humidity) {
        scale.right = PlotX1Humidity;
        const float lo_pct = hlo / 10.0f;
        const float hi_pct = hhi / 10.0f;
        const int span = std::min(100, std::max(20, static_cast<int>(ceilf((hi_pct - lo_pct + 6.0f) / 10.0f)) * 10));
        const int low_pct = static_cast<int>(floorf(((lo_pct + hi_pct) / 2.0f - span / 2.0f) / 5.0f)) * 5;
        scale.hlow = std::max(0, std::min(100 - span, low_pct));
        scale.hhigh = scale.hlow + span;
    }

    const int half = std::max(1, (scale.right - PlotX0) / static_cast<int>(2 * (scale.count - 1)));
    for (size_t index = 0; index < scale.count;) {
        if (!heating(series, scale, index)) {
            ++index;
            continue;
        }
        size_t last = index;
        while (((last + 1) < scale.count) && heating(series, scale, last + 1)) {
            ++last;
        }
        const int x1 = std::max(PlotX0, scale.x(index) - half);
        const int x2 = std::min(scale.right, scale.x(last) + half);
        fill(x1, PlotY0, x2 - x1 + 1, PlotY1 - PlotY0 + 1, Band);
        fill(x1, StripY, x2 - x1 + 1, StripH, color::Heat);
        index = last + 1;
    }

    // degrees
    for (int degree = scale.low + ((every - ((scale.low % every) + every) % every) % every);
        degree <= scale.high; degree += every)
    {
        const int y = scale.y(degree * 10);
        line(PlotX0, y, scale.right, y, Grid);
        xstr(BoxX + 2, y - 9, PlotX0 - BoxX - 6, 18, 0, color::LightGrey, 0, 2, String(degree) + F("\xC2\xB0"));
    }

    hours(series, scale);

    if (humidity) {
        const int every_h = ((scale.hhigh - scale.hlow) > 40) ? 20 : 10;
        for (int percent = ((scale.hlow + every_h - 1) / every_h) * every_h; percent <= scale.hhigh; percent += every_h) {
            const int y = scale.y(scale.humidityAsDegrees(percent * 10));
            xstr(scale.right + 4, y - 9, BoxX + BoxW - scale.right - 6, 18, 0, Humidity, 0, 0,
                String(percent) + '%');
        }

        std::vector<int16_t> mapped;
        mapped.reserve(series.humidity.size());
        for (const auto value : series.humidity) {
            mapped.push_back((value == history::Missing) ? value : scale.humidityAsDegrees(value));
        }
        polyline(mapped, scale, false, Humidity);
    }

    polyline(series.setpoint, scale, true, Target);
    polyline(series.temperature, scale, false, accent());

    int x = legend(PlotX0, accent(), lang::texts().temperature, humidity);
    x = legend(x, Target, lang::texts().setpoint, humidity);
    if (humidity) {
        x = legend(x, Humidity, lang::texts().humidity, true);
    }
    legend(x, color::Heat, lang::texts().heating, humidity);
}

} // namespace chart

// a thermostat's history topic, or the one of a switch tile
const String& historyOf(const Item& item) {
    const auto* thermostat = thermostatOf(item);
    return thermostat ? thermostat->history : item.history;
}

// the chart instead of the dial: subscribes again on a full render (the retained series comes back)
void renderChart(bool full) {
    const auto& item = config.items[state.thermo_item];

    if (full) {
        loadPage(page::Thermostat);
        setText(F("title"), fit(item.name, Title));
        vis(F("title"), true);
        vis(F("close"), true);
        vis(F("close_hs"), true);
        vis(F("swipe"), true);
        state.chart_drawn = false;
        state.chart_since = millis();
        history::open(historyOf(item));
        return;
    }

    history::Series series;
    if (history::take(series)) {
        const size_t count = std::min(std::max(series.temperature.size(), series.on.size()), chart::PointsMax);
        String title(item.name);
        title += F(" \xC2\xB7 ");
        title += (series.step * (count - 1) + 1800) / 3600;
        title += lang::texts().hours;
        setText(F("title"), fit(title, Title));
        chart::draw(series);
        state.chart_drawn = true;
    } else if (!state.chart_drawn && (millis() - state.chart_since >= chart::WaitMs)) {
        chart::message(lang::texts().no_history);
        state.chart_drawn = true;
    }
}

void closeChart() {
    state.thermo_chart = false;
    history::close();
}

// loop(): draws the series when it arrives (after the page load went out), "no history" after WaitMs;
// leaving the thermostat screen in any way drops the subscription
void chartLoop(uint32_t now) {
    if (!state.thermo_chart) {
        return;
    }
    if (state.screen != Screen::Thermostat) {
        closeChart();
        return;
    }
    if (state.queue.empty()
        && (history::fresh || (!state.chart_drawn && (now - state.chart_since >= chart::WaitMs))))
    {
        renderChart(false);
    }
}

void renderThermostat(bool full) {
    if (state.thermo_item >= config.items.size()) {
        return;
    }
    if (state.thermo_chart) {
        if (full) {
            renderChart(true);
        }
        return;
    }

    const auto& item = config.items[state.thermo_item];
    const auto* thermostat = thermostatOf(item);
    if (!thermostat) {
        return;
    }

    if (full) {
        loadPage(page::Thermostat);
        state.knob = -1;
        state.thermo_state = String();
        setText(F("title"), fit(item.name, Title));
        vis(F("title"), true);
        vis(F("close"), true);
        vis(F("close_hs"), true);
        vis(F("swipe"), true);
        refStop();
        setNum(F("minus"), "bco", color::Button);
        setNum(F("minus"), "pco", color::White);
        setNum(F("plus"), "bco", color::Button);
        setNum(F("plus"), "pco", color::White);
        setText(F("minus"), utf8(icons::Minus));
        setText(F("plus"), utf8(icons::Plus));
    }

    const float current = entityNumber(thermostat->current);
    String now;
    if (!std::isnan(current)) {
        now = temperatureShort(current);
        now += F("\xC2\xB0");
    } else {
        now = F("--");
    }
    // a number must never become "22,...": the estimated width is too pessimistic for font 6,
    // only keep the byte limit (the TFT itself clips at worst the degree sign)
    if (now.length() > ThermoNow.bytes) {
        now = String(lroundf(current)) + F("\xC2\xB0");
    }
    setText(F("now"), now);
    vis(F("now"), true);

    const float setpoint = shownSetpoint(state.thermo_item);
    const bool editing = (state.edit_item == static_cast<int>(state.thermo_item)) && !std::isnan(state.edit_value);
    String target(lang::texts().target);
    if (std::isnan(setpoint)) {
        target += F("--");
    } else {
        target += temperatureShort(setpoint);
        target += F("\xC2\xB0");
    }
    setNum(F("target"), "pco", editing ? accent() : color::LightGrey);
    setText(F("target"), fit(target, ThermoTarget));
    vis(F("target"), true);

    if (full) {
        refStar();
        // shapes once the page is drawn: the discs under minus / plus, then the icons over them
        thermo::circle(thermo::MinusX, thermo::ButtonY, thermo::ButtonR, color::Button);
        thermo::circle(thermo::PlusX, thermo::ButtonY, thermo::ButtonR, color::Button);
        vis(F("minus"), true);
        vis(F("plus"), true);
        vis(F("minus_hs"), true);
        vis(F("plus_hs"), true);
        if (thermostat->history.length()) {
            chart::pill();
            vis(F("m11_hs"), true);
        }
    }

    thermo::drawRing(thermo::knobFor(*thermostat, setpoint), full);
    thermo::drawState(*thermostat);
}

// sends the edited setpoint after the debounce, forgets it once reported (or after SentHoldMs)
void thermoLoop(uint32_t now) {
    if (state.edit_item < 0) {
        return;
    }

    const auto* thermostat = (static_cast<size_t>(state.edit_item) < config.items.size())
        ? thermostatOf(config.items[state.edit_item]) : nullptr;
    if (!thermostat) {
        state.edit_item = -1;
        state.edit_value = NAN;
        state.edit_unsent = false;
        return;
    }

    if (state.edit_unsent) {
        if (now - state.edit_tap < thermo::SendDelayMs) {
            return;
        }
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", state.edit_value);
        entitySend(thermostat->setpoint, buf);
        state.edit_unsent = false;
        state.edit_sent = now;
        return;
    }

    const float reported = entityNumber(thermostat->setpoint);
    if ((now - state.edit_sent >= thermo::SentHoldMs)
        || (!std::isnan(reported) && (fabsf(reported - state.edit_value) < 0.01f)))
    {
        state.edit_item = -1;
        state.edit_value = NAN;
        markDirty();
    }
}

// an edit of another thermostat is sent right away
void thermoFlush() {
    if (state.edit_unsent) {
        state.edit_tap = millis() - thermo::SendDelayMs;
        thermoLoop(millis());
    }
}

// minus / plus: local setpoint, snapped and clamped to min..max, sent SendDelayMs after the last tap
void thermoStep(int direction) {
    const size_t index = state.thermo_item;
    if (index >= config.items.size()) {
        return;
    }
    const auto* thermostat = thermostatOf(config.items[index]);
    if (!thermostat) {
        return;
    }

    if ((state.edit_item >= 0) && (state.edit_item != static_cast<int>(index))) {
        thermoFlush();
    }

    float base = shownSetpoint(index);
    if (std::isnan(base)) {
        base = entityNumber(thermostat->current);
    }
    if (std::isnan(base)) {
        base = (thermostatMin(*thermostat) + thermostatMax(*thermostat)) / 2.0f;
    }

    buzzer::beep();
    state.edit_item = index;
    state.edit_value = thermostatClamp(*thermostat, base + direction * thermostat->step);
    state.edit_tap = millis();
    state.edit_unsent = true;

    // the TFT colours the icon with th_accent while it is pressed
    String reset(direction < 0 ? F("minus") : F("plus"));
    reset += F(".pco=");
    reset += color::White;
    cmd(std::move(reset));
    renderThermostat(false);
}

size_t weatherDays() {
    return std::min<size_t>(weather::current.count, config.weather_days);
}

void chip(size_t n, uint16_t glyph, const String& value, const String& label) {
    const auto pic = name("chip", n, "_pic");
    const auto icon = name("chip", n, "_icon");
    const auto text = name("chip", n, "_value");
    const auto caption = name("chip", n, "_text");
    setNum(pic, "pic", chipPic(n));
    setText(icon, utf8(glyph));
    setText(text, fit(value, ChipValue));
    setText(caption, fit(label, ChipText));
    vis(pic, true);
    vis(icon, true);
    vis(text, true);
    vis(caption, true);
}

void renderWeather(bool full) {
    const auto& data = weather::current;
    const size_t days = weatherDays();
    if (state.day >= days) {
        state.day = days ? (days - 1) : 0;
    }

    if (full) {
        loadPage(page::Weather);
        vis(F("close"), true);
        vis(F("close_hs"), true);
        vis(F("swipe"), true);
        refStop();
    }

    if (!days) {
        setText(F("title"), lang::texts().no_weather);
        vis(F("title"), true);
    } else {
        const auto& day = data.days[state.day];
        String title = (state.day == 0) ? String(lang::texts().today)
            : (state.day == 1) ? String(lang::texts().tomorrow)
            : String(lang::weekdayFull(day.wday));
        title += F(" \xC2\xB7 ");
        title += lang::dayMonth(day.mday, day.mon);
        setText(F("title"), fit(title, Title));
        vis(F("title"), true);

        setNum(F("weather_pic"), "pic", background().weather_page + weatherPicture(day.code, true));
        vis(F("weather_pic"), true);
        setText(F("high"), fit(temperatureText(day.tmax), WeatherHigh));
        vis(F("high"), true);
        vis(F("low_arrow"), true);
        setText(F("low"), fit(temperatureText(day.tmin), WeatherLow));
        vis(F("low"), true);

        size_t n = 1;
        char buf[16];
        if (!std::isnan(day.wind)) {
            snprintf(buf, sizeof(buf), lang::texts().wind_speed, lroundf(day.wind));
            chip(n++, icons::WeatherWindy, buf, lang::texts().wind);
        }
        if (day.rain >= 0) {
            snprintf(buf, sizeof(buf), "%d %%", day.rain);
            chip(n++, icons::WeatherRainy, buf, lang::texts().rain);
        }
        if (!std::isnan(day.uv)) {
            snprintf(buf, sizeof(buf), "%ld", lroundf(day.uv));
            chip(n++, icons::WeatherSunny, buf, lang::texts().uv);
        }

        showDots(days, state.day);
    }

    if (full) {
        refStar();
    }
}

void showHome() {
    state.history.clear();
    state.screen = Screen::Home;
    state.dirty = false;
    renderHome(true);
}

// a card of the swipe row ends the sub-page history
void showCard(size_t index) {
    if (index >= state.cards.size()) {
        showHome();
        return;
    }
    if (!state.cards[index].sub) {
        state.history.clear();
    }
    state.screen = Screen::Buttons;
    state.card = index;
    state.dirty = false;
    renderButtons(true);
}

void showWeather(size_t day) {
    state.screen = Screen::Weather;
    state.day = day;
    state.dirty = false;
    renderWeather(true);
}

void showBlank() {
    state.screen = Screen::Blank;
    state.dirty = false;
    loadPage(page::Blank);
}

// settings / thermostat remember the card or home they were opened from
void rememberBack() {
    if ((state.screen == Screen::Buttons) || (state.screen == Screen::Home)) {
        state.back = state.screen;
        state.back_card = state.card;
    }
}

void showSettings() {
    rememberBack();
    state.screen = Screen::Settings;
    state.dirty = false;
    renderSettings(true);
}

void showThermostat(size_t index) {
    if ((index >= config.items.size()) || !thermostatOf(config.items[index])) {
        return;
    }
    rememberBack();
    if (state.thermo_chart) {
        closeChart();
    }
    state.screen = Screen::Thermostat;
    state.thermo_item = index;
    state.dirty = false;
    renderThermostat(true);
}

// a switch tile with "history": its chart on the thermostat page, closing it goes back to where it was opened from
void showChart(size_t index) {
    if ((index >= config.items.size()) || !config.items[index].history.length()) {
        return;
    }
    rememberBack();
    if (state.thermo_chart) {
        closeChart();
    }
    state.screen = Screen::Thermostat;
    state.thermo_item = index;
    state.thermo_chart = true;
    state.dirty = false;
    renderChart(true);
}

void openChart() {
    if ((state.thermo_item >= config.items.size())) {
        return;
    }
    const auto* thermostat = thermostatOf(config.items[state.thermo_item]);
    if (!thermostat || !thermostat->history.length()) {
        return;
    }
    buzzer::beep();
    thermoFlush();
    state.thermo_chart = true;
    renderThermostat(true);
}

void showBack() {
    if ((state.back == Screen::Buttons) && (state.back_card < state.cards.size())) {
        state.screen = Screen::Buttons;
        state.card = state.back_card;
        state.dirty = false;
        renderButtons(true);
        return;
    }
    showHome();
}

// the chart is closed: back to the dial of a thermostat, or where a switch's chart was opened from
void chartClosed() {
    if ((state.thermo_item < config.items.size()) && thermostatOf(config.items[state.thermo_item])) {
        renderThermostat(true);
    } else {
        showBack();
    }
}

void showCurrent() {
    switch (state.screen) {
    case Screen::Buttons:
        showCard(state.card);
        break;
    case Screen::Weather:
        showWeather(state.day);
        break;
    case Screen::Blank:
        showBlank();
        break;
    case Screen::Settings:
        state.screen = Screen::Settings;
        renderSettings(true);
        break;
    case Screen::Thermostat:
        if ((state.thermo_item < config.items.size())
            && (thermostatOf(config.items[state.thermo_item]) || state.thermo_chart))
        {
            renderThermostat(true);
        } else {
            showHome();
        }
        break;
    case Screen::None:
    case Screen::Home:
        showHome();
        break;
    }
}

// a "type": "settings" page in the row, else the built-in settings tiles
void openSettings() {
    for (size_t card = 0; card < state.cards.size(); ++card) {
        if (state.cards[card].settings) {
            showCard(card);
            return;
        }
    }
    showSettings();
}

// a sub-page goes back to where it was opened from (home, a card of the row or another sub-page)
void goBack() {
    while (!state.history.empty()) {
        const int card = state.history.back();
        state.history.pop_back();
        if (card < 0) {
            break;
        }
        if (static_cast<size_t>(card) < state.cards.size()) {
            showCard(card);
            return;
        }
    }
    showHome();
}

constexpr size_t HistoryMax { 8 };

void showTarget(int target) {
    if (target >= 0) {
        const int card = firstCard(state.cards, target);
        if (card < 0) {
            return;
        }
        const bool here = (state.screen == Screen::Buttons) && samePage(state.cards, state.card, card);
        if (state.cards[card].sub && !here) {
            if (state.history.size() >= HistoryMax) {
                state.history.erase(state.history.begin());
            }
            state.history.push_back((state.screen == Screen::Buttons) ? static_cast<int>(state.card) : -1);
        }
        showCard(card);
    } else if (target == PageWeather) {
        showWeather(0);
    } else if (target == PageHome) {
        showHome();
    } else if (target == PageSettings) {
        openSettings();
    }
}

// cards split for the identified design (full grids on a known design, else the stock 4x2 slots); the shown
// card / the card to go back to fall back to home when they no longer exist
void syncCards() {
    state.pages = designGrids();
    state.grids = (design() != nullptr);
    buildCards(state.cards, TilesPerCard, Title.bytes, true, RowsPerSheet, state.grids);
    if (state.card >= state.cards.size()) {
        state.card = 0;
        if (state.screen == Screen::Buttons) {
            state.screen = Screen::Home;
        }
    }
    if (state.back_card >= state.cards.size()) {
        state.back = Screen::Home;
    }
    for (const auto card : state.history) {
        if (card >= static_cast<int>(state.cards.size())) {
            state.history.clear();
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// Link
// -----------------------------------------------------------------------------

void resetTransport() {
    state.queue.clear();
    state.waiting = false;
    state.since_sync = 0;
    state.sync_misses = 0;
    state.ping_misses = 0;
    state.rx_len = 0;
    state.cache.clear();
    state.sendxy = -1;
}

void search() {
    const uint8_t ff[3] {0xFF, 0xFF, 0xFF};
    uart::port().write(ff, sizeof(ff));
    write(F("sendme"));
    write(F("get design_ver"));
    state.last_search = millis();
}

void setLink(Link value) {
    state.link = value;
    state.link_since = millis();
}

void lostLink(const char* reason) {
    DEBUG_MSG_P(PSTR("[NSPANEL] OHpanel link lost: %s\n"), reason);
    resetTransport();
    setLink(Link::Search);
    state.lost = true;
}

void identify() {
    setLink(Link::Identify);
    state.identify_sent = millis();
    if (!state.design_known) {
        write(F("get design_ver"));
    }
    write(F("connect"));
    write(F("sendme")); // swallowed after connect on the device
}

void ready() {
    setLink(Link::Ready);
    state.was_ready = true;
    state.lost = false;
    state.last_rx = millis();
    state.last_ping = millis();
    state.last_touch = millis();
    state.level = -1;
    state.dimmed = false;
    state.asleep = false;

    const char* design = state.design_known ? state.design.c_str() : (state.design_missing ? "none" : "unknown");
    DEBUG_MSG_P(PSTR("[NSPANEL] OHpanel TFT ready: design %s, model %s, page %d\n"),
        design, state.model.length() ? state.model.c_str() : "?", state.tft_page);
    const auto* info = ohpanel::design();
    if (!info) {
        DEBUG_MSG_P(PSTR("[NSPANEL] Unexpected OHpanel design '%s' (expected %s or %s), objects may not match. Flash the pinned TFT\n"),
            design, DesignCustom, Design);
    } else {
        DEBUG_MSG_P(PSTR("[NSPANEL] OHpanel design: %s\n"), designText().c_str());
        if (!info->backgrounds && (config.background != 0)) {
            DEBUG_MSG_P(PSTR("[NSPANEL] Background '%s' is not in design %s, aurora used\n"),
                (config.background < (sizeof(BackgroundNames) / sizeof(BackgroundNames[0])))
                    ? BackgroundNames[config.background] : "?", info->id);
        }
    }

    announceDriver(Driver::Ohpanel, design, state.model);

    resetTransport();
    syncCards();
    sendTheme();
    showCurrent();
    setLevel(config.brightness);
}

void onRebootFrame() {
    if ((state.link == Link::Ready) || (state.link == Link::Identify)) {
        DEBUG_MSG_P(PSTR("[NSPANEL] OHpanel TFT restarted\n"));
        resetTransport();
        state.design_known = false;
        state.design_missing = false;
        state.reboot_at = millis();
        setLink(Link::Rebooted);
    }
    state.reboot_last = millis();
}

// -----------------------------------------------------------------------------
// Events
// -----------------------------------------------------------------------------

// any event but currentpage: true when the event must not act (it woke the screen)
bool touched() {
    state.last_touch = millis();
    if (state.asleep) {
        state.asleep = false;
        state.dimmed = false;
        showHome();
        setLevel(config.brightness);
        return true;
    }
    if (state.dimmed) {
        state.dimmed = false;
        setLevel(config.brightness);
    }
    return false;
}

void tap(size_t index) {
    if (index >= config.items.size()) {
        return;
    }

    const auto& item = config.items[index];
    if (item.type == ItemType::Link) {
        buzzer::beep();
        showTarget(item.target.page);
        return;
    }

    if (item.type == ItemType::Thermostat) {
        buzzer::beep();
        showThermostat(index);
        return;
    }

    if (item.history.length()) {
        buzzer::beep();
        showChart(index);
        return;
    }

    if (item.type == ItemType::Button) {
        if (state.pulse_until.size() != config.items.size()) {
            state.pulse_until.assign(config.items.size(), 0);
        }
        state.pulse_until[index] = millis() + PulseMs;
        state.pulse_check = state.pulse_until[index] + 10;
        markDirty();
    }

    itemTap(index, -1);
}

void swipe(const String& direction) {
    const bool left = (direction == F("left"));
    const bool right = (direction == F("right"));

    switch (state.screen) {
    case Screen::Home:
        if (left && rowCards(state.cards)) {
            showCard(0);
        } else if (!left && !right && config.weather && weatherDays()) {
            showWeather(0);
        }
        break;

    case Screen::Buttons:
        // a sub-page: its own sheets, right of the first one is where it was opened from
        if ((state.card < state.cards.size()) && state.cards[state.card].sub) {
            if (left) {
                if (samePage(state.cards, state.card, state.card + 1)) {
                    showCard(state.card + 1);
                }
            } else if (right) {
                if ((state.card > 0) && samePage(state.cards, state.card, state.card - 1)) {
                    showCard(state.card - 1);
                } else {
                    goBack();
                }
            } else {
                showHome();
            }
            break;
        }
        if (left) {
            if ((state.card + 1) < rowCards(state.cards)) {
                showCard(state.card + 1);
            }
        } else if (right) {
            if (state.card > 0) {
                showCard(state.card - 1);
            } else {
                showHome();
            }
        } else {
            showHome();
        }
        break;

    case Screen::Weather:
        if (left) {
            if ((state.day + 1) < weatherDays()) {
                showWeather(state.day + 1);
            }
        } else if (right) {
            if (state.day > 0) {
                showWeather(state.day - 1);
            } else {
                showHome();
            }
        } else {
            showHome();
        }
        break;

    case Screen::Thermostat:
        // the chart: right goes back to the dial, up / down leave the thermostat
        if (state.thermo_chart && !left) {
            closeChart();
            if (right) {
                chartClosed();
            } else {
                showBack();
            }
            break;
        }
        if (!left) {
            showBack();
        }
        break;

    case Screen::Settings:
        if (!left) {
            showBack();
        }
        break;

    case Screen::Blank:
    case Screen::None:
        break;
    }
}

// settings tile n: toggles, saves (nspIcons / nspTheme) and redraws the page with the new look
void settingsTap(long n) {
    buzzer::beep();
    if (n == 1) {
        saveLayout((config.layout == Layout::Center) ? Layout::Corner : Layout::Center);
    } else if (n == 2) {
        saveTheme((config.theme == Theme::Light) ? Theme::Dark : Theme::Light);
    } else if (n == 3) {
        // the next background; a design with aurora only (d3731fbb) keeps it
        const auto* info = design();
        if (info && !info->backgrounds) {
            return;
        }
        constexpr size_t Count { sizeof(Backgrounds) / sizeof(Backgrounds[0]) };
        saveBackground((backgroundIndex() + 1) % Count);
        sendTheme();
    } else {
        return;
    }
    DEBUG_MSG_P(PSTR("[NSPANEL] Settings: icons %s, theme %s, background %s\n"),
        (config.layout == Layout::Corner) ? "corner" : "center", (config.theme == Theme::Light) ? "light" : "dark",
        BackgroundNames[backgroundIndex()]);
    showCurrent();
}

// a release that survived the swipe window
void act(const String& component) {
    if (component == F("close")) {
        buzzer::beep();
        if ((state.screen == Screen::Thermostat) && state.thermo_chart) {
            closeChart();
            chartClosed();
        } else if ((state.screen == Screen::Settings) || (state.screen == Screen::Thermostat)) {
            showBack();
        } else if ((state.screen == Screen::Buttons) && (state.card < state.cards.size())
            && state.cards[state.card].sub)
        {
            goBack();
        } else {
            showHome();
        }
        return;
    }

    if (state.screen == Screen::Thermostat) {
        if (state.thermo_chart) {
            return;
        }
        if (component == F("minus")) {
            thermoStep(-1);
        } else if (component == F("plus")) {
            thermoStep(1);
        } else if (component == F("mode1")) {
            openChart();
        }
        return;
    }

    if (component.startsWith(F("tile")) && (state.screen == Screen::Settings)) {
        settingsTap(component.substring(4).toInt());
        return;
    }

    if (component.startsWith(F("tile")) && (state.screen == Screen::Buttons) && (state.card < state.cards.size())
        && state.cards[state.card].settings)
    {
        settingsTap(component.substring(4).toInt());
        return;
    }

    if ((component == F("weather")) && (state.screen == Screen::Home)) {
        if (config.weather && weatherDays()) {
            buzzer::beep();
            showWeather(0);
        }
        return;
    }

    if (component.startsWith(F("tile")) && (state.screen == Screen::Buttons) && (state.card < state.cards.size())) {
        const auto& card = state.cards[state.card];
        const int n = card.list ? -1 : slotTile(component.substring(4).toInt(), card.grid);
        if ((n >= 0) && (static_cast<size_t>(n) < card.count)) {
            tap(config.pages[card.page].items[card.first + n]);
        }
        return;
    }

    // "row<n>": a tap on a list row, from the sendxy coordinates
    if (component.startsWith(F("row")) && (state.screen == Screen::Buttons) && (state.card < state.cards.size())) {
        const auto& card = state.cards[state.card];
        const auto n = component.substring(3).toInt();
        if (card.list && (n >= 1) && (static_cast<size_t>(n) <= std::min<size_t>(card.count, RowsPerSheet))) {
            tap(config.pages[card.page].items[card.first + n - 1]);
        }
        return;
    }

    if (component.startsWith(F("shortcut")) && (state.screen == Screen::Home)) {
        const auto n = component.substring(8).toInt();
        if ((n >= 1) && (static_cast<size_t>(n) <= std::min(config.shortcuts.size(), ShortcutsMax))) {
            tap(config.shortcuts[n - 1]);
        }
        return;
    }
}

int pageId(const String& name) {
    if (name == F("home")) {
        return page::Home;
    }
    if (name == F("weather")) {
        return page::Weather;
    }
    if (name == F("blank")) {
        return page::Blank;
    }
    // `buttons` and the grid pages of the custom design
    for (const auto& tiles : TilePages) {
        if (name == tiles.name) {
            return tiles.id;
        }
    }
    if (name == F("entities_rows")) {
        return page::EntitiesRows;
    }
    if (name == F("thermostat")) {
        return page::Thermostat;
    }
    return -1;
}

void onEvent(const char* page_name, const char* component, const char* value) {
    ++state.events;
    const String comp(component);
    const String val(value);

    if (comp == F("currentpage")) {
        state.tft_page_name = page_name;
        const int id = pageId(state.tft_page_name);
        // the TFT never navigates by itself; anything else is put back
        if ((state.link == Link::Ready) && (state.requested_page >= 0) && (id != state.requested_page)
            && (millis() - state.requested_at >= PageRetryMs))
        {
            DEBUG_MSG_P(PSTR("[NSPANEL] TFT shows '%s', expected page %d\n"), page_name, state.requested_page);
            showCurrent();
        }
        return;
    }

    if (state.link != Link::Ready) {
        return;
    }

    if (touched()) {
        return;
    }

    const auto now = millis();
    if (comp == F("swipe")) {
        state.last_swipe = now;
        state.release_pending = false;
        swipe(val);
        return;
    }

    // a touch on the page background acts on nothing; it must not replace a pending row or tile release
    if (comp == F("touch")) {
        return;
    }

    if (val == F("release")) {
        if (now - state.last_swipe < SwipeGuardMs) {
            return;
        }
        // a tile of the page before the last page change: its slot means another item now
        if (comp.startsWith(F("tile")) && (state.requested_page >= 0)) {
            const int id = pageId(String(page_name));
            if ((id >= 0) && (id != state.requested_page)) {
                return;
            }
        }
        state.release_pending = true;
        state.release_component = comp;
        state.release_at = now;
    }
}

constexpr int TapSlop { 40 };  // px a tap may move (a swipe is 100 px or more)

// 67 frame (sendxy=1, only on entities_rows and drawn grids): a row / tile tap is a press and a release
// close to it, at the press point; then the same swipe disambiguation as a tile release
void onTouchXY(int x, int y, bool press) {
    if (state.link != Link::Ready) {
        return;
    }
    state.last_touch = millis();

    if (press) {
        state.touch_x = x;
        state.touch_y = y;
        return;
    }

    const int down_x = state.touch_x;
    const int down_y = state.touch_y;
    state.touch_x = -1;
    state.touch_y = -1;

    if ((state.screen != Screen::Buttons) || (state.card >= state.cards.size())) {
        return;
    }
    const auto& card = state.cards[state.card];
    const bool is_drawn = drawnCard(card);
    if (!card.list && !is_drawn) {
        return;
    }
    if ((down_x < 0) || (std::abs(x - down_x) > TapSlop) || (std::abs(y - down_y) > TapSlop)) {
        return;
    }

    // a list row, or a drawn tile (tile<n>, the slot of the n-th tile on a drawn card); none outside
    const size_t hit = card.list ? rowAt(down_x, down_y)
        : drawn::tileAt(card.grid, std::min<size_t>(card.count, gridTiles(card.grid)), down_x, down_y);
    const auto now = millis();
    if (!hit || (now - state.last_swipe < SwipeGuardMs)) {
        return;
    }

    if (config.debug) {
        DEBUG_MSG_P(PSTR("[NSPANEL] %s tap %d,%d: %u\n"), card.list ? "list" : "tile", down_x, down_y, hit);
    }
    state.release_pending = true;
    state.release_component = String(card.list ? F("row") : F("tile")) + hit;
    state.release_at = now;
}

void onFrame(const uint8_t* data, size_t len) {
    ++state.rx_frames;
    state.last_rx = millis();
    state.ping_misses = 0;

    if (state.link == Link::Rebooted) {
        state.reboot_last = millis();
    }

    if ((len == 3) && !data[0] && !data[1] && !data[2]) {
        onRebootFrame();
        return;
    }

    if (len == 1) {
        switch (data[0]) {
        case 0x88:
            onRebootFrame();
            break;
        case 0x1A:
            if ((state.link == Link::Search) || (state.link == Link::Identify)) {
                state.design_missing = true;
            } else {
                ++state.errors;
                state.last_error = data[0];
            }
            break;
        case 0x24:
            ++state.overflows;
            break;
        default:
            ++state.errors;
            state.last_error = data[0];
            break;
        }
        if (config.debug) {
            DEBUG_MSG_P(PSTR("[NSPANEL] <- status %02X\n"), data[0]);
        }
        return;
    }

    if ((data[0] == 0x66) && (len == 2)) {
        state.tft_page = data[1];
        if (state.waiting) {
            state.waiting = false;
            state.sync_misses = 0;
            state.sync_rtt_max = std::max<uint32_t>(state.sync_rtt_max, millis() - state.wait_since);
        }
        if (state.link == Link::Search) {
            identify();
        }
        return;
    }

    if ((data[0] == 0x71) && (len == 5)) {
        const uint32_t raw = data[1]
            | (static_cast<uint32_t>(data[2]) << 8)
            | (static_cast<uint32_t>(data[3]) << 16)
            | (static_cast<uint32_t>(data[4]) << 24);
        char buf[12];
        snprintf(buf, sizeof(buf), "%08lx", static_cast<unsigned long>(raw));
        state.design = buf;
        state.design_known = true;
        state.design_missing = false;
        if (state.link == Link::Search) {
            identify();
        }
        return;
    }

    if ((len > 6) && (0 == memcmp(data, "comok ", 6))) {
        state.comok = String();
        for (size_t n = 0; (n < len) && (n < 96); ++n) {
            if ((data[n] >= 0x20) && (data[n] < 0x7F)) {
                state.comok += static_cast<char>(data[n]);
            }
        }
        // comok 1,30614-0,NX4832F035_011R,163,61488,D264B8204F0E1828,16777216
        const int first = state.comok.indexOf(',');
        const int second = (first >= 0) ? state.comok.indexOf(',', first + 1) : -1;
        const int third = (second >= 0) ? state.comok.indexOf(',', second + 1) : -1;
        state.model = ((second >= 0) && (third > second)) ? state.comok.substring(second + 1, third) : String();
        return;
    }

    if (data[0] == 0x92) {
        // 92 "nspanelevent" 00 <json> 00
        size_t pos = 1;
        while ((pos < len) && data[pos]) {
            ++pos;
        }
        ++pos;
        size_t end = len;
        if ((end > pos) && !data[end - 1]) {
            --end;
        }
        if (pos >= end) {
            return;
        }

        char json[256];
        const size_t size = std::min<size_t>(end - pos, sizeof(json) - 1);
        memcpy(json, data + pos, size);
        json[size] = '\0';

        if (config.debug) {
            DEBUG_MSG_P(PSTR("[NSPANEL] <- %s\n"), json);
        }
        publishEvent(json);

        StaticJsonBuffer<384> buffer;
        JsonObject& root = buffer.parseObject(json);
        if (!root.success()) {
            ++state.errors;
            return;
        }

        onEvent(root["page"] | "", root["component"] | "", root["value"] | "");
        return;
    }

    // 67 touch coordinates (sendxy=1): x, y big-endian, 01 press / 00 release
    if ((data[0] == 0x67) && (len == 6)) {
        onTouchXY((data[1] << 8) | data[2], (data[3] << 8) | data[4], data[5] != 0);
        return;
    }

    // 91 value reports (sliders), 65 touch, 70 strings: not used by these pages
    if (config.debug) {
        DEBUG_MSG_P(PSTR("[NSPANEL] <- frame %02X, %u bytes\n"), data[0], len);
    }
}

void rxLoop() {
    auto& port = uart::port();
    size_t budget = 1024;
    while (budget-- && port.available()) {
        const int byte = port.read();
        if (byte < 0) {
            break;
        }

        if (state.rx_len >= FrameMax) {
            state.rx_len = 0; // noise or another protocol
            ++state.errors;
        }

        state.rx[state.rx_len++] = byte;
        if ((state.rx_len >= 3)
            && (state.rx[state.rx_len - 1] == 0xFF)
            && (state.rx[state.rx_len - 2] == 0xFF)
            && (state.rx[state.rx_len - 3] == 0xFF))
        {
            const size_t len = state.rx_len - 3;
            state.rx_len = 0;
            if (len) {
                onFrame(state.rx, len);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Periodic work
// -----------------------------------------------------------------------------

void idleTick(uint32_t now) {
    if (state.asleep || !config.timeout) {
        return;
    }

    if (!state.dimmed && (now - state.last_touch >= (config.timeout * 1000ul))) {
        state.dimmed = true;
        if (!config.dim) {
            state.asleep = true;
            showBlank();
            setLevel(0);
            return;
        }
        if (state.screen != Screen::Home) {
            showHome();
        }
        setLevel(config.dim);
    }
}

void uiLoop(uint32_t now) {
    if (state.release_pending && (now - state.release_at >= ReleaseHoldMs)) {
        state.release_pending = false;
        act(state.release_component);
    }

    if (state.pulse_check && (static_cast<int32_t>(now - state.pulse_check) >= 0)) {
        state.pulse_check = 0;
        markDirty();
    }

    thermoLoop(now);
    chartLoop(now);
    idleTick(now);

    if (now - state.last_tick >= 1000) {
        state.last_tick = now;
        if (state.screen == Screen::Home) {
            homeClock();
            homeStatus();
        }
    }

    if (state.dirty && (now - state.dirty_since >= RenderDelayMs) && state.queue.empty()) {
        state.dirty = false;
        switch (state.screen) {
        case Screen::Home:
            renderHome(false);
            break;
        case Screen::Buttons:
            renderButtons(false);
            break;
        case Screen::Weather:
            renderWeather(false);
            break;
        case Screen::Settings:
            renderSettings(false);
            break;
        case Screen::Thermostat:
            renderThermostat(false);
            break;
        case Screen::Blank:
        case Screen::None:
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// Driver interface (link:: / dispatch in nspanel.cpp)
// -----------------------------------------------------------------------------

void begin() {
    uart::setBaud(Baudrate);
    resetTransport();
    state.lost = false;
    state.was_ready = false;
    state.design_known = false;
    state.design_missing = false;
    state.tft_page = -1;
    setLink(Link::Search);
    syncCards();
    search();
}

bool connected() {
    return state.link == Link::Ready;
}

// something answered at 921600: link:: keeps this driver while it identifies the TFT
bool found() {
    return state.link != Link::Search;
}

bool lost() {
    return state.lost;
}

void loop() {
    rxLoop();

    const auto now = millis();
    switch (state.link) {
    case Link::Search:
        if (now - state.last_search >= SearchRetryMs) {
            search();
        }
        if (state.was_ready && !state.lost && (now - state.link_since >= SearchLostMs)) {
            DEBUG_MSG_P(PSTR("[NSPANEL] OHpanel TFT does not answer\n"));
            state.lost = true;
        }
        return;

    case Link::Identify:
        if ((state.design_known || state.design_missing) && (state.comok.length() || (now - state.identify_sent >= 300))) {
            ready();
        } else if (now - state.link_since >= IdentifyMaxMs) {
            ready();
        } else if (now - state.identify_sent >= IdentifyRetryMs) {
            identify();
        }
        return;

    case Link::Rebooted:
        if ((now - state.reboot_last >= RebootQuietMs) || (now - state.reboot_at >= RebootMaxMs)) {
            setLink(Link::Search);
            search();
        }
        return;

    case Link::Ready:
        break;
    }

    txLoop();
    if (state.link != Link::Ready) {
        return;
    }

    // no keepalive in the TFT: ping when it has been quiet
    if (!state.waiting && state.queue.empty() && (now - state.last_rx >= PingMs) && (now - state.last_ping >= PingMs)) {
        if (state.ping_misses >= PingMissesMax) {
            lostLink("pings unanswered");
            return;
        }
        state.last_ping = now;
        ++state.ping_misses;
        write(F("sendme"));
    }

    uiLoop(now);
}

void reload() {
    syncCards();
    state.pulse_until.assign(config.items.size(), 0);
    // item indices may have changed: an unsent setpoint edit is dropped
    state.edit_item = -1;
    state.edit_value = NAN;
    state.edit_unsent = false;
    if ((state.screen == Screen::Thermostat)
        && ((state.thermo_item >= config.items.size())
            || !(thermostatOf(config.items[state.thermo_item])
                || (state.thermo_chart && config.items[state.thermo_item].history.length()))))
    {
        state.screen = Screen::Home;
    }
    if (connected()) {
        sendTheme();
        state.level = -1;
        showCurrent();
        setLevel(state.dimmed ? config.dim : config.brightness);
    }
}

void weatherChanged() {
    if (connected() && ((state.screen == Screen::Home) || (state.screen == Screen::Weather))) {
        markDirty();
    }
}

void temperatureChanged() {
    if (connected() && (state.screen == Screen::Home)) {
        homeIndoor();
    }
}

void entityChanged(int entity) {
    if (!connected()) {
        return;
    }

    if (state.screen == Screen::Buttons) {
        if (cardContains(state.cards, state.card, entity)) {
            markDirty();
        }
    } else if (state.screen == Screen::Thermostat) {
        if ((state.thermo_item < config.items.size()) && itemUses(config.items[state.thermo_item], entity)) {
            markDirty();
        }
    } else if (state.screen == Screen::Home) {
        for (size_t n = 0; n < config.info_count; ++n) {
            if ((config.info[n].entity == entity) || (config.info[n].extra == entity)) {
                markDirty();
                return;
            }
        }
        for (const auto index : config.shortcuts) {
            if (itemUses(config.items[index], entity)) {
                markDirty();
                return;
            }
        }
#if RELAY_SUPPORT
        const auto& item = config.entities[entity];
        if ((item.relay >= 0) && (item.relay < 2) && config.relay_labels[item.relay].length()) {
            markDirty();
        }
#endif
    }
}

void relayChanged(size_t id) {
    if (connected() && (state.screen == Screen::Home) && (id < 2) && config.relay_labels[id].length()) {
        markDirty();
    }
}

void showPage(int target) {
    if (connected()) {
        state.last_touch = millis();
        state.dimmed = false;
        state.asleep = false;
        showTarget(target);
        setLevel(config.brightness);
    }
}

void send(String command) {
    cmd(std::move(command));
}

void info(Print& out) {
    out.printf_P(PSTR("OHpanel: link %s, design %s%s, model %s, TFT page %d ('%s')\n"),
        linkName(state.link),
        state.design_known ? state.design.c_str() : (state.design_missing ? "none" : "unknown"),
        design() ? " (ok)" : " (UNEXPECTED)",
        state.model.length() ? state.model.c_str() : "?",
        state.tft_page, state.tft_page_name.c_str());
    if (design()) {
        const auto& bg = background();
        out.printf_P(PSTR("Design: %s; cards split for %s; background %s (bg_pic %u)\n"),
            designText().c_str(), state.pages ? "the grid pages" : state.grids ? "drawn grids" : "the 4x2 slots",
            BackgroundNames[backgroundIndex()], bg.pic);
    }
    out.printf_P(PSTR("Screen: %s, card %u / %u, day %u, level %d, dimmed %s, asleep %s, idle %u s\n"),
        screenName(state.screen), state.card + 1, state.cards.size(), state.day + 1, state.level,
        state.dimmed ? "yes" : "no", state.asleep ? "yes" : "no",
        static_cast<unsigned>((millis() - state.last_touch) / 1000));
    if ((state.screen == Screen::Buttons) && (state.card < state.cards.size())) {
        const auto& card = state.cards[state.card];
        const bool is_drawn = drawnCard(card);
        out.printf_P(PSTR("Card: '%s', %s, layout %s %s page %s, %u tiles\n"), card.title.c_str(),
            card.list ? "list" : card.settings ? "settings" : "grid", gridName(card.grid),
            is_drawn ? "drawn on" : "on", card.list ? "entities_rows" : tilePage(card.grid).name, card.count);
    }
    if (state.draw_page_commands) {
        out.printf_P(PSTR("Drawn: last page %u tiles, %u commands, %u bytes; last update %u tiles, %u commands, %u bytes; sendxy %d\n"),
            state.draw_page_tiles, state.draw_page_commands, state.draw_page_bytes,
            state.draw_update_tiles, state.draw_update_commands, state.draw_update_bytes, state.sendxy);
    }
    out.printf_P(PSTR("TX:%u commands, %u bytes, queued %u, dropped %u, syncs %u, sync timeouts %u, max rtt %u ms%s\n"),
        state.tx_commands, state.tx_bytes, state.queue.size(), state.dropped, state.syncs,
        state.sync_timeouts, state.sync_rtt_max, state.waiting ? ", waiting" : "");
    out.printf_P(PSTR("RX: %u frames, %u events, overflows (24) %u, errors %u (last %02X), cache %u\n"),
        state.rx_frames, state.events, state.overflows, state.errors, state.last_error, state.cache.size());
    if (state.comok.length()) {
        out.printf_P(PSTR("Display: %s\n"), state.comok.c_str());
    }
}

} // namespace ohpanel
