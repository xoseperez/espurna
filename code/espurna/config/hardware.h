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
    #define BUTTON1_LNGCLICK    BUTTON_MODE_PULSE
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
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
    #define BUTTON3_RELAY       1
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

    // The inching functionality is managed by a misterious IC in the board.
    // You cannot control the inching button and functionality from the ESP8266
    // Besides, enabling the inching functionality using the hardware button
    // will result in the relay switching on and off continuously.
    // Fortunately the unkown IC keeps memory of the hardware inching status
    // so you can just disable it and forget. The inching LED must be lit.
    // You can still use the pulse options from the web interface
    // without problem.

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "1CH_INCHING"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_MOTOR)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "MOTOR"
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define RELAY1_PIN          12
    #define RELAY1_PIN_INVERSE  0
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

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
// Jan Goedeke Wifi Relay
// https://github.com/JanGoe/esp8266-wifi-relay
// -----------------------------------------------------------------------------

#elif defined(WIFI_RELAY_NC)

    #define MANUFACTURER        "JAN_GOEDEKE"
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

    #define MANUFACTURER        "JAN_GOEDEKE"
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
// Jorge García Wifi+Relays Board Kit
// https://www.tindie.com/products/jorgegarciadev/wifi--relays-board-kit
// https://github.com/jorgegarciadev/wifikit
// -----------------------------------------------------------------------------

#elif defined(WIFI_RELAYS_BOARD_KIT)

    #define MANUFACTURER        "JORGE_GARCIA"
    #define DEVICE              "WIFI_RELAYS_BOARD_KIT"
    #define RELAY1_PIN          0
    #define RELAY1_PIN_INVERSE  1
    #define RELAY2_PIN          2
    #define RELAY2_PIN_INVERSE  1

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

// -----------------------------------------------------------------------------
// Default values
// -----------------------------------------------------------------------------

#ifndef BUTTON1_PRESS
#define BUTTON1_PRESS       BUTTON_MODE_NONE
#endif
#ifndef BUTTON2_PRESS
#define BUTTON2_PRESS       BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_PRESS
#define BUTTON3_PRESS       BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_PRESS
#define BUTTON4_PRESS       BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_CLICK
#define BUTTON1_CLICK       BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON2_CLICK
#define BUTTON2_CLICK       BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON3_CLICK
#define BUTTON3_CLICK       BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON4_CLICK
#define BUTTON4_CLICK       BUTTON_MODE_TOGGLE
#endif

#ifndef BUTTON1_DBLCLICK
#define BUTTON1_DBLCLICK    BUTTON_MODE_AP
#endif
#ifndef BUTTON2_DBLCLICK
#define BUTTON2_DBLCLICK    BUTTON_MODE_AP
#endif
#ifndef BUTTON3_DBLCLICK
#define BUTTON3_DBLCLICK    BUTTON_MODE_AP
#endif
#ifndef BUTTON4_DBLCLICK
#define BUTTON4_DBLCLICK    BUTTON_MODE_AP
#endif

#ifndef BUTTON1_LNGCLICK
#define BUTTON1_LNGCLICK    BUTTON_MODE_RESET
#endif
#ifndef BUTTON2_LNGCLICK
#define BUTTON2_LNGCLICK    BUTTON_MODE_RESET
#endif
#ifndef BUTTON3_LNGCLICK
#define BUTTON3_LNGCLICK    BUTTON_MODE_RESET
#endif
#ifndef BUTTON4_LNGCLICK
#define BUTTON4_LNGCLICK    BUTTON_MODE_RESET
#endif

#ifndef BUTTON1_RELAY
#define BUTTON1_RELAY       0
#endif
#ifndef BUTTON2_RELAY
#define BUTTON2_RELAY       0
#endif
#ifndef BUTTON3_RELAY
#define BUTTON3_RELAY       0
#endif
#ifndef BUTTON4_RELAY
#define BUTTON4_RELAY       0
#endif
