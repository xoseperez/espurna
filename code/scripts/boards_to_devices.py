#!/usr/bin/env python3
"""
Board definitions of config/hardware.h as entries of the ESPurna devices list (espurna-devices
devices.json), for the generic ESP8266 images (esp8266-1m, esp8266-1m-light, esp8266-1m-sensor, esp8266-4m) where the
board is picked at runtime from the WebUI device search instead of a build per board.

    py -3 code/scripts/boards_to_devices.py [--devices PATH] [--envs PATH] [--no-check]

- every [env:...] of platformio.ini and platformio_override.ini is classified (target image,
  reason, what still needs a runtime setting); --envs writes that as JSON
- every ESP8266 board / board variant (env with extra hardware -D flags) that the generic images
  can run becomes a devices.json entry. Hand-made entries (notes not starting with the generated
  marker) are kept untouched and first, generated ones follow, sorted by name
- settings use the runtime keys of relay.cpp, button.cpp, led.cpp, light.cpp, uart.cpp,
  rfbridge.cpp, ir.cpp, i2c.cpp and the sensor drivers. A key is only written when it exists in
  the sources (checked at run time); compile-time-only features are listed instead
- self-check: boards of templates.mjs PROFILES (verified on hardware) must come out the same

Only the standard library is used.
"""

import argparse
import configparser
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CODE = os.path.dirname(HERE)
REPO = os.path.dirname(CODE)

HARDWARE_H = os.path.join(CODE, "espurna", "config", "hardware.h")
DEFAULTS_H = os.path.join(CODE, "espurna", "config", "defaults.h")
SOURCES = os.path.join(CODE, "espurna")
INIS = [
    ("platformio.ini", os.path.join(CODE, "platformio.ini")),
    ("platformio_override.ini", os.path.join(CODE, "platformio_override.ini")),
]
TEMPLATES_MJS = os.path.join(CODE, "html", "src", "templates.mjs")
GENERIC_IMAGES_MD = os.path.join(REPO, "generic-images.md")
DEVICES_JSON = os.path.join(os.path.dirname(REPO), "espurna-devices", "devices.json")

MARKER = "Generated from hardware.h"

GPIO_NONE = 153

# boards of other chips, base / helper builds
NOT_ESP8266 = {
    "ESP32_GENERIC", "ESP32_RELAY_X2", "ITEAD_SONOFF_MINIR4", "SONOFF_NSPANEL",
    "BEKEN_GENERIC", "TUYA_CB3S_RELAY_RF", "LIBRETINY",
}
GENERIC_BOARDS = {"ESP8266_GENERIC", "ESPURNA_MINIMAL_ARDUINO_OTA", "ESPURNA_MINIMAL_WEBUI"}

# -D flags of the envs that do not change the hardware
NON_HARDWARE_FLAGS = re.compile(
    r"^(NOWSAUTH|DISABLE_POSTMORTEM_STACKDUMP|ESPURNA_IMAGE|SECURE_CLIENT|MQTT_LIBRARY|OTA_CLIENT"
    r"|ASYNC_TCP_SSL_ENABLED|ALEXA_SUPPORT|ALEXA_ENABLED|EVENTS_SUPPORT|INFLUXDB_SUPPORT"
    r"|NO_GLOBAL_EEPROM|PIO_FRAMEWORK_\w+|DEBUG_\w+|BEARSSL_\w+)$")

IMAGE_1M = "esp8266-1m"
IMAGE_LIGHT = "esp8266-1m-light"
IMAGE_SENSOR = "esp8266-1m-sensor"
IMAGE_4M = "esp8266-4m"

# features that only the -max image has (the 4m image needs them too for 4 MB boards)
MAX_FEATURES = ("light", "rfb", "ir", "sensor", "analog-button")

# the two 1 MB feature images: PWM light and IR / power meters, sensors, RF (rc-switch and EFM8BB1) and
# analog buttons (A0 is the sensor image's ADC anyway). A board that needs both goes to the light image,
# its sensor image part stays off
LIGHT_FEATURES = ("light", "ir")
SENSOR_FEATURES = ("sensor", "rfb", "analog-button")

# -----------------------------------------------------------------------------
# hardware.h / defaults.h
# -----------------------------------------------------------------------------


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return [re.sub(r"//.*", "", line).rstrip() for line in text.splitlines()]


def parse_hardware(path):
    """{BOARD: {"defines": {name: value}, "guarded": {names}}}, the #if/#elif chain only"""
    with open(path, encoding="utf-8") as f:
        lines = strip_comments(f.read())

    boards = {}
    current = None
    depth = 0
    chain = False
    ifndef_stack = []

    for line in lines:
        s = line.strip()
        if not chain:
            if s.startswith("#if defined(MANUFACTURER)"):
                chain = True
            continue

        m = re.match(r"#\s*(?:el)?if\s+defined\s*\(\s*(\w+)\s*\)\s*$", s)
        if depth == 0 and m and s.startswith("#elif"):
            current = m.group(1)
            boards[current] = {"defines": {}, "guarded": set()}
            continue

        if depth == 0 and s.startswith("#else"):
            current = None
            continue

        m = re.match(r"#\s*(if|ifdef|ifndef)\b\s*(\w*)", s)
        if m:
            depth += 1
            ifndef_stack.append(m.group(2) if m.group(1) == "ifndef" else None)
            continue

        if re.match(r"#\s*endif", s):
            if depth == 0:
                break
            depth -= 1
            ifndef_stack.pop()
            continue

        m = re.match(r"#\s*define\s+(\w+)(?:\s+(.*))?$", s)
        if m and current:
            name, value = m.group(1), (m.group(2) or "").strip()
            entry = boards[current]
            entry["defines"][name] = value
            if ifndef_stack and ifndef_stack[-1] == name:
                entry["guarded"].add(name)

    return boards


def parse_defaults(path):
    out = {}
    with open(path, encoding="utf-8") as f:
        for line in strip_comments(f.read()):
            m = re.match(r"\s*#\s*define\s+(\w+)\s+(.*)$", line)
            if m and m.group(1) not in out:
                out[m.group(1)] = m.group(2).strip()
    return out


def parse_sensor_defaults(path):
    """config/sensors.h values of the board builds, the SENSOR_PINS_FROM_SETTINGS block is skipped"""
    out = {}
    if not os.path.exists(path):
        return out
    depth = 0
    skip_at = None
    with open(path, encoding="utf-8") as f:
        for line in strip_comments(f.read()):
            s = line.strip()
            if re.match(r"#\s*if", s):
                depth += 1
                if skip_at is None and re.match(r"#\s*if\s+SENSOR_PINS_FROM_SETTINGS\b", s):
                    skip_at = depth
                continue
            if re.match(r"#\s*endif", s):
                if skip_at == depth:
                    skip_at = None
                depth -= 1
                continue
            if skip_at is not None:
                continue
            m = re.match(r"#\s*define\s+(\w+)\s+(.*)$", s)
            if m and m.group(1) not in out:
                out[m.group(1)] = m.group(2).strip()
    return out


