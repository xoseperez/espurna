// -----------------------------------------------------------------------------
// NODEMCUv2 development board
// -----------------------------------------------------------------------------

#if defined(NODEMCUV2)

    #define MANUFACTURER        "NODEMCU"
    #define DEVICE              "LOLIN"
    #define BUTTON_PIN          0
    #define RELAY_PIN           12
    #define LED_PIN             2
    #define LED_PIN_INVERSE     0

// -----------------------------------------------------------------------------
// Itead Studio boards
// -----------------------------------------------------------------------------

#elif defined(SONOFF)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF"
    #define BUTTON_PIN          0
    #define RELAY_PIN           12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0

#elif defined(SONOFF_TH)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_TH"
    #define BUTTON_PIN          0
    #define RELAY_PIN           12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0

#elif defined(SONOFF_POW)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_POW"
    #define BUTTON_PIN          0
    #define RELAY_PIN           12
    #define LED_PIN             15
    #define LED_PIN_INVERSE     1
    #define ENABLE_POW          1

#elif defined(SLAMPHER)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SLAMPHER"
    #define BUTTON_PIN          0
    #define RELAY_PIN           12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0

#elif defined(S20)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "S20"
    #define BUTTON_PIN          0
    #define RELAY_PIN           12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0

// -----------------------------------------------------------------------------
// ESPurna board (still beta)
// -----------------------------------------------------------------------------

#elif defined(ESPURNA)

    #define MANUFACTURER        "TINKERMAN"
    #define DEVICE              "ESPURNA"
    #define BUTTON_PIN          0
    #define RELAY_PIN           12
    #define LED_PIN             13
    #define LED_PIN_INVERSE     0

// -----------------------------------------------------------------------------
// Unknown hardware
// -----------------------------------------------------------------------------

#else
    #error "UNSUPPORTED HARDWARE!"
#endif
