/*

NSPANEL MODULE - Lovelace UI TFT driver

joBr99 "NSPanel Lovelace UI" TFT (v4.1.1 = protocol 51, v4.7.3 = 53): 55 BB frames with CRC16/MODBUS
at 115200, '~' separated commands, ',' separated events. Protocol: nspanel/lovelace-protocol.md

Included by nspanel.cpp inside namespace espurna::nspanel::{anonymous}, after the shared core
(config, entities, weather, ntc, buzzer, cards). Not a standalone header.

*/

#pragma once

namespace lovelace {

constexpr uint32_t Baudrate { build::LovelaceBaud };
constexpr size_t FrameMax { 1024 };
constexpr size_t QueueMax { 48 };
constexpr uint32_t TxGapMs { 75 };
constexpr uint32_t StartupRetryMs { 10000 };
constexpr uint32_t RenderDelayMs { 100 };
constexpr uint32_t LostMs { 30000 };       // startup requests unanswered this long: detect again
constexpr uint32_t GarbageMax { 32 };      // bytes outside 55 BB frames without a valid frame: detect again

// TFT text limits, bytes (Max Text Size of the 51 HMI)
constexpr size_t HeadingMax { build::HeadingMax };
constexpr size_t NameMax { build::NameMax };
constexpr size_t ForecastMax { build::ForecastMax };

// -----------------------------------------------------------------------------
// Display state & serial transport
// -----------------------------------------------------------------------------

enum class Screen {
    Unknown,
    Startup,
    Screensaver,
    Card,
};

const char* screenName(Screen screen) {
    switch (screen) {
    case Screen::Unknown:
        return "unknown";
    case Screen::Startup:
        return "startup";
    case Screen::Screensaver:
        return "screensaver";
    case Screen::Card:
        return "card";
    }

    return "";
}

struct Display {
    bool connected { false };
    int version { 0 };
    String model;
    Screen screen { Screen::Unknown };
    size_t card { 0 };
    std::vector<Card> cards;
    std::vector<int> history; // nested pages: where each was opened from (-1 screensaver, else a card)

    bool dirty { false };
    uint32_t dirty_since { 0 };

    uint32_t startup_request { 0 };
    int last_minute { -1 };
    int last_mday { -1 };
    uint8_t status_mask { 0xFF };
    String temperature_text;

    std::deque<String> queue;
    uint32_t last_tx { 0 };

    bool was_connected { false };
    bool lost { false };
    uint32_t disconnected_since { 0 };
    uint32_t garbage { 0 };

    uint32_t rx_frames { 0 };
    uint32_t tx_frames { 0 };
    uint32_t crc_errors { 0 };
    uint32_t dropped { 0 };
};

Display display;

namespace serial {

HardwareSerial& port() {
    return uart::port();
}

struct Parser {
    enum class State {
        Header1,
        Header2,
        Length1,
        Length2,
        Payload,
        Crc1,
        Crc2,
    };

    State state { State::Header1 };
    uint16_t length { 0 };
    uint16_t crc { 0xFFFF };
    uint16_t received_crc { 0 };
    uint32_t raw { 0 };
    String payload;

    void update(uint8_t byte) {
        crc = crc16(&byte, 1, crc);
    }