def number(value):
    """Plain or arithmetic C literal as int / float, None when it is not a number"""
    if value is None:
        return None
    v = value.strip()
    if v in ("GPIO_NONE", "RELAY_NONE"):
        return GPIO_NONE
    if v == "A0":
        return 17  # ESP8266 ADC
    if re.fullmatch(r"0x[0-9a-fA-F]+", v):
        return int(v, 16)
    if v in ("HIGH", "true"):
        return 1
    if v in ("LOW", "false"):
        return 0
    if re.fullmatch(r"[\d\s.*+\-/()]+", v) and re.search(r"\d", v):
        try:
            result = eval(v, {"__builtins__": {}}, {})  # digits and operators only
        except Exception:
            return None
        if isinstance(result, float) and result.is_integer() and "." not in v and "/" not in v:
            result = int(result)
        return result
    return None


def fmt(value):
    if isinstance(value, float):
        text = ("%.4f" % value).rstrip("0").rstrip(".")
        return text
    return str(value)


# -----------------------------------------------------------------------------
# platformio*.ini
# -----------------------------------------------------------------------------


def read_ini(name, path, rev=None):
    """Text of the ini, from the working tree or from the git revision rev (board envs that the
    generic images replaced are gone from the working tree, their history is not)"""
    if rev is None:
        if not os.path.exists(path):
            return None
        with open(path, encoding="utf-8") as f:
            return f.read()

    result = subprocess.run(["git", "-C", CODE, "show", "%s:code/%s" % (rev, name)],
                            capture_output=True, encoding="utf-8")
    return result.stdout if result.returncode == 0 else None


def parse_inis(rev=None):
    """[{env, file, options}] in file order, extends resolved for flash and flags"""
    envs = []
    sections = {}
    for name, path in INIS:
        text = read_ini(name, path, rev)
        if text is None:
            continue
        parser = configparser.ConfigParser(interpolation=None, strict=False)
        parser.read_string(text, source=name)
        for section in parser.sections():
            options = dict(parser[section])
            sections.setdefault(section, {}).update(options)
            if section.startswith("env:"):
                envs.append({"env": section[4:], "file": name, "options": options})

    def chain(section, seen=()):
        options = sections.get(section, {})
        out = [options]
        for parent in re.split(r"[,\s]+", options.get("extends", "").strip()):
            if parent and parent not in seen:
                out.extend(chain(parent, seen + (section,)))
        return out

    for env in envs:
        options_chain = chain("env:" + env["env"])
        env["options"] = options_chain[0]

        flash = None
        for options in options_chain:
            for key in ("board_build.ldscript", "board"):
                m = re.search(r"(?:ldscript|board)_(512k|1m|2m|4m)\b", options.get(key, ""))
                if m:
                    flash = m.group(1)
                    break
            if flash:
                break
        env["flash"] = flash

        flags = []
        for options in options_chain:
            text = " ".join(options.get(key, "") for key in ("build_src_flags", "build_flags"))
            for m in re.finditer(r"-D\s*(\w+)(?:=(\S+))?", text):
                flags.append((m.group(1), (m.group(2) or "1").replace('\\"', '"')))
        env["flags"] = flags

    return envs


# -----------------------------------------------------------------------------
# runtime keys present in the sources
# -----------------------------------------------------------------------------


class Sources:
    def __init__(self, root):
        self.text = []
        for base, _, files in os.walk(root):
            if os.sep + "static" in base or os.sep + "data" in base:
                continue
            for name in files:
                if name.endswith((".cpp", ".h", ".ipp")) and not name.startswith("index."):
                    with open(os.path.join(base, name), encoding="utf-8", errors="replace") as f:
                        self.text.append(f.read())
        self.text = "\n".join(self.text)

        # board-build values of the sensor defines (hardware.h overrides them)
        self.sensor_defaults = parse_sensor_defaults(os.path.join(root, "config", "sensors.h"))
        # ESP8266 core variant SDA / SCL
        for name, pin in (("I2C_SDA_PIN", "4"), ("I2C_SCL_PIN", "5")):
            if self.sensor_defaults.get(name) in (None, "SDA", "SCL"):
                self.sensor_defaults[name] = pin

        self.ldr_types = {}
        ldr = os.path.join(root, "sensors", "LDRSensor.h")
        if os.path.exists(ldr):
            with open(ldr, encoding="utf-8") as f:
                for m in re.finditer(r"#define\s+(LDR_\w+)\s+(\d+)", f.read()):
                    self.ldr_types[m.group(1)] = m.group(2)

        with open(os.path.join(root, "sensors", "DigitalSensor.h"), encoding="utf-8") as f:
            self.has_indexed_digital = "getSetting({keys::Pin, index}" in f.read()

    def has(self, key):
        return ('"%s"' % key) in self.text

    def first(self, *keys):
        for key in keys:
            if self.has(key):
                return key
        return None


# -----------------------------------------------------------------------------
# hardware.h values -> runtime settings
# -----------------------------------------------------------------------------

RELAY_TYPES = {
    "RELAY_TYPE_NORMAL": "normal",
    "RELAY_TYPE_INVERSE": "inverse",
    "RELAY_TYPE_LATCHED": "latched",
    "RELAY_TYPE_LATCHED_INVERSE": "latched-inverse",
}

BUTTON_ACTIONS = {
    "NONE": "none", "TOGGLE": "relay-toggle", "ON": "relay-on", "OFF": "relay-off",
    "AP": "wifi-ap", "RESET": "reset", "FACTORY": "factory",
    "DIM_UP": "bri-inc", "DIM_DOWN": "bri-dec", "DISPLAY_ON": "display-on", "CUSTOM": "custom",
    "FAN_LOW": "fan-low", "FAN_MEDIUM": "fan-medium", "FAN_HIGH": "fan-high",
    "TERMINAL_COMMAND": "term-cmd",
    # no settings value, converted to none by the runtime
    "PULSE": None, "WPS": None, "SMART_CONFIG": None,
}

# hardware.h suffix -> settings key, in the order PRESS, RELEASE, CLICK ...
BUTTON_EVENTS = [
    ("PRESS", "btnPress"), ("RELEASE", "btnRlse"), ("CLICK", "btnClick"),
    ("DBLCLICK", "btnDclk"), ("TRIPLECLICK", "btnTclk"), ("LNGCLICK", "btnLclk"),
    ("LNGLNGCLICK", "btnLLclk"),
]
CLICK_KEYS = ["btnClick", "btnDclk", "btnTclk", "btnLclk", "btnLLclk"]

LED_MODES = {
    "LED_MODE_MANUAL": "manual", "LED_MODE_WIFI": "wifi", "LED_MODE_FOLLOW": "relay",
    "LED_MODE_FOLLOW_INVERSE": "relay-inverse", "LED_MODE_FINDME": "findme",
    "LED_MODE_FINDME_WIFI": "findme-wifi", "LED_MODE_ON": "on", "LED_MODE_OFF": "off",
    "LED_MODE_RELAY": "relays", "LED_MODE_RELAY_WIFI": "relays-wifi",
}

UART_PARITY = {"None": "none", "Even": "even", "Odd": "odd"}

