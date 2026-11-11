/*

NSPANEL MODULE

Sonoff NSPanel (gen1 EU, ESP32) backend with two display drivers, chosen by JSON "display"
("auto" detects the TFT at boot, after a TFT upload and when the display goes silent):
- nspanel_ohpanel.h: OHpanel TFT (bc629e1e), plain Nextion commands at 921600, swipes.
  Protocol reference: nspanel/ohpanel-protocol.md
- nspanel_lovelace.h: joBr99 "NSPanel Lovelace UI" TFT, 55 BB frames at 115200.
  Protocol reference: nspanel/lovelace-protocol.md

Shared core (this file):
- UART2 (TX GPIO16, RX GPIO17) ownership, display detection (link::), Nextion TFT upload (flash::)
- config model from /nspanel.json: pages of tiles, groups/scenes, shortcuts, hardware buttons
- MQTT state map (absolute state topics), local relays, hardware button custom actions
- Open-Meteo weather task, room temperature (from the ESPurna NTC sensor), buzzer
- LittleFS on the existing data/spiffs partition, /nspanel and /nspanel.json web endpoints

Flash layout, `safeboot`, crash-loop guard and updates from the release list: arch/esp32/safeboot_esp32.cpp, as on
every ESP32 image

*/

#include "espurna.h"

#if NSPANEL_SUPPORT

#include "nspanel.h"
#include "nspanel_html.h"
#include "nspanel_icons.h"

#if BUTTON_SUPPORT
#include "button.h"
#endif

#if MQTT_SUPPORT
#include "mqtt.h"
#endif

#if RELAY_SUPPORT
#include "relay.h"
#endif

#if SENSOR_SUPPORT
#include "sensor.h"
#endif

#if WEB_SUPPORT
#include "web.h"
#include "ws.h"
#endif

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>

#include <esp_attr.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <atomic>
#include <cmath>
#include <cstdarg>
#include <ctime>
#include <deque>
#include <map>
#include <memory>
#include <new>
#include <vector>

namespace espurna {
namespace nspanel {
namespace {

namespace build {

constexpr int DisplayPowerPin { 4 };      // LOW = display ON
constexpr int TxPin { 16 };
constexpr int RxPin { 17 };
constexpr uint32_t LovelaceBaud { 115200 };
constexpr uint32_t OhpanelBaud { 921600 };
constexpr int BuzzerPin { 21 };
constexpr int BuzzerChannel { 15 };       // pwm.cpp allocates LEDC channels from 0

constexpr size_t EventsMax { 8 };

// display detection (link::)
constexpr uint32_t ProbeOhpanelMs { 600 };      // auto: one `sendme` + `get design_ver` at 921600
constexpr uint32_t ProbeLovelaceMs { 2000 };    // auto: pageType~pageStartup at 115200, wait for event,startup
constexpr uint32_t ProbeForcedOhpanelMs { 2000 };
constexpr uint32_t ProbeForcedLovelaceMs { 5000 };
constexpr uint32_t ProbePowerCycleMs { 30000 }; // no answer for this long: power-cycle the display once
constexpr uint32_t PowerOffMs { 800 };
constexpr uint32_t PowerBootMs { 1500 };
constexpr uint32_t FlashQuietMs { 180000 };     // no automatic power-cycle this long after a TFT upload

constexpr size_t ConfigMax { 16384 };
constexpr char ConfigPath[] = "/nspanel.json";
constexpr char ConfigTmpPath[] = "/nspanel.json.tmp";

// config parse heap budget, ref. heapAvailable(): the Config built from the text takes about
// 3 bytes per byte of JSON; this much stays free for WiFi / lwIP / a TLS session meanwhile
constexpr size_t ConfigHeapPerByte { 3 };
constexpr size_t ConfigHeapReserve { 16384 };
constexpr size_t ConfigBlockReserve { 1024 };

// ArduinoJson 5 stops at 10 levels by default; each nested "page" adds 3 (item, page, items)
constexpr uint8_t ConfigNestingLimit { 24 };

// POST /nspanel.json: loop() waits this long for a running weather fetch (TLS, 40-60 KB)
// before parsing; the client gets 503 when no answer is ready after UploadAnswerMs
constexpr uint32_t UploadWeatherWaitMs { 10000 };
constexpr uint32_t UploadAnswerMs { 15000 };

constexpr uint32_t WeatherStack { 12288 };
constexpr size_t WeatherHeapMin { 50000 };
constexpr uint32_t WeatherRetryMs { 60000 };

// MQTT entity subscriptions are sent from loop(), a few at a time (ref. subscribeLoop())
constexpr size_t SubscribeBurst { 2 };
constexpr uint32_t SubscribeStepMs { 20 };

// TFT text limits, bytes (Max Text Size of the 51 HMI)
constexpr size_t HeadingMax { 40 };
constexpr size_t NameMax { 30 };
constexpr size_t DateMax { 40 };
constexpr size_t ForecastMax { 10 };

} // namespace build

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

String utf8(uint16_t code) {
    String out;
    if (!code) {
        return out;
    }

    char buf[4];
    if (code < 0x80) {
        buf[0] = static_cast<char>(code);
        buf[1] = '\0';
    } else if (code < 0x800) {
        buf[0] = static_cast<char>(0xC0 | (code >> 6));
        buf[1] = static_cast<char>(0x80 | (code & 0x3F));
        buf[2] = '\0';
    } else {
        buf[0] = static_cast<char>(0xE0 | (code >> 12));
        buf[1] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        buf[2] = static_cast<char>(0x80 | (code & 0x3F));
        buf[3] = '\0';
    }

    out = buf;
    return out;
}

// cut at 'max' bytes without splitting a multi-byte sequence
void utf8Truncate(String& value, size_t max) {
    if (value.length() <= max) {
        return;
    }

    size_t len = max;
    while ((len > 0) && ((static_cast<uint8_t>(value[len]) & 0xC0) == 0x80)) {
        --len;
    }

    value.remove(len);
}

// first 'chars' UTF-8 characters
String utf8Prefix(const String& value, size_t chars) {
    size_t pos = 0;
    size_t count = 0;
    while (pos < value.length()) {
        if ((static_cast<uint8_t>(value[pos]) & 0xC0) != 0x80) {
            if (count == chars) {
                break;
            }
            ++count;
        }
        ++pos;
    }

    return value.substring(0, pos);
}

// '~' separates fields of ESP->TFT commands
String sanitize(const char* value, size_t max) {
    String out(value ? value : "");
    out.replace("~", "-");
    out.replace("\r", "");
    out.replace("\n", " ");
    utf8Truncate(out, max);
    return out;
}

uint16_t crc16(const uint8_t* data, size_t len, uint16_t crc = 0xFFFF) {
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }

