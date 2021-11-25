// -----------------------------------------------------------------------------
// Configuration HELP
// -----------------------------------------------------------------------------
//
// Required:
//
// MANUFACTURER: Name of the manufacturer of the board ("string")
// DEVICE: Name of the device ("string")
//
// For example, some configuration options for BUTTON, RELAY and LED modules.
// See general.h, defaults.h and types.h for all of the available flags.
//
// BUTTON[1-8]_CONFIG: Configuration mask (note that BUTTON_PUSHBUTTON and BUTTON_SWITCH cannot be together)
//   - BUTTON_PUSHBUTTON: button event is fired when released
//   - BUTTON_SWITCH: button event is fired when pressed or released
//   - BUTTON_DEFAULT_HIGH: there is a pull up in place, HIGH is default state
//   - BUTTON_DEFAULT_LOW: there is a pull down in place, LOW is default state
//   - BUTTON_DEFAULT_BOOT: use state that we read on firmware boot
//   - BUTTON_SET_PULLUP: set pullup by software
//   - BUTTON_SET_PULLDOWN: set pulldown by software (esp8266: only GPIO16)
// BUTTON[1-8]_... actions: Check types.h for BUTTONx_ACTION_...
// BUTTON[1-8]_PIN: GPIO for the n-th button (1-based, up to 8 buttons)
// BUTTON[1-8]_PROVIDER: Check types.h for the BUTTON_PROVIDER_...
// BUTTON[1-8]_RELAY: Relay number that will be bound to the n-th button (when ACTION is set to ON, OFF or TOGGLE; 1-based)
//
// RELAY[1-8]_PIN: GPIO for the n-th relay (1-based, up to 8 relays)
// RELAY[1-8]_TYPE: Relay can be RELAY_TYPE_NORMAL, RELAY_TYPE_INVERSE, RELAY_TYPE_LATCHED or RELAY_TYPE_LATCHED_INVERSE
//
// LED[1-8]_PIN: GPIO for the n-th LED (1-based, up to 8 LEDs)
// LED[1-8]_PIN_INVERSE: LED has inversed logic (lit when pulled down)
// LED[1-8]_MODE: Check types.h for LED_MODE_...
// LED[1-8]_RELAY: Linked relay (1-based)
//
// Besides, other hardware specific information should be stated here

#pragma once

// -----------------------------------------------------------------------------
// Custom hardware
// -----------------------------------------------------------------------------

#if defined(MANUFACTURER) and defined(DEVICE)

    // user has defined custom hardware, no need to check anything else

// -----------------------------------------------------------------------------
// ESPurna Core
// -----------------------------------------------------------------------------

#elif defined(ESPURNA_CORE)

    // This is a special device targeted to generate a light-weight binary image
    // meant to be able to do two-step-updates:
    // https://github.com/xoseperez/espurna/wiki/TwoStepUpdates

    // Info
    #define MANUFACTURER            "ESPURNA"
    #define DEVICE                  "CORE"

    // Disable non-core modules
    #define ALEXA_SUPPORT           0
    #define API_SUPPORT             0
    #define DEBUG_SERIAL_SUPPORT    0
    #define DEBUG_WEB_SUPPORT       0
    #define DOMOTICZ_SUPPORT        0
    #define HOMEASSISTANT_SUPPORT   0
    #define I2C_SUPPORT             0
    #define MQTT_SUPPORT            0
    #define NTP_SUPPORT             0
    #define RPN_RULES_SUPPORT       0
    #define SCHEDULER_SUPPORT       0
    #define SENSOR_SUPPORT          0
    #define THINGSPEAK_SUPPORT      0
    #define WEB_SUPPORT             0

    #define DEBUG_TELNET_SUPPORT    1
    #define TELNET_AUTHENTICATION   0
    #define TELNET_STA              1

    // Extra light-weight image
    #define BUTTON_SUPPORT          0 // don't need / have buttons
    #define LED_SUPPORT             0 // don't need wifi indicator
    #define RELAY_SUPPORT           0 // don't need to preserve pin state between resets
    //#define OTA_ARDUINOOTA_SUPPORT  0 // when only using the `ota` command
    //#define OTA_WEB_SUPPORT         0 //
    //#define MDNS_SERVER_SUPPORT     0 //
    //#define TELNET_SUPPORT          0 // when only using espota.py
    //#define TERMINAL_SUPPORT        0 //

#elif defined(ESPURNA_CORE_WEBUI)

    // This is a special device with no specific hardware
    // with the basics to easily upgrade it to a device-specific image

    // Info
    #define MANUFACTURER            "ESPURNA"
    #define DEVICE                  "CORE_WEBUI"

    // Disable non-core modules
    #define ALEXA_SUPPORT           0
    #define API_SUPPORT             0
    #define DEBUG_SERIAL_SUPPORT    0
    #define DEBUG_WEB_SUPPORT       0
    #define DOMOTICZ_SUPPORT        0
    #define HOMEASSISTANT_SUPPORT   0
    #define I2C_SUPPORT             0
    #define MQTT_SUPPORT            0
    #define NTP_SUPPORT             0
    #define SCHEDULER_SUPPORT       0
    #define SENSOR_SUPPORT          0
    #define THINGSPEAK_SUPPORT      0

    // Small webpage to upload the .bin
    #define MDNS_SERVER_SUPPORT     0
    #define WEB_SUPPORT             0
    #define OTA_ARDUINOOTA_SUPPORT  0
    #define OTA_WEB_SUPPORT         1

    // Keep the generic uploader
    #define DEBUG_TELNET_SUPPORT    1
    #define TELNET_AUTHENTICATION   0
    #define TELNET_STA              1

    // Extra light-weight image
    #define BUTTON_SUPPORT          0 // don't need / have buttons
    #define LED_SUPPORT             0 // don't need wifi indicator
    #define RELAY_SUPPORT           0 // don't need to preserve pin state between resets
    //#define TELNET_SUPPORT          0
    //#define TERMINAL_SUPPORT        0

// -----------------------------------------------------------------------------
// Development boards
// -----------------------------------------------------------------------------

#elif defined(NODEMCU_LOLIN)

    // Info
    #define MANUFACTURER        "NODEMCU"
    #define DEVICE              "LOLIN"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Hidden button will enter AP mode if dblclick and reset the device when long-long-clicked
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // Light
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

#elif defined(NODEMCU_BASIC)
    // Info
    // Generic NodeMCU Board without any buttons or relays connected.
    #define MANUFACTURER        "NODEMCU"
    #define DEVICE              "BASIC"

#elif defined(WEMOS_D1_MINI)

    // Info
    #define MANUFACTURER        "WEMOS"
    #define DEVICE              "D1_MINI"

    // Buttons
    // No buttons on the D1 MINI alone, but defining it without adding a button doen't create problems
    #define BUTTON1_PIN         0   // Connect a pushbutton between D3 and GND,
                                    // it's the same as using a Wemos one button shield
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    #define I2C_SDA_PIN         4  // D2
    #define I2C_SCL_PIN         5  // D1

#elif defined(WEMOS_D1_MINI_RELAYSHIELD)

    // Info
    #define MANUFACTURER        "WEMOS"
    #define DEVICE              "D1_MINI_RELAYSHIELD"

    // Buttons
    // No buttons on the D1 MINI alone, but defining it without adding a button doen't create problems
    #define BUTTON1_PIN         0   // Connect a pushbutton between D3 and GND,
                                    // it's the same as using a Wemos one button shield
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LED
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // When Wemos relay shield is connected GPIO5 (D1) is used for relay,
    // so I2C must be remapped to other pins
    #define I2C_SDA_PIN         12  // D6
    #define I2C_SCL_PIN         14  // D5

#elif defined(WEMOS_D1_TARPUNA_SHIELD)

    // Info
    #define MANUFACTURER        "WEMOS"
    #define DEVICE              "D1_TARPUNA_SHIELD"

    // Relays
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    #define DHT_SUPPORT         1
    #define DHT_PIN             12

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

    // Normal pushbutton
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_INVERSE

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
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

    // Normal pushbutton
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     5
    #define HLW8012_CF1_PIN     13
    #define HLW8012_CF_PIN      14

#elif defined(TINKERMAN_ESPURNA_SWITCH)

    // Info
    #define MANUFACTURER        "TINKERMAN"
    #define DEVICE              "ESPURNA_SWITCH"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_RELAY       1

    // Touch button
    #define BUTTON1_CONFIG          BUTTON_PUSHBUTTON
    #define BUTTON1_PRESS           BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK           BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK        BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK        BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK     BUTTON_ACTION_NONE

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_INVERSE

// Check http://tinkerman.cat/rfm69-wifi-gateway/
#elif defined(TINKERMAN_RFM69GW)

    // Info
    #define MANUFACTURER                "TINKERMAN"
    #define DEVICE                      "RFM69GW"

    // Buttons
    #define BUTTON1_PIN                 0
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // RFM69GW
    #define RFM69_SUPPORT               1

    // Disable non-core modules
    #define ALEXA_SUPPORT               0
    #define DOMOTICZ_SUPPORT            0
    #define HOMEASSISTANT_SUPPORT       0
    #define I2C_SUPPORT                 0
    #define SCHEDULER_SUPPORT           0
    #define SENSOR_SUPPORT              0
    #define THINGSPEAK_SUPPORT          0

// -----------------------------------------------------------------------------
// Itead Studio boards
// -----------------------------------------------------------------------------

#elif defined(ITEAD_SONOFF_BASIC)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_BASIC"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1
    #define BUTTON2_PIN         14
    #define BUTTON2_CONFIG      BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON2_RELAY       1

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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1
    #define BUTTON2_PIN         14
    #define BUTTON2_CONFIG      BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON2_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_MINI)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_MINI"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    #define BUTTON2_PIN         4
    #define BUTTON2_CONFIG      BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_BOOT
    #define BUTTON2_RELAY       1
    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_RELEASE     BUTTON_ACTION_TOGGLE

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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Jack is connected to GPIO14 (and with a small hack to GPIO4)
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT      1
    #endif
    #define DALLAS_PIN          14

    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT         1
    #endif
    #define DHT_PIN             14
    #define DHT_TYPE            DHT_CHIP_SI7021

    //#define I2C_SDA_PIN         4
    //#define I2C_SCL_PIN         14

#elif defined(ITEAD_SONOFF_SV)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_SV"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_ACTION_RESET
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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            15
    #define LED1_PIN_INVERSE    0

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     5
    #define HLW8012_CF1_PIN     13
    #define HLW8012_CF_PIN      14

#elif defined(ITEAD_SONOFF_POW_R2)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_POW_R2"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT    0

    // CSE7766
    #ifndef CSE7766_SUPPORT
    #define CSE7766_SUPPORT     1
    #endif
    #define CSE7766_RX_PIN      3

#elif defined(ITEAD_SONOFF_DUAL)

    // Info
    #define MANUFACTURER            "ITEAD"
    #define DEVICE                  "SONOFF_DUAL"
    #define SERIAL_BAUDRATE         19230

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Relays
    #define RELAY_PROVIDER_DUAL_SUPPORT 1

    #define RELAY1_PROVIDER     RELAY_PROVIDER_DUAL
    #define RELAY2_PROVIDER     RELAY_PROVIDER_DUAL

    // No need to include generic GPIO support
    // "Buttons" are attached to a secondary MCU and RELAY_PROVIDER_DUAL handles that
    #define BUTTON_PROVIDER_GPIO_SUPPORT    0

    // Conflicts with relay operation
    #define DEBUG_SERIAL_SUPPORT            0

#elif defined(ITEAD_SONOFF_DUAL_R2)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_DUAL_R2"

    // Buttons
    #define BUTTON1_PIN         0       // Button 0 on header
    #define BUTTON2_PIN         9       // Button 1 on header
    #define BUTTON3_PIN         10      // Physical button
    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON3_RELAY       1
    #define BUTTON1_CONFIG      BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON2_CONFIG      BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON3_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

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

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON4_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

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

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON4_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON3_RELAY       3
    #define BUTTON4_RELAY       4

    // Sonoff 4CH Pro uses a secondary STM32 microcontroller to handle
    // buttons and relays, but it also forwards button presses to the ESP8285.
    // This allows ESPurna to handle button presses -almost- the same way
    // as with other devices except:
    // * Double click seems to break/disable the button on the STM32 side
    // * With S6 switch to 1 (self-locking and inching modes) everything's OK
    // * With S6 switch to 0 (interlock mode) if there is a relay ON
    //    and you click on another relay button, the STM32 sends a "press"
    //    event for the button of the first relay (to turn it OFF) but it
    //    does not send a "release" event. It's like it's holding the
    //    button down since you can see it is still LOW.
    //    Whatever reason the result is that it may actually perform a
    //    long click or long-long click.
    // The configuration below make the button toggle the relay on press events
    // and disables any possibly harmful combination with S6 set to 0.
    // If you are sure you will only use S6 to 1 you can comment the
    // BUTTON1_LNGCLICK and BUTTON1_LNGLNGCLICK options below to recover the
    // reset mode and factory reset functionalities, or link other actions like
    // AP mode in the commented line below.

    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK    BUTTON_ACTION_NONE
    //#define BUTTON1_LNGCLICK    BUTTON_ACTION_AP
    #define BUTTON1_LNGLNGCLICK BUTTON_ACTION_NONE
    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK       BUTTON_ACTION_NONE
    #define BUTTON3_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON3_CLICK       BUTTON_ACTION_NONE
    #define BUTTON4_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON4_CLICK       BUTTON_ACTION_NONE

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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

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
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       12

