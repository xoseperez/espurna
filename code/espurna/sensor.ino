/*

SENSOR MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if SENSOR_SUPPORT

#include <vector>
#include "filters/LastFilter.h"
#include "filters/MaxFilter.h"
#include "filters/MedianFilter.h"
#include "filters/MovingAverageFilter.h"
#include "sensors/BaseSensor.h"

#include <float.h>

typedef struct {
    BaseSensor * sensor;        // Sensor object
    BaseFilter * filter;        // Filter object
    unsigned char local;        // Local index in its provider
    unsigned char type;         // Type of measurement
    unsigned char decimals;     // Number of decimals in textual representation
    unsigned char global;       // Global index in its type
    double last;                // Last raw value from sensor (unfiltered)
    double reported;            // Last reported value
    double min_change;          // Minimum value change to report
    double max_change;          // Maximum value change to report
} sensor_magnitude_t;

std::vector<BaseSensor *> _sensors;
std::vector<sensor_magnitude_t> _magnitudes;
bool _sensors_ready = false;

unsigned char _counts[MAGNITUDE_MAX];
bool _sensor_realtime = API_REAL_TIME_VALUES;
unsigned long _sensor_read_interval = 1000 * SENSOR_READ_INTERVAL;
unsigned char _sensor_report_every = SENSOR_REPORT_EVERY;
unsigned char _sensor_save_every = SENSOR_SAVE_EVERY;
unsigned char _sensor_power_units = SENSOR_POWER_UNITS;
unsigned char _sensor_energy_units = SENSOR_ENERGY_UNITS;
unsigned char _sensor_temperature_units = SENSOR_TEMPERATURE_UNITS;
double _sensor_temperature_correction = SENSOR_TEMPERATURE_CORRECTION;
double _sensor_humidity_correction = SENSOR_HUMIDITY_CORRECTION;
double _sensor_lux_correction = SENSOR_LUX_CORRECTION;

#if PZEM004T_SUPPORT
PZEM004TSensor *pzem004t_sensor;
#endif

String _sensor_energy_reset_ts = String();

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

unsigned char _magnitudeDecimals(unsigned char type) {

    // Hardcoded decimals (these should be linked to the unit, instead of the magnitude)

    if (type == MAGNITUDE_ANALOG) return ANALOG_DECIMALS;
    if (type == MAGNITUDE_ENERGY ||
        type == MAGNITUDE_ENERGY_DELTA) {
        _sensor_energy_units = getSetting("eneUnits", SENSOR_ENERGY_UNITS).toInt();
        if (_sensor_energy_units == ENERGY_KWH) return 3;
    }
    if (type == MAGNITUDE_POWER_ACTIVE ||
        type == MAGNITUDE_POWER_APPARENT ||
        type == MAGNITUDE_POWER_REACTIVE) {
        if (_sensor_power_units == POWER_KILOWATTS) return 3;
    }
    if (type < MAGNITUDE_MAX) return pgm_read_byte(magnitude_decimals + type);
    return 0;

}

double _magnitudeProcess(unsigned char type, unsigned char decimals, double value) {

    // Hardcoded conversions (these should be linked to the unit, instead of the magnitude)

    if (type == MAGNITUDE_TEMPERATURE) {
        if (_sensor_temperature_units == TMP_FAHRENHEIT) value = value * 1.8 + 32;
        value = value + _sensor_temperature_correction;
    }

    if (type == MAGNITUDE_HUMIDITY) {
        value = constrain(value + _sensor_humidity_correction, 0, 100);
    }

    if (type == MAGNITUDE_LUX) {
        value = value + _sensor_lux_correction;
    }

    if (type == MAGNITUDE_ENERGY ||
        type == MAGNITUDE_ENERGY_DELTA) {
        if (_sensor_energy_units == ENERGY_KWH) value = value  / 3600000;
    }
    if (type == MAGNITUDE_POWER_ACTIVE ||
        type == MAGNITUDE_POWER_APPARENT ||
        type == MAGNITUDE_POWER_REACTIVE) {
        if (_sensor_power_units == POWER_KILOWATTS) value = value  / 1000;
    }

    return roundTo(value, decimals);

}

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

template<typename T> void _sensorWebSocketMagnitudes(JsonObject& root, T prefix) {

    // ws produces flat list <prefix>Magnitudes
    String ws_name = String(prefix);
    ws_name.concat("Magnitudes");

    // config uses <prefix>Magnitude<index> (cut 's')
    String conf_name = ws_name.substring(0, ws_name.length() - 1);

    JsonObject& list = root.createNestedObject(ws_name);
    list["size"] = magnitudeCount();

    JsonArray& name = list.createNestedArray("name");
    JsonArray& type = list.createNestedArray("type");
    JsonArray& index = list.createNestedArray("index");
    JsonArray& idx = list.createNestedArray("idx");

    for (unsigned char i=0; i<magnitudeCount(); ++i) {
        name.add(magnitudeName(i));
        type.add(magnitudeType(i));
        index.add(magnitudeIndex(i));
        idx.add(getSetting(conf_name, i, 0).toInt());
    }
}

bool _sensorWebSocketOnReceive(const char * key, JsonVariant& value) {
    if (strncmp(key, "pwr", 3) == 0) return true;
    if (strncmp(key, "sns", 3) == 0) return true;
    if (strncmp(key, "tmp", 3) == 0) return true;
    if (strncmp(key, "hum", 3) == 0) return true;
    if (strncmp(key, "ene", 3) == 0) return true;
    if (strncmp(key, "lux", 3) == 0) return true;
    return false;
}

void _sensorWebSocketSendData(JsonObject& root) {

    char buffer[10];
    bool hasTemperature = false;
    bool hasHumidity = false;
    bool hasMICS = false;

    JsonObject& magnitudes = root.createNestedObject("magnitudes");
    uint8_t size = 0;

    JsonArray& index = magnitudes.createNestedArray("index");
    JsonArray& type = magnitudes.createNestedArray("type");
    JsonArray& value = magnitudes.createNestedArray("value");
    JsonArray& units = magnitudes.createNestedArray("units");
    JsonArray& error = magnitudes.createNestedArray("error");
    JsonArray& description = magnitudes.createNestedArray("description");

    for (unsigned char i=0; i<magnitudeCount(); i++) {

        sensor_magnitude_t magnitude = _magnitudes[i];
        if (magnitude.type == MAGNITUDE_EVENT) continue;
        ++size;

        double value_show = _magnitudeProcess(magnitude.type, magnitude.decimals, magnitude.last);
        dtostrf(value_show, 1-sizeof(buffer), magnitude.decimals, buffer);

        index.add<uint8_t>(magnitude.global);
        type.add<uint8_t>(magnitude.type);
        value.add(buffer);
        units.add(magnitudeUnits(magnitude.type));
        error.add(magnitude.sensor->error());

        if (magnitude.type == MAGNITUDE_ENERGY) {
            if (_sensor_energy_reset_ts.length() == 0) _sensorResetTS();
            description.add(magnitude.sensor->slot(magnitude.local) + String(" (since ") + _sensor_energy_reset_ts + String(")"));
        } else {
            description.add(magnitude.sensor->slot(magnitude.local));
        }

        if (magnitude.type == MAGNITUDE_TEMPERATURE) hasTemperature = true;
        if (magnitude.type == MAGNITUDE_HUMIDITY) hasHumidity = true;
        #if MICS2710_SUPPORT || MICS5525_SUPPORT
        if (magnitude.type == MAGNITUDE_CO || magnitude.type == MAGNITUDE_NO2) hasMICS = true;
        #endif
    }

    magnitudes["size"] = size;

    if (hasTemperature) root["temperatureVisible"] = 1;
    if (hasHumidity) root["humidityVisible"] = 1;
    if (hasMICS) root["micsVisible"] = 1;

}

void _sensorWebSocketStart(JsonObject& root) {

    for (unsigned char i=0; i<_sensors.size(); i++) {

        BaseSensor * sensor = _sensors[i];

        #if EMON_ANALOG_SUPPORT
            if (sensor->getID() == SENSOR_EMON_ANALOG_ID) {
                root["emonVisible"] = 1;
                root["pwrVisible"] = 1;
                root["pwrVoltage"] = ((EmonAnalogSensor *) sensor)->getVoltage();
            }
        #endif

        #if HLW8012_SUPPORT
            if (sensor->getID() == SENSOR_HLW8012_ID) {
                root["hlwVisible"] = 1;
                root["pwrVisible"] = 1;
            }
        #endif

        #if CSE7766_SUPPORT
            if (sensor->getID() == SENSOR_CSE7766_ID) {
                root["cseVisible"] = 1;
                root["pwrVisible"] = 1;
            }
        #endif

        #if V9261F_SUPPORT
            if (sensor->getID() == SENSOR_V9261F_ID) {
                root["pwrVisible"] = 1;
            }
        #endif

        #if ECH1560_SUPPORT
            if (sensor->getID() == SENSOR_ECH1560_ID) {
                root["pwrVisible"] = 1;
            }
        #endif

        #if PZEM004T_SUPPORT
            if (sensor->getID() == SENSOR_PZEM004T_ID) {
                root["pzemVisible"] = 1;
                root["pwrVisible"] = 1;
            }
        #endif

        #if PULSEMETER_SUPPORT
            if (sensor->getID() == SENSOR_PULSEMETER_ID) {
                root["pmVisible"] = 1;
                root["pwrRatioE"] = ((PulseMeterSensor *) sensor)->getEnergyRatio();
            }
        #endif

    }

    if (magnitudeCount()) {
        root["snsVisible"] = 1;
        //root["apiRealTime"] = _sensor_realtime;
        root["pwrUnits"] = _sensor_power_units;
        root["eneUnits"] = _sensor_energy_units;
        root["tmpUnits"] = _sensor_temperature_units;
        root["tmpCorrection"] = _sensor_temperature_correction;
        root["humCorrection"] = _sensor_humidity_correction;
        root["snsRead"] = _sensor_read_interval / 1000;
        root["snsReport"] = _sensor_report_every;
        root["snsSave"] = _sensor_save_every;
    }

    /*
    // Sensors manifest
    JsonArray& manifest = root.createNestedArray("manifest");
    #if BMX280_SUPPORT
        BMX280Sensor::manifest(manifest);
    #endif

    // Sensors configuration
    JsonArray& sensors = root.createNestedArray("sensors");
    for (unsigned char i; i<_sensors.size(); i++) {
        JsonObject& sensor = sensors.createNestedObject();
        sensor["index"] = i;
        sensor["id"] = _sensors[i]->getID();
        _sensors[i]->getConfig(sensor);
    }
    */

}