    return crc;
}

// Sakamoto, 0 = Sunday
int weekday(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) {
        y -= 1;
    }
    return (y + y / 4 - y / 100 + y / 400 + t[(m - 1) % 12] + d) % 7;
}

// screen texts, dates and the decimal separator, by "language" in the JSON
namespace lang {

enum class Language : uint8_t {
    English,
    Russian,
};

Language current { Language::English };

struct Texts {
    const char* weekday_full[7];
    const char* weekday_short[7];
    const char* month[12];          // as used in a date: "October" / "октября"
    const char* month_short[12];
    const char* on;
    const char* off;
    const char* on_count;           // followed by the number of devices on
    const char* settings;
    const char* heating;
    const char* target;             // followed by the setpoint
    const char* no_weather;
    const char* today;
    const char* tomorrow;
    const char* wind_speed;         // printf, one %ld (m/s)
    const char* wind;
    const char* rain;
    const char* uv;
    const char* icons;
    const char* icons_large;
    const char* icons_small;
    const char* theme;
    const char* theme_light;
    const char* theme_dark;
    const char* background;
    const char* backgrounds[5];     // same order as BackgroundNames
    const char* history;            // thermostat: the chart button
    const char* no_history;
    const char* hours;              // chart title suffix, followed by the number of hours
    const char* temperature;        // chart legend
    const char* setpoint;
    const char* humidity;
};

const Texts English {
    { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" },
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" },
    { "January", "February", "March", "April", "May", "June",
      "July", "August", "September", "October", "November", "December" },
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" },
    "On", "Off", "On: ",
    "Settings",
    "Heating", "Target ",
    "Weather: no data", "Today", "Tomorrow", "%ld m/s", "Wind", "Rain", "UV index",
    "Icons", "Large", "Small",
    "Theme", "Light", "Dark",
    "Backdrop", { "Aurora", "Black", "Candy", "Dunes", "Forest" }, // "Background" is wider than the tile name
    "History", "No history yet", " h", "Temperature", "Target", "Humidity",
};

const Texts Russian {
    { "Воскресенье", "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота" },
    { "Вс", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб" },
    { "января", "февраля", "марта", "апреля", "мая", "июня",
      "июля", "августа", "сентября", "октября", "ноября", "декабря" },
    { "янв", "фев", "мар", "апр", "мая", "июн", "июл", "авг", "сен", "окт", "ноя", "дек" },
    "Вкл", "Выкл", "Вкл: ",
    "Настройки",
    "Нагрев", "Цель ",
    "Погода: нет данных", "Сегодня", "Завтра", "%ld м/с", "Ветер", "Осадки", "УФ-индекс",
    "Иконки", "Крупные", "Мелкие",
    "Тема", "Светлая", "Тёмная",
    "Фон", { "Сияние", "Чёрный", "Огоньки", "Дюны", "Лес" },
    "История", "Истории пока нет", " ч", "Температура", "Цель", "Влажность",
};

const Texts& texts() {
    return (current == Language::Russian) ? Russian : English;
}

const char* weekdayFull(int wday) {
    return texts().weekday_full[((wday % 7) + 7) % 7];
}

const char* weekdayShort(int wday) {
    return texts().weekday_short[((wday % 7) + 7) % 7];
}

// "Saturday, October 24, 2026" / "Суббота, 24 октября 2026", shorter when longer than DateMax
String date(const tm& t) {
    const char* const full = weekdayFull(t.tm_wday);
    const char* const month = texts().month[((t.tm_mon % 12) + 12) % 12];
    const int year = t.tm_year + 1900;
    const bool ru = (current == Language::Russian);

    char buf[96];
    if (ru) {
        snprintf(buf, sizeof(buf), "%s, %d %s %d", full, t.tm_mday, month, year);
    } else {
        snprintf(buf, sizeof(buf), "%s, %s %d, %d", full, month, t.tm_mday, year);
    }
    if (strlen(buf) <= build::DateMax) {
        return buf;
    }

    if (ru) {
        snprintf(buf, sizeof(buf), "%s, %d %s", full, t.tm_mday, month);
    } else {
        snprintf(buf, sizeof(buf), "%s, %s %d", full, month, t.tm_mday);
    }
    if (strlen(buf) <= build::DateMax) {
        return buf;
    }

    if (ru) {
        snprintf(buf, sizeof(buf), "%s, %d %s %d", weekdayShort(t.tm_wday), t.tm_mday, month, year);
    } else {
        snprintf(buf, sizeof(buf), "%s, %s %d, %d", weekdayShort(t.tm_wday), month, t.tm_mday, year);
    }
    return buf;
}

// "Oct 25" / "25 окт"
String dayMonth(int mday, int mon) {
    const char* const month = texts().month_short[((mon % 12) + 12) % 12];
    char buf[24];
    if (current == Language::Russian) {
        snprintf(buf, sizeof(buf), "%d %s", mday, month);
    } else {
        snprintf(buf, sizeof(buf), "%s %d", month, mday);
    }
    return buf;
}

// "Sun, Oct 25" / "Вс, 25 окт" (OHpanel home: 165 px / 26 bytes)
String dateShort(const tm& t) {
    String out(weekdayShort(t.tm_wday));
    out += F(", ");
    out += dayMonth(t.tm_mday, t.tm_mon);
    return out;
}

// a printf'ed number for the screen: "21.5" / "21,5"
void decimal(char* text) {
    if (current != Language::Russian) {
        return;
    }
    for (auto* p = text; *p; ++p) {
        if (*p == '.') {
            *p = ',';
        }
    }
}

} // namespace lang

// -----------------------------------------------------------------------------
// Config model
// -----------------------------------------------------------------------------

enum class Action : uint8_t {
    Toggle,
    On,
    Off,
};

enum class ItemType : uint8_t {
    Switch,
    Button,
    Scene,
    Text,
    Link,       // "open": shows a page (target.page: page index, PageHome, PageWeather or PageSettings)
    Thermostat, // config.thermostats[Item::thermostat]; target.entity = the setpoint
};

enum class DriverPref : uint8_t {
    Auto,
    Ohpanel,
    Lovelace,
};

const char* driverPrefName(DriverPref value) {
    switch (value) {
    case DriverPref::Auto:
        return "auto";
    case DriverPref::Ohpanel:
        return "ohpanel";
    case DriverPref::Lovelace:
        return "lovelace";
    }

    return "";
}

// Target::page values besides a page index
constexpr int PageNone { -1 };
constexpr int PageHome { -2 };
constexpr int PageWeather { -3 };
constexpr int PageSettings { -4 };

// separates the keys of Entity::path (one allocation instead of a vector of Strings)
constexpr char PathSep { '\x1F' };

struct Entity {
    String topic;    // absolute MQTT state topic, empty for local relays
    String command;  // absolute MQTT command topic
    int relay { -1 };
    int state { -1 }; // -1 unknown, 0 off, 1 on
    String value;

    // JSON state payloads (zigbee2mqtt, Istabai): keys into the payload, PathSep separated.
    // Empty: the payload itself is the value. Several entities may share one topic.
    String path;
    String on;       // state values for this entity, empty: mqtt.on / mqtt.off
    String off;
    String payload;  // command template, "%s" is replaced by the value; empty: the plain value
    float number { NAN }; // value as a number, NAN when it is not one

    // "url" / "on_url" / "off_url": an HTTP GET instead of the MQTT command ("url": the same for on and off)
    String url_on;
    String url_off;
};

// "type": "thermostat": entity indices into Config::entities, -1 when not configured
struct Thermostat {
    int current { -1 };
    int setpoint { -1 };  // also carries the command topic and payload template
    int humidity { -1 };
    int active { -1 };
    int min { -1 };
    int max { -1 };
    float min_value { 5.0f };  // limits while min/max are unknown, or fixed numbers from the config
    float max_value { 30.0f };
    float step { 0.5f };
    String history;       // topic of a retained history series, shown as a chart (empty: none)
};

// home info lines (OHpanel info1_* / info2_*, screensaver.info)
struct InfoLine {
    String icon;
    String text;
    int entity { -1 };
    String format;      // printf with one float conversion, empty: the value text as is
    int extra { -1 };
    String extra_format;
};

constexpr size_t InfoLinesMax { 2 };

enum class Layout : uint8_t {
    Center,  // big centred icon (font 9), name below
    Corner,  // icon top left, state line, name
};

enum class Theme : uint8_t {
    Dark,
    Light,   // light (white) tiles on the background, ref. ohpanel::tileColors()
};

struct Target {
    int entity { -1 };
    int group { -1 };
    int page { -1 };
    Action action { Action::Toggle };
    bool valid { false };
};

// TFT >= 53: an icon text ending in '¬<digit>' switches the grid tile to that font
constexpr uint8_t NoFont { 0xFF };

struct Item {
    String name;
    String icon;
    ItemType type { ItemType::Switch };
    uint8_t font { NoFont };
    int32_t color { -1 }; // OHpanel: icon colour (RGB565) when on, -1 = theme accent
    int16_t thermostat { -1 };
    String history;       // not a thermostat: a tap opens the chart of this retained series instead of switching
    Target target;
};

constexpr size_t TilesMax { 9 };

// "layout" of a grid page: columns x rows of tiles. OHpanel shows cols x rows tiles: the custom design
// d3731fbb on its own pages (buttons33 / buttons32 / buttons22 / buttons12), the stock design drawn at
// runtime on `buttons`; an unknown TFT design and Lovelace use cols columns of the 4x2 tile slots (2 rows)
enum class Grid : uint8_t {
    G4x2,
    G3x3,
    G3x2,
    G2x2,
    G1x2,
};

constexpr const char* GridNames[] { "4x2", "3x3", "3x2", "2x2", "1x2" };

constexpr uint8_t gridCols(Grid grid) {
    return (grid == Grid::G4x2) ? 4
        : ((grid == Grid::G3x3) || (grid == Grid::G3x2)) ? 3
        : (grid == Grid::G2x2) ? 2 : 1;
}

constexpr uint8_t gridRows(Grid grid) {
    return (grid == Grid::G3x3) ? 3 : 2;
}

// tiles of one screen on a TFT that has the grid's own page
constexpr size_t gridTiles(Grid grid) {
    return static_cast<size_t>(gridCols(grid)) * gridRows(grid);
}

const char* gridName(Grid grid) {
    return GridNames[static_cast<size_t>(grid)];
}

struct Page {
    String id;
    String title;
    bool settings { false }; // "type": "settings": the OHpanel settings tiles instead of items
    bool list { false };     // "type": "list": OHpanel entities_rows (one row per item), others draw a grid
    Grid grid { Grid::G4x2 };// "layout": "4x2" | "3x3" | "3x2" | "2x2" | "1x2"; "columns": 3 = "3x2"
    uint8_t per_screen { 0 };// "per_screen": tiles (or list rows) per screen, 0 = as many as fit
    bool draw { false };     // "draw": true: OHpanel draws the tiles at runtime (fill / cirs / xstr) even for 4x2
    bool nested { false };   // an item's "page": a sub-page, not in the swipe row, opened by its tile
    std::vector<uint16_t> items;
};

struct Config {
    String on { "1" };
    String off { "0" };
    String toggle { "2" };
    String set { "/set" };

    uint16_t timeout { 20 };
    uint8_t brightness { 100 };
    uint8_t dim { 20 };

    bool weather { false };
    float lat { 0.0f };
    float lon { 0.0f };
    uint32_t refresh { 900 };

    bool temperature { true };
    bool temperature_show { true };

    bool beep { false };
    bool debug { false };

    uint16_t color_on { icons::colors::On };
    uint16_t color_off { icons::colors::Off };
    uint16_t color_unknown { icons::colors::Unknown };

    DriverPref display { DriverPref::Auto };
    lang::Language language { lang::Language::English };

    // OHpanel ("ohpanel" object)
    uint8_t background { 0 };     // index into ohpanel::Backgrounds
    int32_t accent { -1 };        // RGB565 overrides of the background's theme, -1 = theme
    int32_t text_color { -1 };
    int32_t dim_color { -1 };
    uint8_t weather_days { 7 };
    String relay_labels[2];
    std::vector<uint16_t> shortcuts; // item indices (home shortcuts, screensaver.shortcuts)
    Layout layout { Layout::Center }; // ohpanel.layout; the nspIcons setting wins
    Theme theme { Theme::Dark };      // ohpanel.theme; the nspTheme setting wins
    InfoLine info[InfoLinesMax];
    size_t info_count { 0 };
    std::vector<Thermostat> thermostats;

    // ~124 bytes each and their count is only known after de-duplication: a vector growing to
    // 128 entries needs one 16 KB block (plus the old 8 KB one while it moves), which is what
    // ran the heap out on upload. deque keeps them in 512 byte chunks, nothing ever moves
    std::deque<Entity> entities;
    std::vector<String> group_names;
    std::vector<std::vector<int>> groups;
    std::vector<Item> items;
    std::vector<Page> pages;
    std::vector<Target> buttons;
};

Config config;

// one entity per (topic, path, on, off, payload); a later "command" replaces the default one
int findOrAddEntity(Config& out, Entity&& want) {
    for (size_t index = 0; index < out.entities.size(); ++index) {
        auto& entity = out.entities[index];
        if ((entity.relay < 0) && (entity.topic == want.topic) && (entity.path == want.path)
            && (entity.on == want.on) && (entity.off == want.off) && (entity.payload == want.payload)
            && (entity.url_on == want.url_on) && (entity.url_off == want.url_off))
        {
            if (want.command.length()) {
                entity.command = want.command;
            }
            return index;
        }
    }

    if (!want.command.length()) {
        want.command = want.topic + out.set;
    }
    out.entities.push_back(std::move(want));

    return out.entities.size() - 1;
}

int findOrAddTopic(Config& out, const String& topic, const String& command) {
    Entity entity;
    entity.topic = topic;
    entity.command = command;
    return findOrAddEntity(out, std::move(entity));
}

// ["a", "b", 0] or "a" -> "a\x1Fb\x1F0"
bool parsePath(const JsonVariant& value, String& out) {
    out = String();
    if (value.is<const char*>()) {
        const char* text = value.as<const char*>();  // NULL for a JSON null
        if (!text) {
            return false;
        }
        out = text;
        return out.length() > 0;
    }

    JsonArray& keys = value.as<JsonArray>();
    if (!keys.success() || !keys.size()) {
        return false;
    }

    bool first = true;
    for (auto key : keys) {
        if (!first) {
            out += PathSep;
        }
        first = false;
        if (key.is<const char*>() && key.as<const char*>()) {
            out += key.as<const char*>();
        } else if (key.is<long>()) {
            out += String(key.as<long>());
        } else {
            return false;
        }
    }

    return true;
}

String joinPath(const String& first, const char* second) {
    String out(first);
    out += PathSep;
    out += second;
    return out;
}

int findOrAddRelay(Config& out, int relay) {
    for (size_t index = 0; index < out.entities.size(); ++index) {
        if (out.entities[index].relay == relay) {
            return index;
        }
    }

    Entity entity;
    entity.relay = relay;
    out.entities.push_back(std::move(entity));

    return out.entities.size() - 1;
}

bool parseAction(const char* value, Action& out) {
    if (!value || !strlen(value) || (0 == strcmp(value, "toggle"))) {
        out = Action::Toggle;
    } else if (0 == strcmp(value, "on")) {
        out = Action::On;
    } else if (0 == strcmp(value, "off")) {
        out = Action::Off;
    } else {
        return false;
    }

    return true;
}

bool parseHex(const char* value, uint16_t& out) {
    if (!value) {
        return false;
    }

    if ((value[0] == 'U' || value[0] == 'u') && value[1] == '+') {
        value += 2;
    } else if (value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
        value += 2;
    }

    if (strlen(value) != 4) {
        return false;
    }

    char* end = nullptr;
    const auto result = strtoul(value, &end, 16);
    if (!end || *end != '\0' || result < 0xE000) {
        return false;
    }

    out = result;
    return true;
}

String resolveIcon(const char* value, uint16_t fallback, String& warnings) {
    if (!value || !strlen(value)) {
        return utf8(fallback);
    }

    // literal glyph or short text
    if (static_cast<uint8_t>(value[0]) >= 0x80) {
        String out(value);
        utf8Truncate(out, 9);
        return out;
    }

    const char* name = value;
    if (0 == strncmp(name, "mdi:", 4)) {
        name += 4;
    }

    for (const auto& icon : icons::Table) {
        if (0 == strcmp(icon.name, name)) {
            return utf8(icon.code);
        }
    }

    uint16_t code;
    if (parseHex(name, code)) {
        return utf8(code);
    }

    warnings += F("unknown icon '");
    warnings += value;
    warnings += F("'; ");

    return utf8(icons::AlertCircleOutline);
}

// "x" -> x, 12 -> "12", true -> "true", missing -> ""
String jsonText(const JsonVariant& value) {
    if (value.is<const char*>()) {
        const char* text = value.as<const char*>();  // NULL for a JSON null
        return text ? String(text) : String();
    }
    if (value.is<bool>()) {
        return value.as<bool>() ? F("true") : F("false");
    }
    if (value.is<long>()) {
        return String(value.as<long>());
    }
    if (value.is<double>()) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%g", value.as<double>());
        return buf;
    }
    return String();
}

// "url" (both), "on_url", "off_url": plain http:// or https:// (no certificate check), empty when not set
bool parseUrls(JsonObject& obj, String& on, String& off, String& error) {
    const String url = jsonText(obj["url"]);
    on = jsonText(obj["on_url"]);
    off = jsonText(obj["off_url"]);
    if (!on.length()) {
        on = url;
    }
    if (!off.length()) {
        off = url;
    }

    for (const auto* value : {&on, &off}) {
        if (value->length() && (!(value->startsWith(F("http://")) || value->startsWith(F("https://")))
            || (value->length() > 256)))
        {
            error = F("\"url\" must start with http:// or https://, at most 256 characters");
            return false;
        }
    }

    return true;
}

bool parseTarget(JsonObject& obj, Config& out, Target& target, String& error) {
    if (!parseAction(obj["action"].as<const char*>(), target.action)) {
        error = F("unknown action '");
        error += obj["action"].as<const char*>();
        error += '\'';
        return false;
    }

    String url_on;
    String url_off;
    if (!parseUrls(obj, url_on, url_off, error)) {
        return false;
    }
    const bool urls = url_on.length() || url_off.length();

    if (urls && obj.containsKey("relay")) {
        error = F("\"url\" and \"relay\" do not go together");
        return false;
    }

    if (obj.containsKey("relay")) {
        const int relay = obj["relay"].as<int>();
#if RELAY_SUPPORT
        if ((relay < 0) || (static_cast<size_t>(relay) >= relayCount())) {
#else
        if (true) {
#endif
            error = F("no such relay ");
            error += relay;
            return false;
        }
        target.entity = findOrAddRelay(out, relay);
    } else if (obj.containsKey("topic")) {
        const char* topic = obj["topic"].as<const char*>();
        if (!topic || !strlen(topic) || strchr(topic, '#') || strchr(topic, '+')) {
            error = F("invalid topic");
            return false;
        }

        Entity entity;
        entity.topic = topic;
        entity.command = jsonText(obj["command"]);
        if (obj.containsKey("path") && !parsePath(obj["path"], entity.path)) {
            error = F("bad \"path\" (array of keys)");
            return false;
        }
        entity.on = jsonText(obj["on"]);
        entity.off = jsonText(obj["off"]);
        entity.payload = jsonText(obj["payload"]);
        entity.url_on = url_on;
        entity.url_off = url_off;
        target.entity = findOrAddEntity(out, std::move(entity));
    } else if (urls) {
        // no state topic: an entity of its own, the panel keeps its state (entitySet())
        Entity entity;
        entity.url_on = url_on;
        entity.url_off = url_off;
        out.entities.push_back(std::move(entity));
        target.entity = out.entities.size() - 1;
    }

    if (obj.containsKey("group")) {
        const char* name = obj["group"].as<const char*>();
        for (size_t index = 0; index < out.group_names.size(); ++index) {
            if (name && out.group_names[index] == name) {
                target.group = index;
                break;
            }
        }

        if (target.group < 0) {
            error = F("unknown group '");
            error += name ? name : "";
            error += '\'';
            return false;
        }
    }

    // a page index (hardware buttons); an item's "page" object is a nested page (parsePage())
    if (obj["page"].is<int>()) {
        target.page = obj["page"].as<int>();
    }

    target.valid = (target.entity >= 0) || (target.group >= 0) || (target.page >= 0);
    return true;
}

template <typename T>
T clampValue(long value, long min, long max) {
    return static_cast<T>(std::max(min, std::min(max, value)));
}

// OHpanel backgrounds, same order as ohpanel::Backgrounds
const char* const BackgroundNames[] { "aurora", "black", "candy", "dunes", "forest" };

// "#rrggbb" / "rrggbb" (RGB) or a number (RGB565 decimal)
bool parseColor(const JsonVariant& value, int32_t& out) {
    if (value.is<const char*>()) {
        const char* text = value.as<const char*>();
        if (!text) {
            return false;
        }
        if (text[0] == '#') {
            ++text;
        }
        if (strlen(text) != 6) {
            return false;
        }
        char* end = nullptr;
        const uint32_t rgb = strtoul(text, &end, 16);
        if (!end || *end) {
            return false;
        }
        out = (((rgb >> 19) & 0x1F) << 11) | (((rgb >> 10) & 0x3F) << 5) | ((rgb >> 3) & 0x1F);
        return true;
    }

    if (value.is<long>()) {
        const long number = value.as<long>();
        if ((number >= 0) && (number <= 65535)) {
            out = number;
            return true;
        }
    }

    return false;
}

// "open" references are resolved after all pages are known
struct PendingOpen {
    size_t item;
    String ref;
};

// a JSON value entity of the thermostat: explicit path, or [room, key]; -1 when neither is given
bool thermostatEntity(JsonObject& obj, const char* field, const String& topic, const String& room,
        const char* key, int& out_index, Config& out, String& error)
{
    Entity entity;
    entity.topic = topic;
    if (obj[field].is<double>() || obj[field].is<long>()) {
        out_index = -1; // a fixed number (min / max)
        return true;
    }

    if (obj.containsKey(field)) {
        if (!parsePath(obj[field], entity.path)) {
            error = String(F("bad \"")) + field + F("\" path");
            return false;
        }
    } else if (room.length()) {
        entity.path = joinPath(room, key);
    } else {
        out_index = -1;
        return true;
    }

    out_index = findOrAddEntity(out, std::move(entity));
    return true;
}

// {"type": "thermostat", "topic": "istabai/out", "room": "Гостиная", "command": "istabai/in/Гостиная", "step": 0.5}
// or explicit paths: "current", "setpoint", "humidity", "active", "min", "max" (min / max may be numbers)
bool parseThermostat(JsonObject& obj, Config& out, Item& item, String& error) {
    const char* topic = obj["topic"].as<const char*>();
    if (!topic || !strlen(topic) || strchr(topic, '#') || strchr(topic, '+')) {
        error = F("thermostat needs a state \"topic\"");
        return false;
    }

    const String state_topic(topic);
    const String room = jsonText(obj["room"]);

    Thermostat thermostat;
    if (!thermostatEntity(obj, "current", state_topic, room, "temperature", thermostat.current, out, error)
        || !thermostatEntity(obj, "humidity", state_topic, room, "humidity", thermostat.humidity, out, error)
        || !thermostatEntity(obj, "active", state_topic, room, "active", thermostat.active, out, error)
        || !thermostatEntity(obj, "min", state_topic, room, "min_temperature", thermostat.min, out, error)
        || !thermostatEntity(obj, "max", state_topic, room, "max_temperature", thermostat.max, out, error))
    {
        return false;
    }

    // the setpoint carries the command: plain number unless "payload" has a "%s" template
    {
        Entity entity;
        entity.topic = state_topic;
        if (obj.containsKey("setpoint")) {
            if (!parsePath(obj["setpoint"], entity.path)) {
                error = F("bad \"setpoint\" path");
                return false;
            }
        } else if (room.length()) {
            entity.path = joinPath(room, "set_temperature");
        } else {
            error = F("thermostat needs \"room\" or a \"setpoint\" path");
            return false;
        }
        entity.command = jsonText(obj["command"]);
        entity.payload = jsonText(obj["payload"]);
        thermostat.setpoint = findOrAddEntity(out, std::move(entity));
    }

    if (obj["min"].is<double>() || obj["min"].is<long>()) {
        thermostat.min_value = obj["min"].as<float>();
    }
    if (obj["max"].is<double>() || obj["max"].is<long>()) {
        thermostat.max_value = obj["max"].as<float>();
    }
    if (obj.containsKey("step")) {
        thermostat.step = obj["step"].as<float>();
    }
    if (!(thermostat.step > 0.0f) || (thermostat.step > 10.0f)) {
        thermostat.step = 0.5f;
    }
    if (thermostat.max_value <= thermostat.min_value) {
        thermostat.max_value = thermostat.min_value + thermostat.step;
    }

    const char* history = obj["history"].as<const char*>();
    if (history && strlen(history)) {
        if (strchr(history, '#') || strchr(history, '+')) {
            error = F("\"history\" must be a plain topic");
            return false;
        }
        thermostat.history = history;
    }

    out.thermostats.push_back(thermostat);
    item.thermostat = out.thermostats.size() - 1;
    item.target.entity = thermostat.setpoint;
    item.target.valid = true;
    return true;
}

// a printf format with exactly one float conversion (%f %e %g, flags/width/precision) and %%
bool validFormat(const String& format) {
    size_t conversions = 0;
    for (size_t pos = 0; pos < format.length(); ++pos) {
        if (format[pos] != '%') {
            continue;
        }
        ++pos;
        if ((pos < format.length()) && (format[pos] == '%')) {
            continue;
        }
        while ((pos < format.length()) && strchr("-+ #0", format[pos])) {
            ++pos;
        }
        while ((pos < format.length()) && isdigit(static_cast<unsigned char>(format[pos]))) {
            ++pos;
        }
        if ((pos < format.length()) && (format[pos] == '.')) {
            ++pos;
            while ((pos < format.length()) && isdigit(static_cast<unsigned char>(format[pos]))) {
                ++pos;
            }
        }
        if ((pos >= format.length()) || !strchr("fFeEgG", format[pos])) {
            return false;
        }
        ++conversions;
    }

    return (conversions == 1) && (format.length() <= 24);
}

bool parseItem(JsonObject& item_obj, Config& out, Item& item, std::vector<PendingOpen>& pending,
        const String& where, String& error, String& warnings)
{
    item.name = sanitize(item_obj["name"].as<const char*>(), build::NameMax);

    const char* item_type = item_obj["type"].as<const char*>();
    const bool open = item_obj.containsKey("open") || item_obj.containsKey("page");
    uint16_t fallback = icons::LightSwitch;
    if (open && (!item_type || !strcmp(item_type, "link"))) {
        item.type = ItemType::Link;
        fallback = icons::ArrowRightBold;
    } else if (!item_type || !strcmp(item_type, "switch")) {
        item.type = ItemType::Switch;
    } else if (!strcmp(item_type, "button")) {
        item.type = ItemType::Button;
        fallback = icons::GestureTapButton;
    } else if (!strcmp(item_type, "scene")) {
        item.type = ItemType::Scene;
        fallback = icons::LightbulbGroup;
    } else if (!strcmp(item_type, "text")) {
        item.type = ItemType::Text;
        fallback = 0;
    } else if (!strcmp(item_type, "thermostat")) {
        item.type = ItemType::Thermostat;
        fallback = icons::Thermometer;
    } else {
        error = where + F(": unknown item type '") + item_type + '\'';
        return false;
    }

    if (item.type == ItemType::Thermostat) {
        if (!parseThermostat(item_obj, out, item, error)) {
            error = where + F(" '") + item.name + F("': ") + error;
            return false;
        }
    } else if (!parseTarget(item_obj, out, item.target, error)) {
        error = where + F(" '") + item.name + F("': ") + error;
        return false;
    }

    // "page": a nested page, target.page is set once it is parsed (parsePage())
    if ((item.type == ItemType::Link) && item_obj.containsKey("page")) {
        if (!item_obj["page"].is<JsonObject&>()) {
            error = where + F(" '") + item.name + F("': \"page\" must be an object");
            return false;
        }
    } else if (item.type == ItemType::Link) {
        const auto& value = item_obj["open"];
        if (value.is<int>()) {
            item.target.page = value.as<int>();
            if ((item.target.page < 0) && (item.target.page != PageNone)) {
                item.target.page = PageNone;
            }
        } else {
            const char* ref = value.as<const char*>();
            if (ref && !strcmp(ref, "home")) {
                item.target.page = PageHome;
            } else if (ref && !strcmp(ref, "weather")) {
                item.target.page = PageWeather;
            } else if (ref && !strcmp(ref, "settings")) {
                item.target.page = PageSettings;
            } else {
                pending.push_back(PendingOpen{out.items.size(), String(ref ? ref : "")});
            }
        }
    } else if (item.type != ItemType::Thermostat) {
        const bool need_group = (item.type == ItemType::Scene);
        if (need_group ? (item.target.group < 0) : (item.target.entity < 0)) {
            error = where + F(" '") + item.name
                + (need_group ? F("': scene needs \"group\"") : F("': needs \"topic\", \"relay\" or \"url\""));
            return false;
        }
    }

    const char* history = item_obj["history"].as<const char*>();
    if ((item.type != ItemType::Thermostat) && history && strlen(history)) {
        if (strchr(history, '#') || strchr(history, '+')) {
            error = where + F(" '") + item.name + F("': \"history\" must be a plain topic");
            return false;
        }
        item.history = history;
    }

    item.icon = resolveIcon(item_obj["icon"].as<const char*>(), fallback, warnings);

    if (item_obj.containsKey("color") && !parseColor(item_obj["color"], item.color)) {
        warnings += where + F(" '") + item.name + F("': bad color ignored; ");
    }

    // same names as AppDaemon's grid 'font' option (pages.py), or the TFT font id 0..9
    if (item_obj.containsKey("font")) {
        const auto& font = item_obj["font"];
        const char* font_name = font.as<const char*>();
        if (font.is<int>() && (font.as<int>() >= 0) && (font.as<int>() <= 9)) {
            item.font = font.as<int>();
        } else if (font_name && !strcmp(font_name, "small")) {
            item.font = 0;
        } else if (font_name && !strcmp(font_name, "medium-icon")) {
            item.font = 1;
        } else if (font_name && !strcmp(font_name, "medium")) {
            item.font = 2;
        } else if (font_name && !strcmp(font_name, "large")) {
            item.font = 3;
        } else {
            warnings += F("item '");
            warnings += item.name;
            warnings += F("': unknown font ignored; ");
        }
    }

    return true;
}

// one item ("type" counted for thermostats) and its nested page, for the vector reservations below
void countItem(JsonObject& obj, size_t& items, size_t& thermostats, size_t& pages, int depth = 0) {
    ++items;
    const char* type = obj["type"].as<const char*>();
    if (type && !strcmp(type, "thermostat")) {
        ++thermostats;
    }

    JsonObject& page = obj["page"];
    if (page.success() && (depth < 8)) {
        ++pages;
        JsonArray& nested = page["items"];
        for (auto item_variant : nested) {
            JsonObject& item = item_variant.as<JsonObject>();
            if (item.success()) {
                countItem(item, items, thermostats, pages, depth + 1);
            }
        }
    }
}

// an item's "page", parsed after all top-level pages (their indices stay those of "pages")
struct PendingPage {
    JsonObject* obj;
    size_t item;
};

bool parsePage(JsonObject& page_obj, Config& out, std::vector<PendingOpen>& pending,
        std::vector<PendingPage>& nested, size_t page_index, const String& fallback_title,
        String& error, String& warnings)
{
    const char* type = page_obj["type"].as<const char*>();
    Page page;
    if (type && !strcmp(type, "settings")) {
        page.settings = true;
    } else if (type && !strcmp(type, "list")) {
        page.list = true;
    } else if (type && strcmp(type, "grid")) {
        warnings += F("page type '");
        warnings += type;
        warnings += F("' shown as grid; ");
    }

    page.id = page_obj["id"].as<const char*>();
    if (!page.id.length()) {
        page.id = String('p') + page_index;
    }
    page.title = sanitize(page_obj["title"].as<const char*>(), build::HeadingMax);
    if (!page.title.length()) {
        page.title = sanitize(fallback_title.c_str(), build::HeadingMax);
    }

    if (page_obj.containsKey("columns")) {
        const int columns = page_obj["columns"].as<int>();
        if ((columns == 3) || (columns == 4)) {
            page.grid = (columns == 3) ? Grid::G3x2 : Grid::G4x2;
        } else {
            warnings += F("page ");
            warnings += page_index;
            warnings += F(": columns must be 3 or 4; ");
        }
    }

    // "layout" wins over "columns"
    if (page_obj.containsKey("layout")) {
        const char* layout = page_obj["layout"].as<const char*>();
        bool found = false;
        for (size_t index = 0; index < (sizeof(GridNames) / sizeof(GridNames[0])); ++index) {
            if (layout && !strcmp(layout, GridNames[index])) {
                page.grid = static_cast<Grid>(index);
                found = true;
            }
        }
        if (!found) {
            warnings += F("page ");
            warnings += page_index;
            warnings += F(": layout must be 4x2, 3x3, 3x2, 2x2 or 1x2; ");
        }
    }

    if (page_obj.containsKey("per_screen")) {
        const int per_screen = page_obj["per_screen"].as<int>();
        if ((per_screen >= 1) && (per_screen <= static_cast<int>(TilesMax))) {
            page.per_screen = per_screen;
        } else {
            warnings += F("page ");
            warnings += page_index;
            warnings += F(": per_screen must be 1..9; ");
        }
    }

    if (page_obj.containsKey("draw")) {
        page.draw = page_obj["draw"].as<bool>();
    }

    JsonArray& items = page_obj["items"];
    if (page.settings && items.success() && items.size()) {
        warnings += F("settings page: items ignored; ");
    } else if (items.success()) {
        page.items.reserve(items.size());
        const String where = String(F("page ")) + page_index + F(" item");
        for (auto item_variant : items) {
            JsonObject& item_obj = item_variant.as<JsonObject>();
            if (!item_obj.success()) {
                error = F("item is not an object");
                return false;
            }

            Item item;
            if (!parseItem(item_obj, out, item, pending, where, error, warnings)) {
                return false;
            }

            out.items.push_back(std::move(item));
            page.items.push_back(out.items.size() - 1);

            JsonObject& sub = item_obj["page"];
            if (sub.success() && (out.items.back().type == ItemType::Link)) {
                nested.push_back(PendingPage{&sub, out.items.size() - 1});
            } else if (sub.success()) {
                warnings += where + F(" '") + out.items.back().name + F("': \"page\" needs no \"type\", ignored; ");
            }
        }
    }

    out.pages.push_back(std::move(page));
    return true;
}

bool parseRoot(JsonObject& root, Config& out, String& error, String& warnings) {
    JsonObject& mqtt = root["mqtt"];
    if (mqtt.success()) {
        if (mqtt.containsKey("on")) out.on = mqtt["on"].as<const char*>();
        if (mqtt.containsKey("off")) out.off = mqtt["off"].as<const char*>();
        if (mqtt.containsKey("toggle")) out.toggle = mqtt["toggle"].as<const char*>();
        if (mqtt.containsKey("set")) out.set = mqtt["set"].as<const char*>();
    }

    JsonObject& screensaver = root["screensaver"];
    if (screensaver.success()) {
        if (screensaver.containsKey("timeout")) {
            out.timeout = clampValue<uint16_t>(screensaver["timeout"].as<long>(), 0, 3600);
        }
        if (screensaver.containsKey("brightness")) {
            out.brightness = clampValue<uint8_t>(screensaver["brightness"].as<long>(), 1, 100);
        }
        if (screensaver.containsKey("dim")) {
            out.dim = clampValue<uint8_t>(screensaver["dim"].as<long>(), 0, 100);
        }

        JsonObject& weather = screensaver["weather"];
        if (weather.success() && weather.containsKey("lat") && weather.containsKey("lon")) {
            out.weather = weather["enabled"] | true;
            out.lat = weather["lat"].as<float>();
            out.lon = weather["lon"].as<float>();
            if (weather.containsKey("refresh")) {
                out.refresh = clampValue<uint32_t>(weather["refresh"].as<long>(), 60, 86400);
            }
        }
    }

    // TFT sleep timer only runs when dim != brightness
    if (out.dim == out.brightness) {
        out.dim = out.brightness - 1;
    }

    JsonObject& temperature = root["temperature"];
    if (temperature.success()) {
        out.temperature = temperature["enabled"] | true;
        out.temperature_show = temperature["show"] | true;
        // the NTC is an ESPurna sensor: its correction and MQTT interval are the sensor settings
        if (temperature.containsKey("offset") || temperature.containsKey("publish")) {
            warnings += F("temperature: offset and publish are ignored, use the Sensors settings of the web UI; ");
        }
    }

    JsonObject& colors = root["colors"];
    if (colors.success()) {
        if (colors.containsKey("on")) out.color_on = colors["on"].as<unsigned int>();
        if (colors.containsKey("off")) out.color_off = colors["off"].as<unsigned int>();
        if (colors.containsKey("unknown")) out.color_unknown = colors["unknown"].as<unsigned int>();
    }

    out.beep = root["beep"] | false;
    out.debug = root["debug"] | false;

    if (root.containsKey("display")) {
        const char* display = root["display"].as<const char*>();
        if (display && !strcmp(display, "ohpanel")) {
            out.display = DriverPref::Ohpanel;
        } else if (display && !strcmp(display, "lovelace")) {
            out.display = DriverPref::Lovelace;
        } else if (!display || strcmp(display, "auto")) {
            warnings += F("display: auto, ohpanel or lovelace (using auto); ");
        }
    }

    if (root.containsKey("language")) {
        const char* language = root["language"].as<const char*>();
        if (language && !strcmp(language, "ru")) {
            out.language = lang::Language::Russian;
        } else if (!language || strcmp(language, "en")) {
            warnings += F("language: en or ru (using en); ");
        }
    }

    JsonObject& ohpanel = root["ohpanel"];
    if (ohpanel.success()) {
        if (ohpanel.containsKey("background")) {
            const char* name = ohpanel["background"].as<const char*>();
            bool found = false;
            for (size_t index = 0; index < (sizeof(BackgroundNames) / sizeof(BackgroundNames[0])); ++index) {
                if (name && !strcmp(name, BackgroundNames[index])) {
                    out.background = index;
                    found = true;
                }
            }
            if (!found) {
                warnings += F("ohpanel.background: aurora, black, candy, dunes or forest; ");
            }
        }

        if (ohpanel.containsKey("accent") && !parseColor(ohpanel["accent"], out.accent)) {
            warnings += F("ohpanel.accent: bad colour; ");
        }
        if (ohpanel.containsKey("text") && !parseColor(ohpanel["text"], out.text_color)) {
            warnings += F("ohpanel.text: bad colour; ");
        }
        if (ohpanel.containsKey("dim") && !parseColor(ohpanel["dim"], out.dim_color)) {
            warnings += F("ohpanel.dim: bad colour; ");
        }
        if (ohpanel.containsKey("weather_days")) {
            out.weather_days = clampValue<uint8_t>(ohpanel["weather_days"].as<long>(), 0, 7);
        }

        if (ohpanel.containsKey("layout")) {
            const char* layout = ohpanel["layout"].as<const char*>();
            if (layout && !strcmp(layout, "corner")) {
                out.layout = Layout::Corner;
            } else if (layout && !strcmp(layout, "center")) {
                out.layout = Layout::Center;
            } else {
                warnings += F("ohpanel.layout: center or corner; ");
            }
        }

        if (ohpanel.containsKey("theme")) {
            const char* theme = ohpanel["theme"].as<const char*>();
            if (theme && !strcmp(theme, "light")) {
                out.theme = Theme::Light;
            } else if (theme && !strcmp(theme, "dark")) {
                out.theme = Theme::Dark;
            } else {
                warnings += F("ohpanel.theme: dark or light; ");
            }
        }

        JsonArray& relays = ohpanel["relays"];
        if (relays.success()) {
            for (size_t index = 0; (index < 2) && (index < relays.size()); ++index) {
                out.relay_labels[index] = sanitize(relays[index].as<const char*>(), 26);
            }
        }
    }

    JsonObject& groups = root["groups"];
    if (groups.success()) {
        out.group_names.reserve(groups.size());
        out.groups.reserve(groups.size());
        for (auto kv : groups) {
            JsonArray& members = kv.value.as<JsonArray>();
            if (!members.success()) {
                error = F("group '");
                error += kv.key;
                error += F("' is not an array");
                return false;
            }

            std::vector<int> entities;
            for (auto member : members) {
                if (member.is<const char*>()) {
                    const String topic(member.as<const char*>());
                    if (!topic.length()) {
                        continue;
                    }
                    entities.push_back(findOrAddTopic(out, topic, String()));
                } else if (member.is<int>()) {
                    const int relay = member.as<int>();
#if RELAY_SUPPORT
                    if ((relay >= 0) && (static_cast<size_t>(relay) < relayCount())) {
                        entities.push_back(findOrAddRelay(out, relay));
                        continue;
                    }
#endif
                    error = F("no such relay in group '");
                    error += kv.key;
                    error += '\'';
                    return false;
                }
            }

            out.group_names.push_back(kv.key);
            out.groups.push_back(std::move(entities));
        }
    }

    JsonArray& pages = root["pages"];
    if (!pages.success()) {
        error = F("'pages' must be an array");
        return false;
    }

    // exact sizes up front: no vector doubling (old + new block alive while it moves)
    {
        size_t item_count = 0;
        size_t thermostat_count = 0;
        size_t page_count = pages.size();
        for (auto page_variant : pages) {
            JsonArray& items = page_variant.as<JsonObject>()["items"];
            for (auto item_variant : items) {
                JsonObject& obj = item_variant.as<JsonObject>();
                if (obj.success()) {
                    countItem(obj, item_count, thermostat_count, page_count);
                }
            }
        }

        JsonArray& shortcuts = screensaver["shortcuts"];
        for (auto shortcut : shortcuts) {
            JsonObject& obj = shortcut.as<JsonObject>();
            if (obj.success() && !obj.containsKey("ref")) {
                countItem(obj, item_count, thermostat_count, page_count);
            }
        }
        out.shortcuts.reserve(std::min<size_t>(7, shortcuts.size()));

        out.items.reserve(item_count);
        out.thermostats.reserve(thermostat_count);
        out.pages.reserve(page_count);
    }

    std::vector<PendingOpen> pending;

    std::vector<PendingPage> nested;
    size_t page_index = 0;
    for (auto page_variant : pages) {
        JsonObject& page_obj = page_variant.as<JsonObject>();
        if (!page_obj.success()) {
            error = F("page is not an object");
            return false;
        }
        if (!parsePage(page_obj, out, pending, nested, page_index, String(), error, warnings)) {
            return false;
        }
        ++page_index;
    }

    // nested pages after the swipe row, breadth first (a sub-page may hold further sub-pages)
    for (size_t index = 0; index < nested.size(); ++index) {
        const auto job = nested[index];
        if (!parsePage(*job.obj, out, pending, nested, out.pages.size(), out.items[job.item].name, error, warnings)) {
            return false;
        }
        out.pages.back().nested = true;
        out.items[job.item].target.page = out.pages.size() - 1;
    }

    // home shortcuts (OHpanel, up to 7): an item name, {"ref": "<item name>"} or a full item
    JsonArray& shortcuts = screensaver["shortcuts"];
    if (shortcuts.success()) {
        for (auto shortcut : shortcuts) {
            if (out.shortcuts.size() >= 7) {
                warnings += F("more than 7 shortcuts, rest ignored; ");
                break;
            }

            JsonObject& obj = shortcut.as<JsonObject>();
            const char* ref = shortcut.is<const char*>() ? shortcut.as<const char*>()
                : (obj.success() ? obj["ref"].as<const char*>() : nullptr);
            if (ref) {
                bool found = false;
                for (size_t index = 0; index < out.items.size(); ++index) {
                    if (out.items[index].name == ref) {
                        out.shortcuts.push_back(index);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    warnings += F("shortcut '");
                    warnings += ref;
                    warnings += F("': no such item; ");
                }
                continue;
            }

            if (!obj.success()) {
                error = F("shortcut is not an object or an item name");
                return false;
            }

            Item item;
            if (!parseItem(obj, out, item, pending, String(F("shortcut")), error, warnings)) {
                return false;
            }
            out.items.push_back(std::move(item));
            out.shortcuts.push_back(out.items.size() - 1);
        }
    }

    // home info lines (OHpanel info1 / info2): icon, text, a value from a topic (+ an extra value)
    JsonArray& info = screensaver["info"];
    if (info.success()) {
        for (auto line_variant : info) {
            if (out.info_count >= InfoLinesMax) {
                warnings += F("more than 2 info lines, rest ignored; ");
                break;
            }

            JsonObject& obj = line_variant.as<JsonObject>();
            if (!obj.success()) {
                error = F("info line is not an object");
                return false;
            }

            auto& line = out.info[out.info_count];
            const String where = String(F("info ")) + (out.info_count + 1);
            const char* icon = obj["icon"].as<const char*>();
            line.icon = (icon && strlen(icon)) ? resolveIcon(icon, 0, warnings) : String();
            line.text = sanitize(obj["text"].as<const char*>(), 31);

            const char* topic = obj["topic"].as<const char*>();
            if (topic && strlen(topic)) {
                if (strchr(topic, '#') || strchr(topic, '+')) {
                    error = where + F(": invalid topic");
                    return false;
                }

                Entity entity;
                entity.topic = topic;
                if (obj.containsKey("path") && !parsePath(obj["path"], entity.path)) {
                    error = where + F(": bad \"path\"");
                    return false;
                }
                line.entity = findOrAddEntity(out, std::move(entity));
                line.format = jsonText(obj["format"]);

                JsonObject& extra = obj["extra"];
                if (extra.success()) {
                    Entity second;
                    const char* extra_topic = extra["topic"].as<const char*>();
                    second.topic = (extra_topic && strlen(extra_topic)) ? String(extra_topic) : String(topic);
                    if (extra.containsKey("path") && !parsePath(extra["path"], second.path)) {
                        error = where + F(": bad extra \"path\"");
                        return false;
                    }
                    line.extra = findOrAddEntity(out, std::move(second));
                    line.extra_format = jsonText(extra["format"]);
                }
            }

            if (line.format.length() && !validFormat(line.format)) {
                warnings += where + F(": format needs one %f / %g conversion, ignored; ");
                line.format = String();
            }
            if (line.extra_format.length() && !validFormat(line.extra_format)) {
                warnings += where + F(": extra format needs one %f / %g conversion, ignored; ");
                line.extra_format = String();
            }

            ++out.info_count;
        }
    }

    for (const auto& open : pending) {
        auto& target = out.items[open.item].target;
        for (size_t index = 0; index < out.pages.size(); ++index) {
            if ((out.pages[index].id == open.ref) || (out.pages[index].title == open.ref)) {
                target.page = index;
                break;
            }
        }
        if (target.page == PageNone) {
            error = String(F("item '")) + out.items[open.item].name + F("': unknown page '") + open.ref + '\'';
            return false;
        }
    }

    for (auto& item : out.items) {
        if ((item.type == ItemType::Link) && (item.target.page >= static_cast<int>(out.pages.size()))) {
            error = String(F("item '")) + item.name + F("': no page ") + item.target.page;
            return false;
        }
        // an "open" tile without "name" shows the title of its page
        if ((item.type == ItemType::Link) && (item.target.page >= 0) && !item.name.length()) {
            item.name = sanitize(out.pages[item.target.page].title.c_str(), build::NameMax);
        }
    }
    JsonArray& buttons = root["buttons"];
    if (buttons.success()) {
        out.buttons.reserve(buttons.size());
        for (auto button_variant : buttons) {
            Target target;
            JsonObject& button_obj = button_variant.as<JsonObject>();
            if (button_obj.success() && !parseTarget(button_obj, out, target, error)) {
                error = String(F("button ")) + out.buttons.size() + F(": ") + error;
                return false;
            }
            out.buttons.push_back(target);
        }
    }

#if MQTT_SUPPORT
    // mqtt.cpp drops MQTT_MESSAGE_EVENT payloads of MQTT_BUFFER_MAX_SIZE bytes and more
    // (Istabai istabai/out with 9 rooms is ~1.3 KB)
    if (!out.thermostats.empty() && (MQTT_BUFFER_MAX_SIZE < 2048)) {
        warnings += F("MQTT_BUFFER_MAX_SIZE is ");
        warnings += String(MQTT_BUFFER_MAX_SIZE);
        warnings += F(": longer state payloads are dropped by the MQTT module; ");
    }
#endif

    return true;
}

// ArduinoJson 5 parses a mutable char* in place (strings stay in the text, nothing is copied)
// into one block that we size and allocate ourselves. DynamicJsonBuffer(1024) used to copy all
// strings (the text was const) into blocks doubling from 1 KB: ~15 KB for a 7.4 KB config
struct HeapJsonBuffer : public ArduinoJson::Internals::StaticJsonBufferBase {
    HeapJsonBuffer(char* buffer, size_t capacity) :
        StaticJsonBufferBase(buffer, capacity)
    {}
};

// Exact node storage of an in-place parse: every object, pair, array and array element.
// Unknown syntax (comments) falls back to counting every comma as an array element
size_t jsonBufferSize(const char* text, size_t length) {
    size_t objects = 0;
    size_t arrays = 0;
    size_t pairs = 0;
    size_t elements = 0;
    size_t commas = 0;
    bool exact = true;

    uint32_t is_array = 0; // per nesting level
    size_t depth = 0;
    bool first = false;    // right after '[': the next token is the first element
    char quote = '\0';
    bool escape = false;

    for (size_t pos = 0; pos < length; ++pos) {
        const char c = text[pos];
        if (quote) {
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == quote) {
                quote = '\0';
            }
            continue;
        }

        if ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n')) {
            continue;
        }

        if (first) {
            first = false;
            if (c != ']') {
                ++elements;
            }
        }

        switch (c) {
        case '"':
        case '\'':
            quote = c;
            break;
        case '{':
        case '[':
            if (depth >= 31) {
                exact = false;
                break;
            }
            ++depth;
            if (c == '[') {
                ++arrays;
                is_array |= (1u << depth);
                first = true;
            } else {
                ++objects;
                is_array &= ~(1u << depth);
            }
            break;
        case '}':
        case ']':
            if (depth) {
                --depth;
            }
            break;
        case ':':
            ++pairs;
            break;
        case ',':
            ++commas;
            if (is_array & (1u << depth)) {
                ++elements;
            }
            break;
        case '/':
            exact = false;
            break;
        default:
            break;
        }
    }

    if (!exact) {
        elements = commas + arrays;
    }

    // JSON_*_SIZE() are multiples of the 4 byte alignment the buffer rounds every allocation to
    return (objects * JSON_OBJECT_SIZE(0))
        + (pairs * (JSON_OBJECT_SIZE(1) - JSON_OBJECT_SIZE(0)))
        + (arrays * JSON_ARRAY_SIZE(0))
        + (elements * (JSON_ARRAY_SIZE(1) - JSON_ARRAY_SIZE(0)))
        + 64;
}

// Free heap a parse needs: the JSON nodes plus the Config being built (Strings, entities,
// items: about 3 bytes per byte of text) plus what WiFi / lwIP / TLS must keep
bool heapAvailable(size_t json_size, size_t length, String& error) {
    const size_t need = json_size + (build::ConfigHeapPerByte * length) + build::ConfigHeapReserve;
    const size_t free_heap = ESP.getFreeHeap();
    const size_t block = ESP->getMaxAllocHeap();
    if ((free_heap >= need) && (block >= (json_size + build::ConfigBlockReserve))) {
        return true;
    }

    char buf[96];
    snprintf_P(buf, sizeof(buf), PSTR("low memory (free %u, largest block %u, need %u / %u), retry"),
        free_heap, block, need, json_size + build::ConfigBlockReserve);
    error = buf;
    return false;
}

enum class ParseResult {
    Ok,
    Invalid,
    LowMemory,
};

// 'text' is NUL terminated and destroyed by the in-place parse; 'out' is garbage unless Ok.
// No exception leaves here (-fexceptions build: std::vector / std::deque throw bad_alloc)
ParseResult parseInPlace(char* text, size_t json_size, Config& out, String& error, String& warnings) {
    std::unique_ptr<char[]> nodes(new (std::nothrow) char[json_size]);
    if (!nodes) {
        error = F("out of memory (JSON), retry");
        return ParseResult::LowMemory;
    }

    try {
        HeapJsonBuffer buffer(nodes.get(), json_size);
        JsonObject& root = buffer.parseObject(text, build::ConfigNestingLimit);
        if (!root.success()) {
            error = ((buffer.size() + JSON_OBJECT_SIZE(1)) > json_size)
                ? F("JSON nesting / size not supported")
                : F("invalid JSON");
            return ParseResult::Invalid;
        }

        return parseRoot(root, out, error, warnings) ? ParseResult::Ok : ParseResult::Invalid;
    } catch (const std::bad_alloc&) {
        error = F("out of memory, retry");
    }

    return ParseResult::LowMemory;
}

// heap check + parse, for text the caller does not need afterwards
ParseResult parseConfig(char* text, size_t length, Config& out, String& error, String& warnings) {
    const size_t json_size = jsonBufferSize(text, length);
    if (!heapAvailable(json_size, length, error)) {
        return ParseResult::LowMemory;
    }

    return parseInPlace(text, json_size, out, error, warnings);
}

// -----------------------------------------------------------------------------
// Filesystem
// -----------------------------------------------------------------------------

namespace storage {

bool mounted { false };
String label;

bool mount() {
    if (mounted) {
        return true;
    }

    const auto* part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
    if (!part) {
        DEBUG_MSG_P(PSTR("[NSPANEL] No data/spiffs partition\n"));
        return false;
    }

    label = part->label;
    if (!LittleFS.begin(false, "/littlefs", 4, part->label)) {
        DEBUG_MSG_P(PSTR("[NSPANEL] LittleFS mount failed on '%s', formatting\n"), part->label);
        if (!LittleFS.begin(true, "/littlefs", 4, part->label)) {
            DEBUG_MSG_P(PSTR("[NSPANEL] LittleFS format failed\n"));
            return false;
        }
    }

    mounted = true;
    DEBUG_MSG_P(PSTR("[NSPANEL] LittleFS on '%s' (%u / %u bytes used)\n"),
        part->label, LittleFS.usedBytes(), LittleFS.totalBytes());

    return true;
}

bool read(String& out) {
    if (!mounted || !LittleFS.exists(build::ConfigPath)) {
        return false;
    }

    File file = LittleFS.open(build::ConfigPath, "r");
    if (!file) {
        return false;
    }

    const size_t size = file.size();
    if (!size || (size > build::ConfigMax)) {
        file.close();
        return false;
    }

    out = String();
    out.reserve(size);
    char buf[256];
    while (file.available()) {
        const auto len = file.readBytes(buf, sizeof(buf));
        if (!len) {
            break;
        }
        out.concat(buf, len);
    }
    file.close();

    return out.length() == size;
}

// NUL terminated copy of the file for an in-place parse, exactly size + 1 bytes
bool read(std::unique_ptr<char[]>& out, size_t& out_size) {
    out.reset();
    out_size = 0;
    if (!mounted || !LittleFS.exists(build::ConfigPath)) {
        return false;
    }

    File file = LittleFS.open(build::ConfigPath, "r");
    if (!file) {
        return false;
    }

    const size_t size = file.size();
    if (!size || (size > build::ConfigMax)) {
        file.close();
        return false;
    }

    out.reset(new (std::nothrow) char[size + 1]);
    if (!out) {
        file.close();
        return false;
    }

    const auto len = file.readBytes(out.get(), size);
    file.close();
    if (len != size) {
        out.reset();
        return false;
    }

    out[size] = '\0';
    out_size = size;
    return true;
}

void removeTmp() {
    if (mounted) {
        LittleFS.remove(build::ConfigTmpPath);
    }
}

// the upload is saved before the in-place parse destroys it, and only renamed to
// ConfigPath when it is valid (ref. commitTmp())
bool writeTmp(const char* data, size_t size) {
    if (!mounted) {
        return false;
    }

    File file = LittleFS.open(build::ConfigTmpPath, "w");
    if (!file) {
        return false;
    }

    const auto written = file.write(reinterpret_cast<const uint8_t*>(data), size);
    file.close();
    if (written != size) {
        removeTmp();
        return false;
    }

    return true;
}

bool commitTmp() {
    if (!LittleFS.rename(build::ConfigTmpPath, build::ConfigPath)) {
        LittleFS.remove(build::ConfigPath);
        return LittleFS.rename(build::ConfigTmpPath, build::ConfigPath);
    }

    return true;
}

} // namespace storage

// -----------------------------------------------------------------------------
// UART2 and state shared by the display drivers
// -----------------------------------------------------------------------------

namespace uart {

uint32_t current { 0 };

HardwareSerial& port() {
    return Serial2;
}

void begin() {
    auto& serial = port();
    serial.setRxBufferSize(2048);
    serial.setTxBufferSize(1024);
    serial.begin(build::LovelaceBaud, SERIAL_8N1, build::RxPin, build::TxPin);
    current = build::LovelaceBaud;
}

void drainInput() {
    while (port().read() >= 0) {
    }
}

// waits for the TX FIFO only while switching (probe), at most ~1 KB at 115200
void setBaud(uint32_t value) {
    if (value != current) {
        port().flush(true);
        port().updateBaudRate(value);
        current = value;
    }
    drainInput();
}

// raw Nextion command, terminated by FF FF FF
void writeRaw(const char* command) {
    auto& serial = port();
    serial.write(reinterpret_cast<const uint8_t*>(command), strlen(command));
    const uint8_t end[3] {0xFF, 0xFF, 0xFF};
    serial.write(end, sizeof(end));
}

} // namespace uart

enum class Driver : uint8_t {
    None,
    Lovelace,
    Ohpanel,
};

const char* driverName(Driver value) {
    switch (value) {
    case Driver::None:
        return "none";
    case Driver::Lovelace:
        return "lovelace";
    case Driver::Ohpanel:
        return "ohpanel";
    }

    return "";
}

struct Panel {
    std::deque<String> events;
    String info; // last driver announcement, republished on MQTT connect
};

Panel panel;

void recordEvent(const String& message) {
    if (panel.events.size() >= build::EventsMax) {
        panel.events.pop_front();
    }
    panel.events.push_back(message);
}

// every TFT event: kept for nspanel.info and published to <root>/nspanel
void publishEvent(const String& message) {
    recordEvent(message);
#if MQTT_SUPPORT
    if (mqttConnected()) {
        mqttSendRaw(mqttTopic("nspanel").c_str(), message.c_str(), false);
    }
#endif
}

// {"driver":"ohpanel","version":"bc629e1e","model":"..."} to <root>/nspanel
void announceDriver(Driver driver, const String& version, const String& model) {
    DynamicJsonBuffer buffer(256);
    JsonObject& root = buffer.createObject();
    root["driver"] = driverName(driver);
    root["version"] = version.c_str();
    root["model"] = model.c_str();

    panel.info = String();
    root.printTo(panel.info);
    publishEvent(panel.info);
}


// -----------------------------------------------------------------------------
// Entities
// -----------------------------------------------------------------------------

int entityState(const Entity& entity) {
#if RELAY_SUPPORT
    if (entity.relay >= 0) {
        switch (relayStatus(entity.relay)) {
        case RelayStatus::On:
            return 1;
        case RelayStatus::Off:
            return 0;
        default:
            return -1;
        }
    }
#endif
    return entity.state;
}

int entityState(int index) {
    if ((index < 0) || (static_cast<size_t>(index) >= config.entities.size())) {
        return -1;
    }

    return entityState(config.entities[index]);
}

// -1 when nothing is known, 1 when anything is on
int groupState(int group) {
    if ((group < 0) || (static_cast<size_t>(group) >= config.groups.size())) {
        return -1;
    }

    int out = -1;
    for (const auto entity : config.groups[group]) {
        const auto state = entityState(entity);
        if (state == 1) {
            return 1;
        }
        if (state == 0) {
            out = 0;
        }
    }

    return out;
}

// -----------------------------------------------------------------------------
// "url" tiles: an HTTP GET per tap, each in a task of its own (a slow or dead server must not stall the
// loop); the result is logged from the loop, never the URL itself (it may hold a token)
// -----------------------------------------------------------------------------

namespace http {

constexpr uint32_t Stack { 8192 };
constexpr uint32_t StackTls { 12288 };
constexpr int RunningMax { 2 };

std::atomic<int> running { 0 };
std::atomic<int> last_code { 0 };
std::atomic<bool> finished { false };

// runs in the task below; returns, so that its locals are destroyed before vTaskDelete()
void fetch(const String& url) {
    int code = -1;
    {
        std::unique_ptr<WiFiClient> client;
        if (url.startsWith(F("https://"))) {
            auto* secure = new WiFiClientSecure();
            secure->setInsecure();
            secure->setHandshakeTimeout(10);
            client.reset(secure);
        } else {
            client.reset(new WiFiClient());
        }

        HTTPClient http;
        http.setConnectTimeout(5000);
        http.setTimeout(5000);
        http.setReuse(false);
        if (http.begin(*client, url)) {
            code = http.GET();
            http.end();
        }
    }

    last_code.store(code);
    finished.store(true);
}

void task(void* arg) {
    {
        std::unique_ptr<String> url(static_cast<String*>(arg));
        fetch(*url);
    }
    --running;
    vTaskDelete(nullptr);
}

// false when it could not be started (no WiFi, too many at once, low heap)
bool get(const String& url) {
    if (!wifiConnected() || (running.load() >= RunningMax) || (ESP.getFreeHeap() < build::WeatherHeapMin)) {
        DEBUG_MSG_P(PSTR("[NSPANEL] URL request dropped (no WiFi, busy or low heap)\n"));
        return false;
    }

    auto* arg = new String(url);
    ++running;
    const bool tls = url.startsWith(F("https://"));
    if (pdPASS != xTaskCreate(task, "nspurl", tls ? StackTls : Stack, arg, 1, nullptr)) {
        --running;
        delete arg;
        DEBUG_MSG_P(PSTR("[NSPANEL] URL request: task create failed\n"));
        return false;
    }

    return true;
}

void loop() {
    if (finished.exchange(false)) {
        const int code = last_code.load();
        if (code > 0) {
            DEBUG_MSG_P(PSTR("[NSPANEL] URL request: HTTP %d\n"), code);
        } else {
            DEBUG_MSG_P(PSTR("[NSPANEL] URL request failed: %s\n"), HTTPClient::errorToString(code).c_str());
        }
    }
}

} // namespace http

void entityChanged(int entity);

// publishes a value to the entity's command topic, through its "payload" template ("%s" = value)
void entitySend(int index, const String& value) {
    if ((index < 0) || (static_cast<size_t>(index) >= config.entities.size())) {
        return;
    }

#if MQTT_SUPPORT
    const auto& entity = config.entities[index];
    if (!entity.command.length()) {
        return;
    }

    if (entity.payload.length() && (entity.payload.indexOf(F("%s")) >= 0)) {
        String payload(entity.payload);
        payload.replace(F("%s"), value);
        mqttSendRaw(entity.command.c_str(), payload.c_str(), false);
    } else {
        mqttSendRaw(entity.command.c_str(), value.c_str(), false);
    }
#endif
}

void entitySet(int index, Action action) {
    if ((index < 0) || (static_cast<size_t>(index) >= config.entities.size())) {
        return;
    }

    const auto& entity = config.entities[index];
#if RELAY_SUPPORT
    if (entity.relay >= 0) {
        if (action == Action::Toggle) {
            relayToggle(entity.relay);
        } else {
            relayStatus(entity.relay, action == Action::On);
        }
        return;
    }
#endif

    // "url": a GET instead of the MQTT command. Toggle goes by the known state, else on. Without a state
    // topic and with two different URLs the panel keeps the state itself, as sent
    if (entity.url_on.length() || entity.url_off.length()) {
        const bool on = (action == Action::On) || ((action == Action::Toggle) && (entity.state != 1));
        const String& url = on ? entity.url_on : entity.url_off;
        if (url.length() && http::get(url) && !entity.topic.length() && (entity.url_on != entity.url_off)) {
            const int state = on ? 1 : 0;
            if (config.entities[index].state != state) {
                config.entities[index].state = state;
                entityChanged(index);
            }
        }
        return;
    }

    const String& on = entity.on.length() ? entity.on : config.on;
    const String& off = entity.off.length() ? entity.off : config.off;
    // unknown state: entities with their own "on" value are switched on, plain ones get mqtt.toggle
    const String* value = entity.on.length() ? &on : &config.toggle;
    switch (action) {
    case Action::On:
        value = &on;
        break;
    case Action::Off:
        value = &off;
        break;
    case Action::Toggle:
        if (entity.state >= 0) {
            value = entity.state ? &off : &on;
        }
        break;
    }

    entitySend(index, *value);
}

void groupSet(int group, Action action) {
    if ((group < 0) || (static_cast<size_t>(group) >= config.groups.size())) {
        return;
    }

    if (action == Action::Toggle) {
        action = (groupState(group) == 1) ? Action::Off : Action::On;
    }

    for (const auto entity : config.groups[group]) {
        entitySet(entity, action);
    }
}

// A card is one screen of tiles: a page, or a chunk of a page with more tiles than fit
struct Card {
    uint16_t page;
    uint16_t first;
    uint16_t count;
    String title;
    bool settings { false }; // the settings tiles (count of them), no items
    bool list { false };     // rows of a "type": "list" page (only when the driver draws lists)
    bool sub { false };      // a sheet of a nested page: after all cards of the swipe row
    Grid grid { Grid::G4x2 };// the page's "layout"
};

constexpr size_t SettingsTiles { 3 };

// tiles of a grid page on one screen, the smallest of
// - grids: the driver shows cols x rows (9, 6, 4, 2 or 8): the TFT's page per layout, or drawn tiles;
//   else the layout's columns of the 4x2 slot grid (2 x cols: 6, 6, 4, 2 or 8) and the driver's capacity;
// - "per_screen"
size_t gridCapacity(const Page& page, size_t capacity, bool grids = false) {
    size_t out = grids ? gridTiles(page.grid) : std::min<size_t>(capacity, 2 * gridCols(page.grid));
    if (page.per_screen) {
        out = std::min<size_t>(out, page.per_screen);
    }
    return std::max<size_t>(out, 1);
}

// settings: the driver draws "type": "settings" pages (OHpanel), others skip them
// list_rows: rows per sheet of "type": "list" pages; 0 = the driver shows them as grids
// grids: the driver shows every "layout" in full (gridCapacity())
void buildCards(std::vector<Card>& cards, size_t capacity, size_t title_max, bool settings = false,
        size_t list_rows = 0, bool grids = false)
{
    cards.clear();

    // the swipe row (top-level "pages") first, then the sheets of the nested pages (opened by their tiles)
    for (size_t index = 0; index < (2 * config.pages.size()); ++index) {
        const bool sub = (index >= config.pages.size());
        const size_t page = sub ? (index - config.pages.size()) : index;
        const auto& info = config.pages[page];
        if (info.nested != sub) {
            continue;
        }
        if (info.settings) {
            if (settings) {
                Card card;
                card.page = page;
                card.first = 0;
                card.count = SettingsTiles;
                card.title = info.title.length() ? info.title : String(lang::texts().settings);
                card.settings = true;
                card.sub = sub;
                utf8Truncate(card.title, title_max);
                cards.push_back(std::move(card));
            }
            continue;
        }

        const bool list = info.list && list_rows;
        const size_t per = list
            ? std::max<size_t>(1, info.per_screen ? std::min<size_t>(info.per_screen, list_rows) : list_rows)
            : gridCapacity(info, capacity, grids);
        const size_t total = info.items.size();
        const size_t chunks = std::max<size_t>(1, (total + per - 1) / per);

        for (size_t chunk = 0; chunk < chunks; ++chunk) {
            Card card;
            card.page = page;
            card.first = chunk * per;
            card.count = std::min(per, total - std::min(total, chunk * per));
            card.list = list;
            card.sub = sub;
            card.grid = list ? Grid::G4x2 : info.grid;
            card.title = info.title;
            if (chunks > 1) {
                card.title += ' ';
                card.title += String(chunk + 1);
                card.title += '/';
                card.title += String(chunks);
            }
            utf8Truncate(card.title, title_max);
            cards.push_back(std::move(card));
        }
    }
}

// first card of a config page, -1 when there is none
int firstCard(const std::vector<Card>& cards, int page) {
    for (size_t card = 0; card < cards.size(); ++card) {
        if (cards[card].page == page) {
            return card;
        }
    }
    return -1;
}

// cards of the swipe row (they come before the sub-page sheets)
size_t rowCards(const std::vector<Card>& cards) {
    size_t out = 0;
    while ((out < cards.size()) && !cards[out].sub) {
        ++out;
    }
    return out;
}

// both cards exist and are sheets of the same page
bool samePage(const std::vector<Card>& cards, size_t a, size_t b) {
    return (a < cards.size()) && (b < cards.size()) && (cards[a].page == cards[b].page);
}

// sheets of the card's page and the card's position among them
void pageSheets(const std::vector<Card>& cards, size_t card, size_t& count, size_t& current) {
    count = 0;
    current = 0;
    if (card >= cards.size()) {
        return;
    }
    for (size_t n = 0; n < cards.size(); ++n) {
        if (cards[n].page == cards[card].page) {
            if (n == card) {
                current = count;
            }
            ++count;
        }
    }
}

bool itemUses(const Item& item, int entity) {
    if (item.target.entity == entity) {
        return true;
    }

    if ((item.thermostat >= 0) && (static_cast<size_t>(item.thermostat) < config.thermostats.size())) {
        const auto& thermostat = config.thermostats[item.thermostat];
        if ((thermostat.current == entity) || (thermostat.humidity == entity) || (thermostat.active == entity)
            || (thermostat.min == entity) || (thermostat.max == entity))
        {
            return true;
        }
    }

    if (item.target.group >= 0) {
        for (const auto member : config.groups[item.target.group]) {
            if (member == entity) {
                return true;
            }
        }
    }

    return false;
}

bool cardContains(const std::vector<Card>& cards, size_t card, int entity) {
    if (card >= cards.size()) {
        return false;
    }

    const auto& info = cards[card];
    if (info.settings) {
        return false;
    }

    const auto& page = config.pages[info.page];
    for (size_t n = info.first; n < (info.first + info.count); ++n) {
        if (itemUses(config.items[page.items[n]], entity)) {
            return true;
        }
    }

    return false;
}

// number of group members that are on
int groupOnCount(int group) {
    if ((group < 0) || (static_cast<size_t>(group) >= config.groups.size())) {
        return 0;
    }

    int out = 0;
    for (const auto entity : config.groups[group]) {
        if (entityState(entity) == 1) {
            ++out;
        }
    }
    return out;
}

// implemented by the active display driver (dispatch below the drivers)
void entityChanged(int entity);

namespace driver {

void weatherChanged();
void temperatureChanged();

} // namespace driver

// -----------------------------------------------------------------------------
// Config upload hand-over (POST /nspanel.json)
// -----------------------------------------------------------------------------

// The async_tcp handlers only collect the body; validate + save + apply run in loop()
// (ref. upload::process()) and the HTTP answer is sent from the client's poll callback
// once the result is known (ref. onUploadPoll()). Job is shared by the async_tcp handlers
// (under AsyncGuard) and loop(), which never run at the same time.
namespace upload {

enum class State : uint8_t {
    Idle,
    Queued,  // body complete, waiting for loop()
    Done,    // code / message ready for the client
};

struct Job {
    State state { State::Idle };
    uint32_t id { 0 };
    uint32_t queued_ms { 0 };
    std::unique_ptr<char[]> buffer;
    size_t size { 0 };
    int code { 0 };
    String message;
};

Job job;

// a body is being received into the async_tcp buffer (read by loop() without the lock)
std::atomic<bool> receiving { false };

// no weather TLS session (40-60 KB) is started while a config is on its way
bool busy() {
    return receiving.load() || (job.state == State::Queued);
}

} // namespace upload

// -----------------------------------------------------------------------------
// Weather (Open-Meteo, fetched in a separate task)
// -----------------------------------------------------------------------------

namespace weather {

constexpr size_t DaysMax { 7 };

struct Day {
    int code { -1 };
    float tmax { 0.0f };
    float tmin { 0.0f };
    int wday { 0 };
    int mday { 1 };
    int mon { 0 };         // 0..11
    float wind { NAN };    // m/s, daily max
    int rain { -1 };       // precipitation probability max, %
    float uv { NAN };      // UV index max
};

struct Data {
    bool valid { false };
    float temperature { 0.0f };
    int code { -1 };
    bool day { true };
    Day days[DaysMax];
    size_t count { 0 };
    time_t updated { 0 };
};

enum State : int {
    Idle,
    Running,
    Done,
    Failed,
};

std::atomic<int> state { Idle };
Data result;
Data current;
char url[384];
char last_error[64] { "never" };
uint32_t next_ms { 15000 };
uint32_t failures { 0 };

// runs in the task below; returns, so that its locals (body, TLS client) are destroyed
// before vTaskDelete(), which never returns and would leak them
void fetch() {
    Data out;
    int code = -1;
    String body;

    {
        WiFiClientSecure client;
        client.setInsecure();
        client.setHandshakeTimeout(10);

        HTTPClient http;
        http.setConnectTimeout(5000);
        http.setTimeout(5000);
        http.setReuse(false);
        if (http.begin(client, url)) {
            code = http.GET();
            if (code == 200) {
                body = http.getString();
            }
            http.end();
        }
    }

    if (code != 200) {
        snprintf(last_error, sizeof(last_error), "HTTP %d", code);
        state.store(Failed);
        return;
    }

    {
        DynamicJsonBuffer buffer(1024);
        JsonObject& root = buffer.parseObject(body.c_str());
        JsonObject& cur = root["current"];
        JsonObject& daily = root["daily"];
        if (root.success() && cur.success()) {
            out.temperature = cur["temperature_2m"].as<float>();
            out.code = cur["weather_code"] | -1;
            out.day = (cur["is_day"] | 1) != 0;
            out.valid = true;

            if (daily.success()) {
                JsonArray& times = daily["time"];
                JsonArray& codes = daily["weather_code"];
                JsonArray& tmax = daily["temperature_2m_max"];
                JsonArray& tmin = daily["temperature_2m_min"];
                JsonArray& wind = daily["wind_speed_10m_max"];
                JsonArray& rain = daily["precipitation_probability_max"];
                JsonArray& uv = daily["uv_index_max"];
                const size_t count = std::min<size_t>(DaysMax, times.size());
                for (size_t n = 0; n < count; ++n) {
                    int y = 0, m = 0, d = 0;
                    const char* date = times[n].as<const char*>();
                    if (!date || (sscanf(date, "%d-%d-%d", &y, &m, &d) != 3)) {
                        break;
                    }
                    out.days[n].wday = weekday(y, m, d);
                    out.days[n].mday = d;
                    out.days[n].mon = m - 1;
                    out.days[n].code = codes[n] | -1;
                    out.days[n].tmax = tmax[n].as<float>();
                    out.days[n].tmin = tmin[n].as<float>();
                    if (wind.success() && wind[n].is<float>()) {
                        out.days[n].wind = wind[n].as<float>();
                    }
                    if (rain.success() && rain[n].is<int>()) {
                        out.days[n].rain = rain[n].as<int>();
                    }
                    if (uv.success() && uv[n].is<float>()) {
                        out.days[n].uv = uv[n].as<float>();
                    }
                    out.count = n + 1;
                }
            }
        }
    }

    if (!out.valid) {
        snprintf(last_error, sizeof(last_error), "invalid response (%u bytes)", body.length());
        state.store(Failed);
        return;
    }

    out.updated = ::time(nullptr);
    result = out;
    snprintf(last_error, sizeof(last_error), "ok");
    state.store(Done);
}

void task(void*) {
    fetch();
    vTaskDelete(nullptr);
}

void request() {
    next_ms = millis();
}

// the fetch task holds a TLS session, config parsing waits for it (ref. upload::process())
bool running() {
    return state.load() == Running;
}

void loop() {
    if (!config.weather) {
        return;
    }

    const auto now = millis();
    switch (state.load()) {
    case Running:
        break;

    case Done:
        current = result;
        state.store(Idle);
        failures = 0;
        next_ms = now + config.refresh * 1000ul;
        DEBUG_MSG_P(PSTR("[NSPANEL] Weather: %.1fC code %d, %u days\n"),
            current.temperature, current.code, current.count);
        driver::weatherChanged();
        break;

    case Failed:
        state.store(Idle);
        ++failures;
        next_ms = now + build::WeatherRetryMs;
        DEBUG_MSG_P(PSTR("[NSPANEL] Weather update failed: %s\n"), last_error);
        break;

    case Idle:
        if (static_cast<int32_t>(now - next_ms) < 0) {
            break;
        }

        if (!wifiConnected()) {
            next_ms = now + 5000;
            break;
        }

        // a config upload being received / parsed goes first, ref. upload::process()
        if (upload::busy()) {
            next_ms = now + 2000;
            break;
        }

        if (ESP.getFreeHeap() < build::WeatherHeapMin) {
            snprintf(last_error, sizeof(last_error), "low heap (%u)", ESP.getFreeHeap());
            next_ms = now + build::WeatherRetryMs;
            break;
        }

        snprintf(url, sizeof(url),
            "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
            "&current=temperature_2m,weather_code,is_day"
            "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
            "wind_speed_10m_max,precipitation_probability_max,uv_index_max"
            "&wind_speed_unit=ms&timezone=auto&forecast_days=7",
            config.lat, config.lon);

        state.store(Running);
        if (pdPASS != xTaskCreate(task, "nspweather", build::WeatherStack, nullptr, 1, nullptr)) {
            state.store(Idle);
            snprintf(last_error, sizeof(last_error), "task create failed");
            next_ms = now + build::WeatherRetryMs;
        }
        break;
    }
}

} // namespace weather

// -----------------------------------------------------------------------------
// Room temperature: the NTC on GPIO38 is an ESPurna sensor (NTCSensor), so it shows up in the web UI, the sensor
// MQTT topics and the correction / units settings; the screen shows the first temperature magnitude
// -----------------------------------------------------------------------------

namespace ntc {

float value { NAN };
bool reported { false };

String text() {
    if (!config.temperature || std::isnan(value)) {
        return String();
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f\xC2\xB0" "C", value);
    return buf;
}

#if SENSOR_SUPPORT
// single readings until the first report, then only reports (filtered over snsReport readings)
void update(const espurna::sensor::Value& reading, bool report) {
    if ((reading.type != MAGNITUDE_TEMPERATURE) || (reading.index != 0) || std::isnan(reading.value)) {
        return;
    }

    if (!report && reported) {
        return;
    }

    reported = reported || report;

    // the screen shows Celsius or Fahrenheit as set for the magnitude, never Kelvin
    float next = reading.value;
    if (reading.units == espurna::sensor::Unit::Kelvin) {
        next -= 273.15f;
    }

    if (next != value) {
        value = next;
        driver::temperatureChanged();
    }
}

void onRead(const espurna::sensor::Value& reading) {
    update(reading, false);
}

void onReport(const espurna::sensor::Value& reading) {
    update(reading, true);
}
#endif

void setup() {
#if SENSOR_SUPPORT
    sensorOnMagnitudeRead(onRead);
    sensorOnMagnitudeReport(onReport);
#endif
}

} // namespace ntc

// -----------------------------------------------------------------------------
// Buzzer
// -----------------------------------------------------------------------------

namespace buzzer {

bool active { false };
uint32_t until { 0 };

// core 3.x: ledcWrite() takes the pin, the channel API is ledcAttachChannel() / ledcWriteChannel()
void duty(uint32_t value) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWriteChannel(build::BuzzerChannel, value);
#else
    ledcWrite(build::BuzzerChannel, value);
#endif
}

void setup() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttachChannel(build::BuzzerPin, 2700, 8, build::BuzzerChannel);
#else
    ledcSetup(build::BuzzerChannel, 2700, 8);
    ledcAttachPin(build::BuzzerPin, build::BuzzerChannel);
#endif
    duty(0);
}

void beep() {
    if (!config.beep) {
        return;
    }

    duty(128);
    active = true;
    until = millis() + 40;
}

void loop() {
    if (active && (static_cast<int32_t>(millis() - until) >= 0)) {
        duty(0);
        active = false;
    }
}

} // namespace buzzer

// -----------------------------------------------------------------------------
// Tile actions, the same for both drivers. value: -1 toggle (switch), 0 off, 1 on
// -----------------------------------------------------------------------------

void itemTap(size_t index, int value) {
    if (index >= config.items.size()) {
        return;
    }

    const auto& item = config.items[index];
    buzzer::beep();

    switch (item.type) {
    case ItemType::Switch:
        entitySet(item.target.entity,
            (value < 0) ? Action::Toggle : (value ? Action::On : Action::Off));
        break;

    case ItemType::Button:
        entitySet(item.target.entity, Action::On);
        break;

    case ItemType::Scene:
        groupSet(item.target.group, item.target.action);
        break;

    case ItemType::Text:
    case ItemType::Link:       // navigation, handled by the driver
    case ItemType::Thermostat: // opens the thermostat page (OHpanel driver)
        break;
    }
}

// -----------------------------------------------------------------------------
// Thermostat values, the same for both drivers
// -----------------------------------------------------------------------------

float entityNumber(int index) {
    if ((index < 0) || (static_cast<size_t>(index) >= config.entities.size())) {
        return NAN;
    }
    return config.entities[index].number;
}

// heating: "active" is known and not one of the idle words
bool thermostatActive(const Thermostat& thermostat) {
    if ((thermostat.active < 0) || (static_cast<size_t>(thermostat.active) >= config.entities.size())) {
        return false;
    }

    const auto& value = config.entities[thermostat.active].value;
    if (!value.length()) {
        return false;
    }

    String lower(value);
    lower.toLowerCase();
    return (lower != F("off")) && (lower != F("false")) && (lower != F("0")) && (lower != F("idle"));
}

float thermostatMin(const Thermostat& thermostat) {
    const float value = entityNumber(thermostat.min);
    return std::isnan(value) ? thermostat.min_value : value;
}

float thermostatMax(const Thermostat& thermostat) {
    const float value = entityNumber(thermostat.max);
    const float min = thermostatMin(thermostat);
    return (std::isnan(value) || (value <= min)) ? std::max(thermostat.max_value, min + thermostat.step) : value;
}

// snapped to the step grid above min, clamped to min..max
float thermostatClamp(const Thermostat& thermostat, float value) {
    const float min = thermostatMin(thermostat);
    const float max = thermostatMax(thermostat);
    value = min + roundf((value - min) / thermostat.step) * thermostat.step;
    return std::max(min, std::min(max, value));
}

// "21.5" / "21,5" (1 decimal unless the value is whole and the step too)
String temperatureShort(float value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", value);
    lang::decimal(buf);
    return buf;
}

// -----------------------------------------------------------------------------
// JSON state payloads. The MQTT callback (AsyncTCP task, under the loop lock) only keeps the
// latest payload per topic; loop() parses one per call (ref. jsonLoop()).
// -----------------------------------------------------------------------------

namespace json {

constexpr size_t PendingMax { 8 };

struct Pending {
    String topic;
    String payload;
};

std::vector<Pending> pending;
uint32_t parsed { 0 };
uint32_t failed { 0 };
uint32_t dropped { 0 };

void queue(espurna::StringView topic, espurna::StringView payload) {
    for (auto& entry : pending) {
        if (topic.equals(StringView(entry.topic))) {
            entry.payload = payload.toString();
            return;
        }
    }

    if (pending.size() >= PendingMax) {
        pending.erase(pending.begin());
        ++dropped;
    }

    Pending entry;
    entry.topic = topic.toString();
    entry.payload = payload.toString();
    pending.push_back(std::move(entry));
}

// the value at the entity's path: text and number
bool extract(JsonVariant root, const String& path, String& text, float& number) {
    JsonVariant value = root;
    int start = 0;
    while (start <= static_cast<int>(path.length())) {
        int end = path.indexOf(PathSep, start);
        if (end < 0) {
            end = path.length();
        }
        const String key = path.substring(start, end);
        start = end + 1;

        if (value.is<JsonObject>()) {
            JsonObject& obj = value.as<JsonObject>();
            if (!obj.containsKey(key)) {
                return false;
            }
            value = obj.get<JsonVariant>(key);
        } else if (value.is<JsonArray>()) {
            JsonArray& array = value.as<JsonArray>();
            const long n = key.toInt();
            if ((n < 0) || (static_cast<size_t>(n) >= array.size())) {
                return false;
            }
            value = array.get<JsonVariant>(n);
        } else {
            return false;
        }
    }

    number = NAN;
    if (value.is<const char*>()) {
        // ArduinoJson 5 takes a JSON null for a string and gives NULL for it: a String assigned NULL has no
        // buffer, and strtof(NULL) below crashed the panel on any null at an entity's path (crash loop -> unstable mode)
        const char* raw = value.as<const char*>();
        if (!raw) {
            return false;
        }
        text = raw;
        char* end = nullptr;
        const float parsed = strtof(text.c_str(), &end);
        if (text.length() && end && !*end) {
            number = parsed;
        }
    } else if (value.is<bool>()) {
        text = value.as<bool>() ? F("true") : F("false");
    } else if (value.is<long>() || value.is<double>()) {
        number = value.as<float>();
        char buf[24];
        snprintf(buf, sizeof(buf), "%g", value.as<double>());
        text = buf;
    } else {
        return false; // object, array or null
    }

    utf8Truncate(text, 32);
    return true;
}

void process(Pending& entry) {
    DynamicJsonBuffer buffer(512);
    // parses in place: the strings stay in entry.payload, nothing is copied
    JsonVariant root = buffer.parse(entry.payload.begin());
    if (!root.is<JsonObject>() && !root.is<JsonArray>()) {
        ++failed;
        return;
    }
    ++parsed;

    for (size_t index = 0; index < config.entities.size(); ++index) {
        auto& entity = config.entities[index];
        if (!entity.path.length() || (entity.topic != entry.topic)) {
            continue;
        }

        String text;
        float number = NAN;
        if (!extract(root, entity.path, text, number)) {
            continue; // not in this message (z2m sends partial updates too)
        }

        const String& on = entity.on.length() ? entity.on : config.on;
        const String& off = entity.off.length() ? entity.off : config.off;
        const int state = (text == on) ? 1 : (text == off) ? 0 : -1;

        const bool same_number = (std::isnan(number) && std::isnan(entity.number)) || (number == entity.number);
        if ((state != entity.state) || (text != entity.value) || !same_number) {
            entity.state = state;
            entity.value = std::move(text);
            entity.number = number;
            entityChanged(index);
        }
    }
}

void loop() {
    if (pending.empty()) {
        return;
    }

    Pending entry = std::move(pending.front());
    pending.erase(pending.begin());
    process(entry);
}

} // namespace json

// -----------------------------------------------------------------------------
// History: a thermostat's or a switch tile's "history" topic holds a retained series (e.g. published by an openHAB rule
// from its persistence). It is subscribed only while the chart is open, so nothing is kept on the panel:
// {"from": <unix time of the first point>, "step": <seconds>, "t": [<temperature x10> | null, ...], "s": [<setpoint x10> | null, ...],
//  "h": [<humidity % x10> | null, ...]} ("h" is optional), or for a switch {"from", "step", "p": [<% of the step it was ON> | null, ...]}
// -----------------------------------------------------------------------------

namespace history {

constexpr size_t PayloadMax { 2048 };   // MQTT_BUFFER_MAX_SIZE of the NSPanel build
constexpr size_t PointsMax { 192 };
constexpr int16_t Missing { INT16_MIN };

struct Series {
    uint32_t from { 0 };
    uint32_t step { 0 };
    std::vector<int16_t> temperature;
    std::vector<int16_t> setpoint;
    std::vector<int16_t> humidity;
    std::vector<int16_t> on;    // a switch: % of each step it was ON
};

String topic;               // subscribed while a chart is open, empty otherwise
String payload;             // the last message, parsed in loop()
bool fresh { false };
bool subscribed { false };

void close() {
#if MQTT_SUPPORT
    if (subscribed && mqttConnected()) {
        mqttUnsubscribeRaw(topic.c_str());
    }
#endif
    topic = String();
    payload = String();
    fresh = false;
    subscribed = false;
}

// the broker sends the retained message right after the subscription
void open(const String& name) {
    close();
    topic = name;
#if MQTT_SUPPORT
    subscribed = mqttConnected() && mqttSubscribeRaw(topic.c_str());
#endif
}

// loop(): subscribe again after a reconnect (or when the first attempt found no room)
void loop() {
#if MQTT_SUPPORT
    if (topic.length() && !subscribed && mqttConnected()) {
        subscribed = mqttSubscribeRaw(topic.c_str());
    }
#endif
}

// MQTT callback: only keep the text
void message(StringView name, StringView body) {
    if (topic.length() && name.equals(StringView(topic)) && (body.length() < PayloadMax)) {
        payload = body.toString();
        fresh = true;
    }
}

void read(JsonVariant values, std::vector<int16_t>& out) {
    out.clear();
    JsonArray& array = values;
    if (!array.success()) {
        return;
    }
    out.reserve(std::min(array.size(), PointsMax));
    for (auto value : array) {
        if (out.size() >= PointsMax) {
            break;
        }
        if (value.is<long>() || value.is<double>()) {
            const long number = lroundf(value.as<float>());
            out.push_back(static_cast<int16_t>(std::max(-32000L, std::min(32000L, number))));
        } else {
            out.push_back(Missing);
        }
    }
}

// a new message, parsed into out; false when there is none or it is not a series
bool take(Series& out) {
    if (!fresh) {
        return false;
    }
    fresh = false;

    try {
        DynamicJsonBuffer buffer(4096);
        JsonObject& root = buffer.parseObject(payload.c_str());
        if (!root.success()) {
            return false;
        }
        out.from = root["from"].as<unsigned long>();
        out.step = root["step"].as<unsigned long>();
        read(root["t"], out.temperature);
        read(root["s"], out.setpoint);
        read(root["h"], out.humidity);
        read(root["p"], out.on);
    } catch (const std::bad_alloc&) {
        return false;
    }

    return (out.step > 0) && ((out.temperature.size() > 1) || (out.on.size() > 1));
}

} // namespace history

// -----------------------------------------------------------------------------
// On-panel settings (OHpanel settings tiles), kept in ESPurna settings so that they survive a
// reboot and a JSON reload; ohpanel.layout / ohpanel.theme are only the defaults
// -----------------------------------------------------------------------------

void applyUiSettings(Config& out) {
    const auto icons = getSetting(F("nspIcons"));
    if (icons == F("center")) {
        out.layout = Layout::Center;
    } else if (icons == F("corner")) {
        out.layout = Layout::Corner;
    }

    const auto theme = getSetting(F("nspTheme"));
    if (theme == F("light")) {
        out.theme = Theme::Light;
    } else if (theme == F("dark")) {
        out.theme = Theme::Dark;
    }

    const auto background = getSetting(F("nspBg"));
    for (size_t index = 0; index < (sizeof(BackgroundNames) / sizeof(BackgroundNames[0])); ++index) {
        if (background == BackgroundNames[index]) {
            out.background = index;
            break;
        }
    }
}

void saveLayout(Layout layout) {
    config.layout = layout;
    setSetting(F("nspIcons"), String((layout == Layout::Corner) ? F("corner") : F("center")));
}

void saveTheme(Theme theme) {
    config.theme = theme;
    setSetting(F("nspTheme"), String((theme == Theme::Light) ? F("light") : F("dark")));
}

void saveBackground(uint8_t background) {
    config.background = background;
    setSetting(F("nspBg"), String(BackgroundNames[background])); // a bare const char* picks the bool overload
}

// -----------------------------------------------------------------------------
// Display drivers
// -----------------------------------------------------------------------------

#include "nspanel_lovelace.h"
#include "nspanel_ohpanel.h"

// -----------------------------------------------------------------------------
// Display detection. Exactly one driver owns UART2 at a time.
//
// auto: OHpanel at 921600 (FF FF FF sendme + get design_ver, expects 66 / 71) for 600 ms, then
// Lovelace at 115200 (raw recmod=1, pageType~pageStartup, expects event,startup) for 2 s, repeated
// until one answers. Forced: only that driver, retried. No answer for 30 s: the display is
// power-cycled once (GPIO4 HIGH 800 ms, 1.5 s boot), not within 3 min after a TFT upload.
// Detection runs again at boot, after every TFT upload, when "display" changes, on
// `nspanel.detect`, and when the active driver reports the TFT lost.
// -----------------------------------------------------------------------------

namespace link {

enum class Power {
    None,
    Off,
    Boot,
};

Driver active { Driver::None };
Driver detected { Driver::None };
bool probing { true };
uint32_t window_start { 0 };
uint32_t episode_start { 0 };
uint32_t quiet_until { 0 };     // no automatic power-cycle before this
bool power_cycled { false };
Power power { Power::None };
uint32_t power_at { 0 };
uint32_t probes { 0 };
String reason { "boot" };

Driver first() {
    switch (config.display) {
    case DriverPref::Lovelace:
        return Driver::Lovelace;
    case DriverPref::Ohpanel:
    case DriverPref::Auto:
        break;
    }
    return Driver::Ohpanel;
}

Driver next(Driver current) {
    switch (config.display) {
    case DriverPref::Lovelace:
        return Driver::Lovelace;
    case DriverPref::Ohpanel:
        return Driver::Ohpanel;
    case DriverPref::Auto:
        break;
    }
    return (current == Driver::Ohpanel) ? Driver::Lovelace : Driver::Ohpanel;
}

uint32_t window(Driver driver) {
    const bool forced = (config.display != DriverPref::Auto);
    if (driver == Driver::Lovelace) {
        return forced ? build::ProbeForcedLovelaceMs : build::ProbeLovelaceMs;
    }
    return forced ? build::ProbeForcedOhpanelMs : build::ProbeOhpanelMs;
}

bool connected() {
    switch (active) {
    case Driver::Lovelace:
        return lovelace::connected();
    case Driver::Ohpanel:
        return ohpanel::connected();
    case Driver::None:
        break;
    }
    return false;
}

void start(Driver driver) {
    active = driver;
    window_start = millis();
    ++probes;
    switch (driver) {
    case Driver::Lovelace:
        lovelace::begin();
        break;
    case Driver::Ohpanel:
        ohpanel::begin();
        break;
    case Driver::None:
        break;
    }
}

void probe(const char* why) {
    DEBUG_MSG_P(PSTR("[NSPANEL] Detecting the display (%s, display: %s)\n"), why, driverPrefName(config.display));
    reason = why;
    probing = true;
    episode_start = millis();
    power_cycled = false;
    power = Power::None;
    digitalWrite(build::DisplayPowerPin, LOW);
    start(first());
}

void afterFlash(bool done) {
    quiet_until = millis() + build::FlashQuietMs;
    probe(done ? "TFT uploaded" : "TFT upload failed");
}

void loop() {
    const auto now = millis();

    switch (power) {
    case Power::Off:
        if (now - power_at >= build::PowerOffMs) {
            digitalWrite(build::DisplayPowerPin, LOW);
            power = Power::Boot;
            power_at = now;
        }
        return;

    case Power::Boot:
        if (now - power_at >= build::PowerBootMs) {
            power = Power::None;
            start(first());
        }
        return;

    case Power::None:
        break;
    }

    switch (active) {
    case Driver::Lovelace:
        lovelace::loop();
        break;
    case Driver::Ohpanel:
        ohpanel::loop();
        break;
    case Driver::None:
        break;
    }

    if (!probing) {
        const bool lost = (active == Driver::Lovelace) ? lovelace::lost()
            : (active == Driver::Ohpanel) ? ohpanel::lost()
            : true;
        if (lost) {
            probe("display lost");
        }
        return;
    }

    if (connected()) {
        probing = false;
        detected = active;
        DEBUG_MSG_P(PSTR("[NSPANEL] Display driver: %s (after %u ms, %u probes)\n"),
            driverName(active), static_cast<unsigned>(now - episode_start), probes);
        return;
    }

    if ((active == Driver::Ohpanel) && ohpanel::found()) {
        return; // identifying
    }

    if (now - window_start < window(active)) {
        return;
    }

    if (!power_cycled && (now - episode_start >= build::ProbePowerCycleMs)
        && (static_cast<int32_t>(now - quiet_until) >= 0))
    {
        DEBUG_MSG_P(PSTR("[NSPANEL] No display answer for %u s, power-cycling it\n"),
            static_cast<unsigned>((now - episode_start) / 1000));
        power_cycled = true;
        power = Power::Off;
        power_at = now;
        digitalWrite(build::DisplayPowerPin, HIGH);
        return;
    }

    start(next(active));
}

} // namespace link

// -----------------------------------------------------------------------------
// Dispatch to the active driver
// -----------------------------------------------------------------------------

void entityChanged(int entity) {
    switch (link::active) {
    case Driver::Lovelace:
        lovelace::entityChanged(entity);
        break;
    case Driver::Ohpanel:
        ohpanel::entityChanged(entity);
        break;
    case Driver::None:
        break;
    }
}

namespace driver {

void weatherChanged() {
    switch (link::active) {
    case Driver::Lovelace:
        lovelace::weatherChanged();
        break;
    case Driver::Ohpanel:
        ohpanel::weatherChanged();
        break;
    case Driver::None:
        break;
    }
}

void temperatureChanged() {
    switch (link::active) {
    case Driver::Lovelace:
        lovelace::temperatureChanged();
        break;
    case Driver::Ohpanel:
        ohpanel::temperatureChanged();
        break;
    case Driver::None:
        break;
    }
}

void reload() {
    switch (link::active) {
    case Driver::Lovelace:
        lovelace::reload();
        break;
    case Driver::Ohpanel:
        ohpanel::reload();
        break;
    case Driver::None:
        break;
    }
}

void showPage(int page) {
    switch (link::active) {
    case Driver::Lovelace:
        lovelace::showPage(page);
        break;
    case Driver::Ohpanel:
        ohpanel::showPage(page);
        break;
    case Driver::None:
        break;
    }
}

void send(String payload) {
    switch (link::active) {
    case Driver::Lovelace:
        lovelace::send(std::move(payload));
        break;
    case Driver::Ohpanel:
        ohpanel::send(std::move(payload));
        break;
    case Driver::None:
        break;
    }
}

const char* version() {
    if (!link::connected()) {
        return "";
    }
    if (link::active == Driver::Ohpanel) {
        return ohpanel::state.design_known ? ohpanel::state.design.c_str() : "unknown";
    }
    static char buf[8];
    snprintf(buf, sizeof(buf), "%d", lovelace::display.version);
    return buf;
}

const char* model() {
    if (!link::connected()) {
        return "";
    }
    return (link::active == Driver::Ohpanel) ? ohpanel::state.model.c_str() : lovelace::display.model.c_str();
}

// OHpanel: the design's name and what it has (grid layouts, backgrounds)
String design() {
    if (!link::connected() || (link::active != Driver::Ohpanel)) {
        return String();
    }
    return ohpanel::designText();
}

} // namespace driver

// -----------------------------------------------------------------------------
// Nextion TFT upload (upload protocol v1.2 `whmi-wris`, fallback v1.1 `whmi-wri`)
//
// The upload runs in its own task. While it runs the loop does not touch UART2 (no TX queue,
// no RX parser, no display probing; panelLoop() returns early), and HardwareSerial has no
// onReceive callbacks, so every received byte reaches this task. The task only talks to the
// loop through the atomics below and a message queue, which the loop prints.
//
//   (921600, 115200, 9600) FF FF FF, DRAKJHSUYDGBNCJHGJKSHBDN FF FF FF  leave protocol reparse mode
//                        recmod=0 FF FF FF (x2), sleep=0 FF FF FF
//                        connect FF FF FF, FF FF connect FF FF FF     <- comok ...,<flash size> FF FF FF
//                        50 ms, throw-away `sendme` (the display swallows the command after
//                        `connect`, ohpanel-protocol.md 1.3), drain RX for 250 ms
//                        whmi-wris <size>,<baud>,res0 FF FF FF        switch to <baud>, <- 05 (up to 30 s)
//   per 4096-byte block  <block>                                      <- 05 (next), or
//                                                                     <- 08 + uint32 LE offset (v1.2 only:
//                                                                        continue from that file offset,
//                                                                        0 = next block)
// Attempts, each after a fresh handshake, with a display power-cycle between them:
//   1. whmi-wris <size>,<requested baud>,res0   2. whmi-wris <size>,115200,res0
//   3. whmi-wri <size>,115200,res0 (v1.1)       4. whmi-wris <size>,115200,1    5. whmi-wri <size>,115200,1
// Every byte received during the handshake and while waiting for the first 05 is logged in hex
// (first 64 bytes of each phase).
// The handshake only needs the Nextion OS, not the TFT, so it also works after a failed upload.
// When the display does not answer, it is power-cycled (GPIO4) and asked again.
// -----------------------------------------------------------------------------

namespace flash {

constexpr size_t Block { 4096 };
constexpr uint32_t Stack { 8192 };
constexpr uint32_t StackTls { 12288 };
constexpr size_t HeapMin { 30000 };
constexpr size_t HeapMinTls { 65000 };
constexpr uint32_t ConnectMs { 1000 };
constexpr uint32_t SettleMs { 250 };
constexpr uint32_t ReadyMs { 30000 };
constexpr uint32_t AckMs { 5000 };
constexpr uint32_t OffsetMs { 10000 };
constexpr uint32_t HttpStallMs { 15000 };
constexpr int HttpRetries { 3 };
constexpr int PowerCycles { 2 };
constexpr uint32_t PowerOffMs { 1000 };
constexpr uint32_t BootMs { 3000 };
constexpr uint32_t FallbackBaud { 115200 };
constexpr size_t MessageMax { 112 };
constexpr size_t MessagesMax { 32 };
constexpr size_t CaptureMax { 64 };

enum State : int {
    Idle,
    Running,
    Done,
    Failed,
};

enum Phase : int {
    PhaseNone,
    PhaseHttp,
    PhaseConnect,
    PhaseStart,
    PhaseUpload,
};

const char* phaseName(int value) {
    switch (value) {
    case PhaseHttp:
        return "http";
    case PhaseConnect:
        return "connecting";
    case PhaseStart:
        return "starting";
    case PhaseUpload:
        return "uploading";
    }

    return "-";
}

std::atomic<int> state { Idle };
std::atomic<int> phase { PhaseNone };
std::atomic<uint32_t> total { 0 };
std::atomic<uint32_t> sent { 0 };
std::atomic<uint32_t> baud { 0 };

// written by the loop before the task starts, or by the task while it runs
char url[256];
uint32_t requested { 921600 };
uint32_t started_ms { 0 };
char error[MessageMax] { "" };
char comok[MessageMax] { "" };
char variant[48] { "" };
uint32_t flash_size { 0 };

// loop only
char last_result[MessageMax] { "never" };
QueueHandle_t messages { nullptr };

bool busy() {
    return state.load() != Idle;
}

// ---- task side ----

void message(const char* format, ...) __attribute__((format(printf, 1, 2)));
void message(const char* format, ...) {
    char buf[MessageMax];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    if (messages) {
        xQueueSend(messages, buf, pdMS_TO_TICKS(50));
    }
}

void fail(const char* format, ...) __attribute__((format(printf, 1, 2)));
void fail(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(error, sizeof(error), format, args);
    va_end(args);
    message("%s", error);
}

HardwareSerial& port() {
    return uart::port();
}

void setBaud(uint32_t value) {
    port().flush(true);
    port().updateBaudRate(value);
    baud.store(value);
}

// raw RX bytes of the current phase, logged in hex
struct Capture {
    bool active { false };
    uint8_t data[CaptureMax];
    size_t len { 0 };
    size_t total { 0 };
};

Capture capture;

void captureStart() {
    capture.active = true;
    capture.len = 0;
    capture.total = 0;
}

void captureDump(const char* label) {
    capture.active = false;
    if (!capture.total) {
        message("%s: no bytes received", label);
        return;
    }

    message("%s: %u bytes received%s", label, capture.total,
        (capture.total > capture.len) ? ", first 64:" : ":");

    constexpr size_t PerLine { 24 };
    for (size_t pos = 0; pos < capture.len; pos += PerLine) {
        char line[PerLine * 3 + 1];
        size_t out = 0;
        for (size_t n = pos; (n < capture.len) && (n < pos + PerLine); ++n) {
            out += snprintf(line + out, sizeof(line) - out, "%02X ", capture.data[n]);
        }
        line[out ? out - 1 : 0] = '\0';
        message("  %s", line);
    }
}

int rx() {
    const int byte = port().read();
    if ((byte >= 0) && capture.active) {
        if (capture.len < CaptureMax) {
            capture.data[capture.len++] = byte;
        }
        ++capture.total;
    }
    return byte;
}

void drain(uint32_t ms) {
    const auto start = millis();
    for (;;) {
        while (rx() >= 0) {
        }
        if (millis() - start >= ms) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

int waitByte(uint32_t timeout) {
    const auto start = millis();
    for (;;) {
        const int byte = rx();
        if (byte >= 0) {
            return byte;
        }
        if (millis() - start >= timeout) {
            return -1;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void powerCycle() {
    message("power-cycling the display");
    digitalWrite(build::DisplayPowerPin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(PowerOffMs));
    digitalWrite(build::DisplayPowerPin, LOW);
    drain(BootMs); // power-up announcement, not for us
}

int find(const uint8_t* data, size_t len, const char* pattern) {
    const size_t plen = strlen(pattern);
    for (size_t pos = 0; (pos + plen) <= len; ++pos) {
        if (0 == memcmp(data + pos, pattern, plen)) {
            return pos;
        }
    }
    return -1;
}

// 'comok <touch>,<reserved>,<model>,<fw>,<mcu>,<serial>,<flash size>' FF FF FF
bool connect(uint32_t rate) {
    static constexpr uint8_t Ff[3] { 0xFF, 0xFF, 0xFF };

    setBaud(rate);
    drain(20);
    captureStart();

    port().write(Ff, sizeof(Ff));
    uart::writeRaw("DRAKJHSUYDGBNCJHGJKSHBDN");
    vTaskDelay(pdMS_TO_TICKS(50));
    uart::writeRaw("recmod=0");
    uart::writeRaw("recmod=0");
    uart::writeRaw("sleep=0");
    drain(100);

    uart::writeRaw("connect");
    port().write(Ff, 2);
    uart::writeRaw("connect");

    uint8_t buf[160];
    size_t len = 0;
    const auto start = millis();
    while (millis() - start < ConnectMs) {
        int byte;
        while ((byte = rx()) >= 0) {
            if (len == sizeof(buf)) {
                memmove(buf, buf + sizeof(buf) / 2, sizeof(buf) / 2);
                len = sizeof(buf) / 2;
            }
            buf[len++] = byte;
        }

        const int pos = find(buf, len, "comok");
        if (pos >= 0) {
            const int end = find(buf + pos, len - pos, "\xFF\xFF\xFF");
            if (end >= 0) {
                size_t out = 0;
                for (int n = pos; (n < pos + end) && (out + 1 < sizeof(comok)); ++n) {
                    if ((buf[n] >= 0x20) && (buf[n] < 0x7F)) {
                        comok[out++] = buf[n];
                    }
                }
                comok[out] = '\0';

                const char* last = strrchr(comok, ',');
                flash_size = last ? strtoul(last + 1, nullptr, 10) : 0;

                message("display answered at %u baud: %s", rate, comok);

                // the rest of the line and the second comok; then a throw-away command,
                // because the display may swallow the command right after `connect`
                drain(50);
                uart::writeRaw("sendme");
                drain(SettleMs);

                char label[48];
                snprintf(label, sizeof(label), "handshake RX at %u baud", rate);
                captureDump(label);
                return true;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    char label[48];
    snprintf(label, sizeof(label), "no comok at %u baud", rate);
    captureDump(label);
    return false;
}

bool handshake() {
    static constexpr uint32_t Rates[] { build::OhpanelBaud, build::LovelaceBaud, 9600 };
    for (int cycle = 0; cycle <= PowerCycles; ++cycle) {
        if (cycle) {
            powerCycle();
        }
        for (const auto rate : Rates) {
            if (connect(rate)) {
                return true;
            }
        }
    }

    return false;
}

// HTTP(S) body as a byte stream with a file position; re-opens with a Range request
// to resume after a stall, or to jump to the offset the display asks for
struct Source {
    explicit Source(WiFiClient& client) :
        client(client)
    {}

    ~Source() {
        http.end();
    }

    bool open(uint32_t at) {
        http.end();
        http.setConnectTimeout(5000);
        http.setTimeout(10000);
        http.setReuse(false);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setRedirectLimit(5);

        static const char* Headers[] { "Content-Range" };
        http.collectHeaders(Headers, 1);

        if (!http.begin(client, url)) {
            fail("bad URL");
            return false;
        }

        if (at) {
            char range[32];
            snprintf(range, sizeof(range), "bytes=%u-", at);
            http.addHeader("Range", range);
        }

        const int code = http.GET();
        const int length = http.getSize();
        if ((code == HTTP_CODE_PARTIAL_CONTENT) && at) {
            char expected[24];
            snprintf(expected, sizeof(expected), "bytes %u-", at);
            if (!http.header("Content-Range").startsWith(expected) || (length != static_cast<int>(size - at))) {
                fail("HTTP 206 with unexpected range '%s' (%d bytes)", http.header("Content-Range").c_str(), length);
                return false;
            }
        } else if (code == HTTP_CODE_OK) {
            if (length <= 0) {
                fail("HTTP 200 without Content-Length");
                return false;
            }
            if (!at) {
                size = length;
            } else if (static_cast<uint32_t>(length) != size) {
                fail("file size changed (%d != %u)", length, size);
                return false;
            }
        } else {
            fail("HTTP %d (%s)", code, (code < 0) ? HTTPClient::errorToString(code).c_str() : "status");
            return false;
        }

        pos = 0;
        if ((code == HTTP_CODE_OK) && at) {
            message("server ignores Range, skipping %u bytes", at);
            uint8_t tmp[512];
            while (pos < at) {
                const size_t want = std::min<size_t>(sizeof(tmp), at - pos);
                if (readSome(tmp, want) != want) {
                    fail("HTTP read failed while skipping to %u", at);
                    return false;
                }
            }
        }

        pos = at;
        return true;
    }

    // up to 'len' bytes, short only on a closed connection or a stall
    size_t readSome(uint8_t* out, size_t len) {
        size_t got = 0;
        auto last = millis();
        while (got < len) {
            auto* stream = http.getStreamPtr();
            if (!stream) {
                break;
            }

            const int available = stream->available();
            if (available > 0) {
                const int n = stream->read(out + got, std::min<size_t>(available, len - got));
                if (n > 0) {
                    got += n;
                    pos += n;
                    last = millis();
                    continue;
                }
            }

            if (!stream->connected() && (stream->available() <= 0)) {
                break;
            }
            if (millis() - last >= HttpStallMs) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(2));
        }

        return got;
    }

    bool read(uint8_t* out, size_t len) {
        size_t got = 0;
        int retries = 0;
        while (got < len) {
            got += readSome(out + got, len - got);
            if (got == len) {
                break;
            }

            if (++retries > HttpRetries) {
                fail("HTTP read failed at %u", pos);
                return false;
            }

            message("HTTP stalled at %u, reconnecting (%d/%d)", pos, retries, HttpRetries);
            vTaskDelay(pdMS_TO_TICKS(1000 * retries));
            if (!open(pos)) {
                return false;
            }
        }

        return true;
    }

    WiFiClient& client;
    HTTPClient http;
    uint32_t size { 0 };
    uint32_t pos { 0 };
};

enum class Result {
    Ok,
    Fatal, // retrying with another variant would not help
    Retry, // the display did not accept or stopped answering, try the next variant
};

struct Variant {
    const char* command;
    uint32_t rate;
    const char* param;
    bool v12; // whmi-wris: 0x08 offset replies
};

Result attempt(Source& source, uint8_t* buffer, const Variant& how) {
    phase.store(PhaseHttp);
    if (!source.open(0)) {
        return Result::Fatal;
    }

    const uint32_t size = source.size;
    total.store(size);
    sent.store(0);
    if ((size < Block) || (size > (64ul << 20))) {
        fail("implausible TFT size %u bytes", size);
        return Result::Fatal;
    }

    phase.store(PhaseConnect);
    if (!handshake()) {
        fail("no 'comok' from the display (921600 / 115200 / 9600 baud, %d power cycles)", PowerCycles);
        return Result::Fatal;
    }

    if (flash_size && (size > flash_size)) {
        fail("TFT (%u bytes) is larger than the display flash (%u bytes)", size, flash_size);
        return Result::Fatal;
    }

    phase.store(PhaseStart);
    char command[48];
    snprintf(command, sizeof(command), "%s %u,%u,%s", how.command, size, how.rate, how.param);
    snprintf(variant, sizeof(variant), "%s", command);

    // nothing stale may be taken for the acknowledge
    while (port().read() >= 0) {
    }

    message("%s", command);
    uart::writeRaw(command);
    setBaud(how.rate);

    // anything before the 05 is noise from the speed change (logged)
    captureStart();
    bool ready = false;
    const auto start = millis();
    while (!ready) {
        const uint32_t elapsed = millis() - start;
        if (elapsed >= ReadyMs) {
            break;
        }
        ready = (waitByte(ReadyMs - elapsed) == 0x05);
    }

    char label[64];
    snprintf(label, sizeof(label), "RX before the first 05 (%u ms)", static_cast<unsigned>(millis() - start));
    captureDump(label);

    if (!ready) {
        fail("display did not acknowledge '%s' within %u s", command, ReadyMs / 1000);
        return Result::Retry;
    }

    phase.store(PhaseUpload);
    const auto upload_start = millis();
    uint32_t pos = 0;
    uint32_t next_report = 0;
    bool first = true;
    while (pos < size) {
        const size_t len = std::min<size_t>(Block, size - pos);
        if (!source.read(buffer, len)) {
            return Result::Fatal;
        }

        port().write(buffer, len);
        pos += len;
        sent.store(pos);

        const uint32_t percent = (static_cast<uint64_t>(pos) * 100) / size;
        if (percent >= next_report) {
            message("%u%% (%u / %u bytes, %u baud, %u s)", percent, pos, size, how.rate,
                static_cast<unsigned>((millis() - upload_start) / 1000));
            next_report = percent - (percent % 5) + 5;
        }

        const int ack = waitByte(first ? OffsetMs : AckMs);
        first = false;

        if (ack == 0x05) {
            continue;
        }

        if ((ack == 0x08) && how.v12) {
            uint8_t raw[4];
            for (auto& byte : raw) {
                const int value = waitByte(OffsetMs);
                if (value < 0) {
                    fail("truncated 0x08 reply at %u", pos);
                    return Result::Retry;
                }
                byte = value;
            }

            const uint32_t offset = raw[0]
                | (static_cast<uint32_t>(raw[1]) << 8)
                | (static_cast<uint32_t>(raw[2]) << 16)
                | (static_cast<uint32_t>(raw[3]) << 24);
            if (!offset || (offset == pos)) {
                continue;
            }
            if (offset > size) {
                fail("display asked for offset %u beyond the file (%u)", offset, size);
                return Result::Retry;
            }

            message("display continues from offset %u (skips %d bytes)", offset,
                static_cast<int>(offset) - static_cast<int>(pos));
            if ((offset < size) && !source.open(offset)) {
                return Result::Fatal;
            }
            pos = offset;
            sent.store(pos);
            continue;
        }

        if (ack < 0) {
            fail("no acknowledge for the block ending at %u", pos);
        } else {
            fail("unexpected reply 0x%02X for the block ending at %u", ack, pos);
        }
        return Result::Retry;
    }

    message("all %u bytes sent in %u s, the display installs the TFT and restarts", size,
        static_cast<unsigned>((millis() - upload_start) / 1000));
    return Result::Ok;
}

void run() {
    // nothing but this task may consume UART2 bytes (no event task callbacks)
    port().onReceive(nullptr);
    port().onReceiveError(nullptr);

    std::unique_ptr<uint8_t[]> buffer(new (std::nothrow) uint8_t[Block]);
    if (!buffer) {
        fail("out of memory");
        state.store(Failed);
        return;
    }

    const bool https = (0 == strncmp(url, "https://", 8));
    std::unique_ptr<WiFiClient> client;
    if (https) {
        auto* secure = new (std::nothrow) WiFiClientSecure;
        if (secure) {
            secure->setInsecure();
            secure->setHandshakeTimeout(15);
        }
        client.reset(secure);
    } else {
        client.reset(new (std::nothrow) WiFiClient);
    }

    if (!client) {
        fail("out of memory");
        state.store(Failed);
        return;
    }

    Variant variants[5];
    size_t count = 0;
    variants[count++] = Variant{"whmi-wris", requested, "res0", true};
    if (requested != FallbackBaud) {
        variants[count++] = Variant{"whmi-wris", FallbackBaud, "res0", true};
    }
    variants[count++] = Variant{"whmi-wri", FallbackBaud, "res0", false};
    variants[count++] = Variant{"whmi-wris", FallbackBaud, "1", true};
    variants[count++] = Variant{"whmi-wri", FallbackBaud, "1", false};

    Result result = Result::Fatal;
    {
        Source source(*client);
        for (size_t n = 0; n < count; ++n) {
            message("attempt %u/%u: %s, %s at %u baud", n + 1, count, url, variants[n].command, variants[n].rate);
            result = attempt(source, buffer.get(), variants[n]);
            source.http.end();
            if (result == Result::Ok) {
                break;
            }

            // after whmi-wri(s) the display waits for data until it is power-cycled
            if (phase.load() >= PhaseStart) {
                powerCycle();
            }

            if (result == Result::Fatal) {
                break;
            }
        }
    }

    setBaud(build::LovelaceBaud);
    phase.store(PhaseNone);
    state.store((result == Result::Ok) ? Done : Failed);
}

void task(void*) {
    run();
    vTaskDelete(nullptr);
}

// ---- loop side ----

bool start(const String& link, uint32_t rate, String& out) {
    if (busy()) {
        out = F("an upload is already running");
        return false;
    }

    if (!link.startsWith(F("http://")) && !link.startsWith(F("https://"))) {
        out = F("URL must start with http:// or https://");
        return false;
    }

    if (link.length() >= sizeof(url)) {
        out = F("URL too long");
        return false;
    }

    if (!wifiConnected()) {
        out = F("no WiFi");
        return false;
    }

    const bool https = link.startsWith(F("https://"));
    const auto heap = ESP.getFreeHeap();
    if (heap < (https ? HeapMinTls : HeapMin)) {
        out = String(F("low heap (")) + heap + F(" bytes)");
        if (https) {
            out += F(", try a plain http:// URL");
        }
        return false;
    }

    messages = xQueueCreate(MessagesMax, MessageMax);
    if (!messages) {
        out = F("queue create failed");
        return false;
    }

    strncpy(url, link.c_str(), sizeof(url) - 1);
    url[sizeof(url) - 1] = '\0';
    requested = rate;
    started_ms = millis();
    error[0] = '\0';
    comok[0] = '\0';
    variant[0] = '\0';
    flash_size = 0;
    total.store(0);
    sent.store(0);
    baud.store(0);
    phase.store(PhaseNone);

    state.store(Running);
    if (pdPASS != xTaskCreate(task, "nspflash", https ? StackTls : Stack, nullptr, 1, nullptr)) {
        state.store(Idle);
        vQueueDelete(messages);
        messages = nullptr;
        out = F("task create failed");
        return false;
    }

    DEBUG_MSG_P(PSTR("[NSPANEL] Flash: %s, %u baud\n"), url, rate);
    return true;
}

// The drivers take the UART back and the display is detected again: a new TFT may speak the
// other protocol. No raw recmod=1 here (it would break OHpanel's command mode); the Lovelace
// probe sends it at 115200 only, where an OHpanel TFT (921600) cannot parse it
void resume(bool done) {
    while (uart::port().read() >= 0) {
    }
    link::afterFlash(done);
}

void loop() {
    const auto current = state.load();

    char buf[MessageMax];
    while (messages && (pdTRUE == xQueueReceive(messages, buf, 0))) {
        DEBUG_MSG_P(PSTR("[NSPANEL] Flash: %s\n"), buf);
    }

    if (current == Running) {
        return;
    }

    const unsigned seconds = (millis() - started_ms) / 1000;
    if (current == Done) {
        snprintf(last_result, sizeof(last_result), "ok, %u bytes in %u s (%s)", total.load(), seconds, variant);
        DEBUG_MSG_P(PSTR("[NSPANEL] Flash: done (%s), detecting the display again\n"), last_result);
    } else {
        snprintf(last_result, sizeof(last_result), "FAILED after %u s: %s", seconds, error);
        DEBUG_MSG_P(PSTR("[NSPANEL] Flash: %s\n"), last_result);
    }

    if (messages) {
        vQueueDelete(messages);
        messages = nullptr;
    }

    state.store(Idle);
    resume(current == Done);
}

} // namespace flash

// -----------------------------------------------------------------------------
// Config load / reload
// -----------------------------------------------------------------------------

PROGMEM_STRING(DefaultConfig,
    "{\"pages\":[{\"title\":\"NSPanel\",\"items\":["
    "{\"name\":\"Relay 1\",\"icon\":\"light-switch\",\"relay\":0},"
    "{\"name\":\"Relay 2\",\"icon\":\"light-switch\",\"relay\":1}]}],"
    "\"buttons\":[{\"relay\":0},{\"relay\":1}]}");

std::vector<String> subscribed;

void unsubscribeAll() {
#if MQTT_SUPPORT
    if (mqttConnected()) {
        for (const auto& topic : subscribed) {
            mqttUnsubscribeRaw(topic.c_str());
        }
    }
#endif
    subscribed.clear();
}

// On ESP32 the MQTT connect event usually runs in the async_tcp task. Every subscribe() there
// is a blocking tcpip_api_call(), while each SUBACK / retained message / ACK the broker sends back
// is queued by the tcpip thread into the 32-slot async_tcp event queue with portMAX_DELAY.
// Two dozen subscriptions in one callback can fill that queue: the tcpip thread then waits for
// async_tcp, async_tcp waits for the tcpip thread, and the Task Watchdog resets the device
// (async_tcp is watched while it handles an event, loopTask waits for the loop lock it holds).
// So the subscriptions are only scheduled here and sent from loop(), see subscribeLoop().
size_t subscribe_next { 0 };
bool subscribe_pending { false };
bool announce_pending { false };
uint32_t subscribe_last { 0 };

void subscribeAll() {
    subscribed.clear();
    subscribe_next = 0;
    subscribe_pending = true;
}

void subscribeLoop() {
#if MQTT_SUPPORT
    if (!subscribe_pending && !announce_pending) {
        return;
    }

    // MQTT_CONNECT_EVENT schedules everything again
    if (!mqttConnected()) {
        return;
    }

    const auto now = millis();
    if (now - subscribe_last < build::SubscribeStepMs) {
        return;
    }
    subscribe_last = now;

    size_t budget = build::SubscribeBurst;
    while (subscribe_pending && budget) {
        if (subscribe_next >= config.entities.size()) {
            subscribe_pending = false;
            break;
        }

        const auto& entity = config.entities[subscribe_next];
        bool known = false; // several JSON entities share a topic: subscribe once
        for (const auto& topic : subscribed) {
            if (topic == entity.topic) {
                known = true;
                break;
            }
        }
        if (entity.topic.length() && !known) {
            // 0: no room in the TCP send buffer right now, retry on the next step
            if (!mqttSubscribeRaw(entity.topic.c_str())) {
                return;
            }
            subscribed.push_back(entity.topic);
            --budget;
        }
        ++subscribe_next;
    }

    if (!subscribe_pending && announce_pending) {
        announce_pending = false;
        if (panel.info.length()) {
            mqttSendRaw(mqttTopic("nspanel").c_str(), panel.info.c_str(), false);
        }
    }
#endif
}

String last_load_error;
String last_load_warnings;

// a parsed config replaces the running one
void applyConfig(Config&& next, const String& warnings, const char* source) {
    last_load_warnings = warnings;
    if (warnings.length()) {
        DEBUG_MSG_P(PSTR("[NSPANEL] Config warnings: %s\n"), warnings.c_str());
    }

    // keep known MQTT states across reloads (the old config is dropped right after)
    for (auto& entity : next.entities) {
        for (auto& old : config.entities) {
            if (entity.topic.length() && (entity.topic == old.topic) && (entity.path == old.path)
                && (entity.on == old.on) && (entity.off == old.off))
            {
                entity.state = old.state;
                entity.value = std::move(old.value);
                entity.number = old.number;
                break;
            }
        }
    }

    applyUiSettings(next);

    const bool weather_changed = (next.weather != config.weather)
        || (next.lat != config.lat) || (next.lon != config.lon);
    const bool display_changed = (next.display != config.display);

    unsubscribeAll();
    config = std::move(next);
    lang::current = config.language;
    next.entities.clear(); // whatever the move left there, freed before the drivers rebuild
    subscribeAll();

    DEBUG_MSG_P(PSTR("[NSPANEL] Config %s: %u pages, %u items, %u entities, %u groups, heap %u / %u\n"),
        source, config.pages.size(), config.items.size(), config.entities.size(), config.groups.size(),
        ESP.getFreeHeap(), ESP->getMaxAllocHeap());

    if (weather_changed) {
        weather::current = weather::Data();
        weather::request();
    }

    if (link::active == Driver::None) {
        return; // nspanelSetup() starts the detection
    }

    if (display_changed && !flash::busy()) {
        link::probe("display setting changed");
    } else {
        driver::reload();
    }
}

// built-in config, parsed from a heap copy (in place, like the file)
ParseResult parseDefault(Config& out, String& error, String& warnings) {
    const size_t length = strlen_P(DefaultConfig);
    std::unique_ptr<char[]> text(new (std::nothrow) char[length + 1]);
    if (!text) {
        error = F("out of memory");
        return ParseResult::LowMemory;
    }

    memcpy_P(text.get(), DefaultConfig, length + 1);
    return parseConfig(text.get(), length, out, error, warnings);
}

// Boot and nspanel.reload: /nspanel.json, or the built-in config when there is none. A file that
// cannot be used keeps the running config; only an empty one (boot) falls back to the default.
// Peak: file text + JSON nodes + the new Config (+ the running one on reload), ref. heapAvailable()
void load() {
    String error;
    String warnings;
    const char* source = "(default)";
    bool ok = false;

    try {
        Config next;
        if (storage::mounted && LittleFS.exists(build::ConfigPath)) {
            source = build::ConfigPath;
            std::unique_ptr<char[]> text;
            size_t size = 0;
            if (!storage::read(text, size)) {
                error = String(F("cannot read ")) + build::ConfigPath + F(" (empty, too large or no memory)");
            } else {
                ok = (ParseResult::Ok == parseConfig(text.get(), size, next, error, warnings));
            }
        } else {
            ok = (ParseResult::Ok == parseDefault(next, error, warnings));
        }

        if (ok) {
            last_load_error = String();
        } else {
            last_load_error = error;
            DEBUG_MSG_P(PSTR("[NSPANEL] Config error: %s\n"), error.c_str());
            if (!config.pages.empty()) {
                return;
            }

            next = Config();
            warnings = String();
            String ignored;
            if (ParseResult::Ok != parseDefault(next, ignored, warnings)) {
                DEBUG_MSG_P(PSTR("[NSPANEL] Default config failed: %s\n"), ignored.c_str());
                return;
            }
            source = "(default)";
        }

        applyConfig(std::move(next), warnings, source);
    } catch (const std::bad_alloc&) {
        last_load_error = F("out of memory while applying the config");
        DEBUG_MSG_P(PSTR("[NSPANEL] Config: %s\n"), last_load_error.c_str());
    }
}

namespace upload {

// loop(): validate, save and apply a received config, the answer is picked up by onUploadPoll()
void process() {
    if (job.state != State::Queued) {
        return;
    }

    // its TLS session takes 40-60 KB, the parse goes after it (the crash of 2026-10-25)
    if (weather::running() && ((millis() - job.queued_ms) < build::UploadWeatherWaitMs)) {
        return;
    }

    int code = 500;
    String message;
    String error;
    String warnings;

    try {
        const size_t json_size = jsonBufferSize(job.buffer.get(), job.size);
        if (!heapAvailable(json_size, job.size, error)) {
            code = 503;
            message = String(F("ERROR: ")) + error;
        } else if (!storage::writeTmp(job.buffer.get(), job.size)) {
            message = F("ERROR: write failed");
        } else {
            Config next;
            const auto result = parseInPlace(job.buffer.get(), json_size, next, error, warnings);
            job.buffer.reset(); // the parse destroyed it

            if (result != ParseResult::Ok) {
                storage::removeTmp();
                code = (result == ParseResult::LowMemory) ? 503 : 400;
                message = String(F("ERROR: ")) + error;
            } else if (!storage::commitTmp()) {
                message = F("ERROR: write failed");
            } else {
                code = 200;
                message = F("OK");
                if (warnings.length()) {
                    message += F(", warnings: ");
                    message += warnings;
                }
                last_load_error = String();
                applyConfig(std::move(next), warnings, build::ConfigPath);
            }
        }
    } catch (const std::bad_alloc&) {
        // the file is only renamed after a successful parse, applyConfig() runs after that
        if (code == 200) {
            message = F("OK, saved; the display reload ran out of memory (try nspanel.reload)");
        } else {
            storage::removeTmp();
            code = 503;
            message = F("ERROR: out of memory, retry");
        }
    }

    DEBUG_MSG_P(PSTR("[NSPANEL] Config upload: HTTP %d %s\n"), code, message.c_str());

    job.buffer.reset();
    job.size = 0;
    job.code = code;
    job.message = std::move(message);
    job.state = State::Done;
}

} // namespace upload

// -----------------------------------------------------------------------------
// Callbacks
// -----------------------------------------------------------------------------

#if MQTT_SUPPORT
void mqttCallback(unsigned int type, StringView topic, StringView payload) {
    switch (type) {
    case MQTT_CONNECT_EVENT:
        // nothing is sent from here, ref. subscribeAll()
        subscribeAll();
        announce_pending = true;
        history::subscribed = false;
        break;

    case MQTT_DISCONNECT_EVENT:
        subscribed.clear();
        subscribe_pending = false;
        announce_pending = false;
        history::subscribed = false;
        break;

    case MQTT_MESSAGE_EVENT: {
        history::message(topic, payload);

        bool parse = false;
        for (size_t index = 0; index < config.entities.size(); ++index) {
            auto& entity = config.entities[index];
            if (!entity.topic.length() || !topic.equals(StringView(entity.topic))) {
                continue;
            }

            // JSON payloads are parsed in loop(), nothing heavy here
            if (entity.path.length()) {
                parse = true;
                continue;
            }

            const String& on = entity.on.length() ? entity.on : config.on;
            const String& off = entity.off.length() ? entity.off : config.off;
            const int state = payload.equals(StringView(on)) ? 1
                : payload.equals(StringView(off)) ? 0
                : -1;

            String value = payload.toString();
            utf8Truncate(value, 32);

            char* end = nullptr;
            const float number = strtof(value.c_str(), &end);
            entity.number = (value.length() && end && !*end) ? number : NAN;

            if ((state != entity.state) || (value != entity.value)) {
                entity.state = state;
                entity.value = std::move(value);
                entityChanged(index);
            }
        }

        if (parse) {
            json::queue(topic, payload);
        }
        break;
    }
    }
}
#endif

#if RELAY_SUPPORT
void relayCallback(size_t id, bool) {
    for (size_t index = 0; index < config.entities.size(); ++index) {
        if (config.entities[index].relay == static_cast<int>(id)) {
            entityChanged(index);
        }
    }

    if (link::active == Driver::Ohpanel) {
        ohpanel::relayChanged(id);
    }
}
#endif

#if BUTTON_SUPPORT
void buttonCallback(size_t id, ButtonEvent) {
    if (id >= config.buttons.size()) {
        return;
    }

    const auto& target = config.buttons[id];
    if (!target.valid) {
        return;
    }

    if (target.group >= 0) {
        groupSet(target.group, target.action);
    } else if (target.entity >= 0) {
        entitySet(target.entity, target.action);
    }

    if ((target.page >= 0) && !flash::busy()) {
        driver::showPage(target.page);
    }
}
#endif

// -----------------------------------------------------------------------------
// Web
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void requestAuth(AsyncWebServerRequest* request) {
    request->requestAuthentication(systemHostname().c_str(), true);
}

void onEditor(AsyncWebServerRequest* request) {
    espurna::system::AsyncGuard guard;
    if (!guard.locked()) {
        webSendBusy(request);
        return;
    }

    if (!webAuthenticate(request)) {
        requestAuth(request);
        return;
    }

    request->send_P(200, "text/html; charset=utf-8", web::EditorPage);
}

void onGetConfig(AsyncWebServerRequest* request) {
    espurna::system::AsyncGuard guard;
    if (!guard.locked()) {
        webSendBusy(request);
        return;
    }

    if (!webAuthenticate(request)) {
        requestAuth(request);
        return;
    }

    String text;
    if (!storage::read(text)) {
        text = DefaultConfig;
    }

    request->send(200, "application/json; charset=utf-8", text);
}

// POST /nspanel.json, ref. namespace upload. The body is collected here (async_tcp only, no
// lock needed for 'rx' / 'waiting'), loop() parses it, the answer is sent from the poll callback.
// Contract of firmware_nspanel_json.py and the editor: 200 "OK[, warnings: ...]" when saved and
// applied, 400 "ERROR: ..." for an invalid config, 413 too large, 503 busy / low memory (retry).
struct Receive {
    AsyncWebServerRequest* request { nullptr };
    std::unique_ptr<char[]> buffer;
    size_t size { 0 };
    int code { 0 };
    String message;
    uint32_t id { 0 }; // job id once handed over
};

Receive rx;
uint32_t last_id { 0 };

// request waiting for the result of job 'waiting_id'
AsyncWebServerRequest* waiting { nullptr };
uint32_t waiting_id { 0 };
uint32_t waiting_ms { 0 };

void rxReset() {
    rx.request = nullptr;
    rx.buffer.reset();
    rx.size = 0;
    rx.code = 0;
    rx.message = String();
    rx.id = 0;
    upload::receiving.store(false);
}

void rxFail(int code, const __FlashStringHelper* message) {
    rx.code = code;
    rx.message = message ? String(message) : String();
    rx.buffer.reset();
    upload::receiving.store(false);
}

// the request is deleted right after this (async_tcp)
void uploadForget(AsyncWebServerRequest* request) {
    if (rx.request == request) {
        rxReset();
    }
    if (waiting == request) {
        waiting = nullptr;
    }
}

void sendText(AsyncWebServerRequest* request, int code, const String& message) {
    auto* response = request->beginResponse(code, F("text/plain; charset=utf-8"), message);
    if (code == 503) {
        response->addHeader(F("Retry-After"), F("2"));
    }
    request->send(response);
}

// async_tcp, every ~500 ms per client (CONFIG_ASYNC_TCP_POLL_TIMER) while it waits
void onUploadPoll(AsyncWebServerRequest* request) {
    if (!request || (request != waiting)) {
        return;
    }

    espurna::system::AsyncGuard guard;
    if (!guard.locked()) {
        return; // loop() is busy (maybe with this job), next poll
    }

    int code = 0;
    String message;
    auto& job = upload::job;
    if (job.id != waiting_id) {
        code = 500;
        message = F("ERROR: upload lost");
    } else if (job.state == upload::State::Done) {
        code = job.code;
        message = std::move(job.message);
        job.message = String();
        job.state = upload::State::Idle;
    } else if ((millis() - waiting_ms) >= build::UploadAnswerMs) {
        // dropped under the lock: the answer matches what happens to the config
        job.buffer.reset();
        job.size = 0;
        job.state = upload::State::Idle;
        code = 503;
        message = F("ERROR: busy, retry");
    } else {
        return;
    }

    waiting = nullptr;
    sendText(request, code, message);
}

void onPostBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        // one upload at a time, a second one gets 503 from onPostConfig()
        if ((rx.request && (rx.request != request)) || waiting) {
            return;
        }
        rxReset();
        rx.request = request;
        request->onDisconnect([request]() {
            uploadForget(request);
        });
    }

    if ((rx.request != request) || rx.code) {
        return;
    }

    espurna::system::AsyncGuard guard;
    if (!guard.locked()) {
        rxFail(503, nullptr);
        return;
    }

    if (!webAuthenticate(request)) {
        rxFail(401, nullptr);
        return;
    }

    if (!total || (total > build::ConfigMax)) {
        rxFail(413, F("ERROR: config too large (max 16384 bytes)"));
        return;
    }

    if (index == 0) {
        if (upload::job.state == upload::State::Queued) {
            rxFail(503, F("ERROR: another upload is being processed, retry"));
            return;
        }

        // the body is the only copy of the text (parsed in place later)
        if ((ESP->getMaxAllocHeap() < (total + 1 + build::ConfigBlockReserve))
            || (ESP.getFreeHeap() < (total + build::ConfigHeapReserve)))
        {
            rxFail(503, F("ERROR: low memory, retry"));
            return;
        }

        rx.buffer.reset(new (std::nothrow) char[total + 1]);
        if (!rx.buffer) {
            rxFail(503, F("ERROR: low memory, retry"));
            return;
        }
        rx.size = total;
        upload::receiving.store(true);
    }

    if (!rx.buffer || ((index + len) > rx.size)) {
        rxFail(400, F("ERROR: unexpected body"));
        return;
    }

    memcpy(rx.buffer.get() + index, data, len);
    if ((index + len) < total) {
        return;
    }

    rx.buffer[total] = '\0';

    auto& job = upload::job;
    if (job.state == upload::State::Queued) {
        rxFail(503, F("ERROR: another upload is being processed, retry"));
        return;
    }

    // hand over to loop(), a stale unanswered result is dropped
    job.buffer = std::move(rx.buffer);
    job.size = total;
    job.id = ++last_id;
    job.queued_ms = millis();
    job.code = 0;
    job.message = String();
    job.state = upload::State::Queued;

    rx.id = job.id;
    upload::receiving.store(false);
}

void onPostConfig(AsyncWebServerRequest* request) {
    if (rx.request != request) {
        espurna::system::AsyncGuard guard;
        if (!guard.locked()) {
            webSendBusy(request);
            return;
        }
        if (!webAuthenticate(request)) {
            requestAuth(request);
            return;
        }
        if (rx.request || waiting || (upload::job.state == upload::State::Queued)) {
            sendText(request, 503, F("ERROR: another upload is in progress, retry"));
            return;
        }
        request->send(400, "text/plain", F("ERROR: empty body"));
        return;
    }

    const auto code = rx.code;
    const auto message = rx.message;
    const auto id = rx.id;
    rx.request = nullptr; // the disconnect callback stays, for 'waiting'
    rx.code = 0;
    rx.message = String();
    rx.id = 0;

    if (code == 401) {
        requestAuth(request);
        return;
    }

    if ((code == 503) && !message.length()) {
        webSendBusy(request);
        return;
    }

    if (code) {
        sendText(request, code, message);
        return;
    }

    if (!id) {
        sendText(request, 400, F("ERROR: incomplete body"));
        return;
    }

    // no answer before loop() has saved and applied it; the poll callback replaces the
    // request's own one, which only matters once a response exists (sent from there)
    waiting = request;
    waiting_id = id;
    waiting_ms = millis();
    request->client()->onPoll([](void* arg, AsyncClient*) {
        onUploadPoll(static_cast<AsyncWebServerRequest*>(arg));
    }, request);
}

// active display driver, for the editor's preview
void onStatus(AsyncWebServerRequest* request) {
    espurna::system::AsyncGuard guard;
    if (!guard.locked()) {
        webSendBusy(request);
        return;
    }

    if (!webAuthenticate(request)) {
        requestAuth(request);
        return;
    }

    DynamicJsonBuffer buffer(3072);
    JsonObject& root = buffer.createObject();
    root["display"] = driverPrefName(config.display);
    root["driver"] = driverName(link::active);
    root["connected"] = link::connected();
    root["probing"] = link::probing;
    root["version"] = driver::version();
    root["model"] = driver::model();
    const auto design = driver::design();
    if (design.length()) {
        root["design"] = design;
    }
    root["flashing"] = flash::busy();

    // the icon names of the built-in table, offered by the editor's icon field
    JsonArray& names = root.createNestedArray("icons");
    for (const auto& icon : icons::Table) {
        names.add(icon.name);
    }

    String out;
    root.printTo(out);
    request->send(200, "application/json", out);
}

// the WebUI screen mirror (OHpanel only): "nspanelScreen" starts it, touches come back as the TFT's own events
void onWsAction(uint32_t client_id, const char* action, JsonObject& data) {
    if (!strcmp_P(action, PSTR("nspanelScreen"))) {
        if ((link::active == Driver::Ohpanel) && !flash::busy()) {
            ohpanel::mirrorStart();
        } else {
            wsPost(client_id, [](JsonObject& root) {
                root["nspanelTft"] = F("#unsupported\n");
            });
        }
        return;
    }

    if ((link::active != Driver::Ohpanel) || flash::busy()) {
        return;
    }

    if (!strcmp_P(action, PSTR("nspanelTouch"))) {
        const char* page = data["page"] | "";
        const char* component = data["component"] | "";
        const char* value = data["value"] | "";
        if (*page && *component && *value) {
            ohpanel::onEvent(page, component, value);
        }
    } else if (!strcmp_P(action, PSTR("nspanelXY"))) {
        if (data.is<int>("x") && data.is<int>("y")) {
            ohpanel::onTouchXY(data["x"].as<int>(), data["y"].as<int>(), (data["press"] | 0) != 0);
        }
    }
}

// shows the NSPANEL section of the WebUI
void onWsVisible(JsonObject& root) {
    wsPayloadModule(root, STRING_VIEW("nspanel"));
}

void setupWeb() {
    wsRegister()
        .onVisible(onWsVisible)
        .onAction(onWsAction);

    auto& server = webServer();
    server.on("/nspanel-status", HTTP_GET, onStatus);
    server.on("/nspanel", HTTP_GET, onEditor);
    server.on("/nspanel.json", HTTP_GET, onGetConfig);
    server.on("/nspanel.json", HTTP_POST | HTTP_PUT, onPostConfig, nullptr, onPostBody);
}

#endif // WEB_SUPPORT

// -----------------------------------------------------------------------------
// Terminal
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT

void terminalReload(espurna::terminal::CommandContext&& ctx) {
    load();
    if (last_load_error.length()) {
        terminalError(ctx, last_load_error);
        return;
    }
    terminalOK(ctx);
}

void terminalSend(espurna::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() < 2) {
        terminalError(ctx, F("nspanel.send <payload> (OHpanel: a Nextion command, FF FF FF is added; Lovelace: a '~' command)"));
        return;
    }

    if (flash::busy()) {
        terminalError(ctx, F("TFT upload running"));
        return;
    }

    String payload;
    for (size_t n = 1; n < ctx.argv.size(); ++n) {
        if (n > 1) {
            payload += ' ';
        }
        payload += ctx.argv[n];
    }

    if (!link::connected()) {
        terminalError(ctx, F("no display driver connected"));
        return;
    }

    driver::send(std::move(payload));
    terminalOK(ctx);
}

void terminalDetect(espurna::terminal::CommandContext&& ctx) {
    if (flash::busy()) {
        terminalError(ctx, F("TFT upload running"));
        return;
    }

    link::probe("terminal");
    terminalOK(ctx);
}

void terminalPage(espurna::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 2) {
        terminalError(ctx, F("nspanel.page <page index | home | weather | settings>"));
        return;
    }

    if (flash::busy() || !link::connected()) {
        terminalError(ctx, F("display not ready"));
        return;
    }

    const auto& arg = ctx.argv[1];
    int target = PageHome;
    if (arg == F("weather")) {
        target = PageWeather;
    } else if (arg == F("settings")) {
        target = PageSettings;
    } else if (arg != F("home")) {
        target = arg.toInt();
        if ((target < 0) || (static_cast<size_t>(target) >= config.pages.size())) {
            terminalError(ctx, F("no such page"));
            return;
        }
    }

    driver::showPage(target);
    terminalOK(ctx);
}

void terminalInfo(espurna::terminal::CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("Display: driver %s (setting \"%s\"), %s, version '%s', model '%s'\n"),
        driverName(link::active), driverPrefName(config.display),
        flash::busy() ? "TFT upload running"
            : link::connected() ? "connected"
            : link::probing ? "detecting" : "not connected",
        driver::version(), driver::model());
    ctx.output.printf_P(PSTR("Detect: last '%s', %u probes, %u s ago, last found %s\n"),
        link::reason.c_str(), link::probes,
        static_cast<unsigned>((millis() - link::episode_start) / 1000), driverName(link::detected));
    switch (link::active) {
    case Driver::Lovelace:
        lovelace::info(ctx.output);
        break;
    case Driver::Ohpanel:
        ohpanel::info(ctx.output);
        break;
    case Driver::None:
        break;
    }
    ctx.output.printf_P(PSTR("Config: %u pages, %u items, %u entities, %u groups, %u subscribed\n"),
        config.pages.size(), config.items.size(), config.entities.size(), config.groups.size(), subscribed.size());
    ctx.output.printf_P(PSTR("UI: icons %s, theme %s, %u thermostats, %u info lines; JSON payloads parsed %u, failed %u, dropped %u, pending %u\n"),
        (config.layout == Layout::Corner) ? "corner" : "center", (config.theme == Theme::Light) ? "light" : "dark",
        config.thermostats.size(), config.info_count, json::parsed, json::failed, json::dropped, json::pending.size());
    if (last_load_error.length()) {
        ctx.output.printf_P(PSTR("Config error: %s\n"), last_load_error.c_str());
    }
    if (last_load_warnings.length()) {
        ctx.output.printf_P(PSTR("Config warnings: %s\n"), last_load_warnings.c_str());
    }
    if (storage::mounted) {
        ctx.output.printf_P(PSTR("FS: LittleFS on '%s', %u / %u bytes used\n"),
            storage::label.c_str(), LittleFS.usedBytes(), LittleFS.totalBytes());
    } else {
        ctx.output.print(F("FS: not mounted\n"));
    }
    ctx.output.printf_P(PSTR("Weather: %s, last: %s, %.1fC code %d, %u days\n"),
        config.weather ? "enabled" : "disabled", weather::last_error,
        weather::current.temperature, weather::current.code, weather::current.count);
    ctx.output.printf_P(PSTR("Room temperature: %.1f (%s)\n"), ntc::value,
        ntc::reported ? "sensor report" : std::isnan(ntc::value) ? "no reading yet" : "sensor reading");

    if (flash::state.load() == flash::Running) {
        const auto size = flash::total.load();
        const auto done = flash::sent.load();
        ctx.output.printf_P(PSTR("Flash: %s, %u%% (%u / %u bytes), %u baud, %u s\n"),
            flash::phaseName(flash::phase.load()),
            size ? static_cast<unsigned>((static_cast<uint64_t>(done) * 100) / size) : 0,
            done, size, flash::baud.load(),
            static_cast<unsigned>((millis() - flash::started_ms) / 1000));
    } else {
        ctx.output.printf_P(PSTR("Flash: idle, last: %s\n"), flash::last_result);
        if (flash::comok[0]) {
            ctx.output.printf_P(PSTR("Flash: display %s\n"), flash::comok);
        }
        if (flash::variant[0]) {
            ctx.output.printf_P(PSTR("Flash: last command %s\n"), flash::variant);
        }
    }

    ctx.output.print(F("Last events:\n"));
    for (const auto& event : panel.events) {
        ctx.output.printf_P(PSTR("  %s\n"), event.c_str());
    }

    terminalOK(ctx);
}

// nspanel.flash <http(s)://.../file.tft> [baud]
void terminalFlash(espurna::terminal::CommandContext&& ctx) {
    if ((ctx.argv.size() < 2) || (ctx.argv.size() > 3)) {
        terminalError(ctx, F("nspanel.flash <http(s)://.../nspanel.tft> [baud, default 921600]"));
        return;
    }

    uint32_t rate = 921600;
    if (ctx.argv.size() == 3) {
        static constexpr uint32_t Rates[] { 115200, 230400, 250000, 256000, 512000, 921600 };
        rate = strtoul(ctx.argv[2].c_str(), nullptr, 10);
        bool known = false;
        for (const auto value : Rates) {
            known = known || (value == rate);
        }
        if (!known) {
            terminalError(ctx, F("baud: 115200, 230400, 250000, 256000, 512000 or 921600"));
            return;
        }
    }

    String error;
    if (!flash::start(ctx.argv[1], rate, error)) {
        terminalError(ctx, error);
        return;
    }

    ctx.output.print(F("TFT upload started, progress in the log and in nspanel.info. Do not power off the panel.\n"));
    terminalOK(ctx);
}

// nspanel.capture start | print | stop: records the commands sent to the OHpanel TFT (start also sends the
// theme again, so a capture replays on its own in the Nextion Editor simulator)
void terminalCapture(espurna::terminal::CommandContext&& ctx) {
    const String arg = (ctx.argv.size() == 2) ? ctx.argv[1] : String();
    if (link::active != Driver::Ohpanel) {
        terminalError(ctx, F("OHpanel driver only"));
        return;
    }
    if (arg == F("start")) {
        ohpanel::capture::lines.clear();
        ohpanel::capture::on = true;
        ohpanel::sendTheme();
    } else if (arg == F("stop")) {
        ohpanel::capture::on = false;
        ohpanel::capture::lines.clear();
    } else if (arg == F("print")) {
        for (const auto& line : ohpanel::capture::lines) {
            ctx.output.print(line);
            ctx.output.print('\n');
        }
        ctx.output.printf_P(PSTR("# %u commands\n"), ohpanel::capture::lines.size());
    } else {
        terminalError(ctx, F("nspanel.capture start | print | stop"));
        return;
    }
    terminalOK(ctx);
}

// nspanel.event <page> <component> <value>: as if the OHpanel TFT sent it (a tap without a finger)
void terminalEvent(espurna::terminal::CommandContext&& ctx) {
    if ((ctx.argv.size() != 4) || (link::active != Driver::Ohpanel)) {
        terminalError(ctx, F("nspanel.event <page> <component> <value> (OHpanel driver only)"));
        return;
    }
    ohpanel::onEvent(ctx.argv[1].c_str(), ctx.argv[2].c_str(), ctx.argv[3].c_str());
    terminalOK(ctx);
}

// nspanel.url <http(s)://...>: the GET a "url" tile sends, to try an address from the panel
void terminalUrl(espurna::terminal::CommandContext&& ctx) {
    if ((ctx.argv.size() != 2)
        || !(ctx.argv[1].startsWith(F("http://")) || ctx.argv[1].startsWith(F("https://"))))
    {
        terminalError(ctx, F("nspanel.url <http(s)://...>, the result follows in the log"));
        return;
    }
    if (!http::get(ctx.argv[1])) {
        terminalError(ctx, F("not started (no WiFi, busy or low heap)"));
        return;
    }
    terminalOK(ctx);
}

PROGMEM_STRING(PanelCapture, "NSPANEL.CAPTURE");
PROGMEM_STRING(PanelEvent, "NSPANEL.EVENT");
PROGMEM_STRING(PanelReload, "NSPANEL.RELOAD");
PROGMEM_STRING(PanelSend, "NSPANEL.SEND");
PROGMEM_STRING(PanelInfo, "NSPANEL.INFO");
PROGMEM_STRING(PanelFlash, "NSPANEL.FLASH");
PROGMEM_STRING(PanelDetect, "NSPANEL.DETECT");
PROGMEM_STRING(PanelPage, "NSPANEL.PAGE");
PROGMEM_STRING(PanelUrl, "NSPANEL.URL");

static constexpr espurna::terminal::Command PanelCommands[] PROGMEM {
    {PanelReload, terminalReload},
    {PanelSend, terminalSend},
    {PanelInfo, terminalInfo},
    {PanelFlash, terminalFlash},
    {PanelDetect, terminalDetect},
    {PanelPage, terminalPage},
    {PanelCapture, terminalCapture},
    {PanelEvent, terminalEvent},
    {PanelUrl, terminalUrl},
};

#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------

void panelLoop() {
    upload::process();
    subscribeLoop();
    history::loop();
    json::loop();

    // the upload task owns UART2; frames queued meanwhile are dropped on resume
    if (flash::busy()) {
        flash::loop();
        buzzer::loop();
        return;
    }

    link::loop();

    weather::loop();
    http::loop();
    buzzer::loop();
}

} // namespace
} // namespace nspanel
} // namespace espurna

