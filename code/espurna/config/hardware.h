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
// RELAY#_TYPE: Relay can be RELAY_TYPE_NORMAL, RELAY_TYPE_INVERSE or RELAY_TYPE_LATCHED
// RELAY#_LED: LED number that will be bind to the n-th relay (1-based)
// LED#_PIN: GPIO for the n-th LED (1-based, up to 4 LEDs)
// LED#_PIN_INVERSE: LED has inversed logic (lit when pulled down)
//
// Besides, other hardware specific information should be stated here

// -----------------------------------------------------------------------------
// Development boards
// -----------------------------------------------------------------------------

#if defined(NODEMCU_LOLIN)

    // Info
    #define MANUFACTURER        "NODEMCU"
    #define DEVICE              "LOLIN"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

#elif defined(WEMOS_D1_MINI_RELAYSHIELD)

    // Info
    #define MANUFACTURER        "WEMOS"
    #define DEVICE              "D1_MINI_RELAYSHIELD"

    // Buttons
    // No buttons on the D1 MINI alone, but defining it without adding a button doen't create problems
    #define BUTTON1_PIN         0   // Connect a pushbutton between D3 and GND,
                                    // it's the same as using a Wemos one button shield
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// ESPurna
// -----------------------------------------------------------------------------

#elif defined(TINKERMAN_ESPURNA_H06)

    // Info
    #define MANUFACTURER        "TINKERMAN"
    #define DEVICE              "ESPURNA_H06"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_RELAY       1

    #ifdef USE_TOUCH_BUTTON

        // Touch button
        #define BUTTON1_MODE            BUTTON_PUSHBUTTON
        #define BUTTON1_PRESS           BUTTON_MODE_TOGGLE
        #define BUTTON1_CLICK           BUTTON_MODE_NONE
        #define BUTTON1_DBLCLICK        BUTTON_MODE_NONE
        #define BUTTON1_LNGCLICK        BUTTON_MODE_NONE
        #define BUTTON1_LNGLNGCLICK     BUTTON_MODE_NONE

    #else

        // Normal pushbutton
        #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #endif


    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_INVERSE

    // LEDs
    #define LED1_PIN            5
    #define LED1_PIN_INVERSE    0

    // HLW8012
    #define POWER_PROVIDER      POWER_PROVIDER_HLW8012
    #define HLW8012_SEL_PIN     2
    #define HLW8012_CF1_PIN     13
    #define HLW8012_CF_PIN      14

#elif defined(TINKERMAN_ESPURNA_H08)

    // Info
    #define MANUFACTURER        "TINKERMAN"
    #define DEVICE              "ESPURNA_H08"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_RELAY       1

    #ifdef USE_TOUCH_BUTTON

        // Touch button
        #define BUTTON1_MODE            BUTTON_PUSHBUTTON
        #define BUTTON1_PRESS           BUTTON_MODE_TOGGLE
        #define BUTTON1_CLICK           BUTTON_MODE_NONE
        #define BUTTON1_DBLCLICK        BUTTON_MODE_NONE
        #define BUTTON1_LNGCLICK        BUTTON_MODE_NONE
        #define BUTTON1_LNGLNGCLICK     BUTTON_MODE_NONE

    #else

        // Normal pushbutton
        #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #endif

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // HLW8012
    #define POWER_PROVIDER      POWER_PROVIDER_HLW8012
    #define HLW8012_SEL_PIN     5
    #define HLW8012_CF1_PIN     13
    #define HLW8012_CF_PIN      14

// -----------------------------------------------------------------------------
// Itead Studio boards
// -----------------------------------------------------------------------------