# compile-time-only relay / light / board providers, no generic image has them
SPECIAL_DEFINES = [
    ("RFM69_SUPPORT", "RFM69 gateway (RFM69_SUPPORT), radio module and its own WebUI, compile-time only"),
    ("GARLAND_SUPPORT", "NeoPixel garland (GARLAND_SUPPORT), strip length and pin are compile-time only"),
    ("TUYA_SUPPORT", "Tuya MCU dimmer (TUYA_SUPPORT, LIGHT_PROVIDER_CUSTOM), datapoints are compile-time only"),
    ("IFAN_SUPPORT", "Sonoff iFan02 fan controller (IFAN_SUPPORT, RELAY_PROVIDER_FAN), compile-time only"),
    ("KINGART_CURTAIN_SUPPORT", "KingArt curtain MCU over UART (KINGART_CURTAIN_SUPPORT), compile-time only"),
    ("MCP23S08_SUPPORT", "relays and buttons behind an MCP23S08 SPI expander (MCP23S08_SUPPORT), not in any generic image"),
    ("ENCODER_SUPPORT", "rotary encoder (ENCODER_SUPPORT), pins and channels are compile-time only"),
    ("LIGHTFOX_PORT", "LightFox relays and buttons over UART (FOXEL_LIGHTFOX_DUAL), compile-time only"),
]

SPECIAL_PROVIDERS = {
    "RELAY_PROVIDER_DUAL": "Sonoff Dual relays over UART (RELAY_PROVIDER_DUAL_SUPPORT), not in any generic image",
    "RELAY_PROVIDER_STM": "STM relay board over UART (RELAY_PROVIDER_STM_SUPPORT), not in any generic image",
    "RELAY_PROVIDER_FAN": "fan relay provider (IFAN_SUPPORT), compile-time only",
    "RELAY_PROVIDER_LIGHTFOX": "LightFox relay provider, compile-time only",
    "RELAY_PROVIDER_TUYA": "Tuya MCU relay provider, compile-time only",
}


