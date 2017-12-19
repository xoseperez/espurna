// -----------------------------------------------------------------------------
// SENSORS
// -----------------------------------------------------------------------------

#define SENSOR_READ_INTERVAL            6000            // Read data from sensors every 6 seconds
#define SENSOR_REPORT_EVERY             10              // Report every this many readings
#define SENSOR_USE_INDEX                0               // Use the index in topic (i.e. temperature/0)
                                                        // even if just one sensor (0 for backwards compatibility)

#ifndef SENSOR_TEMPERATURE_UNITS
#define SENSOR_TEMPERATURE_UNITS        TMP_CELSIUS     // Temperature units (TMP_CELSIUS | TMP_FAHRENHEIT)
#endif

#ifndef SENSOR_TEMPERATURE_CORRECTION
#define SENSOR_TEMPERATURE_CORRECTION   0.0             // Offset correction
#endif

#ifndef TEMPERATURE_MIN_CHANGE
#define TEMPERATURE_MIN_CHANGE          0.0             // Minimum temperature change to report
#endif

#ifndef HUMIDITY_MIN_CHANGE
#define HUMIDITY_MIN_CHANGE             0               // Minimum humidity change to report
#endif

#define SENSOR_TEMPERATURE_DECIMALS     1
#define SENSOR_HUMIDITY_DECIMALS        0
#define SENSOR_PRESSURE_DECIMALS        2
#define SENSOR_ANALOG_DECIMALS          0
#define SENSOR_EVENTS_DECIMALS          0
#define SENSOR_CURRENT_DECIMALS         3
#define SENSOR_VOLTAGE_DECIMALS         0
#define SENSOR_POWER_DECIMALS           0
#define SENSOR_POWER_FACTOR_DECIMALS    0
#define SENSOR_ENERGY_DECIMALS          0
#define SENSOR_PM1dot0_DECIMALS         0
#define SENSOR_PM2dot5_DECIMALS         0
#define SENSOR_PM10_DECIMALS            0
#define SENSOR_CO2_DECIMALS             0

#define SENSOR_UNKNOWN_TOPIC            "unknown"
#define SENSOR_TEMPERATURE_TOPIC        "temperature"
#define SENSOR_HUMIDITY_TOPIC           "humidity"
#define SENSOR_PRESSURE_TOPIC           "pressure"
#define SENSOR_CURRENT_TOPIC            "current"
#define SENSOR_VOLTAGE_TOPIC            "voltage"
#define SENSOR_ACTIVE_POWER_TOPIC       "power"
#define SENSOR_APPARENT_POWER_TOPIC     "apparent"
#define SENSOR_REACTIVE_POWER_TOPIC     "reactive"
#define SENSOR_POWER_FACTOR_TOPIC       "factor"
#define SENSOR_ENERGY_TOPIC             "energy"
#define SENSOR_ENERGY_DELTA_TOPIC       "energy_delta"
#define SENSOR_PM1dot0_TOPIC            "pm1dot0"
#define SENSOR_PM2dot5_TOPIC            "pm2dot5"
#define SENSOR_PM10_TOPIC               "pm10"
#define SENSOR_ANALOG_TOPIC             "analog"
#define SENSOR_DIGITAL_TOPIC            "digital"
#define SENSOR_EVENTS_TOPIC             "events"
#define SENSOR_CO2_TOPIC                "co2"

//--------------------------------------------------------------------------------
// DHTXX temperature/humidity sensor
// Enable support by passing DHT_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef DHT_SUPPORT
#define DHT_SUPPORT                 0
#endif

#ifndef DHT_PIN
#define DHT_PIN                     14
#endif

#ifndef DHT_TYPE
#define DHT_TYPE                    DHT22
#endif

#ifndef DHT_PULLUP
#define DHT_PULLUP                  0
#endif

#define HUMIDITY_NORMAL             0
#define HUMIDITY_COMFORTABLE        1
#define HUMIDITY_DRY                2
#define HUMIDITY_WET                3

//--------------------------------------------------------------------------------
// SI7021 temperature & humidity sensor
// Enable support by passing SI7021_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef SI7021_SUPPORT
#define SI7021_SUPPORT              0
#endif

#ifndef SI7021_ADDRESS
#define SI7021_ADDRESS              0x40
#endif

//--------------------------------------------------------------------------------
// BMX280
// Enable support by passing BMX280_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef BMX280_SUPPORT
#define BMX280_SUPPORT              0
#endif

#ifndef BMX280_ADDRESS
#define BMX280_ADDRESS              0x76
#endif

#define BMX280_MODE                 1       // 1 for forced mode, 3 for normal mode
#define BMX280_TEMPERATURE          1       // Oversampling for temperature (set to 0 to disable magnitude)
#define BMX280_HUMIDITY             1       // Oversampling for humidity (set to 0 to disable magnitude, only for BME280)
#define BMX280_PRESSURE             1       // Oversampling for pressure (set to 0 to disable magnitude)

//--------------------------------------------------------------------------------
// DS18B20 temperature sensor
// Enable support by passing DALLAS_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef DALLAS_SUPPORT
#define DALLAS_SUPPORT             0
#endif

#ifndef DALLAS_PIN
#define DALLAS_PIN                 13
#endif

#ifndef DALLAS_PULLUP
#define DALLAS_PULLUP              1
#endif

