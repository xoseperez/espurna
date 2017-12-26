// =============================================================================
// SENSORS - General data
// =============================================================================

#define SENSOR_DEBUG                        0               // Debug sensors

#define SENSOR_READ_INTERVAL                6               // Read data from sensors every 6 seconds
#define SENSOR_READ_MIN_INTERVAL            6               // Minimum read interval
#define SENSOR_READ_MAX_INTERVAL            3600            // Maximum read interval

#define SENSOR_REPORT_EVERY                 10              // Report every this many readings
#define SENSOR_REPORT_MIN_EVERY             1               // Minimum every value
#define SENSOR_REPORT_MAX_EVERY             12              // Maximum

#define SENSOR_USE_INDEX                    0               // Use the index in topic (i.e. temperature/0)
                                                            // even if just one sensor (0 for backwards compatibility)

#ifndef SENSOR_TEMPERATURE_UNITS
#define SENSOR_TEMPERATURE_UNITS            TMP_CELSIUS     // Temperature units (TMP_CELSIUS | TMP_FAHRENHEIT)
#endif

#ifndef SENSOR_TEMPERATURE_CORRECTION
#define SENSOR_TEMPERATURE_CORRECTION       0.0             // Offset correction
#endif

#ifndef TEMPERATURE_MIN_CHANGE
#define TEMPERATURE_MIN_CHANGE              0.0             // Minimum temperature change to report
#endif

#ifndef HUMIDITY_MIN_CHANGE
#define HUMIDITY_MIN_CHANGE                 0               // Minimum humidity change to report
#endif

#define HUMIDITY_NORMAL                     0
#define HUMIDITY_COMFORTABLE                1
#define HUMIDITY_DRY                        2
#define HUMIDITY_WET                        3

//--------------------------------------------------------------------------------
// Magnitudes
//--------------------------------------------------------------------------------

#define MAGNITUDE_TEMPERATURE_DECIMALS      1
#define MAGNITUDE_HUMIDITY_DECIMALS         0
#define MAGNITUDE_PRESSURE_DECIMALS         2
#define MAGNITUDE_ANALOG_DECIMALS           0
#define MAGNITUDE_EVENTS_DECIMALS           0
#define MAGNITUDE_CURRENT_DECIMALS          3
#define MAGNITUDE_VOLTAGE_DECIMALS          0
#define MAGNITUDE_POWER_DECIMALS            0
#define MAGNITUDE_POWER_FACTOR_DECIMALS     0
#define MAGNITUDE_ENERGY_DECIMALS           0
#define MAGNITUDE_PM1dot0_DECIMALS          0
#define MAGNITUDE_PM2dot5_DECIMALS          0
#define MAGNITUDE_PM10_DECIMALS             0
#define MAGNITUDE_CO2_DECIMALS              0

#define MAGNITUDE_UNKNOWN_TOPIC             "unknown"
#define MAGNITUDE_TEMPERATURE_TOPIC         "temperature"
#define MAGNITUDE_HUMIDITY_TOPIC            "humidity"
#define MAGNITUDE_PRESSURE_TOPIC            "pressure"
#define MAGNITUDE_CURRENT_TOPIC             "current"
#define MAGNITUDE_VOLTAGE_TOPIC             "voltage"
#define MAGNITUDE_ACTIVE_POWER_TOPIC        "power"
#define MAGNITUDE_APPARENT_POWER_TOPIC      "apparent"
#define MAGNITUDE_REACTIVE_POWER_TOPIC      "reactive"
#define MAGNITUDE_POWER_FACTOR_TOPIC        "factor"
#define MAGNITUDE_ENERGY_TOPIC              "energy"
#define MAGNITUDE_ENERGY_DELTA_TOPIC        "energy_delta"
#define MAGNITUDE_PM1dot0_TOPIC             "pm1dot0"
#define MAGNITUDE_PM2dot5_TOPIC             "pm2dot5"
#define MAGNITUDE_PM10_TOPIC                "pm10"
#define MAGNITUDE_ANALOG_TOPIC              "analog"
#define MAGNITUDE_DIGITAL_TOPIC             "digital"
#define MAGNITUDE_EVENTS_TOPIC              "events"
#define MAGNITUDE_CO2_TOPIC                 "co2"

//--------------------------------------------------------------------------------
// Sensor ID
// These should remain over time, do not modify them, only add new ones at the end
//--------------------------------------------------------------------------------