class Board:
    """Settings of one board (or board variant) as the generic image reads them"""

    def __init__(self, name, defines, defaults, sources):
        self.name = name
        self.d = defines
        self.defaults = defaults
        self.src = sources
        self.sensor_defaults = sources.sensor_defaults
        self.ldr_types = sources.ldr_types
        self.settings = {}
        self.features = set()
        self.needs = []     # [(driver, text)]
        self.special = []   # reasons
        self.notes = []
        self.used_pins = set()
        self.convert()

    # helpers

    def get(self, name, fallback=None):
        if name in self.d:
            return self.d[name]
        if name in self.defaults:
            return self.defaults[name]
        return fallback

    def num(self, name, fallback=None):
        value = number(self.get(name))
        return fallback if value is None else value

    def set(self, key, value):
        self.settings[key] = fmt(value) if not isinstance(value, str) else value

    def need(self, driver, text):
        if (driver, text) not in self.needs:
            self.needs.append((driver, text))

    def pin(self, value):
        if value is not None and value != GPIO_NONE:
            self.used_pins.add(value)
        return value

    def defined_pin(self, name):
        value = number(self.d.get(name))
        if value is None or value == GPIO_NONE:
            return None
        return value

    def key_or_need(self, driver, define, candidates, value, text):
        """Write the first candidate key the sources know, else list it as needed"""
        key = self.src.first(*candidates)
        if key:
            self.set(key, value)
        else:
            self.need(driver, "%s (%s)" % (text, define))

    # conversion

    def convert(self):
        for define, reason in SPECIAL_DEFINES:
            if number(self.d.get(define)):
                self.special.append(reason)

        provider = self.d.get("LIGHT_PROVIDER", "LIGHT_PROVIDER_NONE")
        if provider == "LIGHT_PROVIDER_MY92XX":
            self.special.append("MY92XX bulb (LIGHT_PROVIDER_MY92XX): the ltMy92xx* keys exist, but the light"
                                " provider and MY92XX_COMMAND are chosen by the build, generic images use PWM")
        elif provider == "LIGHT_PROVIDER_CUSTOM" and not number(self.d.get("TUYA_SUPPORT")):
            self.special.append("custom light provider (LIGHT_PROVIDER_CUSTOM), compile-time only")

        self.relays()
        self.buttons()
        self.leds()
        self.light()
        self.rfbridge()
        self.ir()
        self.i2c()
        self.sensors()
        self.uart()

    def relays(self):
        if number(self.d.get("RELAY_SUPPORT", "1")) == 0:
            return
        index = 0
        for n in range(1, 9):
            provider = self.get("RELAY%d_PROVIDER" % n, "RELAY_PROVIDER_GPIO")
            explicit = ("RELAY%d_PROVIDER" % n) in self.d
            pin = self.defined_pin("RELAY%d_PIN" % n)

            if provider == "RELAY_PROVIDER_GPIO":
                if pin is None:
                    break
                if self.d.get("RELAY%d_PIN_TYPE" % n, "GPIO_TYPE_HARDWARE") != "GPIO_TYPE_HARDWARE":
                    self.special.append("relays on %s" % self.d["RELAY%d_PIN_TYPE" % n])
                self.set("relayGpio%d" % index, self.pin(pin))
                self.set("relayType%d" % index,
                         RELAY_TYPES.get(self.get("RELAY%d_TYPE" % n, "RELAY_TYPE_NORMAL"), "normal"))
                self.set("relayProv%d" % index, "gpio")
                reset = self.defined_pin("RELAY%d_RESET_PIN" % n)
                if reset is not None:
                    self.set("relayResetGpio%d" % index, self.pin(reset))
            elif provider == "RELAY_PROVIDER_LIGHT_STATE" and explicit:
                self.set("relayProv%d" % index, "light-state")
                self.features.add("light")
            elif explicit and provider in SPECIAL_PROVIDERS:
                self.special.append(SPECIAL_PROVIDERS[provider])
            else:
                break
            index += 1

        dummy = number(self.d.get("DUMMY_RELAY_COUNT"))
        if dummy:
            self.set("relayDummy", dummy)

        interlock = number(self.d.get("RELAY_DELAY_INTERLOCK"))
        if interlock is not None:
            self.set("relayIlkDelay", interlock)

    def buttons(self):
        index = 0
        for n in range(1, 9):
            provider = self.get("BUTTON%d_PROVIDER" % n, "BUTTON_PROVIDER_GPIO")
            pin = self.defined_pin("BUTTON%d_PIN" % n)

            if provider == "BUTTON_PROVIDER_LIGHTFOX":
                self.special.append("LightFox buttons (BUTTON_PROVIDER_LIGHTFOX), compile-time only")
                continue
            if provider == "BUTTON_PROVIDER_NONE":
                continue
            if pin is None:
                if ("BUTTON%d_PIN" % n) in self.d or n == 1:
                    continue
                break
            if provider == "BUTTON_PROVIDER_ANALOG":
                # resistor ladder on A0, the level of this button (BUTTON_PROVIDER_ANALOG_SUPPORT in the image)
                if not self.src.has("btnLevel"):
                    self.need("button", "analog buttons on A0 (btnProv analog / btnLevel)")
                    continue
                self.features.add("analog-button")
                self.button(n, index, pin)
                self.set("btnProv%d" % index, "analog")
                self.set("btnLevel%d" % index, number(self.get("BUTTON%d_ANALOG_LEVEL" % n, "0")))
                index += 1
                continue
            if self.d.get("BUTTON%d_PIN_TYPE" % n, "GPIO_TYPE_HARDWARE") != "GPIO_TYPE_HARDWARE":
                self.special.append("buttons on %s" % self.d["BUTTON%d_PIN_TYPE" % n])
                continue

            self.button(n, index, pin)
            index += 1

        ac_freq = self.d.get("BUTTON_AC_FREQ")
        if ac_freq is not None:
            self.set("btnAcFreq", number(ac_freq))
        for define, key in (("BUTTON_LNGCLICK_DELAY", "btnLclkDel"), ("BUTTON_LNGLNGCLICK_DELAY", "btnLLclkDel"),
                            ("BUTTON_DEBOUNCE_DELAY", "btnDebDel"), ("BUTTON_REPEAT_DELAY", "btnRepDel")):
            if define in self.d:
                self.set(key, number(self.d[define]))

    def action(self, define):
        value = self.get(define, "BUTTON_ACTION_NONE")
        name = re.sub(r"^BUTTON_(ACTION|MODE)_", "", value)
        return BUTTON_ACTIONS.get(name, None)

    def button(self, n, index, pin):
        bits = set(re.findall(r"BUTTON_[A-Z_]+", self.get("BUTTON%d_CONFIG" % n, "")))
        frequency = "BUTTON_FREQUENCY" in bits
        mode = "pushbutton" if "BUTTON_PUSHBUTTON" in bits else "switch"

        if "BUTTON_DEFAULT_LOW" in bits:
            default = "low"
        elif "BUTTON_DEFAULT_HIGH" in bits:
            default = "high"
        elif "BUTTON_DEFAULT_BOOT" in bits:
            default = "initial"
        else:
            default = "low"

        if "BUTTON_SET_PULLUP" in bits:
            pin_mode = "pull-up"
        elif "BUTTON_SET_PULLDOWN" in bits:
            pin_mode = "pull-down"
        else:
            pin_mode = "default"

        # AC mains switch: the runtime reads it as a switch, idle (no AC) is HIGH
        if frequency:
            mode, default = "switch", "high"

        self.set("btnGpio%d" % index, self.pin(pin))
        self.set("btnMode%d" % index, mode)
        self.set("btnPinMode%d" % index, pin_mode)
        self.set("btnDefVal%d" % index, default)

        relay = number(self.d.get("BUTTON%d_RELAY" % n))
        if relay and relay != GPIO_NONE and number(self.d.get("RELAY_SUPPORT", "1")) != 0:
            self.set("btnRelay%d" % index, relay - 1)

        board = {suffix: self.action("BUTTON%d_%s" % (n, suffix)) for suffix, _ in BUTTON_EVENTS}

        if mode == "switch":
            # same as the runtime does for a switch: press falls back to click, release to press.
            # Switches never generate clicks
            press = board["PRESS"] if board["PRESS"] not in (None, "none") else (board["CLICK"] or "none")
            release = board["RELEASE"] if board["RELEASE"] not in (None, "none") else press
            self.set("btnPress%d" % index, press)
            self.set("btnRlse%d" % index, release)
            for key in CLICK_KEYS:
                self.set("%s%d" % (key, index), "none")
        else:
            # only what differs from the generic image, which has the defaults.h values of this index
            for suffix, key in BUTTON_EVENTS:
                generic = BUTTON_ACTIONS.get(
                    re.sub(r"^BUTTON_(ACTION|MODE)_", "",
                           self.defaults.get("BUTTON%d_%s" % (index + 1, suffix), "BUTTON_ACTION_NONE")))
                value = board[suffix]
                if value != generic:
                    if value is None:
                        self.notes.append("button %d %s action has no settings value, set to none" % (n, suffix))
                        value = "none"
                    self.set("%s%d" % (key, index), value)

        if frequency:
            self.set("btnFreq%d" % index, "1")

        for suffix, key in (("DEBOUNCE_DELAY", "btnDebDel"), ("LNGCLICK_DELAY", "btnLclkDel"),
                            ("LNGLNGCLICK_DELAY", "btnLLclkDel"), ("REPEAT_DELAY", "btnRepDel")):
            value = number(self.d.get("BUTTON%d_%s" % (n, suffix)))
            if value is not None:
                self.set("%s%d" % (key, index), value)

    def leds(self):
        index = 0
        for n in range(1, 9):
            pin = self.defined_pin("LED%d_PIN" % n)
            if pin is None:
                if ("LED%d_PIN" % n) in self.d:
                    continue
                break
            self.set("ledGpio%d" % index, self.pin(pin))
            self.set("ledInv%d" % index, "1" if number(self.d.get("LED%d_PIN_INVERSE" % n)) else "0")
            mode = LED_MODES.get(self.get("LED%d_MODE" % n, "LED_MODE_MANUAL"), "manual")
            self.set("ledMode%d" % index, mode)
            relay = number(self.d.get("LED%d_RELAY" % n))
            if relay is None and mode in ("relay", "relay-inverse"):
                relay = n
            if relay:
                self.set("ledRelay%d" % index, relay - 1)
            index += 1

    def light(self):
        if self.d.get("LIGHT_PROVIDER") != "LIGHT_PROVIDER_DIMMER":
            return
        self.features.add("light")
        for n in range(1, 6):
            pin = self.defined_pin("LIGHT_CH%d_PIN" % n)
            if pin is None:
                break
            self.set("ltDimmerGPIO%d" % (n - 1), self.pin(pin))
            if number(self.d.get("LIGHT_CH%d_INVERSE" % n)):
                self.set("ltInv%d" % (n - 1), "1")
        enable = self.defined_pin("LIGHT_ENABLE_PIN")
        if enable is not None:
            self.set("ltEnableGPIO", self.pin(enable))
        for define, key in (("LIGHT_COLDWHITE_MIRED", "ltColdMired"), ("LIGHT_WARMWHITE_MIRED", "ltWarmMired"),
                            ("LIGHT_STEP", "ltStep")):
            if define in self.d:
                self.set(key, number(self.d[define]))

    def rfbridge(self):
        if not number(self.d.get("RFB_SUPPORT")):
            return
        self.features.add("rfb")
        if self.get("RFB_PROVIDER", "RFB_PROVIDER_RCSWITCH") == "RFB_PROVIDER_EFM8BB1":
            # the generic image builds both providers, rfbProv picks one (rc-switch when not set)
            self.key_or_need("rfbridge", "RFB_PROVIDER", ("rfbProv",), "efm8bb1",
                             "EFM8BB1 (UART) provider of the Sonoff RF Bridge")
            return
        for define, key in (("RFB_RX_PIN", "rfbRX"), ("RFB_TX_PIN", "rfbTX")):
            pin = self.defined_pin(define)
            if pin is not None:
                self.set(key, self.pin(pin))

    def ir(self):
        if not number(self.d.get("IR_SUPPORT")):
            return
        self.features.add("ir")
        rx = self.defined_pin("IR_RX_PIN")
        if rx is not None:
            self.set("irRx", self.pin(rx))
        tx = self.defined_pin("IR_TX_PIN")
        if tx is not None and number(self.d.get("IR_TX_SUPPORT", "1")):
            self.set("irTx", self.pin(tx))
        if number(self.d.get("IR_RX_PRESET")):
            self.key_or_need("ir", "IR_RX_PRESET=%s" % self.d["IR_RX_PRESET"], ("irRxPreset",),
                             number(self.d["IR_RX_PRESET"]), "remote control button preset")

    def i2c(self):
        for define, key in (("I2C_SDA_PIN", "i2cSDA"), ("I2C_SCL_PIN", "i2cSCL")):
            pin = self.defined_pin(define)
            if pin is not None:
                self.set(key, self.pin(pin))

    def sensor_default(self, name, fallback=None):
        """Value of a board build: hardware.h, else config/sensors.h outside the generic-image block"""
        if name in self.d:
            value = number(self.d[name])
        else:
            value = number(self.sensor_defaults.get(name))
        return fallback if value is None else value

    def sensors(self):
        # keys of sensor.cpp load(): generic images default every sensor pin / port / enable value to
        # 'none' (SENSOR_PINS_FROM_SETTINGS), so the entry writes all of them, not only what differs
        d = self.d
        enabled = lambda name: bool(number(d.get(name)))
        out = self.key_or_need

        if enabled("HLW8012_SUPPORT"):
            self.features.add("sensor")
            for define, key in (("HLW8012_SEL_PIN", "hlw8012SEL"), ("HLW8012_CF_PIN", "hlw8012CF"),
                                ("HLW8012_CF1_PIN", "hlw8012CF1")):
                out("hlw8012", define, (key,), self.pin(self.sensor_default(define)), "pin")
            if "HLW8012_SEL_CURRENT" in d:
                out("hlw8012", "HLW8012_SEL_CURRENT=%s" % d["HLW8012_SEL_CURRENT"], ("hlw8012SelCurr",),
                    "1" if number(d["HLW8012_SEL_CURRENT"]) else "0", "SEL level that selects current")
            if "HLW8012_INTERRUPT_ON" in d:
                edges = {"RISING": 1, "FALLING": 2, "CHANGE": 3}
                out("hlw8012", "HLW8012_INTERRUPT_ON=%s" % d["HLW8012_INTERRUPT_ON"], ("hlw8012IntMode",),
                    edges.get(d["HLW8012_INTERRUPT_ON"], 3), "interrupt edge")
            self.hlw8012_ratios()

        for support, define, key, driver in (
                ("CSE7766_SUPPORT", "CSE7766_PORT", "cse7766Port", "cse7766"),
                ("PZEM004T_SUPPORT", "PZEM004T_PORT", "pzemPort", "pzem004t"),
                ("PZEM004TV30_SUPPORT", "PZEM004TV30_PORT", "pzemv30Port", "pzem004tv30"),
                ("V9261F_SUPPORT", "V9261F_PORT", "v9261fPort", "v9261f")):
            if enabled(support):
                self.features.add("sensor")
                port = number(d.get(define))
                out(driver, define, (key,), port if port else 1, "UART port number")

        if enabled("DHT_SUPPORT"):
            self.features.add("sensor")
            out("dht", "DHT_PIN", ("dhtPin",), self.pin(self.sensor_default("DHT_PIN")), "pin")
            if "DHT_TYPE" in d:
                types = {"DHT_CHIP_DHT11": 11, "DHT_CHIP_DHT12": 12, "DHT_CHIP_DHT21": 21, "DHT_CHIP_DHT22": 22,
                         "DHT_CHIP_AM2301": 2301, "DHT_CHIP_SI7021": 7021}
                out("dht", "DHT_TYPE=%s" % d["DHT_TYPE"], ("dhtType",), types.get(d["DHT_TYPE"], 22), "chip type")

        if enabled("DALLAS_SUPPORT"):
            self.features.add("sensor")
            out("dallas", "DALLAS_PIN", ("dallasPin",), self.pin(self.sensor_default("DALLAS_PIN")), "pin")

        if enabled("DIGITAL_SUPPORT"):
            self.features.add("sensor")
            modes = {"INPUT": "default", "INPUT_PULLUP": "pull-up", "INPUT_PULLDOWN": "pull-down",
                     "INPUT_PULLDOWN_16": "pull-down"}
            for n in range(1, 9):
                pin = number(d.get("DIGITAL%d_PIN" % n))
                if pin is None:
                    break
                out("digital", "DIGITAL%d_PIN" % n, ("digitalPin",), self.pin(pin), "pin")
                if "digitalPin" in self.settings:
                    # indexed, as sensors/DigitalSensor.h reads them
                    self.settings["digitalPin%d" % (n - 1)] = self.settings.pop("digitalPin")
                if "DIGITAL%d_PIN_MODE" % n in d:
                    self.set("digitalPinMode%d" % (n - 1), modes.get(d["DIGITAL%d_PIN_MODE" % n], "default"))
                if "DIGITAL%d_DEFAULT_STATE" % n in d:
                    self.set("digitalDefState%d" % (n - 1),
                             "high" if number(d["DIGITAL%d_DEFAULT_STATE" % n]) else "low")
            if not self.src.has_indexed_digital:
                self.need("digital", "DigitalSensor.h reads the unindexed digitalPin / digitalPinMode /"
                          " digitalDefState, the indexed keys written here are not read")

        if enabled("ECH1560_SUPPORT"):
            self.features.add("sensor")
            out("ech1560", "ECH1560_CLK_PIN", ("ech1560CLK",), self.pin(self.sensor_default("ECH1560_CLK_PIN")),
                "pin")
            out("ech1560", "ECH1560_MISO_PIN", ("ech1560MISO",), self.pin(self.sensor_default("ECH1560_MISO_PIN")),
                "pin")
            if "ECH1560_INVERTED" in d:
                out("ech1560", "ECH1560_INVERTED", ("ech1560Inv",), "1" if number(d["ECH1560_INVERTED"]) else "0",
                    "inverted")

        if enabled("ANALOG_SUPPORT"):
            self.features.add("sensor")
            out("analog", "ANALOG_PIN", ("analogPin",), self.sensor_default("ANALOG_PIN", 17), "ADC pin")

        if enabled("NTC_SUPPORT"):
            self.features.add("sensor")
            out("ntc", "NTC_PIN", ("ntcPin",), self.sensor_default("NTC_PIN", 17), "ADC pin")
            for define, key in (("NTC_BETA", "ntcBeta"), ("NTC_R_UP", "ntcRUp"), ("NTC_R_DOWN", "ntcRDown"),
                                ("NTC_R0", "ntcR0")):
                if define in d:
                    out("ntc", define, (key,), number(d[define]), "parameter")
            if "NTC_SAMPLES" in d:
                self.need("ntc", "sample count (NTC_SAMPLES)")

        if enabled("LDR_SUPPORT"):
            self.features.add("sensor")
            out("ldr", "LDR_PIN", ("ldrPin",), self.sensor_default("LDR_PIN", 17), "ADC pin")
            if "LDR_TYPE" in d:
                value = number(self.ldr_types.get(d["LDR_TYPE"], d["LDR_TYPE"]))
                out("ldr", "LDR_TYPE=%s" % d["LDR_TYPE"], ("ldrType",), value, "photocell type")
            if "LDR_ON_GROUND" in d:
                out("ldr", "LDR_ON_GROUND", ("ldrOnGround",), "1" if number(d["LDR_ON_GROUND"]) else "0",
                    "photocell position")
            if "LDR_RESISTOR" in d:
                out("ldr", "LDR_RESISTOR", ("ldrResistor",), number(d["LDR_RESISTOR"]), "resistor")

        if enabled("ADE7953_SUPPORT"):
            self.features.add("sensor")
            out("ade7953", "ADE7953_ADDRESS", ("ade7953Addr",), self.sensor_default("ADE7953_ADDRESS", 0x38),
                "I2C address (0 = off)")

        if enabled("BMX280_SUPPORT"):
            self.features.add("sensor")
            out("bmx280", "BMX280_NUMBER", ("bmx280Number",), self.sensor_default("BMX280_NUMBER", 1),
                "number of sensors (0 = off)")
            if "BMX280_ADDRESS" in d:
                out("bmx280", "BMX280_ADDRESS", ("bmx280Address",), number(d["BMX280_ADDRESS"]), "I2C address")

        if enabled("GEIGER_SUPPORT"):
            self.features.add("sensor")
            out("geiger", "GEIGER_PIN", ("geigerPin",), self.pin(self.sensor_default("GEIGER_PIN")), "pin")

        # I2C drivers need the bus pins, the generic image has none
        if (enabled("ADE7953_SUPPORT") or enabled("BMX280_SUPPORT")) and "i2cSDA" not in self.settings:
            self.set("i2cSDA", self.pin(self.sensor_default("I2C_SDA_PIN", 4)))
            self.set("i2cSCL", self.pin(self.sensor_default("I2C_SCL_PIN", 14)))

    def hlw8012_ratios(self):
        d = self.d
        v_ref, f_osc = 2.43, 3579000.0  # HLW8012 library, V_REF and F_OSC
        current_r = number(d.get("HLW8012_CURRENT_R"))
        voltage_up = number(d.get("HLW8012_VOLTAGE_R_UP"))
        voltage_down = number(d.get("HLW8012_VOLTAGE_R_DOWN"))
        resistors = any(x is not None for x in (current_r, voltage_up, voltage_down))
        current_r = 0.001 if current_r is None else current_r
        voltage_up = 5 * 470000 if voltage_up is None else voltage_up
        voltage_down = 1000 if voltage_down is None else voltage_down
        divider = (voltage_up + voltage_down) / voltage_down

        computed = {
            "HLW8012_CURRENT_RATIO": 1000000.0 * 512.0 * v_ref / current_r / 24.0 / f_osc,
            "HLW8012_VOLTAGE_RATIO": 1000000.0 * 512.0 * v_ref * divider / 2.0 / f_osc,
            "HLW8012_POWER_RATIO": 1000000.0 * 128.0 * v_ref * v_ref * divider / current_r / 48.0 / f_osc,
        }
        for define, key in (("HLW8012_CURRENT_RATIO", "currRatio0"), ("HLW8012_VOLTAGE_RATIO", "voltRatio0"),
                            ("HLW8012_POWER_RATIO", "pwrPRatio0")):
            value = number(d.get(define))
            if value is None and resistors:
                value = round(computed[define], 2)
            if value is not None:
                self.set(key, value)

    def uart(self):
        # UART<n> of hardware.h is uart<n-1>. The RF Bridge UART is for the EFM8BB1 only
        rcswitch_bridge = (number(self.d.get("RFB_SUPPORT"))
                           and self.get("RFB_PROVIDER", "RFB_PROVIDER_RCSWITCH") == "RFB_PROVIDER_RCSWITCH"
                           and not any(number(self.d.get(x)) for x in (
                               "CSE7766_SUPPORT", "PZEM004T_SUPPORT", "PZEM004TV30_SUPPORT", "V9261F_SUPPORT")))
        explicit = False
        for n in (1, 2):
            if rcswitch_bridge and n == 1:
                continue
            for suffix, key in (("TX_PIN", "uartTx"), ("RX_PIN", "uartRx"), ("BAUDRATE", "uartBaud"),
                                ("DATA_BITS", "uartDataBits"), ("STOP_BITS", "uartStopBits")):
                name = "UART%d_%s" % (n, suffix)
                if name in self.d:
                    self.set("%s%d" % (key, n - 1), number(self.d[name]))
                    explicit = explicit or n == 1
            if "UART%d_PARITY" % n in self.d:
                self.set("uartParity%d" % (n - 1), UART_PARITY.get(self.d["UART%d_PARITY" % n], "none"))
            if "UART%d_INVERT" % n in self.d:
                self.set("uartInv%d" % (n - 1), "1" if number(self.d["UART%d_INVERT" % n]) else "0")

        # GPIO1 / GPIO3 used by the board: serial debug (UART0) has to let them go
        if not explicit and (self.used_pins & {1, 3}):
            self.set("uartTx0", GPIO_NONE)
            self.set("uartRx0", GPIO_NONE)
            self.notes.append("GPIO1 / GPIO3 are used by the board, UART0 is turned off (uartTx0 / uartRx0 153 ="
                              " no pin), no serial terminal")