    // true when 'payload' holds a complete, CRC-checked frame
    bool feed(uint8_t byte, bool& raw_startup) {
        switch (state) {
        case State::Header1:
            // TFT power-up announcement 00 00 00 FF FF FF 88 FF FF FF
            raw = (raw << 8) | byte;
            if (raw == 0x88FFFFFFul) {
                raw_startup = true;
                raw = 0;
            }
            if (byte == 0x55) {
                crc = 0xFFFF;
                update(byte);
                state = State::Header2;
            } else {
                ++display.garbage;
            }
            break;

        case State::Header2:
            if (byte == 0xBB) {
                update(byte);
                state = State::Length1;
            } else if (byte == 0x55) {
                crc = 0xFFFF;
                update(byte);
            } else {
                state = State::Header1;
            }
            break;

        case State::Length1:
            length = byte;
            update(byte);
            state = State::Length2;
            break;

        case State::Length2:
            length |= static_cast<uint16_t>(byte) << 8;
            update(byte);
            if (!length || (length > FrameMax)) {
                state = State::Header1;
                ++display.crc_errors;
                break;
            }
            payload = String();
            payload.reserve(length);
            state = State::Payload;
            break;

        case State::Payload:
            payload += static_cast<char>(byte);
            update(byte);
            if (payload.length() >= length) {
                state = State::Crc1;
            }
            break;

        case State::Crc1:
            received_crc = byte;
            state = State::Crc2;
            break;

        case State::Crc2:
            received_crc |= static_cast<uint16_t>(byte) << 8;
            state = State::Header1;
            if (received_crc == crc) {
                return true;
            }
            ++display.crc_errors;
            if (config.debug) {
                DEBUG_MSG_P(PSTR("[NSPANEL] CRC mismatch (%04X != %04X)\n"), received_crc, crc);
            }
            break;
        }

        return false;
    }
};

Parser parser;

void writeFrame(const String& payload) {
    const size_t len = payload.length();
    const uint8_t header[4] {
        0x55, 0xBB,
        static_cast<uint8_t>(len & 0xFF),
        static_cast<uint8_t>((len >> 8) & 0xFF),
    };

    uint16_t crc = crc16(header, sizeof(header));
    crc = crc16(reinterpret_cast<const uint8_t*>(payload.c_str()), len, crc);

    const uint8_t footer[2] {
        static_cast<uint8_t>(crc & 0xFF),
        static_cast<uint8_t>((crc >> 8) & 0xFF),
    };

    auto& serial = port();
    serial.write(header, sizeof(header));
    serial.write(reinterpret_cast<const uint8_t*>(payload.c_str()), len);
    serial.write(footer, sizeof(footer));

    ++display.tx_frames;
    if (config.debug) {
        DEBUG_MSG_P(PSTR("[NSPANEL] -> %s\n"), payload.c_str());
    }
}

void writeRaw(const char* command) {
    uart::writeRaw(command);
}

String commandName(const String& payload) {
    const auto pos = payload.indexOf('~');
    return (pos < 0) ? payload : payload.substring(0, pos);
}

// 'coalesce' replaces the latest queued frame of the same command, unless there is a
// page change queued after it (frames after pageType~ belong to the new page)
void enqueue(String payload, bool coalesce = false) {
    if (payload.length() > FrameMax) {
        DEBUG_MSG_P(PSTR("[NSPANEL] Frame too long (%u bytes), dropped\n"), payload.length());
        return;
    }

    if (coalesce) {
        const auto name = commandName(payload);
        for (auto it = display.queue.rbegin(); it != display.queue.rend(); ++it) {
            if ((*it).startsWith(F("pageType~"))) {
                break;
            }
            if (commandName(*it) == name) {
                *it = std::move(payload);
                return;
            }
        }
    }

    if (display.queue.size() >= QueueMax) {
        display.queue.pop_front();
        ++display.dropped;
    }

    display.queue.push_back(std::move(payload));
}

void txLoop() {
    if (display.queue.empty()) {
        return;
    }

    const auto now = millis();
    if (now - display.last_tx < TxGapMs) {
        return;
    }

    if (port().availableForWrite() < static_cast<int>(display.queue.front().length() + 6)) {
        return;
    }

    writeFrame(display.queue.front());
    display.queue.pop_front();
    display.last_tx = now;
}

} // namespace serial

void entityChanged(int entity) {
    if ((display.screen == Screen::Card) && cardContains(display.cards, display.card, entity)) {
        if (!display.dirty) {
            display.dirty = true;
            display.dirty_since = millis();
        }
    }
}

// -----------------------------------------------------------------------------
// Rendering
// -----------------------------------------------------------------------------

String forecastValue(const weather::Day& day) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%d\xC2\xB0/%d\xC2\xB0",
        static_cast<int>(lroundf(day.tmax)), static_cast<int>(lroundf(day.tmin)));
    if (strlen(buf) <= ForecastMax) {
        return buf;
    }

    snprintf(buf, sizeof(buf), "%d\xC2\xB0", static_cast<int>(lroundf(day.tmax)));
    return buf;
}

void weatherGroup(String& out, uint16_t icon, uint16_t color, const String& name, const String& value) {
    out += F("~~~");
    out += utf8(icon);
    out += '~';
    if (icon) {
        out += color;
    }
    out += '~';
    out += name;
    out += '~';
    out += value;
}

