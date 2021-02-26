/*
GARLAND MODULE
Copyright (C) 2020 by Dmitry Blinov <dblinov76 at gmail dot com>

Inspired by https://github.com/Vasil-Pahomov/ArWs2812 (currently https://github.com/Vasil-Pahomov/Liana)

Tested on 300 led strip.

The most time consuming operation is actually showing leds by Adafruit Neopixel. It take about 1870 mcs.
More long strip can take more time to show.
Currently animation calculation, brightness calculation/transition and showing makes in one loop cycle.
Debug output shows timings. Overal timing should be not more that 3000 ms.

MQTT control:
Topic: DEVICE_NAME/garland/set
Message: {"command":"string", "enable":"string", "brightness":int, "speed":int, "animation":"string",
          "palette":"string"/int, "duration":int}
All parameters are optional.

"command:["immediate", "queue", "sequence", "reset"] - if not set, than "immediate" by default
    Commands priority:
        - "immediate" - executes immediately, braking current animation.
        - "queue" - if queue is not empty, than next queue command executes after current animation end.
                    after execution command removed from queue.
        - "sequence" - executes commands in sequence in cycle.
        - "reset" - clean queue and sequence, restore default settings, enable garland.
        - random if there are no commands in queue or sequence.

"enable":["true", "false"] - enable or disable garland
"brightness":[0-255] - set brightness
"speed":[30-60] - set animation speed
"animation":["PixieDust", "Sparkr", "Run", "Stars", "Spread", "R"andCyc", "Fly", "Comets", "Assemble", "Dolphins", "Salut"]
    - setup animation. If not set or not recognized, than setup previous anmation
"palette":["RGB", "Rainbow", "Stripe", "Party", "Heat", Fire", "Blue", "Sun", "Lime", "Pastel"]
    - can be one of listed above or can be one color palette.
    - one color palette can be set by string, that represents color in the format "0xRRGGBB" (0xFF0000 for red) or
      integer number, corresponding to it. Examples: "palette":"0x0000FF", "palette":255 equal to Blue color.
"duration":5000 - setup command duration in milliseconds. If not set, than infinite duration will setup.

If command contains animation, palette or duration, than it setup next animation, that will be shown for duration (infinite if
duration does not set), otherwise it just set scene parameters.

Infinite commands can be interrupted by immediate command or by reset command.
*/

#include "garland.h"

#if GARLAND_SUPPORT

#include <Adafruit_NeoPixel.h>

#include <memory>
#include <vector>

#include "garland/color.h"
#include "garland/palette.h"
#include "garland/scene.h"
#include "mqtt.h"
#include "ws.h"

const char* NAME_GARLAND_ENABLED        = "garlandEnabled";
const char* NAME_GARLAND_BRIGHTNESS     = "garlandBrightness";
const char* NAME_GARLAND_SPEED          = "garlandSpeed";

const char* NAME_GARLAND_SWITCH         = "garland_switch";
const char* NAME_GARLAND_SET_BRIGHTNESS = "garland_set_brightness";
const char* NAME_GARLAND_SET_SPEED      = "garland_set_speed";
const char* NAME_GARLAND_SET_DEFAULT    = "garland_set_default";

const char* MQTT_TOPIC_GARLAND          = "garland";

const char* MQTT_PAYLOAD_COMMAND        = "command";
const char* MQTT_PAYLOAD_ENABLE         = "enable";
const char* MQTT_PAYLOAD_BRIGHTNESS     = "brightness";
const char* MQTT_PAYLOAD_ANIM_SPEED     = "speed";
const char* MQTT_PAYLOAD_ANIMATION      = "animation";
const char* MQTT_PAYLOAD_PALETTE        = "palette";
const char* MQTT_PAYLOAD_DURATION       = "duration";

const char* MQTT_COMMAND_IMMEDIATE      = "immediate";
const char* MQTT_COMMAND_RESET          = "reset"; // reset queue
const char* MQTT_COMMAND_QUEUE          = "queue"; // enqueue command payload
const char* MQTT_COMMAND_SEQUENCE       = "sequence"; // place command to sequence

#define EFFECT_UPDATE_INTERVAL_MIN      7000  // 5 sec
#define EFFECT_UPDATE_INTERVAL_MAX      12000 // 10 sec