# -----------------------------------------------------------------------------
# devices.json entries and env records
# -----------------------------------------------------------------------------

UPPER_WORDS = {"LED", "RGB", "RGBW", "RGBWW", "RF", "TH", "SV", "IOT", "WIFI", "USA", "EU", "DIN", "PM", "PWM",
               "DHT", "USB", "IR", "AC", "DC", "HW", "LC", "AI", "SH", "ZJ", "WT", "NX", "SP", "ESP", "WS",
               "WP", "SB", "GU", "LSC", "PSH", "STM", "UAP", "ECH", "PZEM", "BMX", "NTC", "MQTT", "ESPIKE"}


def pretty(text):
    words = []
    for word in re.split(r"[_\s]+", text.strip('"')):
        if not word:
            continue
        if word.upper() in UPPER_WORDS or re.search(r"\d", word):
            words.append(word.upper())
        else:
            words.append(word.capitalize())
    return " ".join(words)


def category(env, board, entry):
    text = " " + re.sub(r"[-_]", " ", (env + " " + board.d.get("DEVICE", "").strip('"')).lower()) + " "
    relays = any(k.startswith("relayGpio") for k in entry)
    if "light" in board.features and not relays:
        return "light"
    if "sensor" in board.features and not relays:
        return "sensor"
    if "rfbridge" in text:
        return "misc"
    if re.search(r"strip|xsssa0|ecoplug", text):
        return "strip"
    if re.search(r"plug|socket|steckdose|stecker| s20 | s31 | sp\d+ | p1 | wp3 | swa1 | pow|outlet| b25 ", text):
        return "plug"
    if re.search(r"touch|switch| t1 |ms 104|hwmod|wall| ws1 ", text):
        return "switch"
    return "relay"


