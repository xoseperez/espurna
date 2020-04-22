/*

SENSOR MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

struct sensor_magnitude_t;

//--------------------------------------------------------------------------------

namespace sensor {

namespace type {

enum Type : unsigned char {
    Base = 0,
    Emon = 1 << 0,
    Analog = 1 << 1
};

} // namespace type

enum class Unit : int {
    Min_,
    None,
    Celcius,
    Farenheit,
    Kelvin,
    Percentage,
    Hectopascal,
    Ampere,
    Volt,
    Voltampere,
    Kilovoltampere,
    VoltampereReactive,
    KilovoltampereReactive,
    Watt,
    Kilowatt,
    WattSecond,
    Joule = WattSecond,
    KilowattHour,
    PartsPerMillion,
    Ohm,
    MicrogrammPerCubicMeter,   // The concentration of an air pollutant
    MilligrammPerCubicMeter,   //
    Lux,
    UltravioletIndex,          // "measurement of the strength of sunburn-producing ultraviolet (UV) radiation at a particular place and time"
                               // (XXX: Not a unit. Distinguish from None and specify decimals)
    CountsPerMinute,           // Unit of local dose rate (Geiger counting)
    MicrosievertPerHour,       // 2nd unit of local dose rate (Geiger counting)
    Meter,
    Max_
};

// Base units are 32 bit since we are the fastest with them.

struct Ws {
    Ws();
    Ws(uint32_t);
    uint32_t value;
};

struct Wh {
    Wh();
    Wh(Ws);
    Wh(uint32_t);
    uint32_t value;
};

struct KWh {
    KWh();
    KWh(Ws);
    KWh(Wh);
    KWh(uint32_t);
    uint32_t value;
};

struct Energy {

    constexpr static uint32_t KwhMultiplier = 3600000ul;
    constexpr static uint32_t KwhLimit = ((1ul << 31ul) / KwhMultiplier);

    Energy() = default;

    // TODO: while we accept ws >= the kwh conversion limit,
    // should this be dealt with on the unit level?
    Energy(double);
    Energy(KWh, Ws);
    Energy(KWh);
    Energy(Wh);
    Energy(Ws);

    // Sets internal counters to zero
    void reset();

    // Check whether we have *any* energy recorded. Can be zero:
    // - on cold boot
    // - on overflow
    // - when we call `reset()`
    operator bool();

    // Generic conversion as-is
    double asDouble();

    // Convert back to input unit, with overflow mechanics when kwh values goes over 32 bit
    Ws asWs();

    // Generic sensors output energy in joules / watt-second
    Energy& operator +=(Ws);
    Energy operator +(Ws);

    // But sometimes we want to accept asDouble() value back
    Energy& operator =(double);

    // We are storing a kind-of integral and fractional parts
    // Using watt-second to avoid loosing precision, we don't expect these to be accessed directly
    KWh kwh;
    Ws ws;
};

}

String magnitudeName(unsigned char index);
String magnitudeUnits(unsigned char index);
unsigned char magnitudeType(unsigned char index);

// XXX: without param name it is kind of vague what exactly unsigned char is
//      consider using index instead of type or adding stronger param type
String magnitudeTopic(unsigned char type);

String magnitudeTopic(const sensor_magnitude_t& magnitude);
String magnitudeUnits(const sensor_magnitude_t& magnitude);

unsigned char sensorCount();
unsigned char magnitudeCount();

double magnitudeValue(unsigned char index);

unsigned char magnitudeIndex(unsigned char index);
String magnitudeTopicIndex(unsigned char index);

void sensorSetup();
void sensorLoop();

//--------------------------------------------------------------------------------

#include "filters/LastFilter.h"
#include "filters/MaxFilter.h"
#include "filters/MedianFilter.h"
#include "filters/MovingAverageFilter.h"
#include "filters/SumFilter.h"

#include "sensors/BaseSensor.h"
#include "sensors/BaseEmonSensor.h"
#include "sensors/BaseAnalogSensor.h"

#if AM2320_SUPPORT
    #include "sensors/AM2320Sensor.h"
#endif

#if ANALOG_SUPPORT
    #include "sensors/AnalogSensor.h"
#endif

#if BH1750_SUPPORT
    #include "sensors/BH1750Sensor.h"
#endif

#if BMP180_SUPPORT
    #include "sensors/BMP180Sensor.h"
#endif

#if BMX280_SUPPORT
    #include "sensors/BMX280Sensor.h"
#endif

#if CSE7766_SUPPORT
    #include "sensors/CSE7766Sensor.h"
#endif

#if DALLAS_SUPPORT
    #include "sensors/DallasSensor.h"
#endif

#if DHT_SUPPORT
    #include "sensors/DHTSensor.h"
#endif

#if DIGITAL_SUPPORT
    #include "sensors/DigitalSensor.h"
#endif

#if ECH1560_SUPPORT
    #include "sensors/ECH1560Sensor.h"
#endif

#if EMON_ADC121_SUPPORT
    #include "sensors/EmonADC121Sensor.h"
#endif

#if EMON_ADS1X15_SUPPORT
    #include "sensors/EmonADS1X15Sensor.h"
#endif

#if EMON_ANALOG_SUPPORT
    #include "sensors/EmonAnalogSensor.h"
#endif

#if EVENTS_SUPPORT
    #include "sensors/EventSensor.h"
#endif

#if EZOPH_SUPPORT
    #include "sensors/EZOPHSensor.h"
#endif

#if GEIGER_SUPPORT
    #include "sensors/GeigerSensor.h"
#endif

#if GUVAS12SD_SUPPORT
    #include "sensors/GUVAS12SDSensor.h"
#endif

#if HLW8012_SUPPORT
    #include "sensors/HLW8012Sensor.h"
#endif

#if LDR_SUPPORT
    #include "sensors/LDRSensor.h"
#endif

#if MAX6675_SUPPORT
    #include "sensors/MAX6675Sensor.h"
#endif 

#if MICS2710_SUPPORT
    #include "sensors/MICS2710Sensor.h"
#endif

#if MICS5525_SUPPORT
    #include "sensors/MICS5525Sensor.h"
#endif

#if MHZ19_SUPPORT
    #include "sensors/MHZ19Sensor.h"
#endif

#if NTC_SUPPORT
    #include "sensors/NTCSensor.h"
#endif

#if SDS011_SUPPORT
    #include "sensors/SDS011Sensor.h"
#endif

#if SENSEAIR_SUPPORT
    #include "sensors/SenseAirSensor.h"
#endif

#if PMSX003_SUPPORT
    #include "sensors/PMSX003Sensor.h"
#endif

#if PULSEMETER_SUPPORT
    #include "sensors/PulseMeterSensor.h"
#endif

#if PZEM004T_SUPPORT
    #include "sensors/PZEM004TSensor.h"
#endif

#if SHT3X_I2C_SUPPORT
    #include "sensors/SHT3XI2CSensor.h"
#endif

#if SI7021_SUPPORT
    #include "sensors/SI7021Sensor.h"
#endif

#if SONAR_SUPPORT
    #include "sensors/SonarSensor.h"
#endif

#if T6613_SUPPORT
    #include "sensors/T6613Sensor.h"
#endif

#if TMP3X_SUPPORT
    #include "sensors/TMP3XSensor.h"
#endif

#if V9261F_SUPPORT
    #include "sensors/V9261FSensor.h"
#endif

#if VEML6075_SUPPORT
    #include "sensors/VEML6075Sensor.h"
#endif

#if VL53L1X_SUPPORT
    #include "sensors/VL53L1XSensor.h"
#endif

#if ADE7953_SUPPORT
    #include "sensors/ADE7953Sensor.h"
#endif

#if SI1145_SUPPORT
    #include "sensors/SI1145Sensor.h"
#endif

#if HDC1080_SUPPORT
    #include "sensors/HDC1080Sensor.h"
#endif
//--------------------------------------------------------------------------------