#elif defined(ITEAD_SONOFF_BASIC)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_BASIC"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_RF)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_RF"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_TH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_TH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_SV)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_SV"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SLAMPHER)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SLAMPHER"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_S20)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "S20"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_TOUCH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_TOUCH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_POW)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_POW"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            15
    #define LED1_PIN_INVERSE    0

    // HLW8012
    #define POWER_PROVIDER      POWER_PROVIDER_HLW8012
    #define HLW8012_SEL_PIN     5
    #define HLW8012_CF1_PIN     13
    #define HLW8012_CF_PIN      14

#elif defined(ITEAD_SONOFF_DUAL)

    // Info
    #define MANUFACTURER            "ITEAD"
    #define DEVICE                  "SONOFF_DUAL"
    #define SERIAL_BAUDRATE         19230
    #define RELAY_PROVIDER          RELAY_PROVIDER_DUAL
    #define DUMMY_RELAY_COUNT       2
    #define DEBUG_SERIAL_SUPPORT    0
    #define TERMINAL_SUPPORT        0

    // Buttons
    #define BUTTON3_RELAY       1

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_4CH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_4CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9
    #define BUTTON3_PIN         10
    #define BUTTON4_PIN         14

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON4_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON3_RELAY       3
    #define BUTTON4_RELAY       4

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5
    #define RELAY3_PIN          4
    #define RELAY4_PIN          15

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL
    #define RELAY4_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_4CH_PRO)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_4CH_PRO"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9
    #define BUTTON3_PIN         10
    #define BUTTON4_PIN         14

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON4_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON3_RELAY       3
    #define BUTTON4_RELAY       4

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5
    #define RELAY3_PIN          4
    #define RELAY4_PIN          15

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL
    #define RELAY4_TYPE         RELAY_TYPE_NORMAL

    // LEDs
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

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "1CH_INCHING"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_MOTOR)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "MOTOR"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_BNSZ01)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "BNSZ01"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Channels
    #define LIGHT_CHANNELS      1
    #define LIGHT_CH1_PIN       12
    #define LIGHT_CH1_INVERSE   0

#elif defined(ITEAD_SONOFF_RFBRIDGE)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_RFBRIDGE"
    #define SERIAL_BAUDRATE     19200
    #define RELAY_PROVIDER      RELAY_PROVIDER_RFBRIDGE
    #ifndef DUMMY_RELAY_COUNT
    #define DUMMY_RELAY_COUNT   6
    #endif
    #define TERMINAL_SUPPORT    0
    #define TRACK_RELAY_STATUS  0

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_B1)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_B1"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY9192
    #define DUMMY_RELAY_COUNT   1
    #define MY9291_DI_PIN       12
    #define MY9291_DCKI_PIN     14
    #define MY9291_COMMAND      MY9291_COMMAND_DEFAULT
    #define MY9291_CHANNELS     5

#elif defined(ITEAD_SONOFF_LED)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_LED"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Channels
    #define LIGHT_CHANNELS      2
    #define LIGHT_CH1_PIN       12  // Cold white
    #define LIGHT_CH2_PIN       14  // Warm white
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0

#elif defined(ITEAD_SONOFF_T1_1CH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_T1_1CH"

    // Buttons
    #define BUTTON1_PIN         9
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_T1_2CH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_T1_2CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         10

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          4

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_T1_3CH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_T1_3CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9
    #define BUTTON3_PIN         10

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON3_RELAY       3

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5
    #define RELAY3_PIN          4

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// Electrodragon boards
// -----------------------------------------------------------------------------

#elif defined(ELECTRODRAGON_WIFI_IOT)

    // Info
    #define MANUFACTURER        "ELECTRODRAGON"
    #define DEVICE              "WIFI_IOT"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         2

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          13

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            16
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// WorkChoice ecoPlug
// -----------------------------------------------------------------------------

#elif defined(WORKCHOICE_ECOPLUG)

    // Info
    #define MANUFACTURER        "WORKCHOICE"
    #define DEVICE              "ECOPLUG"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          15
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// AI Thinker
// -----------------------------------------------------------------------------