#define SENSOR_DHTXX_ID                 0x01
#define SENSOR_DALLAS_ID                0x02
#define SENSOR_EMON_ANALOG_ID           0x03
#define SENSOR_EMON_ADC121_ID           0x04
#define SENSOR_EMON_ADS1X15_ID          0x05
#define SENSOR_HLW8012_ID               0x06
#define SENSOR_V9261F_ID                0x07
#define SENSOR_ECH1560_ID               0x08
#define SENSOR_ANALOG_ID                0x09
#define SENSOR_DIGITAL_ID               0x10
#define SENSOR_EVENTS_ID                0x11
#define SENSOR_PMSX003_ID               0x12
#define SENSOR_BMX280_ID                0x13
#define SENSOR_MHZ19_ID                 0x14
#define SENSOR_SI7021_ID                0x15
#define SENSOR_SHT3X_I2C_ID             0x16

// =============================================================================
// Specific data for each sensor
// =============================================================================

//------------------------------------------------------------------------------
// Analog sensor
// Enable support by passing ANALOG_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef ANALOG_SUPPORT
#define ANALOG_SUPPORT                  0
#endif

#if ANALOG_SUPPORT
#undef ADC_VCC_ENABLED
#define ADC_VCC_ENABLED                 0
#endif

//------------------------------------------------------------------------------
// BME280/BMP280
// Enable support by passing BMX280_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef BMX280_SUPPORT
#define BMX280_SUPPORT                  0
#endif

#ifndef BMX280_ADDRESS
#define BMX280_ADDRESS                  0x00    // 0x00 means auto
#endif

#define BMX280_MODE                     1       // 1 for forced mode, 3 for normal mode
#define BMX280_TEMPERATURE              1       // Oversampling for temperature (set to 0 to disable magnitude)
#define BMX280_HUMIDITY                 1       // Oversampling for humidity (set to 0 to disable magnitude, only for BME280)
#define BMX280_PRESSURE                 1       // Oversampling for pressure (set to 0 to disable magnitude)

#if BMX280_SUPPORT
#undef I2C_SUPPORT
#define I2C_SUPPORT                     1
#endif

//------------------------------------------------------------------------------
// Dallas OneWire temperature sensors
// Enable support by passing DALLAS_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef DALLAS_SUPPORT
#define DALLAS_SUPPORT                  0
#endif

#ifndef DALLAS_PIN
#define DALLAS_PIN                      13
#endif

#define DALLAS_RESOLUTION               9        // Not used atm
#define DALLAS_READ_INTERVAL            2000     // Force sensor read & cache every 2 seconds

//------------------------------------------------------------------------------
// DHTXX temperature/humidity sensor
// Enable support by passing DHT_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef DHT_SUPPORT
#define DHT_SUPPORT                     0
#endif

#ifndef DHT_PIN
#define DHT_PIN                         13
#endif

#ifndef DHT_TYPE
#define DHT_TYPE                        DHT_CHIP_DHT22
#endif

//------------------------------------------------------------------------------
// Digital sensor
// Enable support by passing DIGITAL_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef DIGITAL_SUPPORT
#define DIGITAL_SUPPORT                 0
#endif

#ifndef DIGITAL_PIN
#define DIGITAL_PIN                     2
#endif

#ifndef DIGITAL_PIN_MODE
#define DIGITAL_PIN_MODE                INPUT_PULLUP
#endif

#ifndef DIGITAL_DEFAULT_STATE
#define DIGITAL_DEFAULT_STATE           1
#endif

//------------------------------------------------------------------------------
// ECH1560 based power sensor
// Enable support by passing ECH1560_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef ECH1560_SUPPORT
#define ECH1560_SUPPORT                 0
#endif

#ifndef ECH1560_CLK_PIN
#define ECH1560_CLK_PIN                 4       // CLK pin for the ECH1560
#endif

#ifndef ECH1560_MISO_PIN
#define ECH1560_MISO_PIN                5       // MISO pin for the ECH1560
#endif

#ifndef ECH1560_INVERTED
#define ECH1560_INVERTED                0       // Signal is inverted
#endif

//------------------------------------------------------------------------------
// Energy Monitor general settings
//------------------------------------------------------------------------------

#define EMON_MAX_SAMPLES                1000        // Max number of samples to get
#define EMON_MAX_TIME                   250         // Max time in ms to sample
#define EMON_FILTER_SPEED               512         // Mobile average filter speed
#define EMON_MAINS_VOLTAGE              230         // Mains voltage
#define EMON_REFERENCE_VOLTAGE          3.3         // Reference voltage of the ADC
#define EMON_CURRENT_RATIO              30          // Current ratio in the clamp (30V/1A)
#define EMON_REPORT_CURRENT             0           // Report current
#define EMON_REPORT_POWER               1           // Report power
#define EMON_REPORT_ENERGY              1           // Report energy