void sendWeather() {
    String out(F("weatherUpdate"));
    out.reserve(200);

    const auto& data = weather::current;
    if (config.weather && data.valid) {
        const auto main = icons::weather(data.code, data.day);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d\xC2\xB0" "C", static_cast<int>(lroundf(data.temperature)));
        weatherGroup(out, main.icon, main.color, String(), buf);

        for (size_t n = 0; n < 4; ++n) {
            if (n < data.count) {
                const auto& day = data.days[n];
                const auto icon = icons::weather(day.code, true);
                weatherGroup(out, icon.icon, icon.color, lang::weekdayShort(day.wday), forecastValue(day));
            } else {
                weatherGroup(out, 0, 0, String(), String());
            }
        }
    } else {
        for (size_t n = 0; n < 5; ++n) {
            weatherGroup(out, 0, 0, String(), String());
        }
    }

    // 6th group switches the screensaver to the alternative layout (second main value)
    display.temperature_text = config.temperature_show ? ntc::text() : String();
    if (display.temperature_text.length()) {
        weatherGroup(out, icons::Thermometer, icons::colors::White, String(), display.temperature_text);
    } else {
        out += F("~~~~~~");
    }

    serial::enqueue(std::move(out), true);
}

uint8_t statusMask() {
    uint8_t out = 0;
    if (!wifiConnected()) {
        out |= 1;
    }
#if MQTT_SUPPORT
    if (!mqttConnected()) {
        out |= 2;
    }
#endif
    return out;
}

void sendStatus() {
    display.status_mask = statusMask();

    String out(F("statusUpdate~"));
    if (display.status_mask & 1) {
        out += utf8(icons::WifiOff);
        out += '~';
        out += icons::colors::Red;
    } else if (display.status_mask & 2) {
        out += utf8(icons::AlertCircleOutline);
        out += '~';
        out += icons::colors::Red;
    } else {
        out += '~';
    }
    out += F("~~~~");

    serial::enqueue(std::move(out), true);
}

void sendDimmode() {
    String out(F("dimmode~"));
    out += config.dim;
    out += '~';
    out += config.brightness;
    out += '~';
    out += icons::colors::Background;
    serial::enqueue(std::move(out), true);
}

void sendTimeout() {
    String out(F("timeout~"));
    out += config.timeout;
    serial::enqueue(std::move(out), true);
}

void sendClock(bool force) {
    const time_t now = ::time(nullptr);
    if (now < 1609459200) { // 2021-01-01, not synced yet
        return;
    }

    tm t;
    localtime_r(&now, &t);

    if (force || (t.tm_min != display.last_minute)) {
        display.last_minute = t.tm_min;
        char buf[24];
        snprintf(buf, sizeof(buf), "time~%02d:%02d~", t.tm_hour, t.tm_min);
        serial::enqueue(buf, true);
    }

    if (force || (t.tm_mday != display.last_mday)) {
        display.last_mday = t.tm_mday;
        serial::enqueue(String(F("date~")) + lang::date(t), true);
    }
}

void buildCards() {
    const size_t capacity = (display.version >= 51) ? 8 : 6;
    nspanel::buildCards(display.cards, capacity, HeadingMax);

    if (display.card >= display.cards.size()) {
        display.card = 0;
    }
    display.history.clear();
}

const char* cardType(const Card& card) {
    return ((display.version >= 51) && (card.count > 6))
        ? "cardGrid2"
        : "cardGrid";
}

uint16_t stateColor(int state) {
    return (state == 1) ? config.color_on
        : (state == 0) ? config.color_off
        : config.color_unknown;
}