#elif defined(AITHINKER_AI_LIGHT)

    // Info
    #define MANUFACTURER        "AITHINKER"
    #define DEVICE              "AI_LIGHT"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY9192
    #define DUMMY_RELAY_COUNT   1
    #define MY9291_DI_PIN       13
    #define MY9291_DCKI_PIN     15
    #define MY9291_COMMAND      MY9291_COMMAND_DEFAULT
    #define MY9291_CHANNELS     4

// -----------------------------------------------------------------------------
// LED Controller
// -----------------------------------------------------------------------------

#elif defined(MAGICHOME_LED_CONTROLLER)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "LED_CONTROLLER"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // Channels
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       13      // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

#elif defined(MAGICHOME_LED_CONTROLLER_20)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "LED_CONTROLLER_20"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // Channels
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

    #define LIGHT_IR_PIN        4    // IR LED
    #define LIGHT_PROVIDER_EXPERIMENTAL_RGB_ONLY_HSV_IR  0

    // 24 Buttons Set of the IR Remote
    #ifndef IR_BUTTONS_SET
    #define IR_BUTTONS_SET      1
    #endif

    //Remote Buttons SET 1 (for the original Remote shipped with the controller)
    #if IR_BUTTONS_SET == 1
        #define IR_BUTTON_0  0xFF906F // Brightness +
        #define IR_BUTTON_1  0xFFB847 // Brightness -
        #define IR_BUTTON_2  0xFFF807 // OFF
        #define IR_BUTTON_3  0xFFB04F // ON

        #define IR_BUTTON_4  0xFF9867 // RED
        #define IR_BUTTON_5  0xFFD827 // GREEN
        #define IR_BUTTON_6  0xFF8877 // BLUE
        #define IR_BUTTON_7  0xFFA857 // WHITE

        #define IR_BUTTON_8  0xFFE817 // "Red" 1
        #define IR_BUTTON_9  0xFF48B7 // "Green" 1
        #define IR_BUTTON_10 0xFF6897 // "Blue" 1
        #define IR_BUTTON_11 0xFFB24D // FLASH Mode

        #define IR_BUTTON_12 0xFF02FD // "Red" 2
        #define IR_BUTTON_13 0xFF32CD // "Green" 2
        #define IR_BUTTON_14 0xFF20DF // "Blue" 2
        #define IR_BUTTON_15 0xFF00FF // STROBE Mode

        #define IR_BUTTON_16 0xFF50AF // "Red" 3
        #define IR_BUTTON_17 0xFF7887 // "Green" 3
        #define IR_BUTTON_18 0xFF708F // "Blue" 3
        #define IR_BUTTON_19 0xFF58A7 // FADE Mode

        #define IR_BUTTON_20 0xFF38C7 // "Red" 4
        #define IR_BUTTON_21 0xFF28D7 // "Green" 4
        #define IR_BUTTON_22 0xFFF00F // "Blue" 4
        #define IR_BUTTON_23 0xFF30CF // SMOOTH Mode
    #endif

    //Remote Buttons SET 2 (another identical IR Remote shipped with another controller)
    #if IR_BUTTONS_SET == 2
        #define IR_BUTTON_0  0xFF00FF // Brightness +
        #define IR_BUTTON_1  0xFF807F // Brightness -
        #define IR_BUTTON_2  0xFF40BF // OFF
        #define IR_BUTTON_3  0xFFC03F // ON

        #define IR_BUTTON_4  0xFF20DF // RED
        #define IR_BUTTON_5  0xFFA05F // GREEN
        #define IR_BUTTON_6  0xFF609F // BLUE
        #define IR_BUTTON_7  0xFFE01F // WHITE

        #define IR_BUTTON_8  0xFF10EF // "Red" 1
        #define IR_BUTTON_9  0xFF906F // "Green" 1
        #define IR_BUTTON_10 0xFF50AF // "Blue" 1
        #define IR_BUTTON_11 0xFFD02F // FLASH Mode

        #define IR_BUTTON_12 0xFF30CF // "Red" 2
        #define IR_BUTTON_13 0xFFB04F // "Green" 2
        #define IR_BUTTON_14 0xFF708F // "Blue" 2
        #define IR_BUTTON_15 0xFFF00F // STROBE Mode

        #define IR_BUTTON_16 0xFF08F7 // "Red" 3
        #define IR_BUTTON_17 0xFF8877 // "Green" 3
        #define IR_BUTTON_18 0xFF48B7 // "Blue" 3
        #define IR_BUTTON_19 0xFFC837 // FADE Mode

        #define IR_BUTTON_20 0xFF28D7 // "Red" 4
        #define IR_BUTTON_21 0xFFA857 // "Green" 4
        #define IR_BUTTON_22 0xFF6897 // "Blue" 4
        #define IR_BUTTON_23 0xFFE817 // SMOOTH Mode
    #endif