#define NUMLEDS_CAN_CAUSE_WDT_RESET     100

bool          _garland_enabled          = true;
unsigned long _lastTimeUpdate           = 0;
unsigned long _currentDuration          = ULONG_MAX;
unsigned int  _currentCommandInSequence = 0;
String        _immediate_command;
std::queue<String>  _command_queue;
std::vector<String> _command_sequence;

// Palette should
Palette pals[] = {
    // palettes below are taken from http://www.color-hex.com/color-palettes/ (and modified)
    // RGB: Red,Green,Blue sequence
    Palette("RGB", {0xFF0000, 0x00FF00, 0x0000FF}),

    // Rainbow: Rainbow colors
    Palette("Rainbow", {0xFF0000, 0xAB5500, 0xABAB00, 0x00FF00, 0x00AB55, 0x0000FF, 0x5500AB, 0xAB0055}),

    // RainbowStripe: Rainbow colors with alternating stripes of black
    Palette("Stripe", {0xFF0000, 0x000000, 0xAB5500, 0x000000, 0xABAB00, 0x000000, 0x00FF00, 0x000000,
                       0x00AB55, 0x000000, 0x0000FF, 0x000000, 0x5500AB, 0x000000, 0xAB0055, 0x000000}),

    // Party: Blue purple ping red orange yellow (and back). Basically, everything but the greens.
    // This palette is good for lighting at a club or party.
    Palette("Party", {0x5500AB, 0x84007C, 0xB5004B, 0xE5001B, 0xE81700, 0xB84700, 0xAB7700, 0xABAB00,
                      0xAB5500, 0xDD2200, 0xF2000E, 0xC2003E, 0x8F0071, 0x5F00A1, 0x2F00D0, 0x0007F9}),

    // Heat: Approximate "black body radiation" palette, akin to the FastLED 'HeatColor' function.
    // Recommend that you use values 0-240 rather than the usual 0-255, as the last 15 colors will be
    // 'wrapping around' from the hot end to the cold end, which looks wrong.
    Palette("Heat", {0x700070, 0xFF0000, 0xFFFF00, 0xFFFFCC}),

    // Fire:
    Palette("Fire", {0x000000, 0x220000, 0x880000, 0xFF0000, 0xFF6600, 0xFFCC00}),

    // Blue:
    Palette("Blue", {0xffffff, 0x0000ff, 0x00ffff}),

    // Sun: Slice Of The Sun
    Palette("Sun", {0xfff95b, 0xffe048, 0xffc635, 0xffad22, 0xff930f}),

    // Lime: yellow green mix
    Palette("Lime", {0x51f000, 0x6fff00, 0x96ff00, 0xc9ff00, 0xf0ff00}),

    // Pastel: Pastel Fruity Mixture
    Palette("Pastel", {0x75aa68, 0x5960ae, 0xe4be6c, 0xca5959, 0x8366ac})};

constexpr size_t palsSize() { return sizeof(pals)/sizeof(pals[0]); }

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(GARLAND_LEDS, GARLAND_D_PIN, NEO_GRB + NEO_KHZ800);
Scene scene(&pixels);

Anim* anims[] = {new AnimGlow(), new AnimStart(), new AnimPixieDust(), new AnimSparkr(), new AnimRun(), new AnimStars(), new AnimSpread(),
                 new AnimRandCyc(), new AnimFly(), new AnimComets(), new AnimAssemble(), new AnimDolphins(), new AnimSalut(), new AnimFountain()};

constexpr size_t animsSize() { return sizeof(anims)/sizeof(anims[0]); }

#define START_ANIMATION  1
Anim* _currentAnim       = anims[1];
Palette* _currentPalette = &pals[0];
auto one_color_palette = std::unique_ptr<Palette>(new Palette("White", {0xffffff}));
//------------------------------------------------------------------------------
void garlandDisable() {
    pixels.clear();
}

//------------------------------------------------------------------------------
void garlandEnabled(bool enabled) {
    _garland_enabled = enabled;
    setSetting(NAME_GARLAND_ENABLED, _garland_enabled);
    if (!_garland_enabled) {
        schedule_function([]() {
            pixels.clear();
            pixels.show();
        });
    }
#if WEB_SUPPORT
    char buffer[128];
    snprintf_P(buffer, sizeof(buffer), PSTR("{\"garlandEnabled\": %s}"), enabled ? "true" : "false");
    wsSend(buffer);
#endif
}