#elif defined(ITEAD_SONOFF_RFBRIDGE)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_RFBRIDGE"

    // Number of virtual switches
    #ifndef DUMMY_RELAY_COUNT
    #define DUMMY_RELAY_COUNT   8
    #endif

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    #define RFB_SUPPORT         1

    // When using un-modified harware, ESPurna communicates with the secondary
    // MCU EFM8BB1 via UART at 19200 bps so we need to change the speed of
    // the port and remove UART noise on serial line
    #ifndef RFB_PROVIDER
    #define RFB_PROVIDER        RFB_PROVIDER_EFM8BB1
    #endif

    #ifndef DEBUG_SERIAL_SUPPORT
    #define DEBUG_SERIAL_SUPPORT    0
    #endif

    #define SERIAL_BAUDRATE         19200

    // Only used when RFB_PROVIDER is RCSWITCH
    #define RFB_RX_PIN          4
    #define RFB_TX_PIN          5

#elif defined(ITEAD_SONOFF_B1)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_B1"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX

    // Light

    #define MY92XX_MODEL        MY92XX_MODEL_MY9231
    #define MY92XX_CHIPS        2
    #define MY92XX_DI_PIN       12
    #define MY92XX_DCKI_PIN     14
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT

    #define MY92XX_CHANNELS     5
    #define MY92XX_CH1          4
    #define MY92XX_CH2          3
    #define MY92XX_CH3          5
    #define MY92XX_CH4          0
    #define MY92XX_CH5          1

    #define LIGHT_WHITE_FACTOR  (0.1)                    // White LEDs are way more bright in the B1

#elif defined(ITEAD_SONOFF_LED)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_LED"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       12  // Cold white
    #define LIGHT_CH2_PIN       14  // Warm white

#elif defined(ITEAD_SONOFF_T1_1CH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_T1_1CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_ACTION_RESET
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
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
    #define BUTTON2_PIN         9

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_ACTION_RESET

    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK       BUTTON_ACTION_NONE
    #define BUTTON2_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON2_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON2_LNGLNGCLICK BUTTON_ACTION_RESET

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5

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

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_ACTION_RESET

    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK       BUTTON_ACTION_NONE
    #define BUTTON2_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON2_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON2_LNGLNGCLICK BUTTON_ACTION_RESET

    #define BUTTON3_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON3_CLICK       BUTTON_ACTION_NONE
    #define BUTTON3_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON3_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON3_LNGLNGCLICK BUTTON_ACTION_RESET

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

#elif defined(ITEAD_SONOFF_S31)

    // Info
    #define MANUFACTURER            "ITEAD"
    #define DEVICE                  "SONOFF_S31"

    // Buttons
    #define BUTTON1_PIN             0
    #define BUTTON1_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY           1

    // Relays
    #define RELAY1_PIN              12
    #define RELAY1_TYPE             RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                13
    #define LED1_PIN_INVERSE        1

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT    0

    // CSE7766
    #define CSE7766_SUPPORT         1
    #define CSE7766_RX_PIN          3

#elif defined(ITEAD_SONOFF_S31_LITE)

    // Info
    #define MANUFACTURER            "ITEAD"
    #define DEVICE                  "SONOFF_S31_LITE"

    // Buttons
    #define BUTTON1_PIN             0
    #define BUTTON1_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY           1

    // Relays
    #define RELAY1_PIN              12
    #define RELAY1_TYPE             RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                13
    #define LED1_PIN_INVERSE        1

#elif defined(ITEAD_SONOFF_IFAN02)

    // Info
    #define MANUFACTURER            "ITEAD"
    #define DEVICE                  "SONOFF_IFAN02"

    // Base module
    #define IFAN_SUPPORT            1

    // These buttons are triggered by the remote
    // Fan module adds a custom button handler and a special relay controlling the speed
    #define BUTTON1_PIN             0
    #define BUTTON1_CLICK           BUTTON_ACTION_TOGGLE

    #define BUTTON2_PIN             9
    #define BUTTON2_CLICK           BUTTON_ACTION_FAN_LOW

    #define BUTTON3_PIN             10
    #define BUTTON3_CLICK           BUTTON_ACTION_FAN_MEDIUM

    #define BUTTON4_PIN             14
    #define BUTTON4_CLICK           BUTTON_ACTION_FAN_HIGH

    // LEDs
    #define LED1_PIN                13
    #define LED1_PIN_INVERSE        1

// -----------------------------------------------------------------------------
// ORVIBO
// -----------------------------------------------------------------------------

#elif defined(ORVIBO_B25)

    // Info
    #define MANUFACTURER        "ORVIBO"
    #define DEVICE              "B25"

    // Buttons
    #define BUTTON1_PIN         14
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            12   // 4 blue led
    #define LED1_PIN_INVERSE    1
    #define LED2_PIN            4  // 12 red led
    #define LED2_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// YJZK
// -----------------------------------------------------------------------------

#elif defined(YJZK_SWITCH_1CH)

    // Info
    #define MANUFACTURER        "YJZK"
    #define DEVICE              "SWITCH_1CH"

    // Buttons
    #define BUTTON1_PIN         0

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_ACTION_RESET

    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    0

#elif defined(YJZK_SWITCH_2CH)

    // Info
    #define MANUFACTURER        "YJZK"
    #define DEVICE              "SWITCH_2CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_ACTION_RESET

    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK       BUTTON_ACTION_NONE
    #define BUTTON2_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON2_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON2_LNGLNGCLICK BUTTON_ACTION_RESET

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    0

// YJZK 3CH switch
// Also Lixin Touch Wifi 3M

#elif defined(YJZK_SWITCH_3CH)

    // Info
    #define MANUFACTURER        "YJZK"
    #define DEVICE              "SWITCH_3CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9
    #define BUTTON3_PIN         10

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_ACTION_RESET

    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK       BUTTON_ACTION_NONE
    #define BUTTON2_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON2_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON2_LNGLNGCLICK BUTTON_ACTION_RESET

    #define BUTTON3_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON3_CLICK       BUTTON_ACTION_NONE
    #define BUTTON3_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON3_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON3_LNGLNGCLICK BUTTON_ACTION_RESET

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
    #define LED1_PIN_INVERSE    0

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

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
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
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX

    // Light
    #define MY92XX_MODEL        MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS        1
    #define MY92XX_DI_PIN       13
    #define MY92XX_DCKI_PIN     15
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_CHANNELS     4

// -----------------------------------------------------------------------------
// Lyasi LED
// -----------------------------------------------------------------------------

#elif defined(LYASI_LIGHT)

    // Info
    #define MANUFACTURER        "LYASI"
    #define DEVICE              "RGB_LED"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX

    // Light
    #define MY92XX_MODEL        MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS        1
    #define MY92XX_DI_PIN       4
    #define MY92XX_DCKI_PIN     5
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_CHANNELS     4

// -----------------------------------------------------------------------------
// LED Controller
// -----------------------------------------------------------------------------

#elif defined(MAGICHOME_LED_CONTROLLER)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "LED_CONTROLLER"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       13      // WHITE

    // IR
    #define IR_SUPPORT          1
    #define IR_RX_PIN           4
    #define IR_TX_SUPPORT       0
    #define IR_RX_PRESET        1

#elif defined(MAGICHOME_LED_CONTROLLER_20)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "LED_CONTROLLER_20"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE

    // IR
    #define IR_SUPPORT          1
    #define IR_RX_PIN           4
    #define IR_TX_SUPPORT       0
    #define IR_RX_PRESET        1

#elif defined(MAGICHOME_ZJ_WFMN_A_11)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "ZJ_WFMN_A_11"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1
    #define LED2_PIN            15
    #define LED2_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       14      // WHITE

    // IR
    #define IR_SUPPORT          1
    #define IR_RX_PIN           4
    #define IR_TX_SUPPORT       0
    #define IR_RX_PRESET        1

#elif defined(MAGICHOME_ZJ_WFMN_B_11)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "ZJ_WFMN_B_11"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1
    #define LED2_PIN            15
    #define LED2_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       13      // WHITE

    // RF
    #define RFB_SUPPORT          1
    #define RFB_PROVIDER         RFB_PROVIDER_RCSWITCH
    #define RFB_RX_PIN           4

#elif defined(MAGICHOME_ZJ_WFMN_C_11)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "ZJ_WFMN_C_11"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

	// Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       12      // WHITE

#elif defined(MAGICHOME_ZJ_ESPM_5CH_B_13)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "ZJ_ESPM_5CH_B_13"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       5       // COLD WHITE
    #define LIGHT_CH5_PIN       15      // WARM WHITE

#elif defined(MAGICHOME_ZJ_LB_RGBWW_L)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "ZJ_LB_RGBWW_L"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       4       // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       12      // COLD WHITE
    #define LIGHT_CH5_PIN       13      // WARM WHITE

// -----------------------------------------------------------------------------
// HUACANXING H801 & H802
// -----------------------------------------------------------------------------

#elif defined(HUACANXING_H801)

    // Info
    #define MANUFACTURER        "HUACANXING"
    #define DEVICE              "H801"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DEBUG_PORT          Serial1
    #define SERIAL_RX_ENABLED   1

    // LEDs
    #define LED1_PIN            5
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       15      // RED
    #define LIGHT_CH2_PIN       13      // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       14      // WHITE1
    #define LIGHT_CH5_PIN       4       // WHITE2

#elif defined(HUACANXING_H802)

    // Info
    #define MANUFACTURER        "HUACANXING"
    #define DEVICE              "H802"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DEBUG_PORT          Serial1
    #define SERIAL_RX_ENABLED   1

    // Light
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       14      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE

// -----------------------------------------------------------------------------
// HUGOAI AWP02L-N
// Pin equivalence extracted from https://templates.blakadder.com/hugoai_awp02l-n.html
//
// It follows almost same structure as AOYCOCR X5P with only 1 LED on GPIO02
//
// -----------------------------------------------------------------------------

#elif defined(HUGOAI_AWP02L_N)
    #define MANUFACTURER                "HUGOAI"
    #define DEVICE                      "AWP02L_N"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1
    // the defaults are reasonable, but you can change them as desired
    //#define BUTTON1_PRESS               BUTTON_ACTION_NONE
    //#define BUTTON1_CLICK               BUTTON_ACTION_TOGGLE
    //#define BUTTON1_DBLCLICK            BUTTON_ACTION_AP
    //#define BUTTON1_LNGCLICK            BUTTON_ACTION_RESET
    //#define BUTTON1_LNGLNGCLICK         BUTTON_ACTION_FACTORY

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs

    // LED1 (blue) indicates on/off state; you could use LED_MODE_FOLLOW_INVERSE
    // so that the LED lights the button when 'off' so it can be found easily.
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            1
    #define LED1_MODE                   LED_MODE_FOLLOW
    #define LED1_RELAY                  1

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

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

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

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

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
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          13
    #define RELAY1_TYPE         RELAY_TYPE_LATCHED
    #define RELAY1_RESET_PIN    12

// -----------------------------------------------------------------------------
// EX-Store Wifi Relay v5.0
// -----------------------------------------------------------------------------

#elif defined(EXS_WIFI_RELAY_V50)

    // Info
    #define MANUFACTURER        "EXS"
    #define DEVICE              "WIFI_RELAY_V50"

    // Buttons
    #define BUTTON1_PIN         5
    #define BUTTON1_RELAY       1
    #define BUTTON1_CONFIG      BUTTON_SWITCH | BUTTON_DEFAULT_BOOT | BUTTON_SET_PULLUP
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_RELEASE     BUTTON_ACTION_TOGGLE

    #define BUTTON2_PIN         4
    #define BUTTON2_RELAY       2
    #define BUTTON2_CONFIG      BUTTON_SWITCH | BUTTON_DEFAULT_BOOT | BUTTON_SET_PULLUP
    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_RELEASE     BUTTON_ACTION_TOGGLE

    // Relays
    #define RELAY1_PIN          14
    #define RELAY1_TYPE         RELAY_TYPE_LATCHED
    #define RELAY1_RESET_PIN    16
    #define RELAY2_PIN          13
    #define RELAY2_TYPE         RELAY_TYPE_LATCHED
    #define RELAY2_RESET_PIN    12

    // LEDs
    #define LED1_PIN            15
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// V9261F
// -----------------------------------------------------------------------------

#elif defined(GENERIC_V9261F)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "V9261F"
    #define ALEXA_SUPPORT       0

    // V9261F
    #define V9261F_SUPPORT      1
    #define V9261F_PIN          2
    #define V9261F_PIN_INVERSE  1

// -----------------------------------------------------------------------------
// ECH1560
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ECH1560)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ECH1560"
    #define ALEXA_SUPPORT       0

    // ECH1560
    #define ECH1560_SUPPORT     1
    #define ECH1560_CLK_PIN     4
    #define ECH1560_MISO_PIN    5
    #define ECH1560_INVERTED    0

// -----------------------------------------------------------------------------
// PZEM004T
// -----------------------------------------------------------------------------

#elif defined(GENERIC_PZEM004T)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "PZEM004T"

    #define PZEM004T_SUPPORT     1

    #define ALEXA_SUPPORT              0
    #define DEBUG_SERIAL_SUPPORT       0