// -----------------------------------------------------------------------------
// HUACANXING H801 & H802
// -----------------------------------------------------------------------------

#elif defined(HUACANXING_H801)

    // Info
    #define MANUFACTURER        "HUACANXING"
    #define DEVICE              "H801"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            5
    #define LED1_PIN_INVERSE    1

    // Channels
    #define LIGHT_CHANNELS      5
    #define LIGHT_CH1_PIN       15      // RED
    #define LIGHT_CH2_PIN       13      // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       14      // WHITE1
    #define LIGHT_CH5_PIN       4       // WHITE2
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0
    #define LIGHT_CH5_INVERSE   0

#elif defined(HUACANXING_H802)

    // Info
    #define MANUFACTURER        "HUACANXING"
    #define DEVICE              "H802"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Channels
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       14      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

// -----------------------------------------------------------------------------
// Jan Goedeke Wifi Relay
// https://github.com/JanGoe/esp8266-wifi-relay
// -----------------------------------------------------------------------------

#elif defined(JANGOE_WIFI_RELAY_NC)

    // Info
    #define MANUFACTURER        "JANGOE"
    #define DEVICE              "WIFI_RELAY_NC"

    // Buttons
    #define BUTTON1_PIN         12
    #define BUTTON2_PIN         13

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          2
    #define RELAY2_PIN          14

    #define RELAY1_TYPE         RELAY_TYPE_INVERSE
    #define RELAY2_TYPE         RELAY_TYPE_INVERSE

#elif defined(JANGOE_WIFI_RELAY_NO)

    // Info
    #define MANUFACTURER        "JANGOE"
    #define DEVICE              "WIFI_RELAY_NO"

    // Buttons
    #define BUTTON1_PIN         12
    #define BUTTON2_PIN         13

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          2
    #define RELAY2_PIN          14

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

// -----------------------------------------------------------------------------
// Jorge García Wifi+Relays Board Kit
// https://www.tindie.com/products/jorgegarciadev/wifi--relays-board-kit
// https://github.com/jorgegarciadev/wifikit
// -----------------------------------------------------------------------------

#elif defined(JORGEGARCIA_WIFI_RELAYS)

    // Info
    #define MANUFACTURER        "JORGEGARCIA"
    #define DEVICE              "WIFI_RELAYS"

    // Relays
    #define RELAY1_PIN          0
    #define RELAY2_PIN          2

    #define RELAY1_TYPE         RELAY_TYPE_INVERSE
    #define RELAY2_TYPE         RELAY_TYPE_INVERSE

// -----------------------------------------------------------------------------
// WiFi MQTT Relay / Thermostat
// -----------------------------------------------------------------------------