//------------------------------------------------------------------------------
bool garlandEnabled() {
    return _garland_enabled;
}

//------------------------------------------------------------------------------
// Setup
//------------------------------------------------------------------------------
void _garlandConfigure() {
    _garland_enabled = getSetting(NAME_GARLAND_ENABLED, true);
    DEBUG_MSG_P(PSTR("[GARLAND] _garland_enabled = %d\n"), _garland_enabled);

    byte brightness = getSetting(NAME_GARLAND_BRIGHTNESS, 255);
    scene.setBrightness(brightness);
    DEBUG_MSG_P(PSTR("[GARLAND] brightness = %d\n"), brightness);

    float speed = getSetting(NAME_GARLAND_SPEED, 50);
    scene.setSpeed(speed);
}

//------------------------------------------------------------------------------
void _garlandReload() {
    _garlandConfigure();
}

//------------------------------------------------------------------------------
void setDefault() {
    scene.setDefault();
    byte brightness = scene.getBrightness();
    setSetting(NAME_GARLAND_BRIGHTNESS, brightness);
    byte speed = scene.getSpeed();
    setSetting(NAME_GARLAND_SPEED, speed);
#if WEB_SUPPORT
    char buffer[128];
    snprintf_P(buffer, sizeof(buffer), PSTR("{\"garlandBrightness\": %d, \"garlandSpeed\": %d}"), brightness, speed);
    wsSend(buffer);
#endif
}

#if WEB_SUPPORT
//------------------------------------------------------------------------------
void _garlandWebSocketOnConnected(JsonObject& root) {
    root[NAME_GARLAND_ENABLED] = garlandEnabled();
    root[NAME_GARLAND_BRIGHTNESS] = scene.getBrightness();
    root[NAME_GARLAND_SPEED] = scene.getSpeed();
    root["garlandVisible"] = 1;
}

//------------------------------------------------------------------------------
bool _garlandWebSocketOnKeyCheck(const char* key, JsonVariant& value) {
    if (strncmp(key, NAME_GARLAND_ENABLED, strlen(NAME_GARLAND_ENABLED)) == 0) return true;
    if (strncmp(key, NAME_GARLAND_BRIGHTNESS, strlen(NAME_GARLAND_BRIGHTNESS)) == 0) return true;
    if (strncmp(key, NAME_GARLAND_SPEED, strlen(NAME_GARLAND_SPEED)) == 0) return true;
    return false;
}

//------------------------------------------------------------------------------
void _garlandWebSocketOnAction(uint32_t client_id, const char* action, JsonObject& data) {
    if (strcmp(action, NAME_GARLAND_SWITCH) == 0) {
        if (data.containsKey("status") && data.is<int>("status")) {
            garlandEnabled(1 == data["status"].as<int>());
        }
    }

    if (strcmp(action, NAME_GARLAND_SET_BRIGHTNESS) == 0) {
        if (data.containsKey("brightness")) {
            byte new_brightness = data.get<byte>("brightness");
            DEBUG_MSG_P(PSTR("[GARLAND] new brightness = %d\n"), new_brightness);
            setSetting(NAME_GARLAND_BRIGHTNESS, new_brightness);
            scene.setBrightness(new_brightness);
        }
    }

    if (strcmp(action, NAME_GARLAND_SET_SPEED) == 0) {
        if (data.containsKey("speed")) {
            byte new_speed = data.get<byte>("speed");
            DEBUG_MSG_P(PSTR("[GARLAND] new speed = %d\n"), new_speed);
            setSetting(NAME_GARLAND_SPEED, new_speed);
            scene.setSpeed(new_speed);
        }
    }

    if (strcmp(action, NAME_GARLAND_SET_DEFAULT) == 0) {
        setDefault();
    }
}
#endif