// -----------------------------------------------------------------------------
// ESP-01 generic esp8266 board with 512 kB flash
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01_512KB)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ESP01_512KB"

    // Relays
    #define RELAY1_PIN          2
    #ifndef RELAY1_TYPE
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #endif

    // No need for OTA
    #define OTA_WEB_SUPPORT          0
    #define OTA_ARDUINOOTA_SUPPORT   0
    #define OTA_CLIENT               OTA_CLIENT_NONE

    // Web UI blob & MDNS are pretty large
    #define WEB_EMBEDDED             0
    #define MDNS_SERVER_SUPPORT      0

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

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          13

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // DS18B20
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT             	1
    #endif
    #define DALLAS_PIN                 	2
    #define DALLAS_UPDATE_INTERVAL     	5000
    #define TEMPERATURE_MIN_CHANGE      1.0

// -----------------------------------------------------------------------------
// QuinLED
// http://blog.quindorian.org/2017/02/esp8266-led-lighting-quinled-v2-6-pcb.html
// -----------------------------------------------------------------------------

#elif defined(INTERMITTECH_QUINLED)

    // Info
    #define MANUFACTURER        "INTERMITTECH"
    #define DEVICE              "QUINLED"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // LEDs
    #define LED1_PIN            5
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       0
    #define LIGHT_CH2_PIN       2

// -----------------------------------------------------------------------------
// Arilux AL-LC06
// -----------------------------------------------------------------------------

#elif defined(ARILUX_AL_LC01)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL_LC01"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE

#elif defined(ARILUX_AL_LC02)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL_LC02"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE1

#elif defined(ARILUX_AL_LC02_V14)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL_LC02_V14"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       13      // WHITE1

#elif defined(ARILUX_AL_LC06)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL_LC06"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Light
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE1
    #define LIGHT_CH5_PIN       5       // WHITE2

#elif defined(ARILUX_AL_LC11)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL_LC11"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       4       // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       13      // WHITE1
    #define LIGHT_CH5_PIN       12      // WHITE1

#elif defined(ARILUX_E27)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "E27"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX

    // Light
    #define MY92XX_MODEL        MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS        1
    #define MY92XX_DI_PIN       13
    #define MY92XX_DCKI_PIN     15
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_CHANNELS     4

// -----------------------------------------------------------------------------
// XENON SM-PW701U
// -----------------------------------------------------------------------------

#elif defined(XENON_SM_PW702U)

    // Info
    #define MANUFACTURER        "XENON"
    #define DEVICE              "SM_PW702U"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// ISELECTOR SM-PW702
// -----------------------------------------------------------------------------

#elif defined(ISELECTOR_SM_PW702)

    // Info
    #define MANUFACTURER        "ISELECTOR"
    #define DEVICE              "SM_PW702"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4 //BLUE
    #define LED1_PIN_INVERSE    0
    #define LED2_PIN		5 //RED
    #define LED2_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// AUTHOMETION LYT8266
// https://authometion.com/shop/en/home/13-lyt8266.html
// -----------------------------------------------------------------------------

#elif defined(AUTHOMETION_LYT8266)

    // Info
    #define MANUFACTURER        "AUTHOMETION"
    #define DEVICE              "LYT8266"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       13      // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       2       // WHITE

    #define LIGHT_ENABLE_PIN    15

#elif defined(GIZWITS_WITTY_CLOUD)

    // Info
    #define MANUFACTURER        "GIZWITS"
    #define DEVICE              "WITTY_CLOUD"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK    BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_ACTION_RESET

    #define ANALOG_SUPPORT      1

    // LEDs
    #define LED1_PIN            2      // BLUE build-in
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       15       // RED
    #define LIGHT_CH2_PIN       12       // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE

// -----------------------------------------------------------------------------
// KMC 70011
// https://www.amazon.com/KMC-Monitoring-Required-Control-Compatible/dp/B07313TH7B
// -----------------------------------------------------------------------------

#elif defined(KMC_70011)

    // Info
    #define MANUFACTURER        "KMC"
    #define DEVICE              "70011"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          14
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     12
    #define HLW8012_CF1_PIN     5
    #define HLW8012_CF_PIN      4

    #define HLW8012_VOLTAGE_R_UP            ( 2 * 1000000 )  // Upstream voltage resistor

// -----------------------------------------------------------------------------
// Euromate (?) Wifi Stecker Schuko
// https://www.obi.de/hausfunksteuerung/wifi-stecker-schuko/p/2291706
// Thanks to @Geitde
// -----------------------------------------------------------------------------

#elif defined(EUROMATE_WIFI_STECKER_SCHUKO)

    // Info
    #define MANUFACTURER        "EUROMATE"
    #define DEVICE              "WIFI_STECKER_SCHUKO"

    // Buttons
    #define BUTTON1_PIN         14
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // The relay in the device is not a bistable (latched) relay.
    // The device is reported to have a flip-flop circuit to drive the relay
    // So @Geitde hack is still the only possible

    // Hack: drive GPIO12 low and use GPIO5 as normal relay pin:
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define LED2_PIN            12 /* DUMMY: exploit default off state for GPIO12=low */
    #define LED2_PIN_INVERSE    0

    // LEDs
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// Euromate (?) Wifi Stecker Schuko Version 2
// This configuration is for the second generation of devices sold by OBI.
// https://www.obi.de/hausfunksteuerung/wifi-stecker-schuko-weiss/p/4077806
// -----------------------------------------------------------------------------
#elif defined(EUROMATE_WIFI_STECKER_SCHUKO_V2)

    // Info
    #define MANUFACTURER        "EUROMATE"
    #define DEVICE              "WIFI_STECKER_SCHUKO_V2"

    // Buttons
    #define BUTTON1_PIN         5
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // Green
    #define LED1_PIN            12
    #define LED1_MODE           LED_MODE_WIFI
    #define LED1_PIN_INVERSE    0

    // Red
    #define LED2_PIN            13
    #define LED2_MODE           LED_MODE_RELAY
    #define LED2_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// Generic 8CH
// -----------------------------------------------------------------------------

#elif defined(GENERIC_8CH)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "8CH"

    // Relays
    #define RELAY1_PIN          0
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_PIN          2
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_PIN          4
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL
    #define RELAY4_PIN          5
    #define RELAY4_TYPE         RELAY_TYPE_NORMAL
    #define RELAY5_PIN          12
    #define RELAY5_TYPE         RELAY_TYPE_NORMAL
    #define RELAY6_PIN          13
    #define RELAY6_TYPE         RELAY_TYPE_NORMAL
    #define RELAY7_PIN          14
    #define RELAY7_TYPE         RELAY_TYPE_NORMAL
    #define RELAY8_PIN          15
    #define RELAY8_TYPE         RELAY_TYPE_NORMAL

// -----------------------------------------------------------------------------
// STM RELAY
// -----------------------------------------------------------------------------

#elif defined(STM_RELAY)

    // Info
    #define MANUFACTURER            "STM_RELAY"
    #define DEVICE                  "2CH"

    // Relays
    #define RELAY_PROVIDER_STM_SUPPORT  1

    #define RELAY1_PROVIDER          RELAY_PROVIDER_STM
    #define RELAY2_PROVIDER          RELAY_PROVIDER_STM

    // Make sure we space out serial writes when relays are in sync. ref:
    // - https://github.com/xoseperez/espurna/issues/1130
    // - https://github.com/xoseperez/espurna/issues/1519
    // - https://github.com/xoseperez/espurna/pull/1520
    #define RELAY_DELAY_INTERLOCK    100

    // Remove UART noise on serial line
    // (or use `#define DEBUG_PORT Serial1` instead)
    #define DEBUG_SERIAL_SUPPORT    0

// -----------------------------------------------------------------------------
// Tonbux Powerstrip02
// -----------------------------------------------------------------------------

#elif defined(TONBUX_POWERSTRIP02)

    // Info
    #define MANUFACTURER        "TONBUX"
    #define DEVICE              "POWERSTRIP02"

    // Buttons
    #define BUTTON1_PIN         5
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       0

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_INVERSE
    #define RELAY2_PIN          13
    #define RELAY2_TYPE         RELAY_TYPE_INVERSE
    #define RELAY3_PIN          12
    #define RELAY3_TYPE         RELAY_TYPE_INVERSE
    #define RELAY4_PIN          14
    #define RELAY4_TYPE         RELAY_TYPE_INVERSE
    // Not a relay. USB ports on/off
    #define RELAY5_PIN          16
    #define RELAY5_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            0   // 1 blue led
    #define LED1_PIN_INVERSE    1
    #define LED2_PIN            3   // 3 red leds
    #define LED2_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// Lingan SWA1
// -----------------------------------------------------------------------------

#elif defined(LINGAN_SWA1)

    // Info
    #define MANUFACTURER        "LINGAN"
    #define DEVICE              "SWA1"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// HEYGO HY02
// -----------------------------------------------------------------------------

#elif defined(HEYGO_HY02)

    // Info
    #define MANUFACTURER		"HEYGO"
    #define DEVICE				"HY02"

    // Buttons
    #define BUTTON1_PIN			13
    #define BUTTON1_CONFIG		BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY		1

    // Relays
    #define RELAY1_PIN			12
    #define RELAY1_TYPE			RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN			4
    #define LED1_PIN_INVERSE	0

// -----------------------------------------------------------------------------
// Maxcio W-US002S
// -----------------------------------------------------------------------------

#elif defined(MAXCIO_WUS002S)

    // Info
    #define MANUFACTURER		"MAXCIO"
    #define DEVICE				"WUS002S"

    // Buttons
    #define BUTTON1_PIN			2
    #define BUTTON1_CONFIG		BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY		1

    // Relays
    #define RELAY1_PIN			13
    #define RELAY1_TYPE			RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN			3
    #define LED1_PIN_INVERSE	0

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT		1
    #endif
    #define HLW8012_SEL_PIN		12
    #define HLW8012_CF1_PIN		5
    #define HLW8012_CF_PIN		4

    #define HLW8012_CURRENT_R               0.002            // Current resistor
    #define HLW8012_VOLTAGE_R_UP            ( 2 * 1000000 )  // Upstream voltage resistor

    // LED1 on RX pin
    #define DEBUG_SERIAL_SUPPORT            1

// -----------------------------------------------------------------------------
// Maxcio W-DE004
// -----------------------------------------------------------------------------

#elif defined(MAXCIO_WDE004)

    // Info
    #define MANUFACTURER		"MAXCIO"
    #define DEVICE				"WDE004"

    // Buttons
    #define BUTTON1_PIN			1
    #define BUTTON1_CONFIG		BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY		1

    // Relays
    #define RELAY1_PIN			14
    #define RELAY1_TYPE			RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN			13
    #define LED1_PIN_INVERSE	1

// -----------------------------------------------------------------------------
// Maxcio W-UK007S
// Like this: https://www.amazon.co.uk/Maxcio-Monitoring-Function-Compatible-Required/dp/B07BWFB55Q/ref=pd_rhf_se_p_img_2?_encoding=UTF8&psc=1&refRID=4H63A43SKHV8WV54XH19
// -----------------------------------------------------------------------------

#elif defined(MAXCIO_WUK007S)

    // Info
    #define MANUFACTURER                "MAXCIO"
    #define DEVICE                      "WUK007S"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    0
    #define LED1_PIN_INVERSE            0
    #define LED1_RELAY                  1
    #define LED1_MODE                   LED_MODE_RELAY_WIFI

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       24380
    #define HLW8012_VOLTAGE_RATIO       32048
    #define HLW8012_POWER_RATIO         3509285
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// Oukitel P1 Smart Plug
// https://www.amazon.com/Docooler-OUKITEL-Control-Wireless-Adaptor/dp/B07J3BYFJX/ref=sr_1_fkmrnull_2?keywords=oukitel+p1+smart+switch&qid=1550424399&s=gateway&sr=8-2-fkmrnull
// -----------------------------------------------------------------------------
#elif defined(OUKITEL_P1)

    // Info
    #define MANUFACTURER		"Oukitel"
    #define DEVICE				"P1"

    // Buttons
    #define BUTTON1_PIN			13
    #define BUTTON1_CONFIG		BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY		1

    // Relays
    // Right
    #define RELAY1_PIN			12
    #define RELAY1_TYPE			RELAY_TYPE_NORMAL
    // Left
    #define RELAY2_PIN			15
    #define RELAY2_TYPE			RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN			0  // blue
    #define LED1_PIN_INVERSE	1
    #define LED1_MODE           LED_MODE_WIFI

// -----------------------------------------------------------------------------
// YiDian XS-SSA05
// -----------------------------------------------------------------------------

#elif defined(YIDIAN_XSSSA05)

    // Info
    #define MANUFACTURER		"YIDIAN"
    #define DEVICE				"XSSSA05"

    // Buttons
    #define BUTTON1_PIN			13
    #define BUTTON1_CONFIG		BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY		1

    // Relays
    #define RELAY1_PIN			12
    #define RELAY1_TYPE			RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN			0  // red
    #define LED1_PIN_INVERSE	1
    #define LED1_MODE           LED_MODE_WIFI

    #define LED2_PIN			15  // blue
    #define LED2_PIN_INVERSE	1
    #define LED2_MODE           LED_MODE_RELAY

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     3
    #define HLW8012_CF1_PIN     14
    #define HLW8012_CF_PIN      5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// TONBUX XS-SSA01
// -----------------------------------------------------------------------------

#elif defined(TONBUX_XSSSA01)

    // Info
    #define MANUFACTURER        "TONBUX"
    #define DEVICE              "XSSSA01"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          14
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// TONBUX XS-SSA06
// -----------------------------------------------------------------------------