#elif defined(OPENENERGYMONITOR_MQTT_RELAY)

    // Info
    #define MANUFACTURER        "OPENENERGYMONITOR"
    #define DEVICE              "MQTT_RELAY"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            16
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// WiOn 50055 Indoor Wi-Fi Wall Outlet & Tap
// https://rover.ebay.com/rover/1/711-53200-19255-0/1?icep_id=114&ipn=icep&toolid=20004&campid=5338044841&mpre=http%3A%2F%2Fwww.ebay.com%2Fitm%2FWiOn-50050-Indoor-Wi-Fi-Outlet-Wireless-Switch-Programmable-Timer-%2F263112281551
// https://rover.ebay.com/rover/1/711-53200-19255-0/1?icep_id=114&ipn=icep&toolid=20004&campid=5338044841&mpre=http%3A%2F%2Fwww.ebay.com%2Fitm%2FWiOn-50055-Indoor-Wi-Fi-Wall-Tap-Monitor-Energy-Usage-Wireless-Smart-Switch-%2F263020837777
// -----------------------------------------------------------------------------

#elif defined(WION_50055)

    // Currently untested, does not support energy monitoring

    // Info
    #define MANUFACTURER        "WION"
    #define DEVICE              "50055"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          15
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// EX-Store Wifi Relay v3.1
// https://ex-store.de/ESP8266-WiFi-Relay-V31
// -----------------------------------------------------------------------------

#elif defined(EXS_WIFI_RELAY_V31)

    // Untested

    // Info
    #define MANUFACTURER        "EXS"
    #define DEVICE              "WIFI_RELAY_V31"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          13
    #define RELAY1_TYPE         RELAY_TYPE_LATCHED
    #define RELAY1_RESET_PIN    12

// -----------------------------------------------------------------------------
// V9261F
// -----------------------------------------------------------------------------

#elif defined(GENERIC_V9261F)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "V9261F"

    // V9261F
    #define POWER_PROVIDER      POWER_PROVIDER_V9261F
    #define V9261F_PIN          2
    #define V9261F_PIN_INVERSE  1

// -----------------------------------------------------------------------------
// ECH1560
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ECH1560)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ECH1560"

    // ECH1560
    #define POWER_PROVIDER      POWER_PROVIDER_ECH1560
    #define ECH1560_CLK_PIN     4
    #define ECH1560_MISO_PIN    5
    #define ECH1560_INVERTED    0

// -----------------------------------------------------------------------------
// ESPLive
// https://github.com/ManCaveMade/ESP-Live
// -----------------------------------------------------------------------------

#elif defined(MANCAVEMADE_ESPLIVE)

    // Info
    #define MANUFACTURER        "MANCAVEMADE"
    #define DEVICE              "ESPLIVE"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON2_PIN         5

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          13

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // DB18B20
    #define DS18B20_SUPPORT             1
    #define DS18B20_PIN                 2
    #define DS18B20_UPDATE_INTERVAL     5000
    #define DS18B20_UPDATE_ON_CHANGE    1.0

// -----------------------------------------------------------------------------
// QuinLED
// http://blog.quindorian.org/2017/02/esp8266-led-lighting-quinled-v2-6-pcb.html
// -----------------------------------------------------------------------------

#elif defined(INTERMITTECH_QUINLED)

    // Info
    #define MANUFACTURER        "INTERMITTECH"
    #define DEVICE              "QUINLED"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            5
    #define LED1_PIN_INVERSE    1

    // Channels
    #define LIGHT_CHANNELS      2
    #define LIGHT_CH1_PIN       0
    #define LIGHT_CH2_PIN       2
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0

// -----------------------------------------------------------------------------
// Arilux AL-LC06
// -----------------------------------------------------------------------------

#elif defined(ARILUX_AL_LC06)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL-LC06"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Channels
    #define LIGHT_CHANNELS      5
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       14      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE1
    #define LIGHT_CH5_PIN       5       // WHITE2
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0
    #define LIGHT_CH5_INVERSE   0

// -----------------------------------------------------------------------------
// XENON SM-PW701U
// -----------------------------------------------------------------------------