//------------------------------------------------------------------------------
void setupScene(Anim* new_anim, Palette* new_palette, unsigned long new_duration) {
    unsigned long currentAnimRunTime = millis() - _lastTimeUpdate;
    _lastTimeUpdate = millis();

    int numShows = scene.getNumShows();
    int frameRate = currentAnimRunTime > 0 ? numShows * 1000 / currentAnimRunTime : 0;

    static String palette_name = "Start";
    DEBUG_MSG_P(PSTR("[GARLAND] Anim: %-10s Pal: %-8s timings: calc: %4d pixl: %3d show: %4d frate: %d\n"),
                _currentAnim->name(), palette_name.c_str(),
                scene.getAvgCalcTime(), scene.getAvgPixlTime(), scene.getAvgShowTime(), frameRate);

    _currentDuration = new_duration;
    _currentAnim     = new_anim;
    _currentPalette  = new_palette;
    palette_name = _currentPalette->name();
    DEBUG_MSG_P(PSTR("[GARLAND] Anim: %-10s Pal: %-8s Inter: %d\n"),
                _currentAnim->name(), palette_name.c_str(), _currentDuration);

    scene.setAnim(_currentAnim);
    scene.setPalette(_currentPalette);
    scene.setup();
}

//------------------------------------------------------------------------------
bool executeCommand(const String& command) {
    DEBUG_MSG_P(PSTR("[GARLAND] Executing command \"%s\"\n"), command.c_str());
    // Parse JSON input
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(command);
    if (!root.success()) {
        DEBUG_MSG_P(PSTR("[GARLAND] Error parsing command\n"));
        return false;
    }

    bool scene_setup_required = false;

    if (root.containsKey(MQTT_PAYLOAD_ENABLE)) {
        auto enable = root[MQTT_PAYLOAD_ENABLE].as<String>();
        garlandEnabled(enable != "false");
    }

    if (root.containsKey(MQTT_PAYLOAD_BRIGHTNESS)) {
        auto brightness = root[MQTT_PAYLOAD_BRIGHTNESS].as<byte>();
        scene.setBrightness(brightness);
    }

    if (root.containsKey(MQTT_PAYLOAD_ANIM_SPEED)) {
        auto speed = root[MQTT_PAYLOAD_ANIM_SPEED].as<byte>();
        scene.setSpeed(speed);
    }

    Anim* newAnim = _currentAnim;
    if (root.containsKey(MQTT_PAYLOAD_ANIMATION)) {
        auto animation = root[MQTT_PAYLOAD_ANIMATION].as<const char*>();
        for (size_t i = 0; i < animsSize(); ++i) {
            auto anim_name = anims[i]->name();
            if (strcmp(animation, anim_name) == 0) {
                newAnim = anims[i];
                scene_setup_required = true;
                break;
            }
        }
    }

    Palette* newPalette = _currentPalette;
    if (root.containsKey(MQTT_PAYLOAD_PALETTE)) {
        if (root.is<int>(MQTT_PAYLOAD_PALETTE)) {
            one_color_palette.reset(new Palette("Color", {root[MQTT_PAYLOAD_PALETTE].as<uint32_t>()}));
            newPalette = one_color_palette.get();
        } else {
            auto palette = root[MQTT_PAYLOAD_PALETTE].as<const char*>();
            bool palette_found = false;
            for (size_t i = 0; i < palsSize(); ++i) {
                auto pal_name = pals[i].name();
                if (strcmp(palette, pal_name) == 0) {
                    newPalette = &pals[i];
                    palette_found = true;
                    scene_setup_required = true;
                    break;
                }
            }
            if (!palette_found) {
                uint32_t color = (uint32_t)strtoul(palette, NULL, 0);
                if (color != 0) {
                    one_color_palette.reset(new Palette("Color", {color}));
                    newPalette = one_color_palette.get();
                }
            }
        }
    }

    unsigned long newAnimDuration = LONG_MAX;
    if (root.containsKey(MQTT_PAYLOAD_DURATION)) {
        newAnimDuration = root[MQTT_PAYLOAD_DURATION].as<unsigned long>();
        scene_setup_required = true;
    }

    if (scene_setup_required) {
        setupScene(newAnim, newPalette, newAnimDuration);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
// Loop
//------------------------------------------------------------------------------
void garlandLoop(void) {
    if (!_immediate_command.isEmpty()) {
        executeCommand(_immediate_command);
        _immediate_command.clear();
    }

    if (!garlandEnabled())
        return;

    scene.run();

    unsigned long currentAnimRunTime = millis() - _lastTimeUpdate;
    if (currentAnimRunTime > _currentDuration && scene.finishedAnimCycle()) {
        bool scene_setup_done = false;
        if (!_command_queue.empty()) {
            scene_setup_done = executeCommand(_command_queue.front());
            _command_queue.pop();
        } else if (!_command_sequence.empty()) {
            scene_setup_done = executeCommand(_command_sequence[_currentCommandInSequence]);
            ++_currentCommandInSequence;
            if (_currentCommandInSequence >= _command_sequence.size())
                _currentCommandInSequence = 0;
        }

        if (!scene_setup_done) {
            Anim* newAnim = _currentAnim;
            while (newAnim == _currentAnim) {
                newAnim = anims[secureRandom(START_ANIMATION + 1, animsSize())];
            }

            Palette* newPalette = _currentPalette;
            while (newPalette == _currentPalette) {
                newPalette = &pals[secureRandom(palsSize())];
            }

            unsigned long newAnimDuration = secureRandom(EFFECT_UPDATE_INTERVAL_MIN, EFFECT_UPDATE_INTERVAL_MAX);

            setupScene(newAnim, newPalette, newAnimDuration);
        }
    }
}

//------------------------------------------------------------------------------
void garlandMqttCallback(unsigned int type, const char * topic, const char * payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_GARLAND);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        // Match topic
        String t = mqttMagnitude((char*)topic);

        if (t.equals(MQTT_TOPIC_GARLAND)) {
            // Parse JSON input
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(payload);
            if (!root.success()) {
                DEBUG_MSG_P(PSTR("[GARLAND] Error parsing mqtt data\n"));
                return;
            }

            String command = MQTT_COMMAND_IMMEDIATE;
            if (root.containsKey(MQTT_PAYLOAD_COMMAND)) {
                command = root[MQTT_PAYLOAD_COMMAND].as<String>();
            }

            if (command == MQTT_COMMAND_IMMEDIATE) {
                _immediate_command = payload;
            } else if (command == MQTT_COMMAND_RESET) {
                std::queue<String> empty_queue;
                std::swap(_command_queue, empty_queue);
                std::vector<String> empty_sequence;
                std::swap(_command_sequence, empty_sequence);
                _immediate_command.clear();
                _currentDuration = 0;
                setDefault();
                garlandEnabled(true);
            } else if (command == MQTT_COMMAND_QUEUE) {
                _command_queue.push(payload);
            } else if (command == MQTT_COMMAND_SEQUENCE) {
                _command_sequence.push_back(payload);
            }
        }
    }
}