//------------------------------------------------------------------------------
// Energy Monitor based on ADC121
// Enable support by passing EMON_ADC121_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef EMON_ADC121_SUPPORT
#define EMON_ADC121_SUPPORT             0       // Do not build support by default
#endif

#define EMON_ADC121_I2C_ADDRESS         0x00    // 0x00 means auto

#if EMON_ADC121_SUPPORT
#undef I2C_SUPPORT
#define I2C_SUPPORT                     1
#endif

//------------------------------------------------------------------------------
// Energy Monitor based on ADS1X15
// Enable support by passing EMON_ADS1X15_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef EMON_ADS1X15_SUPPORT
#define EMON_ADS1X15_SUPPORT            0       // Do not build support by default
#endif

#define EMON_ADS1X15_I2C_ADDRESS        0x00    // 0x00 means auto
#define EMON_ADS1X15_TYPE               ADS1X15_CHIP_ADS1115
#define EMON_ADS1X15_GAIN               ADS1X15_REG_CONFIG_PGA_4_096V
#define EMON_ADS1X15_MASK               0x0F    // A0=1 A1=2 A2=4 A4=8

#if EMON_ADS1X15_SUPPORT
#undef I2C_SUPPORT
#define I2C_SUPPORT                     1
#endif

//------------------------------------------------------------------------------
// Energy Monitor based on interval analog GPIO
// Enable support by passing EMON_ANALOG_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef EMON_ANALOG_SUPPORT
#define EMON_ANALOG_SUPPORT             0       // Do not build support by default
#endif

#if EMON_ANALOG_SUPPORT
#undef ADC_VCC_ENABLED
#define ADC_VCC_ENABLED                 0
#endif

//------------------------------------------------------------------------------
// Counter sensor
// Enable support by passing EVENTS_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef EVENTS_SUPPORT
#define EVENTS_SUPPORT                  0       // Do not build with counter support by default
#endif

#ifndef EVENTS_PIN
#define EVENTS_PIN                      2       // GPIO to monitor
#endif

#ifndef EVENTS_PIN_MODE
#define EVENTS_PIN_MODE                 INPUT   // INPUT, INPUT_PULLUP
#endif

#ifndef EVENTS_INTERRUPT_MODE
#define EVENTS_INTERRUPT_MODE           RISING  // RISING, FALLING, BOTH
#endif

#define EVENTS_DEBOUNCE                 50      // Do not register events within less than 10 millis

//------------------------------------------------------------------------------
// HLW8012 Energy monitor IC
// Enable support by passing HLW8012_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef HLW8012_SUPPORT
#define HLW8012_SUPPORT                 0
#endif

#ifndef HLW8012_SEL_PIN
#define HLW8012_SEL_PIN                 5
#endif

#ifndef HLW8012_CF1_PIN
#define HLW8012_CF1_PIN                 13
#endif

#ifndef HLW8012_CF_PIN
#define HLW8012_CF_PIN                  14
#endif

#define HLW8012_USE_INTERRUPTS          1       // Use interrupts to trap HLW8012 signals
#define HLW8012_SEL_CURRENT             HIGH    // SEL pin to HIGH to measure current
#define HLW8012_CURRENT_R               0.001   // Current resistor
#define HLW8012_VOLTAGE_R_UP            ( 5 * 470000 )  // Upstream voltage resistor
#define HLW8012_VOLTAGE_R_DOWN          ( 1000 )        // Downstream voltage resistor

//------------------------------------------------------------------------------
// MHZ19 CO2 sensor
// Enable support by passing MHZ19_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef MHZ19_SUPPORT
#define MHZ19_SUPPORT                   0
#endif

#define MHZ19_RX_PIN                    13
#define MHZ19_TX_PIN                    15

//------------------------------------------------------------------------------
// Particle Monitor based on Plantower PMSX003
// Enable support by passing PMSX003_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef PMSX003_SUPPORT
#define PMSX003_SUPPORT                 0
#endif

#define PMS_RX_PIN                      13
#define PMS_TX_PIN                      15

//------------------------------------------------------------------------------
// SHT3X I2C (Wemos) temperature & humidity sensor
// Enable support by passing SHT3X_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef SHT3X_I2C_SUPPORT
#define SHT3X_I2C_SUPPORT               0
#endif

#ifndef SHT3X_I2C_ADDRESS
#define SHT3X_I2C_ADDRESS               0x00    // 0x00 means auto
#endif