def flash_reason(flash):
    if flash == "2m":
        return ("2 MB chip: the 1 MB image fits, but its settings sectors are elsewhere than in the 2 MB layout"
                " (back up /config before the switch)")
    if flash == "4m":
        return "4 MB layout: only the esp8266-4m image keeps its settings"
    return None


def classify(env_name, flash, board):
    """(target, reason)"""
    if board.special:
        return "special", "; ".join(dict.fromkeys(board.special))
    if flash == "512k":
        return "special", "512 KB flash, no generic image fits"

    features = [f for f in MAX_FEATURES if f in board.features]
    names = {"light": "PWM light", "rfb": "RF 433 MHz", "ir": "IR receiver", "sensor": "sensors",
             "analog-button": "analog buttons"}
    what = ", ".join(names[f] for f in features) if features else "relays / buttons / LEDs only"

    if flash == "4m":
        reason = flash_reason(flash)
        if features:
            reason += "; esp8266-4m has to build " + what + " (now relays / buttons / LEDs only)"
        else:
            reason = what + "; " + reason
        return IMAGE_4M, reason

    light = [f for f in features if f in LIGHT_FEATURES]
    sensor = [f for f in features if f in SENSOR_FEATURES]
    target = IMAGE_LIGHT if light else IMAGE_SENSOR if sensor else IMAGE_1M
    reason = what
    if features:
        reason += " -> " + target
    if light and sensor:
        reason += " (NEEDS BOTH: " + ", ".join(names[f] for f in sensor) + " are only in " + IMAGE_SENSOR + ")"
    if flash == "2m":
        reason += "; " + flash_reason(flash)
    elif flash is None:
        reason += "; no env, flash size not known (1 MB assumed)"
    return target, reason