//------------------------------------------------------------------------------
void garlandSetup() {
    _garlandConfigure();

    mqttRegister(garlandMqttCallback);
// Websockets
#if WEB_SUPPORT
    wsRegister()
        .onConnected(_garlandWebSocketOnConnected)
        .onKeyCheck(_garlandWebSocketOnKeyCheck)
        .onAction(_garlandWebSocketOnAction);
#endif

    espurnaRegisterLoop(garlandLoop);
    espurnaRegisterReload(_garlandReload);

    pixels.begin();
    scene.setAnim(_currentAnim);
    scene.setPalette(_currentPalette);
    scene.setup();

    _currentDuration = secureRandom(EFFECT_UPDATE_INTERVAL_MIN, EFFECT_UPDATE_INTERVAL_MAX);
}

/*#######################################################################
  _____
 / ____|
| (___     ___    ___   _ __     ___
 \___ \   / __|  / _ \ | '_ \   / _ \
 ____) | | (__  |  __/ | | | | |  __/
|_____/   \___|  \___| |_| |_|  \___|
#######################################################################*/

#define GARLAND_SCENE_TRANSITION_MS      1000    // transition time between animations, ms
#define GARLAND_SCENE_SPEED_MAX          70
#define GARLAND_SCENE_SPEED_FACTOR       10
#define GARLAND_SCENE_DEFAULT_SPEED      50
#define GARLAND_SCENE_DEFAULT_BRIGHTNESS 255