#if SHT3X_I2C_SUPPORT
#undef I2C_SUPPORT
#define I2C_SUPPORT                     1
#endif

//------------------------------------------------------------------------------
// SI7021 temperature & humidity sensor
// Enable support by passing SI7021_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef SI7021_SUPPORT
#define SI7021_SUPPORT                  0
#endif

#ifndef SI7021_ADDRESS
#define SI7021_ADDRESS                  0x00    // 0x00 means auto
#endif

#if SI7021_SUPPORT
#undef I2C_SUPPORT
#define I2C_SUPPORT                     1
#endif

//------------------------------------------------------------------------------
// V9261F based power sensor
// Enable support by passing SI7021_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef V9261F_SUPPORT
#define V9261F_SUPPORT                  0
#endif

#ifndef V9261F_PIN
#define V9261F_PIN                      2       // TX pin from the V9261F
#endif

#ifndef V9261F_PIN_INVERSE
#define V9261F_PIN_INVERSE              1       // Signal is inverted
#endif

#define V9261F_SYNC_INTERVAL            600     // Sync signal length (ms)
#define V9261F_BAUDRATE                 4800    // UART baudrate

// Default ratios
#define V9261F_CURRENT_FACTOR           79371434.0
#define V9261F_VOLTAGE_FACTOR           4160651.0
#define V9261F_POWER_FACTOR             153699.0
#define V9261F_RPOWER_FACTOR            V9261F_CURRENT_FACTOR

// =============================================================================
// Sensor helpers configuration
// =============================================================================

// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------

#ifndef I2C_SUPPORT
#define I2C_SUPPORT                     0       // I2C enabled (1.98Kb)
#endif

#define I2C_USE_BRZO                    0       // Use brzo_i2c library or standard Wire

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN                     SDA     // SDA GPIO (Sonoff => 4)
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN                     SCL     // SCL GPIO (Sonoff => 14)
#endif

#define I2C_CLOCK_STRETCH_TIME          200     // BRZO clock stretch time
#define I2C_SCL_FREQUENCY               1000    // BRZO SCL frequency
#define I2C_CLEAR_BUS                   0       // Clear I2C bus at boot

//--------------------------------------------------------------------------------
// Internal power monitor
// Enable support by passing ADC_VCC_ENABLED=1 build flag
// Do not enable this if using the analog GPIO for any other thing
//--------------------------------------------------------------------------------

#ifndef ADC_VCC_ENABLED
#define ADC_VCC_ENABLED                 1
#endif

#if ADC_VCC_ENABLED
    ADC_MODE(ADC_VCC);
#endif

//--------------------------------------------------------------------------------
// Class loading
//--------------------------------------------------------------------------------

#include "../sensors/BaseSensor.h"

#if ANALOG_SUPPORT
    #include "../sensors/AnalogSensor.h"
#endif

#if BMX280_SUPPORT
    #include <SparkFunBME280.h>
    #include "../sensors/BMX280Sensor.h"
#endif

#if DALLAS_SUPPORT
    #include <OneWire.h>
    #include "../sensors/DallasSensor.h"
#endif

#if DHT_SUPPORT
    #include "../sensors/DHTSensor.h"
#endif

#if DIGITAL_SUPPORT
    #include "../sensors/DigitalSensor.h"
#endif

#if ECH1560_SUPPORT
    #include "../sensors/ECH1560Sensor.h"
#endif

#if EMON_ADC121_SUPPORT
    #include "../sensors/EmonADC121Sensor.h"
#endif

#if EMON_ADS1X15_SUPPORT
    #include "../sensors/EmonADS1X15Sensor.h"
#endif

#if EMON_ANALOG_SUPPORT
    #include "../sensors/EmonAnalogSensor.h"
#endif

#if EVENTS_SUPPORT
    #include "../sensors/EventSensor.h"
#endif

#if HLW8012_SUPPORT
    #include <HLW8012.h>
    #include "../sensors/HLW8012Sensor.h"
#endif

#if MHZ19_SUPPORT
    #include <SoftwareSerial.h>
    #include "../sensors/MHZ19Sensor.h"
#endif

#if PMSX003_SUPPORT
    #include <SoftwareSerial.h>
    #include <PMS.h>
    #include "../sensors/PMSX003Sensor.h"
#endif

#if SI7021_SUPPORT
    #include "../sensors/SI7021Sensor.h"
#endif

#if SHT3X_I2C_SUPPORT
    #include "../sensors/SHT3XI2CSensor.h"
#endif

#if V9261F_SUPPORT
    #include <SoftwareSerial.h>
    #include "../sensors/V9261FSensor.h"
#endif