void renderItem(String& out, uint16_t index) {
    const auto& item = config.items[index];

    const char* type = "switch";
    String icon = item.icon;
    uint16_t color = config.color_unknown;
    String value;

    switch (item.type) {
    case ItemType::Switch: {
        const auto state = entityState(item.target.entity);
        color = stateColor(state);
        value = (state == 1) ? F("1") : F("0");
        break;
    }

    case ItemType::Button:
        type = "button";
        color = stateColor(entityState(item.target.entity));
        break;

    case ItemType::Scene:
        type = "button";
        color = stateColor(groupState(item.target.group));
        break;

    case ItemType::Text: {
        type = "text";
        color = icons::colors::White;
        if ((item.target.entity >= 0) && !icon.length()) {
            icon = utf8Prefix(sanitize(config.entities[item.target.entity].value.c_str(), 32), 4);
        }
        if (item.target.entity >= 0) {
            value = sanitize(config.entities[item.target.entity].value.c_str(), 32);
        }
        break;
    }

    case ItemType::Link:
        type = "button";
        color = icons::colors::White;
        break;

    // read-only here: current temperature (the OHpanel driver has the thermostat page)
    case ItemType::Thermostat: {
        type = "text";
        color = icons::colors::White;
        if ((item.thermostat >= 0) && (static_cast<size_t>(item.thermostat) < config.thermostats.size())) {
            const float current = entityNumber(config.thermostats[item.thermostat].current);
            if (!std::isnan(current)) {
                value = temperatureShort(current) + "\xC2\xB0";
            }
        }
        break;
    }
    }

    // TFT 53 treats '¬' (C2 AC) as the second-to-last character of the icon as a font switch
    icon.replace("\xC2\xAC", "-");
    if ((item.font != NoFont) && (display.version >= 53)) {
        icon += "\xC2\xAC";
        icon += static_cast<char>('0' + item.font);
    }

    out += '~';
    out += type;
    out += F("~i");
    out += index;
    out += '~';
    out += icon;
    out += '~';
    out += color;
    out += '~';
    out += item.name;
    out += '~';
    out += value;
}

void renderNavHidden(String& out) {
    out += F("~delete~~~~~");
}

void renderNav(String& out, const char* id, uint16_t icon, const String& name) {
    String label = name;
    utf8Truncate(label, NameMax);

    out += F("~button~");
    out += id;
    out += '~';
    out += utf8(icon);
    out += '~';
    out += icons::colors::White;
    out += '~';
    out += label;
    out += '~';
}

void showScreensaver();

void showCard(size_t index, bool full) {
    if (display.cards.empty()) {
        buildCards();
    }

    if (display.cards.empty()) {
        showScreensaver();
        return;
    }

    if (index >= display.cards.size()) {
        index = 0;
    }

    display.card = index;
    display.screen = Screen::Card;
    display.dirty = false;

    const auto& card = display.cards[index];
    if (!card.sub) {
        display.history.clear();
    }
    // the row: home + its cards; a nested page: its own sheets, left of the first is where it was opened from
    const size_t total = card.sub ? display.cards.size() : rowCards(display.cards);

    if (full) {
        serial::enqueue(String(F("pageType~")) + cardType(card));
        sendTimeout();
    }

    String out(F("entityUpd~"));
    out.reserve(640);
    out += card.title;

    // Linear navigation: the screensaver sits left of the first card, the last card has no right arrow
    if (card.sub && !samePage(display.cards, index, index - 1)) {
        renderNav(out, "navPrev", icons::ArrowLeftBold, String());
    } else if (index == 0) {
        renderNav(out, "navHome", icons::Home, String());
    } else {
        renderNav(out, "navPrev", icons::ArrowLeftBold, display.cards[index - 1].title);
    }

    if (((index + 1) < total) && (!card.sub || samePage(display.cards, index, index + 1))) {
        renderNav(out, "navNext", icons::ArrowRightBold, display.cards[index + 1].title);
    } else {
        renderNavHidden(out);
    }

    const auto& page = config.pages[card.page];
    for (size_t n = card.first; n < (card.first + card.count); ++n) {
        renderItem(out, page.items[n]);
    }

    if (out.length() > 700) {
        DEBUG_MSG_P(PSTR("[NSPANEL] entityUpd is %u bytes, TFT may truncate it (700)\n"), out.length());
    }

    serial::enqueue(std::move(out), true);
}

void showScreensaver() {
    display.history.clear();
    display.screen = Screen::Screensaver;
    display.dirty = false;
    serial::enqueue(F("pageType~screensaver"));
    sendTimeout();
    sendWeather();
    sendStatus();
}

void requestStartup() {
    display.startup_request = millis();
    serial::enqueue(F("pageType~pageStartup"));
}

// "open" items and hardware buttons with a page: first card of that page, the screensaver for home / weather
void showTarget(int page) {
    if (page >= 0) {
        const int card = firstCard(display.cards, page);
        if (card < 0) {
            return;
        }
        const bool here = (display.screen == Screen::Card) && samePage(display.cards, display.card, card);
        if (display.cards[card].sub && !here) {
            if (display.history.size() >= 8) {
                display.history.erase(display.history.begin());
            }
            display.history.push_back((display.screen == Screen::Card) ? static_cast<int>(display.card) : -1);
        }
        showCard(card, true);
    } else if ((page == PageHome) || (page == PageWeather)) {
        showScreensaver();
    }
}