#elif defined(XENON_SM_PW702U)

    // Info
    #define MANUFACTURER        "XENON"
    #define DEVICE              "SM_PW702U"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// AUTHOMETION LYT8266
// https://authometion.com/shop/en/home/13-lyt8266.html
// -----------------------------------------------------------------------------

#elif defined(AUTHOMETION_LYT8266)

    // Info
    #define MANUFACTURER        "AUTHOMETION"
    #define DEVICE              "LYT8266"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Channels
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       13      // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       2       // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

    #define LIGHT_ENABLE_PIN    15

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
#define BUTTON2_DBLCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_DBLCLICK
#define BUTTON3_DBLCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_DBLCLICK
#define BUTTON4_DBLCLICK    BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_LNGCLICK
#define BUTTON1_LNGCLICK    BUTTON_MODE_RESET
#endif
#ifndef BUTTON2_LNGCLICK
#define BUTTON2_LNGCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_LNGCLICK
#define BUTTON3_LNGCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_LNGCLICK
#define BUTTON4_LNGCLICK    BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_LNGLNGCLICK
#define BUTTON1_LNGLNGCLICK BUTTON_MODE_FACTORY
#endif
#ifndef BUTTON2_LNGLNGCLICK
#define BUTTON2_LNGLNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_LNGLNGCLICK
#define BUTTON3_LNGLNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_LNGLNGCLICK
#define BUTTON4_LNGLNGCLICK BUTTON_MODE_NONE
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

#ifndef RELAY1_RESET_PIN
#define RELAY1_RESET_PIN      0
#endif
#ifndef RELAY2_RESET_PIN
#define RELAY2_RESET_PIN      0
#endif
#ifndef RELAY3_RESET_PIN
#define RELAY3_RESET_PIN      0
#endif
#ifndef RELAY4_RESET_PIN
#define RELAY4_RESET_PIN      0
#endif

#ifndef RELAY1_DELAY_ON
#define RELAY1_DELAY_ON       0
#endif
#ifndef RELAY2_DELAY_ON
#define RELAY2_DELAY_ON       0
#endif
#ifndef RELAY3_DELAY_ON
#define RELAY3_DELAY_ON       0
#endif
#ifndef RELAY4_DELAY_ON
#define RELAY4_DELAY_ON       0
#endif

#ifndef RELAY1_DELAY_OFF
#define RELAY1_DELAY_OFF      0
#endif
#ifndef RELAY2_DELAY_OFF
#define RELAY2_DELAY_OFF      0
#endif
#ifndef RELAY3_DELAY_OFF
#define RELAY3_DELAY_OFF      0
#endif
#ifndef RELAY4_DELAY_OFF
#define RELAY4_DELAY_OFF      0
#endif

#ifndef RELAY1_LED
#define RELAY1_LED          0
#endif
#ifndef RELAY2_LED
#define RELAY2_LED          0
#endif
#ifndef RELAY3_LED
#define RELAY3_LED          0
#endif
#ifndef RELAY4_LED
#define RELAY4_LED          0
#endif

// Needed for ESP8285 boards under Windows using PlatformIO (?)
#ifndef BUTTON_PUSHBUTTON
#define BUTTON_PUSHBUTTON   0
#define BUTTON_SWITCH       1
#define BUTTON_DEFAULT_HIGH 2
#define BUTTON_SET_PULLUP   4
#endif

// Does the board track the relay status?
#ifndef TRACK_RELAY_STATUS
#define TRACK_RELAY_STATUS  1
#endif

// Serial baudrate
#ifndef SERIAL_BAUDRATE
#define SERIAL_BAUDRATE         115200
#endif

// Relay providers
#ifndef RELAY_PROVIDER
#define RELAY_PROVIDER          RELAY_PROVIDER_RELAY
#endif

// Light provider
#ifndef LIGHT_PROVIDER
#define LIGHT_PROVIDER          LIGHT_PROVIDER_NONE
#endif