#elif defined(TONBUX_XSSSA06)

    // Info
    #define MANUFACTURER        "TONBUX"
    #define DEVICE              "XSSSA06"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          15
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            0   // R - 8 rgb led ring
    #define LED1_PIN_INVERSE    0
    #define LED2_PIN            5   // G
    #define LED2_PIN_INVERSE    0
    #define LED3_PIN            2   // B
    #define LED3_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// GREEN ESP8266 RELAY MODULE
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180323113846&SearchText=Green+ESP8266
// -----------------------------------------------------------------------------

#elif defined(GREEN_ESP8266RELAY)

    // Info
    #define MANUFACTURER        "GREEN"
    #define DEVICE              "ESP8266RELAY"

    // Buttons
    // Not a button but input via Optocoupler
    #define BUTTON1_PIN         5
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// Henrique Gravina ESPIKE
// https://github.com/Henriquegravina/Espike
// -----------------------------------------------------------------------------

#elif defined(IKE_ESPIKE)

    #define MANUFACTURER            "IKE"
    #define DEVICE                  "ESPIKE"

    #define BUTTON1_LNGLNGCLICK     BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK        BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK        BUTTON_ACTION_NONE

    #define BUTTON1_PIN             13
    #define BUTTON1_RELAY           1
    #define BUTTON1_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON2_PIN             12
    #define BUTTON2_RELAY           2
    #define BUTTON2_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON3_PIN             14
    #define BUTTON3_RELAY           3
    #define BUTTON3_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define RELAY1_PIN              4
    #define RELAY1_TYPE             RELAY_TYPE_NORMAL

    #define RELAY2_PIN              5
    #define RELAY2_TYPE             RELAY_TYPE_NORMAL

    #define RELAY3_PIN              16
    #define RELAY3_TYPE             RELAY_TYPE_NORMAL

    #define LED1_PIN                2
    #define LED1_PIN_INVERSE        1

// -----------------------------------------------------------------------------
// SWIFITCH
// https://github.com/ArnieX/swifitch
// -----------------------------------------------------------------------------

#elif defined(ARNIEX_SWIFITCH)

    // Info
    #define MANUFACTURER          "ARNIEX"
    #define DEVICE                "SWIFITCH"

    // Buttons
    #define BUTTON1_PIN           4 // D2
    #define BUTTON1_CONFIG        BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_BOOT
    #define BUTTON1_RELAY         1

    #define BUTTON1_PRESS         BUTTON_ACTION_TOGGLE
    #define BUTTON1_RELEASE       BUTTON_ACTION_TOGGLE

    // Relays
    #define RELAY1_PIN            5 // D1
    #define RELAY1_TYPE           RELAY_TYPE_INVERSE

    // LEDs
    #define LED1_PIN              12 // D6
    #define LED1_PIN_INVERSE      1

// -----------------------------------------------------------------------------
// ESP-01S RELAY v4.0
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180404024035&SearchText=esp-01s+relay
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_RELAY_V40)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ESP01S_RELAY_40"

    // Relays
    #define RELAY1_PIN          0
    #ifndef RELAY1_TYPE
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL   // See #1504 and #1554
    #endif

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// ESP-01S RGB LED v1.0 (some sold with ws2818)
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180404023816&SearchText=esp-01s+led+controller
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_RGBLED_V10)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ESP01S_RGBLED_10"

    // This board is sold as RGB LED module BUT it has on board 3 pin ph2.0 connector (VCC, GPIO2, GND)
    // so, if you wish, you may connect LED, BUTTON, RELAY, SENSOR etc.

    // Buttons
    //#define BUTTON1_PIN         2

    // Relays
    //#define RELAY1_PIN          2

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0


// -----------------------------------------------------------------------------
// ESP-01S DHT11 v1.0
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180410105907&SearchText=esp-01s+dht11
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_DHT11_V10)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ESP01S_DHT11_10"

    // DHT11
    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT         1
    #endif
    #define DHT_PIN             2
    #define DHT_TYPE            DHT_CHIP_DHT11

// -----------------------------------------------------------------------------
// ESP-01S DS18B20 v1.0
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180410105933&SearchText=esp-01s+ds18b20
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_DS18B20_V10)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ESP01S_DS18B20_10"

    // DB18B20
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT      1
    #endif
    #define DALLAS_PIN          2

// -----------------------------------------------------------------------------
// ESP-DIN relay board V1
// https://github.com/pilotak/esp_din
// -----------------------------------------------------------------------------

#elif defined(PILOTAK_ESP_DIN_V1)

    // Info
    #define MANUFACTURER        "PILOTAK"
    #define DEVICE              "ESP_DIN_V1"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    #define RELAY2_PIN          5
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            15
    #define LED1_PIN_INVERSE    0

    #define I2C_SDA_PIN         12
    #define I2C_SCL_PIN         13

    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT      1
    #endif
    #define DALLAS_PIN          2

    #define RFB_SUPPORT          1
    #define RFB_PROVIDER         RFB_PROVIDER_RCSWITCH
    #define RFB_RX_PIN           14

    #ifndef DIGITAL_SUPPORT
    #define DIGITAL_SUPPORT      1
    #endif
    #define DIGITAL1_PIN          16
    #define DIGITAL1_PIN_MODE     INPUT
    #define DIGITAL1_DEFAULT_STATE 0

// -----------------------------------------------------------------------------
// Heltec Touch Relay
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180408043114&SearchText=esp8266+touch+relay
// -----------------------------------------------------------------------------

#elif defined(HELTEC_TOUCHRELAY)

    // Info
    #define MANUFACTURER        "HELTEC"
    #define DEVICE              "TOUCH_RELAY"

    // Buttons
    #define BUTTON1_PIN         14
    #define BUTTON1_RELAY       1
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL


// -----------------------------------------------------------------------------
// Zhilde ZLD-44EU-W
// http://www.zhilde.com/product/60705150109-805652505/EU_WiFi_Surge_Protector_Extension_Socket_4_Outlets_works_with_Amazon_Echo_Smart_Power_Strip.html
// -----------------------------------------------------------------------------

#elif defined(ZHILDE_44EU_W)

    // Info
    #define MANUFACTURER            "ZHILDE"
    #define DEVICE                  "44EU_W"

    // Based on the reporter, this product uses GPIO1 and 3 for the button
    // and onboard LED, so hardware serial should be disabled...
    #define DEBUG_SERIAL_SUPPORT    0

    // Buttons
    #define BUTTON1_PIN             3
    #define BUTTON1_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN              5
    #define RELAY2_PIN              4
    #define RELAY3_PIN              12
    #define RELAY4_PIN              13
    #define RELAY5_PIN              14
    #define RELAY1_TYPE             RELAY_TYPE_NORMAL
    #define RELAY2_TYPE             RELAY_TYPE_NORMAL
    #define RELAY3_TYPE             RELAY_TYPE_NORMAL
    #define RELAY4_TYPE             RELAY_TYPE_NORMAL
    #define RELAY5_TYPE             RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                1
    #define LED1_PIN_INVERSE        1

// -----------------------------------------------------------------------------
// Zhilde ZLD-64EU-W
// -----------------------------------------------------------------------------

#elif defined(ZHILDE_64EU_W)

    // Info
    #define MANUFACTURER            "ZHILDE"
    #define DEVICE                  "64EU_W"

    // Based on https://templates.blakadder.com/ZLD64-EU-W.html ,
    // This product uses GPIO1 for LED and 3 for the button, so hardware serial should be disabled...
    #define DEBUG_SERIAL_SUPPORT    0

    #define BUTTON1_PIN             3
    #define BUTTON1_CONFIG          BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS           BUTTON_ACTION_NONE
    #define BUTTON1_RELAY           3

    #define RELAY1_PIN              5
    #define RELAY2_PIN              4
    #define RELAY3_PIN              14

    #define LED1_PIN                1
    #define LED1_PIN_INVERSE        1

// -----------------------------------------------------------------------------
// Allnet 4duino ESP8266-UP-Relais
// http://www.allnet.de/de/allnet-brand/produkte/neuheiten/p/allnet-4duino-iot-wlan-relais-unterputz-esp8266-up-relais/
// https://shop.allnet.de/fileadmin/transfer/products/148814.pdf
// -----------------------------------------------------------------------------

#elif defined(ALLNET_4DUINO_IOT_WLAN_RELAIS)

    // Info
    #define MANUFACTURER            "ALLNET"
    #define DEVICE                  "4DUINO_IOT_WLAN_RELAIS"

    // Relays
    #define RELAY1_PIN              14
    #define RELAY1_RESET_PIN        12
    #define RELAY1_TYPE             RELAY_TYPE_LATCHED

    // LEDs
    #define LED1_PIN                0
    #define LED1_PIN_INVERSE        1

    // Buttons
    //#define BUTTON1_PIN             0
    //#define BUTTON1_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Using pins labelled as SDA & SCL as buttons
    #define BUTTON2_PIN             4
    #define BUTTON2_CONFIG          BUTTON_PUSHBUTTON
    #define BUTTON2_PRESS           BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK           BUTTON_ACTION_NONE
    #define BUTTON2_DBLCLICK        BUTTON_ACTION_NONE
    #define BUTTON2_LNGCLICK        BUTTON_ACTION_NONE
    #define BUTTON2_LNGLNGCLICK     BUTTON_ACTION_NONE

    #define BUTTON3_PIN             5
    #define BUTTON3_CONFIG          BUTTON_PUSHBUTTON

    // Using pins labelled as SDA & SCL for I2C
    //#define I2C_SDA_PIN             4
    //#define I2C_SCL_PIN             5


// -----------------------------------------------------------------------------
// Luani HVIO
// https://luani.de/projekte/esp8266-hvio/
// https://luani.de/blog/esp8266-230v-io-modul/
// -----------------------------------------------------------------------------

#elif defined(LUANI_HVIO)

    // Info
    #define MANUFACTURER            "LUANI"
    #define DEVICE                  "HVIO"

    // Buttons
    #define BUTTON1_PIN             12
    #define BUTTON1_RELAY           1
    #define BUTTON1_CONFIG          BUTTON_SWITCH | BUTTON_DEFAULT_BOOT //Hardware Pullup

    #define BUTTON1_PRESS           BUTTON_ACTION_TOGGLE
    #define BUTTON1_RELEASE         BUTTON_ACTION_TOGGLE

    #define BUTTON2_PIN             13
    #define BUTTON2_RELAY           2
    #define BUTTON2_CONFIG          BUTTON_SWITCH | BUTTON_DEFAULT_BOOT //Hardware Pullup

    #define BUTTON2_PRESS           BUTTON_ACTION_TOGGLE
    #define BUTTON2_RELEASE         BUTTON_ACTION_TOGGLE

    // Relays
    #define RELAY1_PIN              4
    #define RELAY2_PIN              5
    #define RELAY1_TYPE             RELAY_TYPE_NORMAL
    #define RELAY2_TYPE             RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                15
    #define LED1_PIN_INVERSE        0

// -----------------------------------------------------------------------------
// Tonbux 50-100M Smart Mosquito Killer USB
// https://www.aliexpress.com/item/Original-Tonbux-50-100M-Smart-Mosquito-Killer-USB-Plug-No-Noise-Repellent-App-Smart-Module/32859330820.html
// -----------------------------------------------------------------------------

#elif defined(TONBUX_MOSQUITO_KILLER)

    // Info
    #define MANUFACTURER        "TONBUX"
    #define DEVICE              "MOSQUITO_KILLER"

    // Buttons
    #define BUTTON1_PIN         2
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          5   // not a relay, fan
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            15  // blue led
    #define LED1_PIN_INVERSE    1
    #define LED1_MODE           LED_MODE_WIFI
    #define LED2_PIN            14  // red led
    #define LED2_PIN_INVERSE    1
    #define LED2_MODE           LED_MODE_RELAY

    #define LED3_PIN            12  // UV leds (1-2-3-4-5-6-7-8)
    #define LED3_PIN_INVERSE    0
    #define LED3_RELAY          1
    #define LED4_PIN            16  // UV leds (9-10-11)
    #define LED4_PIN_INVERSE    0
    #define LED4_RELAY          1

// -----------------------------------------------------------------------------
// Avatto NAS-WR01W Wifi Smart Power Plug
// https://www.aliexpress.com/item/33011753732.html
// https://todo...
// -----------------------------------------------------------------------------

#elif defined(AVATTO_NAS_WR01W)

    // Info
    #define MANUFACTURER        "AVATTO"
    #define DEVICE              "NAS_WR01W"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          14
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             5
    #define HLW8012_CF_PIN              4

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// NEO Coolcam NAS-WR01W Wifi Smart Power Plug
// https://es.aliexpress.com/item/-/32854589733.html?spm=a219c.12010608.0.0.6d084e68xX0y5N
// https://www.fasttech.com/product/9649426-neo-coolcam-nas-wr01w-wifi-smart-power-plug-eu
// -----------------------------------------------------------------------------

#elif defined(NEO_COOLCAM_NAS_WR01W)

    // Info
    #define MANUFACTURER        "NEO_COOLCAM"
    #define DEVICE              "NAS_WR01W"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    1


// -----------------------------------------------------------------------------
// Deltaco SH_P01 Wifi Smart Power Plug
// -----------------------------------------------------------------------------

#elif defined(DELTACO_SH_P01)

    // Info
    #define MANUFACTURER        "DELTACO"
    #define DEVICE              "SH_P01"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            5
    #define LED1_PIN_INVERSE    1
    #define LED1_MODE           LED_MODE_FINDME