// -----------------------------------------------------------------------------
// Events
// -----------------------------------------------------------------------------

void onStartup(int version, const String& model) {
    display.connected = true;
    display.was_connected = true;
    display.lost = false;
    display.garbage = 0;
    display.version = version;
    display.model = model;
    display.screen = Screen::Startup;

    DEBUG_MSG_P(PSTR("[NSPANEL] Lovelace TFT startup: version %d, model '%s'\n"), version, model.c_str());
    if ((version < 50) || (model != F("eu"))) {
        DEBUG_MSG_P(PSTR("[NSPANEL] Unexpected TFT version / model, expected >= 50 and 'eu'\n"));
    }

    announceDriver(Driver::Lovelace, String(version), model);

    buildCards();
    sendClock(true);
    sendDimmode();
    showScreensaver();
}

void tap(size_t index, int value) {
    if (index >= config.items.size()) {
        return;
    }

    const auto& item = config.items[index];
    if (item.type == ItemType::Link) {
        buzzer::beep();
        showTarget(item.target.page);
        return;
    }

    itemTap(index, value);
}

void onMessage(const String& message) {
    ++display.rx_frames;
    display.garbage = 0;
    publishEvent(message);

    if (config.debug) {
        DEBUG_MSG_P(PSTR("[NSPANEL] <- %s\n"), message.c_str());
    }

    String parts[6];
    size_t count = 0;
    int start = 0;
    while ((count < 6) && (start <= static_cast<int>(message.length()))) {
        int pos = message.indexOf(',', start);
        if ((pos < 0) || (count == 5)) {
            pos = message.length();
        }
        parts[count++] = message.substring(start, pos);
        start = pos + 1;
    }

    if ((count < 2) || (parts[0] != F("event"))) {
        return;
    }

    const auto& type = parts[1];
    if (type == F("startup")) {
        onStartup(parts[2].toInt(), parts[3]);
        return;
    }

    if (type == F("sleepReached")) {
        showScreensaver();
        return;
    }

    if (type == F("renderCurrentPage")) {
        if (display.screen == Screen::Card) {
            showCard(display.card, false);
        }
        return;
    }

    if (type != F("buttonPress2") || (count < 4)) {
        return;
    }

    const auto& id = parts[2];
    const auto& action = parts[3];

    if (id == F("screensaver")) {
        if ((action == F("bExit")) || action.startsWith(F("swipe"))) {
            showCard(0, true);
        }
        return;
    }

    if ((action == F("bExit")) || (action == F("notifyAction"))) {
        showCard(display.card, true);
        return;
    }

    if (id == F("navHome")) {
        buzzer::beep();
        showScreensaver();
        return;
    }

    if (id == F("navPrev") || id == F("navNext")) {
        const bool sub = (display.card < display.cards.size()) && display.cards[display.card].sub;
        const size_t total = sub ? display.cards.size() : rowCards(display.cards);
        if (id == F("navNext")) {
            if (((display.card + 1) < total) && (!sub || samePage(display.cards, display.card, display.card + 1))) {
                buzzer::beep();
                showCard(display.card + 1, true);
            }
        } else if (sub && !samePage(display.cards, display.card, display.card - 1)) {
            // back to where the nested page was opened from
            buzzer::beep();
            int back = -1;
            while (!display.history.empty()) {
                back = display.history.back();
                display.history.pop_back();
                if ((back < 0) || (static_cast<size_t>(back) < display.cards.size())) {
                    break;
                }
                back = -1;
            }
            if (back >= 0) {
                showCard(back, true);
            } else {
                showScreensaver();
            }
        } else if (display.card > 0) {
            buzzer::beep();
            showCard(display.card - 1, true);
        } else {
            buzzer::beep();
            showScreensaver();
        }
        return;
    }

    if ((id.length() > 1) && (id[0] == 'i')) {
        const auto index = id.substring(1).toInt();
        if (action == F("button")) {
            tap(index, -1);
        } else if ((action == F("OnOff")) && (count >= 5)) {
            tap(index, (parts[4].toInt() != 0) ? 1 : 0);
        }
    }
}