#define DALLAS_RESOLUTION          9        // Not used atm

//--------------------------------------------------------------------------------
// Digital sensor
// Enable support by passing DIGITAL_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef DIGITAL_SUPPORT
#define DIGITAL_SUPPORT             0
#endif

#ifndef DIGITAL_PIN
#define DIGITAL_PIN                 2
#endif

#ifndef DIGITAL_PIN_MODE
#define DIGITAL_PIN_MODE            INPUT_PULLUP
#endif

#ifndef DIGITAL_DEFAULT_STATE
#define DIGITAL_DEFAULT_STATE       1
#endif


//--------------------------------------------------------------------------------
// Analog sensor
// Enable support by passing ANALOG_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef ANALOG_SUPPORT
#define ANALOG_SUPPORT              0
#endif

#ifndef ANALOG_PIN
#define ANALOG_PIN                  0
#endif

#ifndef ANALOG_PIN_MODE
#define ANALOG_PIN_MODE             INPUT
#endif

#if ANALOG_SUPPORT
    #undef ADC_VCC_ENABLED
    #define ADC_VCC_ENABLED         0
#endif

//--------------------------------------------------------------------------------
// Counter sensor
// Enable support by passing EVENTS_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef EVENTS_SUPPORT
#define EVENTS_SUPPORT             0           // Do not build with counter support by default
#endif

#ifndef EVENTS_PIN
#define EVENTS_PIN                 2           // GPIO to monitor
#endif

#ifndef EVENTS_PIN_MODE
#define EVENTS_PIN_MODE            INPUT       // INPUT, INPUT_PULLUP
#endif

#ifndef EVENTS_INTERRUPT_MODE
#define EVENTS_INTERRUPT_MODE      RISING      // RISING, FALLING, BOTH
#endif

#define EVENTS_DEBOUNCE            50          // Do not register events within less than 10 millis

//--------------------------------------------------------------------------------
// Energy Monitor general settings
//--------------------------------------------------------------------------------

#define EMON_MAX_SAMPLES            1000        // Max number of samples to get
#define EMON_MAX_TIME               250         // Max time in ms to sample
#define EMON_FILTER_SPEED           512         // Mobile average filter speed
#define EMON_MAINS_VOLTAGE          230         // Mains voltage
#define EMON_REPORT_CURRENT         0           // Report current
#define EMON_REPORT_POWER           1           // Report power
#define EMON_REPORT_ENERGY          1           // Report energy

//--------------------------------------------------------------------------------
// Energy Monitor based on interval analog GPIO
// Enable support by passing EMON_ANALOG_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef EMON_ANALOG_SUPPORT
#define EMON_ANALOG_SUPPORT             0       // Do not build support by default
#endif

#define EMON_ANALOG_CURRENT_RATIO       30      // Current ratio in the clamp (30V/1A)
#define EMON_ANALOG_ADC_BITS            10      // ADC depth
#define EMON_ANALOG_REFERENCE_VOLTAGE   3.3     // Reference voltage of the ADC

#if EMON_ANALOG_SUPPORT
    #undef ADC_VCC_ENABLED
    #define ADC_VCC_ENABLED             0
#endif

//--------------------------------------------------------------------------------
// Energy Monitor based on ADC121
// Enable support by passing EMON_ADC121_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef EMON_ADC121_SUPPORT
#define EMON_ADC121_SUPPORT             0       // Do not build support by default
#endif

#define EMON_ADC121_I2C_ADDRESS         0x50    // I2C address of the ADC121

#define EMON_ADC121_CURRENT_RATIO       30      // Current ratio in the clamp (30V/1A)
#define EMON_ADC121_ADC_BITS            12      // ADC depth
#define EMON_ADC121_REFERENCE_VOLTAGE   3.3     // Reference voltage of the ADC

//--------------------------------------------------------------------------------
// Energy Monitor based on ADS1X15
// Enable support by passing EMON_ADS1X15_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef EMON_ADS1X15_SUPPORT
#define EMON_ADS1X15_SUPPORT            0       // Do not build support by default
#endif

#define EMON_ADS1X15_ADS1115            1       // 0 for ADS10115, 1 for ADS1115
#define EMON_ADS1X15_PORT_MASK          0x08    // A0=1 A1=2 A2=4 A4=8
#define EMON_ADS1X15_I2C_ADDRESS        0x48    // I2C address of the ADS1115

#define EMON_ADS1X15_CURRENT_RATIO      30      // Current ratio in the clamp (30V/1A)
#define EMON_ADS1X15_ADC_BITS           16      // ADC depth
#define EMON_ADS1X15_REFERENCE_VOLTAGE  8.192   // Double the gain for peak-to-peak

//--------------------------------------------------------------------------------
// Particle Monitor based on Plantower PMSX003
// Enable support by passing PMSX003_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef PMSX003_SUPPORT
#define PMSX003_SUPPORT                 0
#endif

#define PMS_RX_PIN                      13
#define PMS_TX_PIN                      15

//--------------------------------------------------------------------------------
// MHZ19 CO2 sensor
// Enable support by passing MHZ19_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef MHZ19_SUPPORT
#define MHZ19_SUPPORT                   0
#endif

#define MHZ19_RX_PIN                    13
#define MHZ19_TX_PIN                    15

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