// ------------------------------------------------------------------------------
// DELTACO_SH_P03USB Wifi Smart Power Plug
// -----------------------------------------------------------------------------

#elif defined(DELTACO_SH_P03USB)

    // Info
    #define MANUFACTURER        "DELTACO"
    #define DEVICE              "SH_P03USB"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON1_RELAY       2

    // Relays
    #define RELAY1_PIN          15  // USB power
    #define RELAY2_PIN          12  // power plug 1
    #define RELAY3_PIN          14  // power plug 2
    #define RELAY4_PIN          5   // power plug 3

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL
    #define RELAY4_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            0   // power led
    #define LED1_PIN_INVERSE    1
    #define LED1_MODE           LED_MODE_FINDME



// ------------------------------------------------------------------------------
// Fornorm Wi-Fi USB Extension Socket (ZLD-34EU)
// https://www.aliexpress.com/item/Fornorm-WiFi-Extension-Socket-with-Surge-Protector-Smart-Power-Strip-3-Outlets-and-4-USB-Charging/32849743948.html
// Also: Estink Wifi Power Strip
// -----------------------------------------------------------------------------

#elif defined(FORNORM_ZLD_34EU)

    // Info
    #define MANUFACTURER        "FORNORM"
    #define DEVICE              "ZLD_34EU"

    // Disable UART noise since this board uses GPIO3
    #define DEBUG_SERIAL_SUPPORT    0

    // Buttons
    #define BUTTON1_PIN         16
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       4

    // Relays
    #define RELAY1_PIN          14  // USB power
    #define RELAY2_PIN          13  // power plug 1
    #define RELAY3_PIN          4   // power plug 2
    #define RELAY4_PIN          15  // power plug 3

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL
    #define RELAY4_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            0   // power led
    #define LED2_PIN            12  // power plug 1
    #define LED3_PIN            3   // power plug 2
    #define LED4_PIN            5   // power plug 3

    #define LED1_PIN_INVERSE    1
    #define LED2_PIN_INVERSE    1
    #define LED3_PIN_INVERSE    1
    #define LED4_PIN_INVERSE    1

    #define LED1_MODE           LED_MODE_FINDME
    #define LED2_MODE           LED_MODE_FOLLOW
    #define LED3_MODE           LED_MODE_FOLLOW
    #define LED4_MODE           LED_MODE_FOLLOW

    #define LED2_RELAY          2
    #define LED3_RELAY          3
    #define LED4_RELAY          4


// -----------------------------------------------------------------------------
// Bruno Horta's OnOfre
// https://www.bhonofre.pt/
// https://github.com/brunohorta82/BH_OnOfre/
// -----------------------------------------------------------------------------

#elif defined(BH_ONOFRE)

    // Info
    #define MANUFACTURER        "BH"
    #define DEVICE              "ONOFRE"

    // Buttons
    #define BUTTON1_PIN         12
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON1_RELAY       1
    #define BUTTON2_PIN         13
    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_PIN          5
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

// -----------------------------------------------------------------------------
// BlitzWolf SHP2 and SHP6
// Also several boards under different names uing a power chip labelled BL0937 or HJL-01
// * Blitzwolf (https://www.amazon.es/Inteligente-Temporización-Dispositivos-Cualquier-BlitzWolf/dp/B07BMQP142)
// * HomeCube (https://www.amazon.de/Steckdose-Homecube-intelligente-Verbrauchsanzeige-funktioniert/dp/B076Q2LKHG)
// * Coosa (https://www.amazon.com/COOSA-Monitoring-Function-Campatible-Assiatant/dp/B0788W9TDR)
// * Gosund (http://www.gosund.com/?m=content&c=index&a=show&catid=6&id=5)
// * Ablue (https://www.amazon.de/Intelligente-Steckdose-Ablue-Funktioniert-Assistant/dp/B076DRFRZC)
// * DIY Tech Smart Home (https://www.amazon.es/gp/product/B07HHKXYS9)
// -----------------------------------------------------------------------------

#elif defined(BLITZWOLF_BWSHPX)

    // Info
    #define MANUFACTURER                "BLITZWOLF"
    #define DEVICE                      "BWSHPX"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    0
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// Same as the above but new board version marked V2.3
// BlitzWolf SHP2 V2.3
// Gosund SP1 V2.3
// -----------------------------------------------------------------------------

#elif defined(BLITZWOLF_BWSHPX_V23)

    // Info
    #define MANUFACTURER                "BLITZWOLF"
    #define DEVICE                      "BWSHPX_V23"

    // Buttons
    #define BUTTON1_PIN                 3
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  14
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    1
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    13
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             5
    #define HLW8012_CF_PIN              4

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

    // BUTTON1 and LED1 are using Serial pins
    #define DEBUG_SERIAL_SUPPORT        0

// -----------------------------------------------------------------------------
// Similar to both devices above but also with switchable USB ports
// and other sensor (CSE7766).
// the pin layout is different to the above two versions
// BlitzWolf SHP5
// -----------------------------------------------------------------------------
#elif defined(BLITZWOLF_BWSHP5)

    // Info
    #define MANUFACTURER                "BLITZWOLF"
    #define DEVICE                      "BWSHP5"

    // Buttons
    #define BUTTON1_PIN                 16
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    // Power plug
    #define RELAY1_PIN                  14
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL
    // USB
    #define RELAY2_PIN                  5
    #define RELAY2_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    0
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT        0

    // CSE7766
    #ifndef CSE7766_SUPPORT
    #define CSE7766_SUPPORT     1
    #endif
    #define CSE7766_RX_PIN      3

// -----------------------------------------------------------------------------
// Teckin SP21
// -----------------------------------------------------------------------------

#elif defined(TECKIN_SP21)

    // Info
    #define MANUFACTURER                "TECKIN"
    #define DEVICE                      "SP21"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            1


// -----------------------------------------------------------------------------
// Teckin SP22 v1.4 - v1.6
// -----------------------------------------------------------------------------

#elif defined(TECKIN_SP22_V14)

    // Info
    #define MANUFACTURER                "TECKIN"
    #define DEVICE                      "SP22_V14"

    // Buttons
    #define BUTTON1_PIN                 1
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  14
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    3
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    13
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             5
    #define HLW8012_CF_PIN              4

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       20730
    #define HLW8012_VOLTAGE_RATIO       264935
    #define HLW8012_POWER_RATIO         2533110
    #define HLW8012_INTERRUPT_ON        FALLING

    // BUTTON1 and LED1 are using Serial pins
    #define DEBUG_SERIAL_SUPPORT        0

// -----------------------------------------------------------------------------
// Teckin SP22 v1.4 - v1.6
//
// NB Notes suggest that energy monitoring is removed from later versions
// -----------------------------------------------------------------------------

#elif defined(TECKIN_SP23_V13)

    // Info  .. NB Newer versions apparently lack energy monitor
    // The board revision is not indicated externally
    #define MANUFACTURER                "TECKIN"
    #define DEVICE                      "SP23_V13"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    4
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    2
    #define LED2_PIN_INVERSE            0
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       23324
    #define HLW8012_VOLTAGE_RATIO       324305
    #define HLW8012_POWER_RATIO         3580841
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// The Gosund WP3 is based on ESP8285, so 1 MB internal flash (DOUT required)
// The module has no-connect:  TX, RX, RST, AD, GPIO5, (and GPIO0,
//     GPIO2 via test points on the back of the module)
// and these are wired to devices:
// GPIO4: /BTN
// GPIO12: /LED red
// GPIO13: /LED blue
// GPIO14: RELAY
// -----------------------------------------------------------------------------

#elif defined(GOSUND_WP3)

    // Info
    #define MANUFACTURER                "GOSUND"
    #define DEVICE                      "WP3"

    // Buttons
    #define BUTTON1_PIN                 4
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1
    // the defaults are reasonable, but you can change them as desired
    //#define BUTTON1_PRESS               BUTTON_ACTION_NONE
    //#define BUTTON1_CLICK               BUTTON_ACTION_TOGGLE
    //#define BUTTON1_DBLCLICK            BUTTON_ACTION_AP
    //#define BUTTON1_LNGCLICK            BUTTON_ACTION_RESET
    //#define BUTTON1_LNGLNGCLICK         BUTTON_ACTION_FACTORY

    // Relays
    #define RELAY1_PIN                  14
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs

    // LED1 (red) indicates on/off state; you could use LED_MODE_FOLLOW_INVERSE
    // so that the LED lights the button when 'off' so it can be found easily.
    #define LED1_PIN                    12
    #define LED1_PIN_INVERSE            1
    #define LED1_MODE                   LED_MODE_FOLLOW
    #define LED1_RELAY                  1

    // LED2 (blue) indicates wifi activity
    #define LED2_PIN                    13
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_WIFI

// -----------------------------------------------------------------------------
// Several boards under different names uing a power chip labelled BL0937 or HJL-01
// Also model number KS-602S
// -----------------------------------------------------------------------------

#elif defined(GOSUND_WS1)

    // Info
    #define MANUFACTURER        "GOSUND"
    #define DEVICE              "WS1"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          14
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            1
    #define LED1_PIN_INVERSE    1

    // LED1 is using TX pin
    #define DEBUG_SERIAL_SUPPORT 0

// ----------------------------------------------------------------------------------------
//  Power socket 16A similar to BLITZWOLF_BWSHPX but button pin differs
//  IMPORTANT, This template is for hardware version SP111_A_Wifi_Ver1.1 (as printed on the PCB)
//  hhttps://www.amazon.de/-/en/Smallest-Consumption-Measuring-Function-Compatible/dp/B07PSMF47W
// ----------------------------------------------------------------------------------------

#elif defined(GOSUND_SP111)

    // Info
    #define MANUFACTURER                "GOSUND"
    #define DEVICE                      "SP111"

   // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    0
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             4
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// ----------------------------------------------------------------------------------------
//  Power strip 15A - 3 Sockets + 3 USB ports 
//  This device uses just one digital button (main one). All other three buttons are connected 
//  to the ADC pin via resistors with different values. This approach is known under the name of
//  "Resistor Ladder" - https://en.wikipedia.org/wiki/Resistor_ladder
//  https://www.amazon.de/-/en/gp/product/B085XXCPRD
// ----------------------------------------------------------------------------------------

#elif defined(GOSUND_P1)

    // Info
    #define MANUFACTURER                "GOSUND"
    #define DEVICE                      "P1"

    
    //Enable this to view buttons analog level.
    //Or, use adc terminal command
    //#define ANALOG_SUPPORT                1

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT            0

    // Buttons
    #define BUTTON_PROVIDER_GPIO_SUPPORT    1
    #define BUTTON_PROVIDER_ANALOG_SUPPORT  1

    #define BUTTON1_PIN                     16
    #define BUTTON1_CONFIG                  BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY                   4

    #define BUTTON2_PIN                     17
    #define BUTTON2_RELAY                   3
    #define BUTTON2_PROVIDER                BUTTON_PROVIDER_ANALOG
    #define BUTTON2_ANALOG_LEVEL            220

    #define BUTTON3_PIN                     17
    #define BUTTON3_RELAY                   2
    #define BUTTON3_PROVIDER                BUTTON_PROVIDER_ANALOG
    #define BUTTON3_ANALOG_LEVEL            470

    #define BUTTON4_PIN                     17
    #define BUTTON4_RELAY                   1
    #define BUTTON4_PROVIDER                BUTTON_PROVIDER_ANALOG
    #define BUTTON4_ANALOG_LEVEL            730

    // Relays
    #define RELAY1_PIN                      14
    #define RELAY2_PIN                      12
    #define RELAY3_PIN                      13
    #define RELAY4_PIN                      5
    #define RELAY4_TYPE                     RELAY_TYPE_INVERSE

    // LEDs
    #define LED1_PIN                        2
    #define LED1_PIN_INVERSE                1    

    // CSE7766
    #define CSE7766_SUPPORT                 1
    #define CSE7766_RX_PIN                  3

// ----------------------------------------------------------------------------------------
//  Homecube 16A is similar but some pins differ and it also has RGB LEDs
//  https://www.amazon.de/gp/product/B07D7RVF56/ref=oh_aui_detailpage_o00_s01?ie=UTF8&psc=1
// ----------------------------------------------------------------------------------------

#elif defined(HOMECUBE_16A)

    // Info
    #define MANUFACTURER                "HOMECUBE"
    #define DEVICE                      "16A"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    //LED Pin 4 - ESP8266 onboard LED
    //Red   LED: 0
    //Green LED: 12
    //Blue  LED: 2

    // Blue
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            0

    // Green
    #define LED2_PIN                    12
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_RELAY

    // Red
    #define LED3_PIN                    0
    #define LED3_PIN_INVERSE            0
    #define LED3_MODE                   LED_MODE_OFF

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             16
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// VANZAVANZU Smart Outlet Socket (based on BL0937 or HJL-01)
// https://www.amazon.com/Smart-Plug-Wifi-Mini-VANZAVANZU/dp/B078PHD6S5
// -----------------------------------------------------------------------------

#elif defined(VANZAVANZU_SMART_WIFI_PLUG_MINI)

    // Info
    #define MANUFACTURER                "VANZAVANZU"
    #define DEVICE                      "SMART_WIFI_PLUG_MINI"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    0
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT        0

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             3
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------

