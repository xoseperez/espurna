//--------------------------------------------------------------------------------
// HARDWARE
// This setting is normally provided by PlatformIO
// Uncomment the appropiate line to build from the Arduino IDE
//--------------------------------------------------------------------------------

//#define NODEMCUV2
//#define SONOFF
//#define SONOFF_TH
//#define SONOFF_POW
//#define SONOFF_DUAL
//#define SONOFF_4CH
//#define SLAMPHER
//#define S20
//#define ESP_RELAY_BOARD
//#define ECOPLUG
//#define ESPURNA

// -----------------------------------------------------------------------------
// NODEMCUv2 development board
// -----------------------------------------------------------------------------

#if defined(NODEMCUV2)

    #define MANUFACTURER        "NODEMCU"
    #define DEVICE              "LOLIN"
    #define BUTTON1_PIN         0
    #define RELAY1_PIN          12
    #define LED_PIN             2
    #define LED_PIN_INVERSE     0

// -----------------------------------------------------------------------------
// Itead Studio boards
// -----------------------------------------------------------------------------

#elif defined(SONOFF)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF"
    #define BUTTON1_PIN         0
    #define RELAY1_PIN          12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0

#elif defined(SONOFF_TH)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_TH"
    #define BUTTON1_PIN         0
    #define RELAY1_PIN          12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0

#elif defined(SONOFF_TOUCH)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_TOUCH"
    #define BUTTON1_PIN         0
    #define RELAY1_PIN          12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     1

#elif defined(SONOFF_POW)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_POW"
    #define BUTTON1_PIN         0
    #define RELAY1_PIN          12
    #define LED_PIN             15
    #define LED_PIN_INVERSE     1
    #define ENABLE_POW          1

#elif defined(SONOFF_DUAL)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_DUAL"
    #define BUTTON1_PIN         0
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0
    #undef SERIAL_BAUDRATE
    #define SERIAL_BAUDRATE     19230

#elif defined(SONOFF_4CH)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_4CH"
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9
    #define BUTTON3_PIN         10
    #define BUTTON4_PIN         14
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5
    #define RELAY3_PIN          4
    #define RELAY4_PIN          15
    #define LED_PIN             13
    #define LED_PIN_INVERSE     1

#elif defined(SONOFF_SV)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_SV"
    #define BUTTON1_PIN         0
    #define RELAY1_PIN          12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     1

#elif defined(SLAMPHER)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SLAMPHER"
    #define BUTTON1_PIN         0
    #define RELAY1_PIN          12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0

#elif defined(S20)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "S20"
    #define BUTTON1_PIN         0
    #define RELAY1_PIN          12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0

// -----------------------------------------------------------------------------
// Electrodragon boards
// -----------------------------------------------------------------------------

#elif defined(ESP_RELAY_BOARD)

    #define MANUFACTURER        "ELECTRODRAGON"
    #define DEVICE              "ESP_RELAY_BOARD"
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         2
    #define RELAY1_PIN          12
    #define RELAY2_PIN          13
    #define LED_PIN             16
    #define LED_PIN_INVERSE     1

// -----------------------------------------------------------------------------
// WorkChoice ecoPlug
// -----------------------------------------------------------------------------

#elif defined(ECOPLUG)

    #define MANUFACTURER        "WORKCHOICE"
    #define DEVICE              "ECOPLUG"
    #define BUTTON1_PIN         13
    #define RELAY_PIN           15
    #define LED_PIN             2
    #define LED_PIN_INVERSE     1

// -----------------------------------------------------------------------------
// ESPurna board (still beta)
// -----------------------------------------------------------------------------

#elif defined(ESPURNA)

    #define MANUFACTURER        "TINKERMAN"
    #define DEVICE              "ESPURNA"
    #define BUTTON1_PIN         0
    #define RELAY1_PIN          12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0

// -----------------------------------------------------------------------------
// Unknown hardware
// -----------------------------------------------------------------------------

#else
    #error "UNSUPPORTED HARDWARE!"
#endif