def effective_defines(board_define, board_entry, flags):
    defines = dict(board_entry["defines"])
    extra = []
    for name, value in flags:
        if NON_HARDWARE_FLAGS.match(name) or name == board_define:
            continue
        extra.append((name, value))
        if name not in defines or name in board_entry["guarded"]:
            defines[name] = value
    return defines, extra


def needs_text(board):
    return ["%s: %s" % (driver, text) for driver, text in board.needs]


def build(args):
    hardware = parse_hardware(HARDWARE_H)
    defaults = parse_defaults(DEFAULTS_H)
    sources = Sources(SOURCES)

    ours = set()
    if os.path.exists(GENERIC_IMAGES_MD):
        with open(GENERIC_IMAGES_MD, encoding="utf-8") as f:
            ours = set(re.findall(r"★ `([\w-]+)`", f.read()))

    envs = parse_inis(args.ini_rev)
    records = []
    variants = {}   # (board, extra flags, image) -> entry info

    for env in envs:
        name = env["env"]
        board_define = None
        for flag, _ in env["flags"]:
            if flag in hardware:
                board_define = flag
                break

        record = {
            "env": name,
            "file": env["file"],
            "ours": env["file"] == "platformio_override.ini" or name in ours,
            "board_define": board_define,
            "flash": env["flash"],
        }

        if board_define is None or board_define in GENERIC_BOARDS:
            images = {"esp8266-1m": "generic image esp8266-1m", "esp8266-4m": "generic image esp8266-4m",
                      "esp8266-1m-light": "generic image esp8266-1m-light (PWM light, IR)",
                      "esp8266-1m-sensor": "generic image esp8266-1m-sensor (power meters, sensors, RF)"}
            if name in images:
                reason = images[name] + ", stays"
            elif board_define in ("ESPURNA_MINIMAL_ARDUINO_OTA", "ESPURNA_MINIMAL_WEBUI"):
                reason = "minimal 2-step OTA helper" + (
                    " (esp8266-1m-mini release image), stays" if name == "espurna-minimal-webui-1m"
                    else ", only the 1m one is released")
            else:
                reason = "base env (flash layout / platform), stays"
            record.update(target="keep-generic", reason=reason, needs_runtime_setting=[], device_entry=None)
            records.append(record)
            continue

        defines, extra = effective_defines(board_define, hardware[board_define], env["flags"])
        board = Board(board_define, defines, defaults, sources)
        target, reason = classify(name, env["flash"], board)
        if extra:
            reason = "variant of %s with %s; %s" % (
                board_define, " ".join("%s=%s" % x for x in extra), reason)
        record.update(target=target, reason=reason, needs_runtime_setting=needs_text(board), device_entry=None)
        records.append(record)

        if target != "special":
            key = (board_define, tuple(extra), target)
            variants.setdefault(key, {"envs": [], "board": board, "target": target, "flash": env["flash"],
                                      "extra": extra, "record": []})
            variants[key]["envs"].append(name)
            variants[key]["record"].append(record)

    # boards of hardware.h without an env
    with_env = {key[0] for key in variants} | {r["board_define"] for r in records if r["board_define"]}
    no_env = []
    for board_define, entry in hardware.items():
        if board_define in with_env or board_define in NOT_ESP8266 or board_define in GENERIC_BOARDS:
            continue
        board = Board(board_define, dict(entry["defines"]), defaults, sources)
        target, reason = classify(None, None, board)
        no_env.append({"board_define": board_define, "target": target, "reason": reason,
                       "needs_runtime_setting": needs_text(board)})
        if target != "special":
            variants[(board_define, (), target)] = {"envs": [], "board": board, "target": target, "flash": None,
                                                    "extra": [], "record": []}

    # devices.json entries
    entries = []
    for (board_define, extra, target), info in variants.items():
        board = info["board"]
        manufacturer = board.d.get("MANUFACTURER", '""').strip('"')
        device = board.d.get("DEVICE", '""').strip('"')
        settings = dict(board.settings)
        settings["sysManufacturer"] = manufacturer
        settings["sysDevice"] = device

        notes = ["%s %s%s." % (MARKER, board_define,
                               (" with " + " ".join("-D%s=%s" % x for x in extra)) if extra else "")]
        if info["envs"]:
            notes.append("Build env: %s." % ", ".join(info["envs"]))
        if info["flash"] == "2m":
            notes.append(flash_reason("2m") + ".")
        for note in board.notes:
            notes.append(note[0].upper() + note[1:] + ".")
        sensor = [f for f in SENSOR_FEATURES if f in board.features]
        if target == IMAGE_LIGHT and sensor:
            notes.append("Needs both %s and %s: on %s the %s do not work." % (
                IMAGE_LIGHT, IMAGE_SENSOR, IMAGE_LIGHT,
                " and ".join({"sensor": "sensors", "rfb": "RF 433 MHz", "analog-button": "analog buttons"}[f]
                         for f in sensor)))
        if board.needs:
            notes.append("Not set by this entry (compile-time only in the firmware): "
                         + "; ".join(needs_text(board)) + ".")
        notes.append("Not verified on hardware.")

        entry = {
            "name": pretty(manufacturer) + " " + pretty(device),
            "model": "%s %s" % (manufacturer, device),
            "category": category(info["envs"][0] if info["envs"] else board_define, board, settings),
            "arch": "esp8266",
            "mcu": "ESP8266",
            "image": target,
            "notes": " ".join(notes),
            "settings": settings,
            "_env": info["envs"][0] if info["envs"] else board_define.lower(),
            "_variant": bool(extra),
            "_records": info["record"],
        }
        entries.append(entry)

    # unique names: variants (env with extra hardware flags) get their env in brackets, then the
    # same board on another flash layout
    for only_variants in (True, False):
        by_name = {}
        for entry in entries:
            by_name.setdefault(entry["name"], []).append(entry)
        for name, same in by_name.items():
            if len(same) > 1:
                for entry in same:
                    if entry["_variant"] or not only_variants:
                        entry["name"] = "%s (%s)" % (name, entry["_env"])
    for entry in entries:
        entry.pop("_variant")

    for entry in entries:
        for record in entry.pop("_records"):
            record["device_entry"] = entry["name"]
        entry.pop("_env")

    entries.sort(key=lambda e: e["name"].lower())
    return hardware, defaults, sources, records, no_env, entries


# -----------------------------------------------------------------------------
# templates.mjs PROFILES, the same helpers in Python
# -----------------------------------------------------------------------------


def split_top(text, sep=","):
    out, depth, quote, current = [], 0, None, ""
    for ch in text:
        if quote:
            current += ch
            if ch == quote:
                quote = None
            continue
        if ch in "'\"":
            quote = ch
        elif ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        elif ch == sep and depth == 0:
            out.append(current.strip())
            current = ""
            continue
        current += ch
    if current.strip():
        out.append(current.strip())
    return out