#elif defined(GENERIC_AG_L4)

    // Info
    #define MANUFACTURER                "GENERIC"
    #define DEVICE                      "AG_L4"
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER

    // button 1: "power" button
    #define BUTTON1_PIN                 4
    #define BUTTON1_RELAY               1
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS               BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK               BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK            BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK            BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK         BUTTON_ACTION_RESET

    // button 2: "wifi" button
    #define BUTTON2_PIN                 2
    #define BUTTON2_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS               BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK               BUTTON_ACTION_NONE
    #define BUTTON2_DBLCLICK            BUTTON_ACTION_NONE
    #define BUTTON2_LNGCLICK            BUTTON_ACTION_NONE
    #define BUTTON2_LNGLNGCLICK         BUTTON_ACTION_NONE

    // LEDs
    #define LED1_PIN                    5      // red status led
    #define LED1_PIN_INVERSE            0

    #define LED2_PIN                    16      // master light power
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_RELAY

    // Light
    #define LIGHT_CH1_PIN               14       // RED
    #define LIGHT_CH2_PIN               13       // GREEN
    #define LIGHT_CH3_PIN               12      // BLUE

// -----------------------------------------------------------------------------

#elif defined(GENERIC_AG_L4_V3)

    // Info
    #define MANUFACTURER                "GENERIC"
    #define DEVICE                      "AG_L4_V3"
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER

    // button 1: "power" button
    #define BUTTON1_PIN                 13
    #define BUTTON1_RELAY               1
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS               BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK               BUTTON_ACTION_NONE
    #define BUTTON1_DBLCLICK            BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK            BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK         BUTTON_ACTION_RESET

    // button 2: "wifi" button
    #define BUTTON2_PIN                 2
    #define BUTTON2_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS               BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK               BUTTON_ACTION_NONE
    #define BUTTON2_DBLCLICK            BUTTON_ACTION_NONE
    #define BUTTON2_LNGCLICK            BUTTON_ACTION_NONE
    #define BUTTON2_LNGLNGCLICK         BUTTON_ACTION_NONE

    // LEDs
    #define LED1_PIN                    5      // red status led
    #define LED1_PIN_INVERSE            0

    #define LED2_PIN                    16      // master light power
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_RELAY

    // Light
    #define LIGHT_CH1_PIN               4        // RED
    #define LIGHT_CH2_PIN               12       // GREEN
    #define LIGHT_CH3_PIN               14       // BLUE

// -----------------------------------------------------------------------------

#elif defined(ALLTERCO_SHELLY1)

    // Info
    #define MANUFACTURER        "ALLTERCO"
    #define DEVICE              "SHELLY1"

    // Buttons
    #define BUTTON1_PIN         5
    #define BUTTON1_CONFIG      BUTTON_SWITCH | BUTTON_DEFAULT_BOOT
    #define BUTTON1_RELAY       1

    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_RELEASE     BUTTON_ACTION_TOGGLE

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

#elif defined(ALLTERCO_SHELLY2)

    // Info
    #define MANUFACTURER        "ALLTERCO"
    #define DEVICE              "SHELLY2"

    // Buttons
    #define BUTTON1_PIN         12
    #define BUTTON1_RELAY       1
    #define BUTTON1_CONFIG      BUTTON_SWITCH | BUTTON_DEFAULT_BOOT

    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_RELEASE     BUTTON_ACTION_TOGGLE

    #define BUTTON2_PIN         14
    #define BUTTON2_CONFIG      BUTTON_SWITCH | BUTTON_DEFAULT_BOOT
    #define BUTTON2_RELAY       2

    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_RELEASE     BUTTON_ACTION_TOGGLE

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_PIN          5
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

#elif defined(ALLTERCO_SHELLY1PM)
    // Info
    #define MANUFACTURER        "ALLTERCO"
    #define DEVICE              "SHELLY1PM"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_CONFIG      BUTTON_SWITCH | BUTTON_DEFAULT_BOOT
    #define BUTTON1_RELAY       1

    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_RELEASE     BUTTON_ACTION_TOGGLE

    #define BUTTON2_PIN         2
    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_LNGCLICK    BUTTON_ACTION_RESET
    #define BUTTON2_LNGLNGCLICK BUTTON_ACTION_FACTORY

    // Relays
    #define RELAY1_PIN          15
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // Light
    #define LED1_PIN            0
    #define LED1_PIN_INVERSE    1

    // HJL01 / BL0937
    #define HLW8012_SUPPORT             1
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             13
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

    //Temperature
     #define NTC_SUPPORT        1
     #define SENSOR_SUPPORT     1
     #define NTC_BETA           3350
     #define NTC_R_UP           10000
     #define NTC_R_DOWN         0
     #define NTC_R0             8000

#elif defined(ALLTERCO_SHELLY25)
    // Info
    #define MANUFACTURER        "ALLTERCO"
    #define DEVICE              "SHELLY25"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_SWITCH | BUTTON_DEFAULT_BOOT
    #define BUTTON1_RELAY       1

    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_RELEASE     BUTTON_ACTION_TOGGLE

    #define BUTTON2_PIN         5
    #define BUTTON2_CONFIG      BUTTON_SWITCH | BUTTON_DEFAULT_BOOT
    #define BUTTON2_RELAY       2

    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_RELEASE     BUTTON_ACTION_TOGGLE

    #define BUTTON3_PIN         2
    #define BUTTON3_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_LNGCLICK    BUTTON_ACTION_RESET
    #define BUTTON3_LNGLNGCLICK BUTTON_ACTION_FACTORY

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    #define RELAY2_PIN          15
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // Light
    #define LED1_PIN            0
    #define LED1_PIN_INVERSE    1

    //Temperature
     #define NTC_SUPPORT        1
     #define SENSOR_SUPPORT     1
     #define NTC_BETA           3350
     #define NTC_R_UP           10000
     #define NTC_R_DOWN         0
     #define NTC_R0             8000

    //Current
    #define ADE7953_SUPPORT     1
    #define I2C_SDA_PIN         12
    #define I2C_SCL_PIN         14

// -----------------------------------------------------------------------------
// This device has teh same behaviour as the GOSUND WP3, but with different GPIO pin values
// GPIO equivalents extracted from https://templates.blakadder.com/aoycocr_X5P.html

#elif defined(AOYCOCR_X5P)

    // Info
    #define MANUFACTURER                "AOYCOCR"
    #define DEVICE                      "X5P"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1
    // the defaults are reasonable, but you can change them as desired
    //#define BUTTON1_PRESS               BUTTON_ACTION_NONE
    //#define BUTTON1_CLICK               BUTTON_ACTION_TOGGLE
    //#define BUTTON1_DBLCLICK            BUTTON_ACTION_AP
    //#define BUTTON1_LNGCLICK            BUTTON_ACTION_RESET
    //#define BUTTON1_LNGLNGCLICK         BUTTON_ACTION_FACTORY

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs

    // LED1 (red) indicates on/off state; you could use LED_MODE_FOLLOW_INVERSE
    // so that the LED lights the button when 'off' so it can be found easily.
    #define LED1_PIN                    0
    #define LED1_PIN_INVERSE            1
    #define LED1_MODE                   LED_MODE_FOLLOW
    #define LED1_RELAY                  1

    // LED2 (blue) indicates wifi activity
    #define LED2_PIN                    2
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_WIFI

// -----------------------------------------------------------------------------

// also works with https://www.amazon.com/gp/product/B07TMY394G/
// see https://github.com/xoseperez/espurna/issues/2055

#elif defined(LOHAS_E27_9W)

    // Info
    #define MANUFACTURER        "LOHAS"
    #define DEVICE              "E27_9W"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX

    // Light
    #define MY92XX_MODEL        MY92XX_MODEL_MY9231
    #define MY92XX_CHIPS        2
    #define MY92XX_DI_PIN       13
    #define MY92XX_DCKI_PIN     15
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_CHANNELS     5

    #define LIGHT_WHITE_FACTOR  (0.1)                    // White LEDs are way more bright in the B1

// https://www.amazon.com/gp/product/B07T7W7ZMW

#elif defined(LOHAS_E26_A19)

    // Info
    #define MANUFACTURER        "LOHAS"
    #define DEVICE              "E26_A19"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       4       // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       14      // WHITE1
    #define LIGHT_CH5_PIN       12      // WHITE1

// -----------------------------------------------------------------------------

#elif defined(TECKIN_SB53)

    // Info
    #define MANUFACTURER        "TECKIN"
    #define DEVICE              "SB53"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       4       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       13      // WARM WHITE
    #define LIGHT_CH5_PIN       5       // COLD WHITE

// -----------------------------------------------------------------------------

#elif defined(XIAOMI_SMART_DESK_LAMP)

    // Info
    #define MANUFACTURER        "XIAOMI"
    #define DEVICE              "SMART_DESK_LAMP"

    // Buttons
    #define BUTTON1_PIN         2
    #define BUTTON2_PIN         14

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP

    // This button doubles as switch here and as encoder mode switch below
    // Clicking it (for less than 500ms) will turn the light on and off
    // Double and Long clicks will not work as these are used to modify the encoder action
    #define BUTTON1_RELAY           1
    #define BUTTON_LNGCLICK_DELAY   500
    #define BUTTON1_DBLCLICK        BUTTON_ACTION_NONE
    #define BUTTON1_LNGCLICK        BUTTON_ACTION_NONE
    #define BUTTON1_LNGLNGCLICK     BUTTON_ACTION_NONE

    // Hidden button will enter AP mode if dblclick and reset the device when long-long-clicked
    #define BUTTON2_DBLCLICK        BUTTON_ACTION_AP
    #define BUTTON2_LNGLNGCLICK     BUTTON_ACTION_RESET

    // Light
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define LIGHT_STEP          8
    #define LIGHT_CH1_PIN       5   // warm white
    #define LIGHT_CH2_PIN       4   // cold white

    // https://www.xiaomitoday.com/xiaomi-mijia-mjtd01yl-led-desk-lamp-review/
    #define LIGHT_COLDWHITE_MIRED 153
    #define LIGHT_WARMWHITE_MIRED 370

    // Encoder
    // If mode is ENCODER_MODE_RATIO, the value ratio between both channels is changed
    // when the button is not pressed, and the overall brightness when pressed
    // If mode is ENCODER_MODE_CHANNEL, the first channel value is changed
    // when the button is not pressed, and the second channel when pressed
    // If no ENCODERX_BUTTON_PIN defined it will only change the value of the first defined channel
    #define ENCODER_SUPPORT     1
    #define ENCODER1_PIN1       12
    #define ENCODER1_PIN2       13
    #define ENCODER1_BUTTON_PIN 2   // active low by default, with software pullup
    #define ENCODER1_CHANNEL1   0   // please note this value is 0-based (LIGHT_CH1 above)
    #define ENCODER1_CHANNEL2   1   // please note this value is 0-based (LIGHT_CH2 above)
    #define ENCODER1_MODE       ENCODER_MODE_RATIO

#elif defined(PHYX_ESP12_RGB)

    // Info
    #define MANUFACTURER        "PHYX"
    #define DEVICE              "ESP12_RGB"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       4       // RED
    #define LIGHT_CH2_PIN       14      // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE

// -----------------------------------------------------------------------------
// iWoole LED Table Lamp
// http://iwoole.com/newst-led-smart-night-light-7w-smart-table-light-rgbw-wifi-app-remote-control-110v-220v-us-eu-plug-smart-lamp-google-home-decore-p00022p1.html
// -----------------------------------------------------------------------------

#elif defined(IWOOLE_LED_TABLE_LAMP)

    // Info
    #define MANUFACTURER        "IWOOLE"
    #define DEVICE              "LED_TABLE_LAMP"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       4       // WHITE

// -----------------------------------------------------------------------------
// Generic GU10
// https://www.ebay.com/itm/1-10PC-GU10-RGB-Smart-Bulb-Wireless-WiFi-App-Remote-Ctrl-Light-for-Alexa-Google/173724116351
// -----------------------------------------------------------------------------

#elif defined(GENERIC_GU10)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "GU10"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       4       // WHITE

// -----------------------------------------------------------------------------
// Generic E14
// https://www.ebay.com/itm/LED-Bulb-Wifi-E14-4-5W-Candle-RGB-W-4in1-Dimmable-V-tac-Smart-VT-5114/163899840601
// -----------------------------------------------------------------------------

#elif defined(GENERIC_E14)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "E14"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       4       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       5       // WHITE

// -----------------------------------------------------------------------------
// Deltaco white e14 (SH-LE14W) and e27 (SH-LE27W)
// -----------------------------------------------------------------------------

#elif defined(DELTACO_SH_LEXXW)

    // Info
    #define MANUFACTURER        "DELTACO"
    #define DEVICE              "SH_LEXXW"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       12      // WARM WHITE
    #define LIGHT_CH2_PIN       14      // COLD WHITE

// -----------------------------------------------------------------------------
// Deltaco rgbw e27 (SH-LE27RGB)
// -----------------------------------------------------------------------------

#elif defined(DELTACO_SH_LEXXRGB)

    // Info
    #define MANUFACTURER        "DELTACO"
    #define DEVICE              "SH_LEXXRGB"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       5        // RED
    #define LIGHT_CH2_PIN       4        // GREEN
    #define LIGHT_CH3_PIN       13       // BLUE
    #define LIGHT_CH4_PIN       14       // WARM WHITE
    #define LIGHT_CH5_PIN       12       // COLD WHITE

// -----------------------------------------------------------------------------
// Nexete A19
// https://www.ebay.com/itm/Wifi-Smart-LED-light-Bulb-9W-60W-A19-850LM-RGBW-Dimmable-for-Alexa-Google-Home/283514779201
// -----------------------------------------------------------------------------