#endif // WEB_SUPPORT

#if API_SUPPORT

void _sensorAPISetup() {

    for (unsigned char magnitude_id=0; magnitude_id<_magnitudes.size(); magnitude_id++) {

        sensor_magnitude_t magnitude = _magnitudes[magnitude_id];

        String topic = magnitudeTopic(magnitude.type);
        if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) topic = topic + "/" + String(magnitude.global);

        apiRegister(topic.c_str(), [magnitude_id](char * buffer, size_t len) {
            sensor_magnitude_t magnitude = _magnitudes[magnitude_id];
            double value = _sensor_realtime ? magnitude.last : magnitude.reported;
            dtostrf(value, 1-len, magnitude.decimals, buffer);
        });

    }

}

#endif // API_SUPPORT

#if TERMINAL_SUPPORT

void _sensorInitCommands() {
    terminalRegisterCommand(F("MAGNITUDES"), [](Embedis* e) {
        for (unsigned char i=0; i<_magnitudes.size(); i++) {
            sensor_magnitude_t magnitude = _magnitudes[i];
            DEBUG_MSG_P(PSTR("[SENSOR] * %2d: %s @ %s (%s/%d)\n"),
                i,
                magnitudeTopic(magnitude.type).c_str(),
                magnitude.sensor->slot(magnitude.local).c_str(),
                magnitudeTopic(magnitude.type).c_str(),
                magnitude.global
            );
        }
        terminalOK();
    });
    #if PZEM004T_SUPPORT
    terminalRegisterCommand(F("PZ.ADDRESS"), [](Embedis* e) {
        if (e->argc == 1) {
            DEBUG_MSG_P(PSTR("[SENSOR] PZEM004T\n"));
            unsigned char dev_count = pzem004t_sensor->getAddressesCount();
            for(unsigned char dev = 0; dev < dev_count; dev++) {
                DEBUG_MSG_P(PSTR("Device %d/%s\n"), dev, pzem004t_sensor->getAddress(dev).c_str());
            }
            terminalOK();
        } else if(e->argc == 2) {
            IPAddress addr;
            if (addr.fromString(String(e->argv[1]))) {
                if(pzem004t_sensor->setDeviceAddress(&addr)) {
                    terminalOK();
                }
            } else {
                terminalError(F("Invalid address argument"));
            }
        } else {
            terminalError(F("Wrong arguments"));
        }
    });
    terminalRegisterCommand(F("PZ.RESET"), [](Embedis* e) {
        if(e->argc > 2) {
            terminalError(F("Wrong arguments"));
        } else {
            unsigned char init = e->argc == 2 ? String(e->argv[1]).toInt() : 0;
            unsigned char limit = e->argc == 2 ? init +1 : pzem004t_sensor->getAddressesCount();
            DEBUG_MSG_P(PSTR("[SENSOR] PZEM004T\n"));
            for(unsigned char dev = init; dev < limit; dev++) {
                float offset = pzem004t_sensor->resetEnergy(dev);
                setSetting("pzemEneTotal", dev, offset);
                DEBUG_MSG_P(PSTR("Device %d/%s - Offset: %s\n"), dev, pzem004t_sensor->getAddress(dev).c_str(), String(offset).c_str());
            }
            terminalOK();
        }
    });
    terminalRegisterCommand(F("PZ.VALUE"), [](Embedis* e) {
        if(e->argc > 2) {
            terminalError(F("Wrong arguments"));
        } else {
            unsigned char init = e->argc == 2 ? String(e->argv[1]).toInt() : 0;
            unsigned char limit = e->argc == 2 ? init +1 : pzem004t_sensor->getAddressesCount();
            DEBUG_MSG_P(PSTR("[SENSOR] PZEM004T\n"));
            for(unsigned char dev = init; dev < limit; dev++) {
                DEBUG_MSG_P(PSTR("Device %d/%s - Current: %s Voltage: %s Power: %s Energy: %s\n"), //
                            dev,
                            pzem004t_sensor->getAddress(dev).c_str(),
                            String(pzem004t_sensor->value(dev * PZ_MAGNITUDE_CURRENT_INDEX)).c_str(),
                            String(pzem004t_sensor->value(dev * PZ_MAGNITUDE_VOLTAGE_INDEX)).c_str(),
                            String(pzem004t_sensor->value(dev * PZ_MAGNITUDE_POWER_ACTIVE_INDEX)).c_str(),
                            String(pzem004t_sensor->value(dev * PZ_MAGNITUDE_ENERGY_INDEX)).c_str());
            }
            terminalOK();
        }
    });
    #endif
}