Scene::Scene(Adafruit_NeoPixel* pixels)
    : _pixels(pixels),
      _numLeds(pixels->numPixels()),
      _leds1(_numLeds),
      _leds2(_numLeds),
      _ledstmp(_numLeds),
      _seq(_numLeds) {
}

void Scene::setPalette(Palette* palette) {
    _palette = palette;
    if (setUpOnPalChange) {
        setupImpl();
    }
}

void Scene::setBrightness(byte brightness) {
    DEBUG_MSG_P(PSTR("[GARLAND] Scene::setBrightness = %d\n"), brightness);
    this->brightness = brightness;
}

byte Scene::getBrightness() {
    DEBUG_MSG_P(PSTR("[GARLAND] Scene::getBrightness = %d\n"), brightness);
    return brightness;
}

// Speed is reverse to cycleFactor and 10x
void Scene::setSpeed(byte speed) {
    this->speed = speed;
    cycleFactor = (float)(GARLAND_SCENE_SPEED_MAX - speed) / GARLAND_SCENE_SPEED_FACTOR;
    DEBUG_MSG_P(PSTR("[GARLAND] Scene::setSpeed %d cycleFactor = %d\n"), speed, (int)(cycleFactor * 1000));
}

byte Scene::getSpeed() {
    DEBUG_MSG_P(PSTR("[GARLAND] Scene::getSpeed %d cycleFactor = %d\n"), speed, (int)(cycleFactor * 1000));
    return speed;
}

void Scene::setDefault() {
    speed = GARLAND_SCENE_DEFAULT_SPEED;
    cycleFactor = (float)(GARLAND_SCENE_SPEED_MAX - speed) / GARLAND_SCENE_SPEED_FACTOR;
    brightness = GARLAND_SCENE_DEFAULT_BRIGHTNESS;
    DEBUG_MSG_P(PSTR("[GARLAND] Scene::setDefault speed = %d cycleFactor = %d brightness = %d\n"), speed, (int)(cycleFactor * 1000), brightness);
}

void Scene::run() {
    unsigned long iteration_start_time = micros();

    if (state == Calculate || cyclesRemain < 1) {
        // Calculate number of cycles for this animation iteration
        float cycleSum = cycleFactor * (_anim ? _anim->getCycleFactor() : 1.0) + cycleTail;
        cyclesRemain = cycleSum;
        if (cyclesRemain < 1) {
            cyclesRemain = 1;
            cycleSum = 0;
            cycleTail = 0;
        } else {
            cycleTail = cycleSum - cyclesRemain;
        }

        if (_anim) {
            _anim->Run();
        }

        sum_calc_time += (micros() - iteration_start_time);
        iteration_start_time = micros();
        ++calc_num;
        state = Transition;
    }

    if (state == Transition && cyclesRemain < 3) {
        // transition coef, if within 0..1 - transition is active
        // changes from 1 to 0 during transition, so we interpolate from current
        // color to previous
        float transc = (float)((long)transms - (long)millis()) / GARLAND_SCENE_TRANSITION_MS;
        Color* leds_prev = (_leds == &_leds1[0]) ? &_leds2[0] : &_leds1[0];

        if (transc > 0) {
            for (int i = 0; i < _numLeds; i++) {
                // transition is in progress
                Color c = _leds[i].interpolate(leds_prev[i], transc);
                byte r = (int)(bri_lvl[c.r]) * brightness / 256;
                byte g = (int)(bri_lvl[c.g]) * brightness / 256;
                byte b = (int)(bri_lvl[c.b]) * brightness / 256;
                _pixels->setPixelColor(i, _pixels->Color(r, g, b));
            }
        } else {
            for (int i = 0; i < _numLeds; i++) {
                // regular operation
                byte r = (int)(bri_lvl[_leds[i].r]) * brightness / 256;
                byte g = (int)(bri_lvl[_leds[i].g]) * brightness / 256;
                byte b = (int)(bri_lvl[_leds[i].b]) * brightness / 256;
                _pixels->setPixelColor(i, _pixels->Color(r, g, b));
            }
        }

        sum_pixl_time += (micros() - iteration_start_time);
        iteration_start_time = micros();
        ++pixl_num;
        state = Show;
    }

    if (state == Show && cyclesRemain < 2) {
        /* Showing pixels (actually transmitting their RGB data) is most time consuming operation in the
        garland workflow. Using 800 kHz gives 1.25 μs per bit. -> 30 μs (0.03 ms) per RGB LED.
        So for example 3 ms for 100 LEDs. Unfortunately it can't be postponed and resumed later as it
        will lead to reseting the transmition operation. From other hand, long operation can cause
        Soft WDT reset. To avoid wdt reset we need to switch soft wdt off for long strips.
        It is not best practice, but assuming that it is only garland, it can be acceptable.
        Tested up to 300 leds. */
        if (_numLeds > NUMLEDS_CAN_CAUSE_WDT_RESET) {
            ESP.wdtDisable();
        }
        _pixels->show();
        if (_numLeds > NUMLEDS_CAN_CAUSE_WDT_RESET) {
            ESP.wdtEnable(5000);
        }
        sum_show_time += (micros() - iteration_start_time);
        ++show_num;
        state = Calculate;
        ++numShows;
    }
    --cyclesRemain;
}