#elif defined(NEXETE_A19)

    // Info
    #define MANUFACTURER        "NEXETE"
    #define DEVICE              "A19"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       15      // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       5       // WHITE

// -----------------------------------------------------------------------------
// Lombex Lux Nova 2 Tunable White
// https://www.amazon.com/Lombex-Compatible-Equivalent-Dimmable-2700K-6500K/dp/B07B8K72PR
// -----------------------------------------------------------------------------
#elif defined(LOMBEX_LUX_NOVA2_TUNABLE_WHITE)

    // Info
    #define MANUFACTURER        "LOMBEX"
    #define DEVICE              "LUX_NOVA2_TUNABLE_WHITE"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX

    // Light
    #define MY92XX_MODEL        MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS        1
    #define MY92XX_DI_PIN       4
    #define MY92XX_DCKI_PIN     5
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_CHANNELS     2

    // No RGB on this bulb. Warm white on channel 0, cool white on channel 3
    #define MY92XX_CH1          3
    #define MY92XX_CH2          0

// -----------------------------------------------------------------------------
// Lombex Lux Nova 2 White and Color
// https://www.amazon.com/Lombex-Compatible-Equivalent-Dimmable-2700K-6500K/dp/B07B8K72PR
// -----------------------------------------------------------------------------
#elif defined(LOMBEX_LUX_NOVA2_WHITE_COLOR)

    // Info
    #define MANUFACTURER        "LOMBEX"
    #define DEVICE              "LUX_NOVA2_WHITE_COLOR"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX

    // Light
    #define MY92XX_MODEL        MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS        1
    #define MY92XX_DI_PIN       4
    #define MY92XX_DCKI_PIN     5
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_CHANNELS     4
    // RGB on channels 0/1/2, either cool or warm white on channel 3
    // The bulb *should* have cool leds, but could also have warm leds as a common defect

// -----------------------------------------------------------------------------
// Bestek Smart Plug with 2 USB ports
// https://www.bestekcorp.com/bestek-smart-plug-works-with-amazon-alexa-google-assistant-and-ifttt-with-2-usb
// -----------------------------------------------------------------------------

#elif defined(BESTEK_MRJ1011)

    // Info
    #define MANUFACTURER        "BESTEK"
    #define DEVICE              "MRJ1011"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relay
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LED
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// GBLIFE RGBW SOCKET
// -----------------------------------------------------------------------------

#elif defined(GBLIFE_RGBW_SOCKET)

    // Info
    #define MANUFACTURER        "GBLIFE"
    #define DEVICE              "RGBW_SOCKET"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          15
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // Light RGBW
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       14      // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       4       // WHITE

// ----------------------------------------------------------------------------------------
// Smart life Mini Smart Socket is similar Homecube 16A but some GPIOs differ
// https://www.ebay.de/itm/Smart-Steckdose-WIFI-WLAN-Amazon-Alexa-Fernbedienung-Home-Socket-Zeitschaltuh-DE/123352026749?hash=item1cb85a8e7d:g:IasAAOSwk6dbj390
// Also labeled NETVIP
// https://www.amazon.es/Inteligente-NETVIP-Inal%C3%A1mbrico-Interruptor-Funciona/dp/B07KH8YWS5
// ----------------------------------------------------------------------------------------

#elif defined(SMARTLIFE_MINI_SMART_SOCKET)

    // Info
    #define MANUFACTURER                "SMARTLIFE"
    #define DEVICE                      "MINI_SMART_SOCKET"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    //Red   LED: 0
    //Green LED: 4
    //Blue  LED: 2

    // Light
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CH1_PIN               0       // RED
    #define LIGHT_CH2_PIN               4       // GREEN
    #define LIGHT_CH3_PIN               2       // BLUE

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// ----------------------------------------------------------------------------------------
// Hama WiFi Steckdose (00176533)
// https://at.hama.com/00176533/hama-wifi-steckdose-3500w-16a
// ----------------------------------------------------------------------------------------

#elif defined(HAMA_WIFI_STECKDOSE_00176533)

    // Info
    #define MANUFACTURER        "HAMA"
    #define DEVICE              "WIFI_STECKDOSE_00176533"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// Oxaoxe NX-SP202
// Digoo NX-SP202 (not tested)
// Digoo DG-SP202 (not tested)
// https://github.com/xoseperez/espurna/issues/1502
// -----------------------------------------------------------------------------

#elif defined(DIGOO_NX_SP202)

    // Info
    #define MANUFACTURER                "DIGOO"
    #define DEVICE                      "NX_SP202"

    // Buttons
    #define BUTTON1_PIN                 0
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1
    #define BUTTON2_PIN                 16
    #define BUTTON2_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_RELAY               2

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL
    #define RELAY2_PIN                  14
    #define RELAY2_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    13
    #define LED1_PIN_INVERSE            1

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             5
    #define HLW8012_CF_PIN              4

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       23296
    #define HLW8012_VOLTAGE_RATIO       310085
    #define HLW8012_POWER_RATIO         3368471
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// Foxel's LightFox dual
// https://github.com/foxel/esp-dual-rf-switch
// -----------------------------------------------------------------------------

#elif defined(FOXEL_LIGHTFOX_DUAL)

    // Info
    #define MANUFACTURER            "FOXEL"
    #define DEVICE                  "LIGHTFOX_DUAL"
    #define SERIAL_BAUDRATE         19200

    // Relays
    #define LIGHTFOX_RELAYS         2

    // Buttons
    #define LIGHTFOX_BUTTONS        4

    #define BUTTON1_CLICK           BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK           BUTTON_ACTION_TOGGLE
    #define BUTTON3_CLICK           BUTTON_ACTION_TOGGLE
    #define BUTTON4_CLICK           BUTTON_ACTION_TOGGLE

    #define BUTTON1_RELAY           1
    #define BUTTON2_RELAY           2
    #define BUTTON3_RELAY           2
    #define BUTTON4_RELAY           1

    // Conflicts with relay operation
    #define DEBUG_SERIAL_SUPPORT            0

// -----------------------------------------------------------------------------
// Teckin SP20
// -----------------------------------------------------------------------------

#elif defined(TECKIN_SP20)

    // Info
    #define MANUFACTURER                "TECKIN"
    #define DEVICE                      "SP20"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  4
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    0
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  0

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// Charging Essentials / LITESUN LA-WF3
// -----------------------------------------------------------------------------

#elif defined(LITESUN_LA_WF3)

    // Info
    #define MANUFACTURER        "LITESUN"
    #define DEVICE              "LA_WF3"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4  // 4 blue led
    #define LED1_MODE           LED_MODE_WIFI
    #define LED1_PIN_INVERSE    1

    #define LED2_PIN            5  // 5 red led
    #define LED2_MODE           LED_MODE_RELAY
    #define LED2_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// PSH
// -----------------------------------------------------------------------------

#elif defined(PSH_WIFI_PLUG)

    // Info
    #define MANUFACTURER        "PSH"
    #define DEVICE              "WIFI_PLUG"

    // Relays
    #define RELAY1_PIN          2
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            0
    #define LED1_PIN_INVERSE    0

#elif defined(PSH_RGBW_CONTROLLER)

    // Info
    #define MANUFACTURER        "PSH"
    #define DEVICE              "RGBW_CONTROLLER"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CH1_PIN       5      // RED
    #define LIGHT_CH2_PIN       4      // GREEN
    #define LIGHT_CH3_PIN       12     // BLUE
    #define LIGHT_CH4_PIN       14     // WHITE1

#elif defined(PSH_WIFI_SENSOR)

    // Info
    #define MANUFACTURER        "PSH"
    #define DEVICE              "WIFI_SENSOR"

    // DHT12 Sensor
    #define DHT_SUPPORT         1
    #define DHT_PIN             14
    #define DHT_TYPE            DHT_CHIP_DHT12

    // LDR Sensor
    #define LDR_SUPPORT         1
    #define LDR_TYPE            LDR_GL5528
    #define LDR_ON_GROUND       false
    #define LDR_RESISTOR        10000

#elif defined(JINVOO_VALVE_SM_AW713)

    // Reflashing from original Tuya firmware
    // to thirdparty firmware like espurna by:
    // https://github.com/ct-Open-Source/tuya-convert

    // Info
    #define MANUFACTURER        "JINVOO"
    #define DEVICE              "VALVE_SM_AW713"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LED
    #define LED1_PIN            5  // 5 red led
    #define LED1_PIN_INVERSE    0
    #define LED1_RELAY          1
    #define LED1_MODE           LED_MODE_RELAY

    #define LED2_PIN            4  // 4 blue led
    #define LED2_PIN_INVERSE    0
    #define LED2_RELAY          1
    #define LED2_MODE           LED_MODE_FINDME_WIFI

#elif defined(TUYA_GENERIC_DIMMER)

    #define MANUFACTURER        "TUYA"
    #define DEVICE              "GENERIC_DIMMER"

    #define TUYA_SUPPORT        1
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_CUSTOM

    #define LED1_GPIO           14

    #define TUYA_CH_STATE_DPID  1
    #define TUYA_CH1_DPID       2

    #define DEBUG_SERIAL_SUPPORT    0

// -----------------------------------------------------------------------------
// Etekcity ESW01-USA
// https://www.amazon.com/Etekcity-Voltson-Outlet-Monitoring-Required/dp/B01M3MYIFS
// -----------------------------------------------------------------------------

#elif defined(ETEKCITY_ESW01_USA)

    // Info
    #define MANUFACTURER                "ETEKCITY"
    #define DEVICE                      "ESW01_USA"

    // Buttons
    #define BUTTON1_PIN                 14
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  4
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    // Blue
    #define LED1_PIN                    5
    #define LED1_MODE                   LED_MODE_WIFI
    // Yellow
    #define LED2_PIN                    16
    #define LED2_MODE                   LED_MODE_FOLLOW
    #define LED2_RELAY                  1

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             15
    #define HLW8012_CF1_PIN             12
    #define HLW8012_CF_PIN              13

    #define HLW8012_SEL_CURRENT         HIGH    // SEL pin to HIGH to measure current
    #define HLW8012_CURRENT_R           0.001   // Current resistor
    #define HLW8012_VOLTAGE_R_UP        ( 4 * 470000 )  // Upstream voltage resistor
    #define HLW8012_VOLTAGE_R_DOWN      ( 1000 )        // Downstream voltage resistor
    #define HLW8012_INTERRUPT_ON        CHANGE

// -----------------------------------------------------------------------------
// FS UAP1
// http://frank-schuetz.de/index.php/fhem/13-hoermann-torantrieb-mit-espeasy-in-fhem-einbinden

#elif defined(FS_UAP1)

    // Info
    #define MANUFACTURER            "FS"
    #define DEVICE                  "UAP1"

    // Inputs
    #define DIGITAL1_PIN            4
    #define DIGITAL2_PIN            5

    // Relays
    #define RELAY1_PIN              12
    #define RELAY2_PIN              13
    #define RELAY3_PIN              14
    #define RELAY4_PIN              15

    #define RELAY1_TYPE             RELAY_TYPE_NORMAL
    #define RELAY2_TYPE             RELAY_TYPE_NORMAL
    #define RELAY3_TYPE             RELAY_TYPE_NORMAL
    #define RELAY4_TYPE             RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                2

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT    0

// -----------------------------------------------------------------------------
// TFLAG NX-SM100 & NX-SM200
// -----------------------------------------------------------------------------

#elif defined(TFLAG_NX_SMX00)

    // Info
    #define MANUFACTURER                "TFLAG"
    #define DEVICE                      "NX_SMX00"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  12
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    0
    #define LED1_PIN_INVERSE            1
    #define LED1_MODE                   LED_MODE_FOLLOW_INVERSE
    #define LED1_RELAY                  1
    #define LED2_PIN                    15
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_WIFI

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             16
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       632
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3711185
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// MUVIT_IO_MIOBULB001
// -----------------------------------------------------------------------------

#elif defined(MUVIT_IO_MIOBULB001)

    // Info
    #define MANUFACTURER        "MUVIT_IO"
    #define DEVICE              "MIOBULB001"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       4       // WHITE

// -----------------------------------------------------------------------------
// Hykker Power Plug (Smart Home Series) available in Jerónimo Martins Polska (Biedronka)
// https://www.hykker.com/akcesoria/gniazdo-wi-fi-z-licznikiem-energii/
// Reflashing from original Tuya firmware
// to thirdparty firmware like espurna by:
// https://github.com/ct-Open-Source/tuya-convert
// -----------------------------------------------------------------------------

#elif defined(HYKKER_SMART_HOME_POWER_PLUG)

    // Info
    #define MANUFACTURER                "HYKKER"
    #define DEVICE                      "SMART_HOME_POWER_PLUG"

    // Buttons
    #define BUTTON1_PIN                 0
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  14
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LED
    // Red
    #define LED1_PIN                    13
    #define LED1_MODE                   LED_MODE_WIFI
    #define LED1_PIN_INVERSE            1
    // Blue connected to relay

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             5
    #define HLW8012_CF_PIN              4

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       282060
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// Kogan Smarter Home Plug with Energy Meter (Australia)
// Product code: KASPEMHA
// https://www.kogan.com/au/buy/kogan-smarterhome-smart-plug-energy-meter/
// Reflashing from original Tuya firmware
// to thirdparty firmware like espurna by:
// https://github.com/ct-Open-Source/tuya-convert
// -----------------------------------------------------------------------------