#endif

void _sensorTick() {
    for (unsigned char i=0; i<_sensors.size(); i++) {
        _sensors[i]->tick();
    }
}

void _sensorPre() {
    for (unsigned char i=0; i<_sensors.size(); i++) {
        _sensors[i]->pre();
        if (!_sensors[i]->status()) {
            DEBUG_MSG_P(PSTR("[SENSOR] Error reading data from %s (error: %d)\n"),
                _sensors[i]->description().c_str(),
                _sensors[i]->error()
            );
        }
    }
}

void _sensorPost() {
    for (unsigned char i=0; i<_sensors.size(); i++) {
        _sensors[i]->post();
    }
}

void _sensorResetTS() {
    #if NTP_SUPPORT
        if (ntpSynced()) {
            if (_sensor_energy_reset_ts.length() == 0) {
                _sensor_energy_reset_ts = ntpDateTime(now() - millis() / 1000);
            } else {
                _sensor_energy_reset_ts = ntpDateTime(now());
            }
        } else {
            _sensor_energy_reset_ts = String();
        }
        setSetting("snsResetTS", _sensor_energy_reset_ts);
    #endif
}

double _sensorEnergyTotal() {
    double value = 0;

    if (rtcmemStatus()) {
        value = Rtcmem->energy;
    } else {
        value = (_sensor_save_every > 0) ? getSetting("eneTotal", 0).toInt() : 0;
    }

    return value;
}


void _sensorEnergyTotal(double value) {
    static unsigned long save_count = 0;

    // Save to EEPROM every '_sensor_save_every' readings
    if (_sensor_save_every > 0) {
        save_count = (save_count + 1) % _sensor_save_every;
        if (0 == save_count) {
            setSetting("eneTotal", value);
            saveSettings();
        }
    }

    // Always save to RTCMEM
    Rtcmem->energy = value;
}

// -----------------------------------------------------------------------------
// Sensor initialization
// -----------------------------------------------------------------------------