void nspanelBootSetup() {
    using namespace espurna::nspanel;

    pinMode(build::DisplayPowerPin, OUTPUT);
    digitalWrite(build::DisplayPowerPin, LOW);
    gpioLock(build::DisplayPowerPin);

    pinMode(build::BuzzerPin, OUTPUT);
    digitalWrite(build::BuzzerPin, LOW);
    gpioLock(build::BuzzerPin);
}

void nspanelSetup() {
    using namespace espurna::nspanel;

    gpioLock(build::TxPin);
    gpioLock(build::RxPin);
#if NTC_SUPPORT
    gpioLock(NTC_PIN);
#endif

    ntc::setup();
    buzzer::setup();
    uart::begin();

    storage::mount();
    load();

#if MQTT_SUPPORT
    mqttRegister(mqttCallback);
#endif
#if RELAY_SUPPORT
    relayOnStatusChange(relayCallback);
#endif
#if BUTTON_SUPPORT
    buttonSetCustomAction(buttonCallback);
#endif
#if WEB_SUPPORT
    setupWeb();
#endif
#if TERMINAL_SUPPORT
    espurna::terminal::add(PanelCommands);
#endif

    link::probe("boot");
    espurnaRegisterLoop(panelLoop);
}

#endif // NSPANEL_SUPPORT