#elif defined(KOGAN_SMARTER_HOME_PLUG_W_POW)

    // Info
    #define MANUFACTURER                "KOGAN"
    #define DEVICE                      "SMARTER_HOME_PLUG_W_POW"

    // Buttons
    #define BUTTON1_PIN                 0
    #define BUTTON1_CONFIG              BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  14
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LED
    // Red
    #define LED1_PIN                    13
    #define LED1_MODE                   LED_MODE_WIFI
    #define LED1_PIN_INVERSE            1
    // Blue connected to relay

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             5
    #define HLW8012_CF_PIN              4

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       282060
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// KINGART_CURTAIN_SWITCH
// -----------------------------------------------------------------------------
#elif defined(KINGART_CURTAIN_SWITCH)

    // Info
    #define MANUFACTURER            "KINGART"
    #define DEVICE                  "CURTAIN_SWITCH"

    // LEDs
    #define LED1_PIN                13
    #define LED1_PIN_INVERSE        1

    // KINGART module handles the UART, can't print any debug messages
    #define KINGART_CURTAIN_SUPPORT     1
    #define DEBUG_SERIAL_SUPPORT        0

// -----------------------------------------------------------------------------
// LSC Smart LED Light Strip (Smart CXonnect Series) available ACTION (Germany)
// https://www.action.com/de-de/p/lsc-smart-connect-intelligenter-multicolor-led-strip-/
// Reflashing from original Tuya firmware
// to thirdparty firmware like espurna by:
// https://github.com/ct-Open-Source/tuya-convert
// -----------------------------------------------------------------------------

#elif defined(LSC_SMART_LED_LIGHT_STRIP)
    // Info
    #define MANUFACTURER        "LSC"
    #define DEVICE              "SMART_LED_LIGHT_STRIP"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light RGBW
    #define LIGHT_CH1_PIN       4       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       13      // WHITE
    // #define LIGHT_CH5_PIN    5       // CW (not connected, but circuit supports it)

    // IR
    #define IR_SUPPORT          1
    #define IR_RX_PIN           0
    #define IR_TX_SUPPORT       0
    #define IR_RX_PRESET        5

// -----------------------------------------------------------------------------
// eHomeDIY WT02
// https://github.com/eHomeDIY/WT02-hardware
// -----------------------------------------------------------------------------
#elif defined(EHOMEDIY_WT02)

    // Info
    #define MANUFACTURER        "EHOMEDIY"
    #define DEVICE              "WT02"

    #define I2C_SDA_PIN         0
    #define I2C_SCL_PIN         2

    #define BMX280_SUPPORT        1
    // #define SI7021_SUPPORT        1

// -----------------------------------------------------------------------------
// eHomeDIY WT03
// https://github.com/eHomeDIY/WT03-hardware
// -----------------------------------------------------------------------------

#elif defined(EHOMEDIY_WT03)

    // Info
    #define MANUFACTURER        "EHOMEDIY"
    #define DEVICE              "WT03"

    #define I2C_SDA_PIN         2
    #define I2C_SCL_PIN         0

    #define BMX280_SUPPORT        1
    // #define SI7021_SUPPORT        1

// -----------------------------------------------------------------------------
// Linksprite R4
// http://linksprite.com/wiki/index.php?title=LinkNode_R4:_Arduino-compatible_WiFi_relay_controller
// -----------------------------------------------------------------------------

#elif defined(LINKSPRITE_LINKNODE_R4)

    // Info
    #define MANUFACTURER                "LINKSPRITE"
    #define DEVICE                      "LINKNODE_R4"

    // Relays
    #define RELAY1_PIN              12
    #define RELAY2_PIN              13
    #define RELAY3_PIN              14
    #define RELAY4_PIN              16

    #define RELAY1_TYPE             RELAY_TYPE_NORMAL
    #define RELAY2_TYPE             RELAY_TYPE_NORMAL
    #define RELAY3_TYPE             RELAY_TYPE_NORMAL
    #define RELAY4_TYPE             RELAY_TYPE_NORMAL

// -----------------------------------------------------------------------------
// NEDIS WIFIP310FWT Wi-Fi Smart Extension Socket
// 3x Schuko Type F, 4x USB, 16 A
// https://nedis.com/en-us/product/smart-living/smart-home/energy/550672299/wi-fi-smart-extension-socket-3x-schuko-type-f-4x-usb-16-a
// -----------------------------------------------------------------------------

#elif defined(NEDIS_WIFIP310FWT)

    // Info
    #define MANUFACTURER            "NEDIS"
    #define DEVICE                  "WIFIP310FWT"

    // Based on the reporter, this product uses GPIO1 and 3 for the button
    // and onboard LED, so hardware serial should be disabled...
    #define DEBUG_SERIAL_SUPPORT    0

    // Buttons
    #define BUTTON1_PIN             3
    #define BUTTON1_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN              5
    #define RELAY2_PIN              4
    #define RELAY3_PIN              13
    #define RELAY4_PIN              14

    // LEDs
    #define LED1_PIN                1
    #define LED1_PIN_INVERSE        1

// -----------------------------------------------------------------------------
// Arlec Smart PC190HA Plug
// https://templates.blakadder.com/arlec_PC190HA.html
// -----------------------------------------------------------------------------

#elif defined(ARLEC_PC190HA)

    // Info
    #define MANUFACTURER        "ARLEC"
    #define DEVICE              "PC190HA"

    // Buttons
    #define BUTTON1_PIN         14
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4   // blue LED
    #define LED1_PIN_INVERSE    1
    #define LED2_PIN            13  // red LED
    #define LED2_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// Arlec Smart PB89HA Power Strip
// https://templates.blakadder.com/arlec_PB89HA.html
// -----------------------------------------------------------------------------

#elif defined(ARLEC_PB89HA)

    // Info
    #define MANUFACTURER        "ARLEC"
    #define DEVICE              "PB89HA"

    // Buttons
    #define BUTTON1_PIN         3
    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          5
    #define RELAY2_PIN          4
    #define RELAY3_PIN          13
    #define RELAY4_PIN          12

    // LEDs
    #define LED1_PIN            1
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// Prodino WIFI
// https://kmpelectronics.eu/products/prodino-wifi-v1/
// -----------------------------------------------------------------------------

#elif defined(PRODINO_WIFI)

    // Info
    #define MANUFACTURER        "PRODINO"
    #define DEVICE              "WIFI"

    // MCP23S08
    #define MCP23S08_SUPPORT        1

    // Relays
    #define RELAY1_PIN              4
    #define RELAY1_PIN_TYPE         GPIO_TYPE_MCP23S08

    #define RELAY2_PIN              5
    #define RELAY2_PIN_TYPE         GPIO_TYPE_MCP23S08

    #define RELAY3_PIN              6
    #define RELAY3_PIN_TYPE         GPIO_TYPE_MCP23S08

    #define RELAY4_PIN              7
    #define RELAY4_PIN_TYPE         GPIO_TYPE_MCP23S08

    // Buttons
    #define BUTTON1_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PIN             0
    #define BUTTON1_PIN_TYPE        GPIO_TYPE_MCP23S08

    #define BUTTON2_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PIN             1
    #define BUTTON2_PIN_TYPE        GPIO_TYPE_MCP23S08

    #define BUTTON3_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_PIN             2
    #define BUTTON3_PIN_TYPE        GPIO_TYPE_MCP23S08

    #define BUTTON4_CONFIG          BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON4_PIN             3
    #define BUTTON4_PIN_TYPE        GPIO_TYPE_MCP23S08

    #define BUTTON1_RELAY           1
    #define BUTTON2_RELAY           2
    #define BUTTON3_RELAY           3
    #define BUTTON4_RELAY           4

    // LEDs
    #define LED1_PIN                2
    #define LED1_PIN_INVERSE        1

// -----------------------------------------------------------------------------
// Fcmila E27 7W RGB+W Bulb
// https://www.aliexpress.com/item/32925895199.html
// -----------------------------------------------------------------------------

#elif defined(FCMILA_E27_7W_RGBW)

    // Info
    #define MANUFACTURER        "FCMILA"
    #define DEVICE              "E27_7W_RGBW"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       4       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       5       // WHITE

// -----------------------------------------------------------------------------
// Benexmart 5W GU5.3 (MR16) RGBWW
// https://www.aliexpress.com/item/4001245365644.html
// -----------------------------------------------------------------------------

#elif defined(BENEXMART_GU53_RGBWW)

    // Info
    #define MANUFACTURER        "BENEXMART"
    #define DEVICE              "GU53_RGBWW"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       4       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       5       // WARM WHITE
    #define LIGHT_CH5_PIN       13      // COLD WHITE

// LSC E27 10W White Bulb with TYLC6E ESP8266 module
// https://www.action.com/de-at/p/lsc-smart-connect-intelligente-led-lampe/
// -----------------------------------------------------------------------------

#elif defined(LSC_E27_10W_WHITE)

    // Info
    #define MANUFACTURER                  "LSC"
    #define DEVICE                        "E27_10W_WHITE"
    #define LIGHT_PROVIDER                LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN                 5       // WARM WHITE LED PWM PIN
    #define LIGHT_CH1_INVERSE             0
    #define LIGHT_CH2_PIN                 4       // COLD WHITE LED PWM PIN
    #define LIGHT_CH2_INVERSE             0

// -----------------------------------------------------------------------------
// Mirabella Genio White A60
// https://www.woolworths.com.au/shop/productdetails/877102/mirabella-smart-led-gls-es-9w-cool-white
// Like https://www.mirabellagenio.com.au/product-range/mirabella-genio-wi-fi-dimmable-9w-led-gls-bulb/ 
// but in cardboard box, Item # I002604
// -----------------------------------------------------------------------------

#elif defined(MIRABELLA_GENIO_W_A60)

    // Info
    #define MANUFACTURER        "MIRABELLA"
    #define DEVICE              "GENIO_W_A60"
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER

    // Light
    #define LIGHT_CH1_PIN       14       // WHITE

// -----------------------------------------------------------------------------
// https://www.amazon.co.uk/gp/product/B086MV5MC8
//
// These don't come with an esp8266 anymore, but can be trivially converted
// as the new chip is pin compatible. Note, GPIO15 needs to be connected to
// GND on 1 and 2 gang switches in order to enable the ESP to boot.
//
// Older versions and some US models may still ship with ESP8266.
//
// Caution, do NOT solder a serial port while the board is connected to the mains baseboard,
// it will blow your fuse and your USB!
// If you need to flash with a programmer, use a bench power supply on the logic board only!
// -----------------------------------------------------------------------------

#elif defined(YAGUSMART_TOUCH_HWMOD_1G)
    #define MANUFACTURER        "YAGUSMART"
    #define DEVICE              "TOUCH_HWMOD_1G"

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PIN         3
    #define BUTTON1_RELAY       1
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY1_PIN          13

    #define LED2_PIN                    14
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FOLLOW
    #define LED2_RELAY                  1

    #define LED1_PIN                    0
    #define LED1_PIN_INVERSE            1
    #define LED1_MODE                   LED_MODE_WIFI

    #define DEBUG_SERIAL_SUPPORT        0

#elif defined(YAGUSMART_TOUCH_HWMOD_2G)
    #define MANUFACTURER        "YAGUSMART"
    #define DEVICE              "TOUCH_HWMOD_2G"

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PIN         3
    #define BUTTON1_RELAY       1
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE

    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PIN         5
    #define BUTTON2_RELAY       2
    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK       BUTTON_ACTION_NONE

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY1_PIN          13
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_PIN          4

    #define LED2_PIN                    14
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FOLLOW
    #define LED2_RELAY                  1

    #define LED3_PIN                    1
    #define LED3_PIN_INVERSE            1
    #define LED3_MODE                   LED_MODE_FOLLOW
    #define LED3_RELAY                  2

    #define LED1_PIN                    0
    #define LED1_PIN_INVERSE            1
    #define LED1_MODE                   LED_MODE_WIFI

    #define DEBUG_SERIAL_SUPPORT        0

#elif defined(YAGUSMART_TOUCH_HWMOD_3G)
    #define MANUFACTURER        "YAGUSMART"
    #define DEVICE              "TOUCH_HWMOD_3G"

    #define BUTTON1_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON1_PIN         12
    #define BUTTON1_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON1_CLICK       BUTTON_ACTION_NONE

    #define BUTTON2_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON2_PIN         3
    #define BUTTON2_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON2_CLICK       BUTTON_ACTION_NONE

    #define BUTTON3_CONFIG      BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON3_PIN         5
    #define BUTTON3_PRESS       BUTTON_ACTION_TOGGLE
    #define BUTTON3_CLICK       BUTTON_ACTION_NONE

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY1_PIN          13
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_PIN          4
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_PIN          15

    #define LED2_PIN                    16
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FOLLOW
    #define LED2_RELAY                  1

    #define LED3_PIN                    14
    #define LED3_PIN_INVERSE            1
    #define LED3_MODE                   LED_MODE_FOLLOW
    #define LED3_RELAY                  2

    #define LED4_PIN                    1
    #define LED4_PIN_INVERSE            1
    #define LED4_MODE                   LED_MODE_FOLLOW
    #define LED4_RELAY                  3

    #define LED1_PIN                    0
    #define LED1_PIN_INVERSE            1
    #define LED1_MODE                   LED_MODE_WIFI

    #define DEBUG_SERIAL_SUPPORT        0

#else

    #error "UNSUPPORTED HARDWARE!!"

#endif