def js_value(text):
    text = text.strip()
    if text.startswith(("'", '"')):
        return text[1:-1]
    if text in ("true", "false"):
        return text == "true"
    if text == "null":
        return None
    if text.startswith("{"):
        return {k.strip(): js_value(v) for k, v in (split_top(item, ":") for item in split_top(text[1:-1]))}
    return int(text)


def js_relay(index, gpio, inverse=False):
    return {"relayGpio%d" % index: gpio, "relayType%d" % index: "inverse" if inverse else "normal",
            "relayProv%d" % index: "gpio"}


def js_button_relay(index, relay):
    return {} if relay is None else {"btnRelay%d" % index: str(relay)}


def js_pushbutton(index, gpio, relay, options=None):
    options = options or {}
    out = {"btnGpio%d" % index: gpio, "btnMode%d" % index: "pushbutton",
           "btnPinMode%d" % index: options.get("pull", "default"), "btnDefVal%d" % index: options.get("defVal", "high")}
    out.update(js_button_relay(index, relay))
    return out


def js_wall_switch(index, gpio, relay):
    out = {"btnGpio%d" % index: gpio, "btnMode%d" % index: "switch", "btnPinMode%d" % index: "pull-up",
           "btnDefVal%d" % index: "initial"}
    out.update(js_button_relay(index, relay))
    out.update({"btnPress%d" % index: "relay-toggle", "btnRlse%d" % index: "relay-toggle"})
    for key in CLICK_KEYS:
        out["%s%d" % (key, index)] = "none"
    return out


def js_ac_switch(index, gpio, relay):
    out = js_wall_switch(index, gpio, relay)
    out.update({"btnPinMode%d" % index: "default", "btnDefVal%d" % index: "high", "btnFreq%d" % index: "1",
                "btnDebDel%d" % index: "100"})
    return out


def js_led(index, gpio, inverse, mode="wifi"):
    return {"ledGpio%d" % index: gpio, "ledInv%d" % index: "1" if inverse else "0", "ledMode%d" % index: mode}


JS_HELPERS = {"relay": js_relay, "pushbutton": js_pushbutton, "wallSwitch": js_wall_switch,
              "acSwitch": js_ac_switch, "led": js_led}


def parse_profiles(path):
    with open(path, encoding="utf-8") as f:
        text = f.read()
    body = re.search(r"export const PROFILES = \[(.*?)\n\];", text, re.S).group(1)
    profiles = []
    for item in split_top(body):
        fields = {}
        for part in split_top(item.strip()[1:-1]):
            key, value = split_top(part, ":")[0], ":".join(split_top(part, ":")[1:])
            fields[key.strip()] = value.strip()
        settings = {}
        for part in split_top(fields["settings"].strip()[1:-1]):
            m = re.match(r"\.\.\.(\w+)\((.*)\)$", part, re.S)
            if m:
                settings.update(JS_HELPERS[m.group(1)](*[js_value(a) for a in split_top(m.group(2))]))
            else:
                key, value = split_top(part, ":")
                settings[key.strip()] = js_value(value)
        profiles.append({"arch": js_value(fields["arch"]), "name": js_value(fields["name"]),
                         "model": js_value(fields["model"]), "settings": settings})
    return profiles


def js_key_set(path, name):
    with open(path, encoding="utf-8") as f:
        text = f.read()
    body = re.search(r"const %s = new Set\(\[(.*?)\]\);" % name, text, re.S).group(1)
    return set(re.findall(r"'(\w+)'", body))


def hardware_keys(path):
    """templates.mjs HARDWARE_KEYS, it includes PIN_KEYS"""
    return js_key_set(path, "PIN_KEYS") | js_key_set(path, "HARDWARE_KEYS")


def has_pins(settings, pin_keys):
    """templates.mjs hasPins(), 153 is GPIO_NONE"""
    return any(re.sub(r"\d+$", "", key) in pin_keys and str(value) not in ("", str(GPIO_NONE))
               for key, value in settings.items())


def self_check(hardware, defaults, sources):
    """PROFILES boards (verified on hardware) against the generator, any chip"""
    by_model = {}
    for board_define, entry in hardware.items():
        d = entry["defines"]
        if "MANUFACTURER" in d and "DEVICE" in d:
            by_model["%s %s" % (d["MANUFACTURER"].strip('"'), d["DEVICE"].strip('"'))] = board_define

    ok = True
    checked = 0
    for profile in parse_profiles(TEMPLATES_MJS):
        board_define = by_model.get(profile["model"])
        if not board_define:
            print("self-check: %s (%s) has no hardware.h entry" % (profile["name"], profile["model"]))
            ok = False
            continue
        board = Board(board_define, dict(hardware[board_define]["defines"]), defaults, sources)
        generated = board.settings
        expected = profile["settings"]
        checked += 1
        if generated != expected:
            ok = False
            print("self-check MISMATCH %s (%s):" % (profile["name"], board_define))
            for key in sorted(set(generated) | set(expected)):
                if generated.get(key) != expected.get(key):
                    print("    %-16s generated=%r PROFILES=%r" % (key, generated.get(key), expected.get(key)))
    print("self-check: %d PROFILES boards, %s" % (checked, "all equal" if ok else "MISMATCH"))
    return ok


# -----------------------------------------------------------------------------


def write_devices(path, entries):
    with open(path, encoding="utf-8") as f:
        current = json.load(f)
    kept = [e for e in current if not str(e.get("notes", "")).startswith(MARKER)]
    out = kept + entries
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(json.dumps(out, indent=4, ensure_ascii=False))
        f.write("\n")
    return len(kept)


def main():
    parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    parser.add_argument("--devices", default=DEVICES_JSON, help="devices.json to merge into")
    parser.add_argument("--envs", default=None, help="write the env classification as JSON here")
    parser.add_argument("--no-write", action="store_true", help="do not touch devices.json")
    parser.add_argument("--no-check", action="store_true", help="skip the PROFILES self-check")
    parser.add_argument("--ini-rev", default=None,
                        help="read platformio*.ini from this git revision (e.g. HEAD) instead of the working tree")
    args = parser.parse_args()

    hardware, defaults, sources, records, no_env, entries = build(args)

    ok = True
    if not args.no_check:
        ok = self_check(hardware, defaults, sources)

    if not args.no_write:
        kept = write_devices(args.devices, entries)
        print("%s: %d hand-made + %d generated entries" % (args.devices, kept, len(entries)))

    if args.envs:
        with open(args.envs, "w", encoding="utf-8", newline="\n") as f:
            json.dump({"envs": records, "boards_without_env": no_env}, f, indent=2, ensure_ascii=False)
            f.write("\n")
        print("%s: %d envs, %d boards without env" % (args.envs, len(records), len(no_env)))

    counts = {}
    for record in records:
        counts[record["target"]] = counts.get(record["target"], 0) + 1
    print("targets: " + ", ".join("%s %d" % item for item in sorted(counts.items())))

    allowed = hardware_keys(TEMPLATES_MJS)
    dropped = sorted({re.sub(r"\d+$", "", key) for e in entries for key in e["settings"]}
                     - allowed)
    if dropped:
        print("keys the WebUI drops (not in templates.mjs HARDWARE_KEYS): " + ", ".join(dropped))
    pin_keys = js_key_set(TEMPLATES_MJS, "PIN_KEYS")
    no_pins = [e["name"] for e in entries if not has_pins(e["settings"], pin_keys)]
    if no_pins:
        print("entries without any hardware pin (hidden by the WebUI search): " + ", ".join(no_pins))

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
