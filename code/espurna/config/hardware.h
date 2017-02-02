// -----------------------------------------------------------------------------
// Configuration HELP
// -----------------------------------------------------------------------------
//
// MANUFACTURER: Name of the manufacturer of the board ("string")
// DEVICE: Name of the device ("string")
// BUTTON#_PIN: GPIO for the n-th button (1-based, up to 4 buttons)
// BUTTON#_RELAY: Relay number that will be bind to the n-th button (1-based)
// BUTTON#_MODE: A mask of options (BUTTON_PUSHBUTTON and BUTTON_SWITCH cannot be together)
//   - BUTTON_PUSHBUTTON: button event is fired when released
//   - BUTTON_SWITCH: button event is fired when pressed or released
//   - BUTTON_DEFAULT_HIGH: there is a pull up in place
//   - BUTTON_SET_PULLUP: set pullup by software
// RELAY#_PIN: GPIO for the n-th relay (1-based, up to 4 relays)
// RELAY#_PIN_INVERSE: Relay has inversed logic (closed or ON when pulled down)
// LED#_PIN: GPIO for the n-th LED (1-based, up to 4 LEDs)
// LED#_PIN_INVERSE: LED has inversed logic (lit when pulled down)

// -----------------------------------------------------------------------------
// Development boards
// -----------------------------------------------------------------------------

#if defined(NODEMCUV2)

    #define MANUFACTURER        "NODEMCU"
    #define DEVICE              "LOLIN"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_SWITCH | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
	#define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

#elif defined(D1_RELAYSHIELD)

    #define MANUFACTURER        "WEMOS"
    #define DEVICE              "D1_MINI"
    #define RELAY1_PIN          5
    #define RELAY1_PIN_INVERSE  0
	#define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// Itead Studio boards
// -----------------------------------------------------------------------------

#elif defined(SONOFF)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(SONOFF_TH)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_TH"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(SONOFF_SV)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_SV"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(SLAMPHER)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SLAMPHER"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(S20)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "S20"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(SONOFF_TOUCH)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_TOUCH"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(SONOFF_POW)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_POW"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            15
    #define LED1_PIN_INVERSE    0
    #define ENABLE_POW          1

#elif defined(SONOFF_DUAL)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_DUAL"
    #define BUTTON1_PIN         0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1
    #undef SERIAL_BAUDRATE
    #define SERIAL_BAUDRATE     19230

#elif defined(SONOFF_4CH)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_4CH"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PIN         9
    #define BUTTON2_RELAY       2
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_PIN         10
    #define BUTTON3_RELAY       3
    #define BUTTON3_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON4_PIN         14
    #define BUTTON4_RELAY       4
    #define BUTTON4_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define RELAY2_PIN          5
    #define RELAY2_PIN_INVERSE  0
    #define RELAY3_PIN          4
    #define RELAY3_PIN_INVERSE  0
    #define RELAY4_PIN          15
    #define RELAY4_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_1CH_INCHING)

    // Note: definitions for this board are based on third party data
    // and have not been fully tested yet. If you have the chance to
    // test them, please report back. Thank you.

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "1CH_INCHING"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PIN         15
    #define BUTTON2_RELAY       0
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    0

    // Special LED that shows pulse mode status
    #define LED_PULSE           14

// -----------------------------------------------------------------------------
// Electrodragon boards
// -----------------------------------------------------------------------------

#elif defined(ESP_RELAY_BOARD)

    #define MANUFACTURER        "ELECTRODRAGON"
    #define DEVICE              "ESP_RELAY_BOARD"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PIN         2
    #define BUTTON2_RELAY       2
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define RELAY2_PIN          13
    #define RELAY2_PIN_INVERSE  0
    #define LED1_PIN            16
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// WorkChoice ecoPlug
// -----------------------------------------------------------------------------

#elif defined(ECOPLUG)

    #define MANUFACTURER        "WORKCHOICE"
    #define DEVICE              "ECOPLUG"
    #define BUTTON1_PIN         13
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          15
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// JanGoe Wifi Relay (https://github.com/JanGoe/esp8266-wifi-relay)
// -----------------------------------------------------------------------------

#elif defined(WIFI_RELAY_NC)

    #define MANUFACTURER        "JANGOE"
    #define DEVICE              "WIFI_RELAY_NC"
    #define BUTTON1_PIN         12
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PIN         13
    #define BUTTON2_RELAY       2
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          2
    #define RELAY1_PIN_INVERSE  1
    #define RELAY2_PIN          14
    #define RELAY2_PIN_INVERSE  1

#elif defined(WIFI_RELAY_NO)

    #define MANUFACTURER        "JANGOE"
    #define DEVICE              "WIFI_RELAY_NO"
    #define BUTTON1_PIN         12
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PIN         13
    #define BUTTON2_RELAY       2
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          2
    #define RELAY1_PIN_INVERSE  0
    #define RELAY2_PIN          14
    #define RELAY2_PIN_INVERSE  0

// -----------------------------------------------------------------------------
// ESPurna board (still beta)
// -----------------------------------------------------------------------------

#elif defined(ESPURNA)

    #define MANUFACTURER        "TINKERMAN"
    #define DEVICE              "ESPURNA"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// WiFi MQTT Relay / Thermostat
// -----------------------------------------------------------------------------

#elif defined(MQTT_RELAY)

    #define MANUFACTURER        "OPENENERGYMONITOR"
    #define DEVICE              "MQTT_RELAY"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            16
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// Unknown hardware
// -----------------------------------------------------------------------------

#else
    #error "UNSUPPORTED HARDWARE!"
#endif