void rxLoop() {
    auto& port = serial::port();
    size_t budget = 512;
    while (budget-- && port.available()) {
        const int byte = port.read();
        if (byte < 0) {
            break;
        }

        bool raw_startup = false;
        if (serial::parser.feed(static_cast<uint8_t>(byte), raw_startup)) {
            const String message = serial::parser.payload;
            serial::parser.payload = String();
            onMessage(message);
        }

        if (raw_startup) {
            DEBUG_MSG_P(PSTR("[NSPANEL] Lovelace TFT power-up, sending recmod=1\n"));
            serial::writeRaw("recmod=1");
            display.garbage = 0;
            if (display.connected) {
                display.disconnected_since = millis();
            }
            display.connected = false;
            display.screen = Screen::Unknown;
        }
    }
}

// -----------------------------------------------------------------------------
// Driver interface (link:: / dispatch in nspanel.cpp)
// -----------------------------------------------------------------------------

// take UART2 at 115200 and ask for event,startup. recmod=1 first: a Lovelace TFT left in command
// mode (e.g. by an aborted upload handshake) goes back to protocol reparse mode. At 115200 an
// OHpanel TFT (921600) cannot parse it
void begin() {
    uart::setBaud(Baudrate);
    display.queue.clear();
    serial::parser = serial::Parser();
    display.connected = false;
    display.was_connected = false;
    display.lost = false;
    display.garbage = 0;
    display.screen = Screen::Unknown;
    display.last_tx = millis() - TxGapMs;

    const uint8_t ff[3] {0xFF, 0xFF, 0xFF};
    serial::port().write(ff, sizeof(ff));
    serial::writeRaw("recmod=1");
    requestStartup();
}

bool connected() {
    return display.connected;
}

// the TFT stopped answering or something else talks on the line: link:: detects again
bool lost() {
    return display.lost;
}

void loop() {
    rxLoop();
    serial::txLoop();

    const auto now = millis();
    if (!display.connected) {
        if (now - display.startup_request >= StartupRetryMs) {
            requestStartup();
        }
        if (display.was_connected && (now - display.disconnected_since >= LostMs)) {
            DEBUG_MSG_P(PSTR("[NSPANEL] Lovelace TFT silent for %u s\n"), LostMs / 1000);
            display.lost = true;
        }
        return;
    }

    if (display.garbage >= GarbageMax) {
        DEBUG_MSG_P(PSTR("[NSPANEL] %u bytes outside Lovelace frames, another TFT?\n"), display.garbage);
        display.garbage = 0;
        display.connected = false;
        display.lost = true;
        return;
    }

    static uint32_t last_check = 0;
    if (now - last_check >= 1000) {
        last_check = now;
        sendClock(false);
        if ((display.screen == Screen::Screensaver) && (statusMask() != display.status_mask)) {
            sendStatus();
        }
    }

    if (display.dirty && (now - display.dirty_since >= RenderDelayMs)) {
        if (display.screen == Screen::Card) {
            showCard(display.card, false);
        }
        display.dirty = false;
    }
}

void reload() {
    buildCards();
    if (display.connected) {
        sendDimmode();
        if (display.screen == Screen::Card) {
            showCard(display.card, true);
        } else {
            showScreensaver();
        }
    }
}

void weatherChanged() {
    if (display.connected && (display.screen == Screen::Screensaver)) {
        sendWeather();
    }
}

void temperatureChanged() {
    if (display.connected && config.temperature_show && (display.screen == Screen::Screensaver)
        && (ntc::text() != display.temperature_text))
    {
        sendWeather();
    }
}

void showPage(int page) {
    if (display.connected) {
        showTarget(page);
    }
}

void send(String payload) {
    serial::enqueue(std::move(payload));
}

void info(Print& out) {
    out.printf_P(PSTR("Lovelace: %s, version %d, model '%s'\n"),
        display.connected ? "connected" : "not connected",
        display.version, display.model.c_str());
    out.printf_P(PSTR("Screen: %s, card %u / %u\n"),
        screenName(display.screen), display.card + 1, display.cards.size());
    out.printf_P(PSTR("Frames: rx %u, tx %u, crc errors %u, dropped %u, queued %u\n"),
        display.rx_frames, display.tx_frames, display.crc_errors, display.dropped, display.queue.size());
}

} // namespace lovelace