void _sensorLoad() {

    /*

    This is temporal, in the future sensors will be initialized based on
    soft configuration (data stored in EEPROM config) so you will be able
    to define and configure new sensors on the fly

    At the time being, only enabled sensors (those with *_SUPPORT to 1) are being
    loaded and initialized here. If you want to add new sensors of the same type
    just duplicate the block and change the arguments for the set* methods.
    Check the DHT block below for an example

     */

    #if AM2320_SUPPORT
    {
     AM2320Sensor * sensor = new AM2320Sensor();
     sensor->setAddress(AM2320_ADDRESS);
     _sensors.push_back(sensor);
    }
    #endif

    #if ANALOG_SUPPORT
    {
        AnalogSensor * sensor = new AnalogSensor();
        sensor->setSamples(ANALOG_SAMPLES);
        sensor->setDelay(ANALOG_DELAY);
        //CICM For analog scaling
        sensor->setFactor(ANALOG_FACTOR);
        sensor->setOffset(ANALOG_OFFSET);
        _sensors.push_back(sensor);
    }
    #endif

    #if BH1750_SUPPORT
    {
        BH1750Sensor * sensor = new BH1750Sensor();
        sensor->setAddress(BH1750_ADDRESS);
        sensor->setMode(BH1750_MODE);
        _sensors.push_back(sensor);
    }
    #endif

    #if BMP180_SUPPORT
    {
        BMP180Sensor * sensor = new BMP180Sensor();
        sensor->setAddress(BMP180_ADDRESS);
        _sensors.push_back(sensor);
    }
    #endif

    #if BMX280_SUPPORT
    {
        // Support up to two sensors with full auto-discovery.
        const unsigned char number = constrain(getSetting("bmx280Number", BMX280_NUMBER).toInt(), 1, 2);

        // For second sensor, if BMX280_ADDRESS is 0x00 then auto-discover
        // otherwise choose the other unnamed sensor address
        const unsigned char first = getSetting("bmx280Address", BMX280_ADDRESS).toInt();
        const unsigned char second = (first == 0x00) ? 0x00 : (0x76 + 0x77 - first);

        const unsigned char address_map[2] = { first, second };

        for (unsigned char n=0; n < number; ++n) {
            BMX280Sensor * sensor = new BMX280Sensor();
            sensor->setAddress(address_map[n]);
            _sensors.push_back(sensor);
        }
    }
    #endif

    #if CSE7766_SUPPORT
    {
        CSE7766Sensor * sensor = new CSE7766Sensor();
        sensor->setRX(CSE7766_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if DALLAS_SUPPORT
    {
        DallasSensor * sensor = new DallasSensor();
        sensor->setGPIO(DALLAS_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if DHT_SUPPORT
    {
        DHTSensor * sensor = new DHTSensor();
        sensor->setGPIO(DHT_PIN);
        sensor->setType(DHT_TYPE);
        _sensors.push_back(sensor);
    }
    #endif

    /*
    // Example on how to add a second DHT sensor
    // DHT2_PIN and DHT2_TYPE should be defined in sensors.h file
    #if DHT_SUPPORT
    {
        DHTSensor * sensor = new DHTSensor();
        sensor->setGPIO(DHT2_PIN);
        sensor->setType(DHT2_TYPE);
        _sensors.push_back(sensor);
    }
    #endif
    */

    #if DIGITAL_SUPPORT
    {
        DigitalSensor * sensor = new DigitalSensor();
        sensor->setGPIO(DIGITAL_PIN);
        sensor->setMode(DIGITAL_PIN_MODE);
        sensor->setDefault(DIGITAL_DEFAULT_STATE);
        _sensors.push_back(sensor);
    }
    #endif

    #if ECH1560_SUPPORT
    {
        ECH1560Sensor * sensor = new ECH1560Sensor();
        sensor->setCLK(ECH1560_CLK_PIN);
        sensor->setMISO(ECH1560_MISO_PIN);
        sensor->setInverted(ECH1560_INVERTED);
        _sensors.push_back(sensor);
    }
    #endif

    #if EMON_ADC121_SUPPORT
    {
        EmonADC121Sensor * sensor = new EmonADC121Sensor();
        sensor->setAddress(EMON_ADC121_I2C_ADDRESS);
        sensor->setVoltage(EMON_MAINS_VOLTAGE);
        sensor->setReference(EMON_REFERENCE_VOLTAGE);
        sensor->setCurrentRatio(0, EMON_CURRENT_RATIO);
        _sensors.push_back(sensor);
    }
    #endif

    #if EMON_ADS1X15_SUPPORT
    {
        EmonADS1X15Sensor * sensor = new EmonADS1X15Sensor();
        sensor->setAddress(EMON_ADS1X15_I2C_ADDRESS);
        sensor->setType(EMON_ADS1X15_TYPE);
        sensor->setMask(EMON_ADS1X15_MASK);
        sensor->setGain(EMON_ADS1X15_GAIN);
        sensor->setVoltage(EMON_MAINS_VOLTAGE);
        sensor->setCurrentRatio(0, EMON_CURRENT_RATIO);
        sensor->setCurrentRatio(1, EMON_CURRENT_RATIO);
        sensor->setCurrentRatio(2, EMON_CURRENT_RATIO);
        sensor->setCurrentRatio(3, EMON_CURRENT_RATIO);
        _sensors.push_back(sensor);
    }
    #endif

    #if EMON_ANALOG_SUPPORT
    {
        EmonAnalogSensor * sensor = new EmonAnalogSensor();
        sensor->setVoltage(EMON_MAINS_VOLTAGE);
        sensor->setReference(EMON_REFERENCE_VOLTAGE);
        sensor->setCurrentRatio(0, EMON_CURRENT_RATIO);
        _sensors.push_back(sensor);
    }
    #endif

    #if EVENTS_SUPPORT
    {
        EventSensor * sensor = new EventSensor();
        sensor->setGPIO(EVENTS_PIN);
        sensor->setTrigger(EVENTS_TRIGGER);
        sensor->setPinMode(EVENTS_PIN_MODE);
        sensor->setDebounceTime(EVENTS_DEBOUNCE);
        sensor->setInterruptMode(EVENTS_INTERRUPT_MODE);
        _sensors.push_back(sensor);
    }
    #endif

    #if GEIGER_SUPPORT
    {
        GeigerSensor * sensor = new GeigerSensor();        // Create instance of thr Geiger module.
        sensor->setGPIO(GEIGER_PIN);                       // Interrupt pin of the attached geiger counter board.
        sensor->setMode(GEIGER_PIN_MODE);                  // This pin is an input.
        sensor->setDebounceTime(GEIGER_DEBOUNCE);          // Debounce time 25ms, because https://github.com/Trickx/espurna/wiki/Geiger-counter
        sensor->setInterruptMode(GEIGER_INTERRUPT_MODE);   // Interrupt triggering: edge detection rising.
        sensor->setCPM2SievertFactor(GEIGER_CPM2SIEVERT);  // Conversion factor from counts per minute to µSv/h
        _sensors.push_back(sensor);
    }
    #endif

    #if GUVAS12SD_SUPPORT
    {
        GUVAS12SDSensor * sensor = new GUVAS12SDSensor();
        sensor->setGPIO(GUVAS12SD_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if SONAR_SUPPORT
    {
        SonarSensor * sensor = new SonarSensor();
        sensor->setEcho(SONAR_ECHO);
        sensor->setIterations(SONAR_ITERATIONS);
        sensor->setMaxDistance(SONAR_MAX_DISTANCE);
        sensor->setTrigger(SONAR_TRIGGER);
        _sensors.push_back(sensor);
    }
    #endif

    #if HLW8012_SUPPORT
    {
        HLW8012Sensor * sensor = new HLW8012Sensor();
        sensor->setSEL(HLW8012_SEL_PIN);
        sensor->setCF(HLW8012_CF_PIN);
        sensor->setCF1(HLW8012_CF1_PIN);
        sensor->setSELCurrent(HLW8012_SEL_CURRENT);
        _sensors.push_back(sensor);
    }
    #endif

    #if LDR_SUPPORT
    {
        LDRSensor * sensor = new LDRSensor();
        sensor->setSamples(LDR_SAMPLES);
        sensor->setDelay(LDR_DELAY);
        sensor->setType(LDR_TYPE);
        sensor->setPhotocellPositionOnGround(LDR_ON_GROUND);
        sensor->setResistor(LDR_RESISTOR);
        sensor->setPhotocellParameters(LDR_MULTIPLICATION, LDR_POWER);
        _sensors.push_back(sensor);
    }
    #endif

    #if MHZ19_SUPPORT
    {
        MHZ19Sensor * sensor = new MHZ19Sensor();
        sensor->setRX(MHZ19_RX_PIN);
        sensor->setTX(MHZ19_TX_PIN);
        if (getSetting("mhz19CalibrateAuto", 0).toInt() == 1)
            sensor->setCalibrateAuto(true);
        _sensors.push_back(sensor);
    }
    #endif

    #if MICS2710_SUPPORT
    {
        MICS2710Sensor * sensor = new MICS2710Sensor();
        sensor->setAnalogGPIO(MICS2710_NOX_PIN);
        sensor->setPreHeatGPIO(MICS2710_PRE_PIN);
        sensor->setRL(MICS2710_RL);
        _sensors.push_back(sensor);
    }
    #endif

    #if MICS5525_SUPPORT
    {
        MICS5525Sensor * sensor = new MICS5525Sensor();
        sensor->setAnalogGPIO(MICS5525_RED_PIN);
        sensor->setRL(MICS5525_RL);
        _sensors.push_back(sensor);
    }
    #endif

    #if NTC_SUPPORT
    {
        NTCSensor * sensor = new NTCSensor();
        sensor->setSamples(NTC_SAMPLES);
        sensor->setDelay(NTC_DELAY);
        sensor->setUpstreamResistor(NTC_R_UP);
        sensor->setDownstreamResistor(NTC_R_DOWN);
        sensor->setBeta(NTC_BETA);
        sensor->setR0(NTC_R0);
        sensor->setT0(NTC_T0);
        _sensors.push_back(sensor);
    }
    #endif

    #if PMSX003_SUPPORT
    {
        PMSX003Sensor * sensor = new PMSX003Sensor();
        #if PMS_USE_SOFT
            sensor->setRX(PMS_RX_PIN);
            sensor->setTX(PMS_TX_PIN);
        #else
            sensor->setSerial(& PMS_HW_PORT);
        #endif
        sensor->setType(PMS_TYPE);
        _sensors.push_back(sensor);
    }
    #endif

    #if PULSEMETER_SUPPORT
    {
        PulseMeterSensor * sensor = new PulseMeterSensor();
        sensor->setGPIO(PULSEMETER_PIN);
        sensor->setEnergyRatio(PULSEMETER_ENERGY_RATIO);
        sensor->setDebounceTime(PULSEMETER_DEBOUNCE);
        _sensors.push_back(sensor);
    }
    #endif

    #if PZEM004T_SUPPORT
    {
        String addresses = getSetting("pzemAddr", PZEM004T_ADDRESSES);
        if (!addresses.length()) {
            DEBUG_MSG_P(PSTR("[SENSOR] PZEM004T Error: no addresses are configured\n"));
            return;
        }

        PZEM004TSensor * sensor = pzem004t_sensor = new PZEM004TSensor();
        sensor->setAddresses(addresses.c_str());

        if (getSetting("pzemSoft", PZEM004T_USE_SOFT).toInt() == 1) {
            sensor->setRX(getSetting("pzemRX", PZEM004T_RX_PIN).toInt());
            sensor->setTX(getSetting("pzemTX", PZEM004T_TX_PIN).toInt());
        } else {
            sensor->setSerial(& PZEM004T_HW_PORT);
        }

        // Read saved energy offset
        unsigned char dev_count = sensor->getAddressesCount();
        for(unsigned char dev = 0; dev < dev_count; dev++) {
            float value = getSetting("pzemEneTotal", dev, 0).toFloat();
            if (value > 0) sensor->resetEnergy(dev, value);
        }
        _sensors.push_back(sensor);
    }
    #endif

    #if SENSEAIR_SUPPORT
    {
        SenseAirSensor * sensor = new SenseAirSensor();
        sensor->setRX(SENSEAIR_RX_PIN);
        sensor->setTX(SENSEAIR_TX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if SDS011_SUPPORT
    {
        SDS011Sensor * sensor = new SDS011Sensor();
        sensor->setRX(SDS011_RX_PIN);
        sensor->setTX(SDS011_TX_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if SHT3X_I2C_SUPPORT
    {
        SHT3XI2CSensor * sensor = new SHT3XI2CSensor();
        sensor->setAddress(SHT3X_I2C_ADDRESS);
        _sensors.push_back(sensor);
    }
    #endif

    #if SI7021_SUPPORT
    {
        SI7021Sensor * sensor = new SI7021Sensor();
        sensor->setAddress(SI7021_ADDRESS);
        _sensors.push_back(sensor);
    }
    #endif

    #if TMP3X_SUPPORT
    {
        TMP3XSensor * sensor = new TMP3XSensor();
        sensor->setType(TMP3X_TYPE);
        _sensors.push_back(sensor);
    }
    #endif

    #if V9261F_SUPPORT
    {
        V9261FSensor * sensor = new V9261FSensor();
        sensor->setRX(V9261F_PIN);
        sensor->setInverted(V9261F_PIN_INVERSE);
        _sensors.push_back(sensor);
    }
    #endif

    #if MAX6675_SUPPORT
    {
        MAX6675Sensor * sensor = new MAX6675Sensor();
        sensor->setCS(MAX6675_CS_PIN);
        sensor->setSO(MAX6675_SO_PIN);
        sensor->setSCK(MAX6675_SCK_PIN);
        _sensors.push_back(sensor);
    }
    #endif

    #if VEML6075_SUPPORT
    {
        VEML6075Sensor * sensor = new VEML6075Sensor();
        sensor->setIntegrationTime(VEML6075_INTEGRATION_TIME);
        sensor->setDynamicMode(VEML6075_DYNAMIC_MODE);
        _sensors.push_back(sensor);
    }
    #endif

    #if VL53L1X_SUPPORT
    {
        VL53L1XSensor * sensor = new VL53L1XSensor();
        sensor->setInterMeasurementPeriod(VL53L1X_INTER_MEASUREMENT_PERIOD);
        sensor->setDistanceMode(VL53L1X_DISTANCE_MODE);
        sensor->setMeasurementTimingBudget(VL53L1X_MEASUREMENT_TIMING_BUDGET);
        _sensors.push_back(sensor);
    }
    #endif

    #if EZOPH_SUPPORT
    {
        EZOPHSensor * sensor = new EZOPHSensor();
        sensor->setRX(EZOPH_RX_PIN);
        sensor->setTX(EZOPH_TX_PIN);
        _sensors.push_back(sensor);
    }
    #endif
}

void _sensorCallback(unsigned char i, unsigned char type, double value) {

    DEBUG_MSG_P(PSTR("[SENSOR] Sensor #%u callback, type %u, payload: '%s'\n"), i, type, String(value).c_str());

    for (unsigned char k=0; k<_magnitudes.size(); k++) {
        if ((_sensors[i] == _magnitudes[k].sensor) && (type == _magnitudes[k].type)) {
            _sensorReport(k, value);
            return;
        }
    }

}

void _sensorInit() {

    _sensors_ready = true;
    _sensor_save_every = getSetting("snsSave", 0).toInt();

    for (unsigned char i=0; i<_sensors.size(); i++) {

        // Do not process an already initialized sensor
        if (_sensors[i]->ready()) continue;
        DEBUG_MSG_P(PSTR("[SENSOR] Initializing %s\n"), _sensors[i]->description().c_str());

        // Force sensor to reload config
        _sensors[i]->begin();
        if (!_sensors[i]->ready()) {
            if (_sensors[i]->error() != 0) DEBUG_MSG_P(PSTR("[SENSOR]  -> ERROR %d\n"), _sensors[i]->error());
            _sensors_ready = false;
            continue;
        }

        // Initialize magnitudes
        for (unsigned char k=0; k<_sensors[i]->count(); k++) {

            unsigned char type = _sensors[i]->type(k);
	        signed char decimals = _sensors[i]->decimals(type);
	        if (decimals < 0) decimals = _magnitudeDecimals(type);

            sensor_magnitude_t new_magnitude;
            new_magnitude.sensor = _sensors[i];
            new_magnitude.local = k;
            new_magnitude.type = type;
	        new_magnitude.decimals = (unsigned char) decimals;
            new_magnitude.global = _counts[type];
            new_magnitude.last = 0;
            new_magnitude.reported = 0;
            new_magnitude.min_change = 0;
            new_magnitude.max_change = 0;

            // TODO: find a proper way to extend this to min/max of any magnitude
            if (MAGNITUDE_ENERGY == type) {
                new_magnitude.max_change = getSetting("eneMaxDelta", ENERGY_MAX_CHANGE).toFloat();
            } else if (MAGNITUDE_TEMPERATURE == type) {
                new_magnitude.min_change = getSetting("tmpMinDelta", TEMPERATURE_MIN_CHANGE).toFloat();
            } else if (MAGNITUDE_HUMIDITY == type) {
                new_magnitude.min_change = getSetting("humMinDelta", HUMIDITY_MIN_CHANGE).toFloat();
            }

            if (MAGNITUDE_ENERGY == type) {
                new_magnitude.filter = new LastFilter();
            } else if (MAGNITUDE_DIGITAL == type) {
                new_magnitude.filter = new MaxFilter();
            } else if (MAGNITUDE_COUNT == type || MAGNITUDE_GEIGER_CPM == type || MAGNITUDE_GEIGER_SIEVERT == type) {  // For geiger counting moving average filter is the most appropriate if needed at all.
                new_magnitude.filter = new MovingAverageFilter();
            } else {
                new_magnitude.filter = new MedianFilter();
            }
            new_magnitude.filter->resize(_sensor_report_every);

            _magnitudes.push_back(new_magnitude);

            DEBUG_MSG_P(PSTR("[SENSOR]  -> %s:%d\n"), magnitudeTopic(type).c_str(), _counts[type]);

            _counts[type] = _counts[type] + 1;

        }

        // Hook callback
        _sensors[i]->onEvent([i](unsigned char type, double value) {
            _sensorCallback(i, type, value);
        });

        // Custom initializations

        #if MICS2710_SUPPORT
            if (_sensors[i]->getID() == SENSOR_MICS2710_ID) {
                MICS2710Sensor * sensor = (MICS2710Sensor *) _sensors[i];
                sensor->setR0(getSetting("snsR0", MICS2710_R0).toInt());
            }
        #endif // MICS2710_SUPPORT

        #if MICS5525_SUPPORT
            if (_sensors[i]->getID() == SENSOR_MICS5525_ID) {
                MICS5525Sensor * sensor = (MICS5525Sensor *) _sensors[i];
                sensor->setR0(getSetting("snsR0", MICS5525_R0).toInt());
            }
        #endif // MICS5525_SUPPORT

        #if EMON_ANALOG_SUPPORT

            if (_sensors[i]->getID() == SENSOR_EMON_ANALOG_ID) {
                EmonAnalogSensor * sensor = (EmonAnalogSensor *) _sensors[i];
                sensor->setCurrentRatio(0, getSetting("pwrRatioC", EMON_CURRENT_RATIO).toFloat());
                sensor->setVoltage(getSetting("pwrVoltage", EMON_MAINS_VOLTAGE).toInt());

                double value = _sensorEnergyTotal();

                if (value > 0) sensor->resetEnergy(0, value);
            }

        #endif // EMON_ANALOG_SUPPORT

        #if HLW8012_SUPPORT

            if (_sensors[i]->getID() == SENSOR_HLW8012_ID) {

                HLW8012Sensor * sensor = (HLW8012Sensor *) _sensors[i];

                double value;

                value = getSetting("pwrRatioC", HLW8012_CURRENT_RATIO).toFloat();
                if (value > 0) sensor->setCurrentRatio(value);

                value = getSetting("pwrRatioV", HLW8012_VOLTAGE_RATIO).toFloat();
                if (value > 0) sensor->setVoltageRatio(value);

                value = getSetting("pwrRatioP", HLW8012_POWER_RATIO).toFloat();
                if (value > 0) sensor->setPowerRatio(value);

                value = _sensorEnergyTotal();
                if (value > 0) sensor->resetEnergy(value);

            }

        #endif // HLW8012_SUPPORT

        #if CSE7766_SUPPORT

            if (_sensors[i]->getID() == SENSOR_CSE7766_ID) {

                CSE7766Sensor * sensor = (CSE7766Sensor *) _sensors[i];

                double value;

                value = getSetting("pwrRatioC", 0).toFloat();
                if (value > 0) sensor->setCurrentRatio(value);

                value = getSetting("pwrRatioV", 0).toFloat();
                if (value > 0) sensor->setVoltageRatio(value);

                value = getSetting("pwrRatioP", 0).toFloat();
                if (value > 0) sensor->setPowerRatio(value);

                value = _sensorEnergyTotal();
                if (value > 0) sensor->resetEnergy(value);

            }

        #endif // CSE7766_SUPPORT

        #if PULSEMETER_SUPPORT
            if (_sensors[i]->getID() == SENSOR_PULSEMETER_ID) {
                PulseMeterSensor * sensor = (PulseMeterSensor *) _sensors[i];
                sensor->setEnergyRatio(getSetting("pwrRatioE", PULSEMETER_ENERGY_RATIO).toInt());
            }
        #endif // PULSEMETER_SUPPORT

    }

}

void _sensorConfigure() {

    // General sensor settings
    _sensor_read_interval = 1000 * constrain(getSetting("snsRead", SENSOR_READ_INTERVAL).toInt(), SENSOR_READ_MIN_INTERVAL, SENSOR_READ_MAX_INTERVAL);
    _sensor_report_every = constrain(getSetting("snsReport", SENSOR_REPORT_EVERY).toInt(), SENSOR_REPORT_MIN_EVERY, SENSOR_REPORT_MAX_EVERY);
    _sensor_save_every = getSetting("snsSave", SENSOR_SAVE_EVERY).toInt();
    _sensor_realtime = getSetting("apiRealTime", API_REAL_TIME_VALUES).toInt() == 1;
    _sensor_power_units = getSetting("pwrUnits", SENSOR_POWER_UNITS).toInt();
    _sensor_energy_units = getSetting("eneUnits", SENSOR_ENERGY_UNITS).toInt();
    _sensor_temperature_units = getSetting("tmpUnits", SENSOR_TEMPERATURE_UNITS).toInt();
    _sensor_temperature_correction = getSetting("tmpCorrection", SENSOR_TEMPERATURE_CORRECTION).toFloat();
    _sensor_humidity_correction = getSetting("humCorrection", SENSOR_HUMIDITY_CORRECTION).toFloat();
    _sensor_energy_reset_ts = getSetting("snsResetTS", "");
    _sensor_lux_correction = getSetting("luxCorrection", SENSOR_LUX_CORRECTION).toFloat();

    // Specific sensor settings
    for (unsigned char i=0; i<_sensors.size(); i++) {

        #if MICS2710_SUPPORT

            if (_sensors[i]->getID() == SENSOR_MICS2710_ID) {
                if (getSetting("snsResetCalibration", 0).toInt() == 1) {
                    MICS2710Sensor * sensor = (MICS2710Sensor *) _sensors[i];
                    sensor->calibrate();
                    setSetting("snsR0", sensor->getR0());
                }
            }

        #endif // MICS2710_SUPPORT

        #if MICS5525_SUPPORT

            if (_sensors[i]->getID() == SENSOR_MICS5525_ID) {
                if (getSetting("snsResetCalibration", 0).toInt() == 1) {
                    MICS5525Sensor * sensor = (MICS5525Sensor *) _sensors[i];
                    sensor->calibrate();
                    setSetting("snsR0", sensor->getR0());
                }
            }

        #endif // MICS5525_SUPPORT

        #if EMON_ANALOG_SUPPORT

            if (_sensors[i]->getID() == SENSOR_EMON_ANALOG_ID) {

                double value;
                EmonAnalogSensor * sensor = (EmonAnalogSensor *) _sensors[i];

                if ((value = getSetting("pwrExpectedP", 0).toInt())) {
                    sensor->expectedPower(0, value);
                    setSetting("pwrRatioC", sensor->getCurrentRatio(0));
                }

                if (getSetting("pwrResetCalibration", 0).toInt() == 1) {
                    sensor->setCurrentRatio(0, EMON_CURRENT_RATIO);
                    delSetting("pwrRatioC");
                }

                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    delSetting("eneTotal");
                    _sensorResetTS();
                }

                sensor->setVoltage(getSetting("pwrVoltage", EMON_MAINS_VOLTAGE).toInt());

            }

        #endif // EMON_ANALOG_SUPPORT

        #if EMON_ADC121_SUPPORT
            if (_sensors[i]->getID() == SENSOR_EMON_ADC121_ID) {
                EmonADC121Sensor * sensor = (EmonADC121Sensor *) _sensors[i];
                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    delSetting("eneTotal");
                    _sensorResetTS();
                }
            }
        #endif

        #if EMON_ADS1X15_SUPPORT
            if (_sensors[i]->getID() == SENSOR_EMON_ADS1X15_ID) {
                EmonADS1X15Sensor * sensor = (EmonADS1X15Sensor *) _sensors[i];
                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    delSetting("eneTotal");
                    _sensorResetTS();
                }
            }
        #endif

        #if HLW8012_SUPPORT


            if (_sensors[i]->getID() == SENSOR_HLW8012_ID) {

                double value;
                HLW8012Sensor * sensor = (HLW8012Sensor *) _sensors[i];

                if (value = getSetting("pwrExpectedC", 0).toFloat()) {
                    sensor->expectedCurrent(value);
                    setSetting("pwrRatioC", sensor->getCurrentRatio());
                }

                if (value = getSetting("pwrExpectedV", 0).toInt()) {
                    sensor->expectedVoltage(value);
                    setSetting("pwrRatioV", sensor->getVoltageRatio());
                }

                if (value = getSetting("pwrExpectedP", 0).toInt()) {
                    sensor->expectedPower(value);
                    setSetting("pwrRatioP", sensor->getPowerRatio());
                }

                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    delSetting("eneTotal");
                    _sensorResetTS();
                }

                if (getSetting("pwrResetCalibration", 0).toInt() == 1) {
                    sensor->resetRatios();
                    delSetting("pwrRatioC");
                    delSetting("pwrRatioV");
                    delSetting("pwrRatioP");
                }

            }

        #endif // HLW8012_SUPPORT

        #if CSE7766_SUPPORT

            if (_sensors[i]->getID() == SENSOR_CSE7766_ID) {

                double value;
                CSE7766Sensor * sensor = (CSE7766Sensor *) _sensors[i];

                if ((value = getSetting("pwrExpectedC", 0).toFloat())) {
                    sensor->expectedCurrent(value);
                    setSetting("pwrRatioC", sensor->getCurrentRatio());
                }

                if ((value = getSetting("pwrExpectedV", 0).toInt())) {
                    sensor->expectedVoltage(value);
                    setSetting("pwrRatioV", sensor->getVoltageRatio());
                }

                if ((value = getSetting("pwrExpectedP", 0).toInt())) {
                    sensor->expectedPower(value);
                    setSetting("pwrRatioP", sensor->getPowerRatio());
                }

                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    delSetting("eneTotal");
                    _sensorResetTS();
                }

                if (getSetting("pwrResetCalibration", 0).toInt() == 1) {
                    sensor->resetRatios();
                    delSetting("pwrRatioC");
                    delSetting("pwrRatioV");
                    delSetting("pwrRatioP");
                }

            }

        #endif // CSE7766_SUPPORT

        #if PULSEMETER_SUPPORT
            if (_sensors[i]->getID() == SENSOR_PULSEMETER_ID) {
                PulseMeterSensor * sensor = (PulseMeterSensor *) _sensors[i];
                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    delSetting("eneTotal");
                    _sensorResetTS();
                }

                sensor->setEnergyRatio(getSetting("pwrRatioE", PULSEMETER_ENERGY_RATIO).toInt());
            }
        #endif // PULSEMETER_SUPPORT

        #if PZEM004T_SUPPORT

            if (_sensors[i]->getID() == SENSOR_PZEM004T_ID) {
                PZEM004TSensor * sensor = (PZEM004TSensor *) _sensors[i];
                if (getSetting("pwrResetE", 0).toInt() == 1) {
                    unsigned char dev_count = sensor->getAddressesCount();
                    for(unsigned char dev = 0; dev < dev_count; dev++) {
                        sensor->resetEnergy(dev, 0);
                        delSetting("pzemEneTotal", dev);
                    }
                    _sensorResetTS();
                }
            }

        #endif // PZEM004T_SUPPORT

    }

    // Update filter sizes
    for (unsigned char i=0; i<_magnitudes.size(); i++) {
        _magnitudes[i].filter->resize(_sensor_report_every);
    }

    // General processing
    if (0 == _sensor_save_every) {
        delSetting("eneTotal");
    }

    // Save settings
    delSetting("snsResetCalibration");
    delSetting("pwrExpectedP");
    delSetting("pwrExpectedC");
    delSetting("pwrExpectedV");
    delSetting("pwrResetCalibration");
    delSetting("pwrResetE");
    saveSettings();

}

void _sensorReport(unsigned char index, double value) {

    sensor_magnitude_t magnitude = _magnitudes[index];
    unsigned char decimals = magnitude.decimals;

    char buffer[10];
    dtostrf(value, 1-sizeof(buffer), decimals, buffer);

    #if BROKER_SUPPORT
        brokerPublish(BROKER_MSG_TYPE_SENSOR ,magnitudeTopic(magnitude.type).c_str(), magnitude.local, buffer);
    #endif

    #if MQTT_SUPPORT

        mqttSend(magnitudeTopicIndex(index).c_str(), buffer);

        #if SENSOR_PUBLISH_ADDRESSES
            char topic[32];
            snprintf(topic, sizeof(topic), "%s/%s", SENSOR_ADDRESS_TOPIC, magnitudeTopic(magnitude.type).c_str());
            if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) {
                mqttSend(topic, magnitude.global, magnitude.sensor->address(magnitude.local).c_str());
            } else {
                mqttSend(topic, magnitude.sensor->address(magnitude.local).c_str());
            }
        #endif // SENSOR_PUBLISH_ADDRESSES

    #endif // MQTT_SUPPORT

    #if THINGSPEAK_SUPPORT
        tspkEnqueueMeasurement(index, buffer);
    #endif

    #if DOMOTICZ_SUPPORT
    {
        char key[15];
        snprintf_P(key, sizeof(key), PSTR("dczMagnitude%d"), index);
        if (magnitude.type == MAGNITUDE_HUMIDITY) {
            int status;
            if (value > 70) {
                status = HUMIDITY_WET;
            } else if (value > 45) {
                status = HUMIDITY_COMFORTABLE;
            } else if (value > 30) {
                status = HUMIDITY_NORMAL;
            } else {
                status = HUMIDITY_DRY;
            }
            char status_buf[5];
            itoa(status, status_buf, 10);
            domoticzSend(key, buffer, status_buf);
        } else {
            domoticzSend(key, 0, buffer);
        }
    }
    #endif // DOMOTICZ_SUPPORT

}

// -----------------------------------------------------------------------------
// Public
// -----------------------------------------------------------------------------

unsigned char sensorCount() {
    return _sensors.size();
}

unsigned char magnitudeCount() {
    return _magnitudes.size();
}

String magnitudeName(unsigned char index) {
    if (index < _magnitudes.size()) {
        sensor_magnitude_t magnitude = _magnitudes[index];
        return magnitude.sensor->slot(magnitude.local);
    }
    return String();
}

unsigned char magnitudeType(unsigned char index) {
    if (index < _magnitudes.size()) {
        return int(_magnitudes[index].type);
    }
    return MAGNITUDE_NONE;
}

double magnitudeValue(unsigned char index) {
    if (index < _magnitudes.size()) {
        return _sensor_realtime ? _magnitudes[index].last : _magnitudes[index].reported;
    }
    return DBL_MIN;
}

unsigned char magnitudeIndex(unsigned char index) {
    if (index < _magnitudes.size()) {
        return int(_magnitudes[index].global);
    }
    return 0;
}

String magnitudeTopic(unsigned char type) {
    char buffer[16] = {0};
    if (type < MAGNITUDE_MAX) strncpy_P(buffer, magnitude_topics[type], sizeof(buffer));
    return String(buffer);
}

String magnitudeTopicIndex(unsigned char index) {
    char topic[32] = {0};
    if (index < _magnitudes.size()) {
        sensor_magnitude_t magnitude = _magnitudes[index];
        if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) {
            snprintf(topic, sizeof(topic), "%s/%u", magnitudeTopic(magnitude.type).c_str(), magnitude.global);
        } else {
            snprintf(topic, sizeof(topic), "%s", magnitudeTopic(magnitude.type).c_str());
        }
    }
    return String(topic);
}


String magnitudeUnits(unsigned char type) {
    char buffer[8] = {0};
    if (type < MAGNITUDE_MAX) {
        if ((type == MAGNITUDE_TEMPERATURE) && (_sensor_temperature_units == TMP_FAHRENHEIT)) {
            strncpy_P(buffer, magnitude_fahrenheit, sizeof(buffer));
        } else if (
            (type == MAGNITUDE_ENERGY || type == MAGNITUDE_ENERGY_DELTA) &&
            (_sensor_energy_units == ENERGY_KWH)) {
            strncpy_P(buffer, magnitude_kwh, sizeof(buffer));
        } else if (
            (type == MAGNITUDE_POWER_ACTIVE || type == MAGNITUDE_POWER_APPARENT || type == MAGNITUDE_POWER_REACTIVE) &&
            (_sensor_power_units == POWER_KILOWATTS)) {
            strncpy_P(buffer, magnitude_kw, sizeof(buffer));
        } else {
            strncpy_P(buffer, magnitude_units[type], sizeof(buffer));
        }
    }
    return String(buffer);
}

// -----------------------------------------------------------------------------

void sensorSetup() {

    // Backwards compatibility
    moveSetting("powerUnits", "pwrUnits");
    moveSetting("energyUnits", "eneUnits");

	// Update PZEM004T energy total across multiple devices
    moveSettings("pzEneTotal", "pzemEneTotal");

    // Load sensors
    _sensorLoad();
    _sensorInit();

    // Configure stored values
    _sensorConfigure();

    // Websockets
    #if WEB_SUPPORT
        wsOnSendRegister(_sensorWebSocketStart);
        wsOnReceiveRegister(_sensorWebSocketOnReceive);
        wsOnSendRegister(_sensorWebSocketSendData);
    #endif

    // API
    #if API_SUPPORT
        _sensorAPISetup();
    #endif

    // Terminal
    #if TERMINAL_SUPPORT
        _sensorInitCommands();
    #endif

    // Main callbacks
    espurnaRegisterLoop(sensorLoop);
    espurnaRegisterReload(_sensorConfigure);

}

void sensorLoop() {

    // Check if we still have uninitialized sensors
    static unsigned long last_init = 0;
    if (!_sensors_ready) {
        if (millis() - last_init > SENSOR_INIT_INTERVAL) {
            last_init = millis();
            _sensorInit();
        }
    }

    if (_magnitudes.size() == 0) return;

    // Tick hook
    _sensorTick();

    // Check if we should read new data
    static unsigned long last_update = 0;
    static unsigned long report_count = 0;
    if (millis() - last_update > _sensor_read_interval) {

        last_update = millis();
        report_count = (report_count + 1) % _sensor_report_every;

        double value_raw;       // holds the raw value as the sensor returns it
        double value_show;      // holds the processed value applying units and decimals
        double value_filtered;  // holds the processed value applying filters, and the units and decimals

        // Pre-read hook
        _sensorPre();

        // Get the first relay state
        #if SENSOR_POWER_CHECK_STATUS
            bool relay_off = (relayCount() == 1) && (relayStatus(0) == 0);
        #endif

        // Get readings
        for (unsigned char i=0; i<_magnitudes.size(); i++) {

            sensor_magnitude_t magnitude = _magnitudes[i];

            if (magnitude.sensor->status()) {

                // -------------------------------------------------------------
                // Instant value
                // -------------------------------------------------------------

                value_raw = magnitude.sensor->value(magnitude.local);

                // Completely remove spurious values if relay is OFF
                #if SENSOR_POWER_CHECK_STATUS
                    if (relay_off) {
                        if (magnitude.type == MAGNITUDE_POWER_ACTIVE ||
                            magnitude.type == MAGNITUDE_POWER_REACTIVE ||
                            magnitude.type == MAGNITUDE_POWER_APPARENT ||
                            magnitude.type == MAGNITUDE_CURRENT ||
                            magnitude.type == MAGNITUDE_ENERGY_DELTA
                        ) {
                            value_raw = 0;
                        }
                    }
                #endif

                _magnitudes[i].last = value_raw;

                // -------------------------------------------------------------
                // Processing (filters)
                // -------------------------------------------------------------

                magnitude.filter->add(value_raw);

                // Special case for MovingAverageFilter
                if (MAGNITUDE_COUNT == magnitude.type ||
                    MAGNITUDE_GEIGER_CPM ==magnitude. type ||
                    MAGNITUDE_GEIGER_SIEVERT == magnitude.type) {
                    value_raw = magnitude.filter->result();
                }

                // -------------------------------------------------------------
                // Procesing (units and decimals)
                // -------------------------------------------------------------

                value_show = _magnitudeProcess(magnitude.type, magnitude.decimals, value_raw);

                // -------------------------------------------------------------
                // Debug
                // -------------------------------------------------------------

                #if SENSOR_DEBUG
                {
                    char buffer[64];
                    dtostrf(value_show, 1-sizeof(buffer), magnitude.decimals, buffer);
                    DEBUG_MSG_P(PSTR("[SENSOR] %s - %s: %s%s\n"),
                        magnitude.sensor->slot(magnitude.local).c_str(),
                        magnitudeTopic(magnitude.type).c_str(),
                        buffer,
                        magnitudeUnits(magnitude.type).c_str()
                    );
                }
                #endif // SENSOR_DEBUG

                // -------------------------------------------------------------
                // Report
                // (we do it every _sensor_report_every readings)
                // -------------------------------------------------------------

                bool report = (0 == report_count);
                if ((MAGNITUDE_ENERGY == magnitude.type) && (magnitude.max_change > 0)) {
                    // for MAGNITUDE_ENERGY, filtered value is last value
                    report = (fabs(value_show - magnitude.reported) >= magnitude.max_change);
                } // if ((MAGNITUDE_ENERGY == magnitude.type) && (magnitude.max_change > 0))

                if (report) {

                    value_filtered = magnitude.filter->result();
                    value_filtered = _magnitudeProcess(magnitude.type, magnitude.decimals, value_filtered);
                    magnitude.filter->reset();

                    // Check if there is a minimum change threshold to report
                    if (fabs(value_filtered - magnitude.reported) >= magnitude.min_change) {
                        _magnitudes[i].reported = value_filtered;
                        _sensorReport(i, value_filtered);
                    } // if (fabs(value_filtered - magnitude.reported) >= magnitude.min_change)


                    // Persist total energy value
                    if (MAGNITUDE_ENERGY == magnitude.type) {
                        _sensorEnergyTotal(value_raw);
                    }

                } // if (report_count == 0)

            } // if (magnitude.sensor->status())
        } // for (unsigned char i=0; i<_magnitudes.size(); i++)

        // Post-read hook
        _sensorPost();

        #if WEB_SUPPORT
            wsSend(_sensorWebSocketSendData);
        #endif

        #if THINGSPEAK_SUPPORT
            if (report_count == 0) tspkFlush();
        #endif

    }

}

#endif // SENSOR_SUPPORT