void Scene::setupImpl() {
    transms = millis() + GARLAND_SCENE_TRANSITION_MS;

    // switch operation buffers (for transition to operate)
    if (_leds == &_leds1[0]) {
        _leds = &_leds2[0];
    } else {
        _leds = &_leds1[0];
    }

    if (_anim) {
        _anim->Setup(_palette, _numLeds, _leds, &_ledstmp[0], &_seq[0]);
    }
}

void Scene::setup() {
    sum_calc_time = 0;
    sum_pixl_time = 0;
    sum_show_time = 0;
    calc_num = 0;
    pixl_num = 0;
    show_num = 0;
    numShows = 0;

    if (!setUpOnPalChange) {
        setupImpl();
    }
}

unsigned long Scene::getAvgCalcTime() { return sum_calc_time / calc_num; }
unsigned long Scene::getAvgPixlTime() { return sum_pixl_time / pixl_num; }
unsigned long Scene::getAvgShowTime() { return sum_show_time / show_num; }

/*#######################################################################
                    _                       _     _
    /\             (_)                     | |   (_)
   /  \     _ __    _   _ __ ___     __ _  | |_   _    ___    _ __
  / /\ \   | '_ \  | | | '_ ` _ \   / _` | | __| | |  / _ \  | '_ \
 / ____ \  | | | | | | | | | | | | | (_| | | |_  | | | (_) | | | | |
/_/    \_\ |_| |_| |_| |_| |_| |_|  \__,_|  \__| |_|  \___/  |_| |_|
#######################################################################*/

Anim::Anim(const char* name) : _name(name) {}

void Anim::Setup(Palette* palette, uint16_t numLeds, Color* leds, Color* ledstmp, byte* seq) {
    this->palette = palette;
    this->numLeds = numLeds;
    this->leds = leds;
    this->ledstmp = ledstmp;
    this->seq = seq;
    SetupImpl();
}

void Anim::initSeq() {
    for (int i = 0; i < numLeds; ++i)
        seq[i] = i;
}

void Anim::shuffleSeq() {
    for (int i = 0; i < numLeds; ++i) {
        byte ind = (unsigned int)(rngb() * numLeds / 256);
        if (ind != i) {
            std::swap(seq[ind], seq[i]);
        }
    }
}

void Anim::glowSetUp() {
    braPhaseSpd = secureRandom(4, 13);
    if (braPhaseSpd > 8) {
        braPhaseSpd = braPhaseSpd - 17;
    }
    braFreq = secureRandom(20, 60);
}

void Anim::glowForEachLed(int i) {
    int8 bra = braPhase + i * braFreq;
    bra = BRA_OFFSET + (abs(bra) >> BRA_AMP_SHIFT);
    leds[i] = leds[i].brightness(bra);
}

void Anim::glowRun() { braPhase += braPhaseSpd; }

bool operator== (const Color &c1, const Color &c2)
{
    return (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b);
}

unsigned int rng() {
    static unsigned int y = 0;
    y += micros();  // seeded with changing number
    y ^= y << 2;
    y ^= y >> 7;
    y ^= y << 7;
    return (y);
}

// Ranom numbers generator in byte range (256) much faster than secureRandom.
// For usage in time-critical places.
byte rngb() { return (byte)rng(); }

#endif  // GARLAND_SUPPORT
