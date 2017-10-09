//--------------------------------------------------------------------------------
// Custom RF module
// Check http://tinkerman.cat/adding-rf-to-a-non-rf-itead-sonoff/
// Enable support by passing RF_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef RF_SUPPORT
#define RF_SUPPORT                  0
#endif

#define RF_PIN                      14
#define RF_CHANNEL                  31
#define RF_DEVICE                   1

//--------------------------------------------------------------------------------
// DHTXX temperature/humidity sensor
// Enable support by passing DHT_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef DHT_SUPPORT
#define DHT_SUPPORT                 0
#endif

#define DHT_PIN                     14
#define DHT_UPDATE_INTERVAL         60000
#define DHT_TYPE                    DHT22
#define DHT_TIMING                  11
#define DHT_TEMPERATURE_TOPIC       "temperature"
#define DHT_HUMIDITY_TOPIC          "humidity"

#define HUMIDITY_NORMAL             0
#define HUMIDITY_COMFORTABLE        1
#define HUMIDITY_DRY                2
#define HUMIDITY_WET                3

//--------------------------------------------------------------------------------
// Analog sensor
// Enable support by passing ANALOG_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef ANALOG_SUPPORT
#define ANALOG_SUPPORT              0
#endif

#define ANALOG_PIN                  0
#define ANALOG_UPDATE_INTERVAL      60000
#define ANALOG_TOPIC                "analog"

#if ANALOG_SUPPORT
    #undef ADC_VCC_ENABLED
    #define ADC_VCC_ENABLED         0
#endif

//--------------------------------------------------------------------------------
// Counter sensor
// Enable support by passing COUNTER_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef COUNTER_SUPPORT
#define COUNTER_SUPPORT             0           // Do not build with counter support by default
#endif

#define COUNTER_PIN                 2           // GPIO to monitor
#define COUNTER_PIN_MODE            INPUT       // INPUT, INPUT_PULLUP
#define COUNTER_INTERRUPT_MODE      RISING      // RISING, FALLING, BOTH
#define COUNTER_UPDATE_INTERVAL     5000        // Update counter every this millis
#define COUNTER_REPORT_EVERY        12          // Report counter every this updates (1 minute)
#define COUNTER_DEBOUNCE            10          // Do not register events within less than 10 millis
#define COUNTER_TOPIC               "counter"   // Default topic for MQTT, API and InfluxDB

//--------------------------------------------------------------------------------
// DS18B20 temperature sensor
// Enable support by passing DS18B20_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef DS18B20_SUPPORT
#define DS18B20_SUPPORT             1
#endif

#define DS18B20_PIN                 2
#define DS18B20_UPDATE_INTERVAL     5000
#define DS18B20_TEMPERATURE_TOPIC   "temperature"
//Will only send MQTT update if the value has changed by this amount (0.0 sends every interval)
#define DS18B20_UPDATE_ON_CHANGE    1.0 

//--------------------------------------------------------------------------------
// Internal power montior
// Enable support by passing ADC_VCC_ENABLED=1 build flag
// Do not enable this if using the analog GPIO for any other thing
//--------------------------------------------------------------------------------

#ifndef ADC_VCC_ENABLED
#define ADC_VCC_ENABLED             1
#endif

#if ADC_VCC_ENABLED
    ADC_MODE(ADC_VCC);
#endif
