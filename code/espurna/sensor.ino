/*

SENSOR MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

Module key prefix: sns
Magnitude-based key prefix: pwr ene cur vol tmp hum
Sensor-based key previs: air am ana bh bmx cse dht dig ds ech emon evt gei guv hlw mhz ntc pms pzem sht son tmp3x  v92

*/

#if SENSOR_SUPPORT

#include <vector>
#include "filters/MaxFilter.h"
#include "filters/MedianFilter.h"
#include "filters/MovingAverageFilter.h"
#include "sensors/BaseSensor.h"

typedef struct {
    BaseSensor * sensor;        // Sensor object
    BaseFilter * filter;        // Filter object
    unsigned char local;        // Local index in its provider
    unsigned char type;         // Type of measurement
    unsigned char global;       // Global index in its type
    double current;             // Current (last) value, unfiltered
    double filtered;            // Filtered (averaged) value
    double reported;            // Last reported value
    double min_change;          // Minimum value change to report
} sensor_magnitude_t;

std::vector<BaseSensor *> _sensors;
std::vector<sensor_magnitude_t> _magnitudes;
bool _sensors_ready = false;

unsigned char _counts[MAGNITUDE_MAX];
bool _sensor_realtime = API_REAL_TIME_VALUES;
unsigned long _sensor_read_interval = 1000 * SENSOR_READ_INTERVAL;
unsigned char _sensor_report_every = SENSOR_REPORT_EVERY;
unsigned char _sensor_power_units = SENSOR_POWER_UNITS;
unsigned char _sensor_energy_units = SENSOR_ENERGY_UNITS;
unsigned char _sensor_temperature_units = SENSOR_TEMPERATURE_UNITS;
double _sensor_temperature_correction = SENSOR_TEMPERATURE_CORRECTION;
double _sensor_humidity_correction = SENSOR_HUMIDITY_CORRECTION;

String _sensor_energy_reset_ts = String();

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

unsigned char _magnitudeDecimals(unsigned char type) {

    // Hardcoded decimals (these should be linked to the unit, instead of the magnitude)

    if (type == MAGNITUDE_ENERGY ||
        type == MAGNITUDE_ENERGY_DELTA) {
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

double _magnitudeProcess(unsigned char type, double value) {

    // Hardcoded conversions (these should be linked to the unit, instead of the magnitude)

    if (type == MAGNITUDE_TEMPERATURE) {
        if (_sensor_temperature_units == TMP_FAHRENHEIT) value = value * 1.8 + 32;
        value = value + _sensor_temperature_correction;
    }

    if (type == MAGNITUDE_HUMIDITY) {
        value = constrain(value + _sensor_humidity_correction, 0, 100);
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

    return roundTo(value, _magnitudeDecimals(type));

}

// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _sensorWebSocketSendData(JsonObject& root) {

    char buffer[10];
    bool hasTemperature = false;
    bool hasHumidity = false;

    JsonArray& list = root.createNestedArray("magnitudes");
    for (unsigned char i=0; i<_magnitudes.size(); i++) {

        sensor_magnitude_t magnitude = _magnitudes[i];
        if (magnitude.type == MAGNITUDE_EVENT) continue;

        unsigned char decimals = _magnitudeDecimals(magnitude.type);
        dtostrf(magnitude.current, 1-sizeof(buffer), decimals, buffer);

        JsonObject& element = list.createNestedObject();
        element["index"] = int(magnitude.global);
        element["type"] = int(magnitude.type);
        element["value"] = String(buffer);
        element["units"] = magnitudeUnits(magnitude.type);
        element["error"] = magnitude.sensor->error();

        if (magnitude.type == MAGNITUDE_ENERGY) {
            if (_sensor_energy_reset_ts.length() == 0) _sensorReset();
            element["description"] = magnitude.sensor->slot(magnitude.local) + _sensor_energy_reset_ts;
        } else {
            element["description"] = magnitude.sensor->slot(magnitude.local);
        }

        if (magnitude.type == MAGNITUDE_TEMPERATURE) hasTemperature = true;
        if (magnitude.type == MAGNITUDE_HUMIDITY) hasHumidity = true;

    }

    if (hasTemperature) root["tmpVisible"] = 1;
    if (hasHumidity) root["humVisible"] = 1;

}

void _sensorWebSocketStart(JsonObject& root) {

    for (unsigned char i=0; i<_sensors.size(); i++) {

        BaseSensor * sensor = _sensors[i];

        #if EMON_ANALOG_SUPPORT
            if (sensor->getID() == SENSOR_EMON_ANALOG_ID) {
                root["emonVisible"] = 1;
                root["pwrVisible"] = 1;
                root["volNominal"] = ((EmonAnalogSensor *) sensor)->getVoltage();
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

    }

    if (_magnitudes.size() > 0) {
        root["snsVisible"] = 1;
        root["pwrUnits"] = _sensor_power_units;
        root["eneUnits"] = _sensor_energy_units;
        root["tmpUnits"] = _sensor_temperature_units;
        root["tmpOffset"] = _sensor_temperature_correction;
        root["humOffset"] = _sensor_humidity_correction;
        root["snsRead"] = _sensor_read_interval / 1000;
        root["snsReport"] = _sensor_report_every;
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
            unsigned char decimals = _magnitudeDecimals(magnitude.type);
            double value = _sensor_realtime ? magnitude.current : magnitude.filtered;
            dtostrf(value, 1-len, decimals, buffer);
        });

    }

}

#endif // API_SUPPORT

#if TERMINAL_SUPPORT

void _sensorInitCommands() {
    settingsRegisterCommand(F("MAGNITUDES"), [](Embedis* e) {
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
        DEBUG_MSG_P(PSTR("+OK\n"));
    });
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

void _sensorReset() {
    #if NTP_SUPPORT
        if (ntpSynced()) {
            _sensor_energy_reset_ts = String(" (since ") + ntpDateTime() + String(")");
        }
    #endif
}

// -----------------------------------------------------------------------------
// Sensor initialization
// -----------------------------------------------------------------------------

void _sensorLoad() {

    /*
    Only loaded (those with *_SUPPORT to 1) and enabled (*Enabled setting to 1)
    sensors are being initialized here.
    */

    unsigned char index = 0;
    unsigned char gpio = GPIO_NONE;

    #if AM2320_SUPPORT
    if (getSetting("amEnabled", 0).toInt() == 1) {
         AM2320Sensor * sensor = new AM2320Sensor();
         sensor->setAddress(getSetting("amAddress", AM2320_ADDRESS).toInt());
         _sensors.push_back(sensor);
    }
    #endif

    #if ANALOG_SUPPORT
    if (getSetting("anaEnabled", 0).toInt() == 1) {
        AnalogSensor * sensor = new AnalogSensor();
        sensor->setSamples(getSetting("anaSamples", ANALOG_SAMPLES).toInt());
        sensor->setDelay(getSetting("anaDelay", ANALOG_DELAY).toInt());
        _sensors.push_back(sensor);
    }
    #endif

    #if BH1750_SUPPORT
    if (getSetting("bhEnabled", 0).toInt() == 1) {
        BH1750Sensor * sensor = new BH1750Sensor();
        sensor->setAddress(getSetting("bhAddress", BH1750_ADDRESS).toInt());
        sensor->setMode(getSetting("bhMode", BH1750_MODE).toInt());
        _sensors.push_back(sensor);
    }
    #endif

    #if BMX280_SUPPORT
    if (getSetting("bmx280Enabled", 0).toInt() == 1) {
        BMX280Sensor * sensor = new BMX280Sensor();
        sensor->setAddress(getSetting("bmx280Address", BMX280_ADDRESS).toInt());
        _sensors.push_back(sensor);
    }
    #endif

    #if CSE7766_SUPPORT
    if (getSetting("cseEnabled", 0).toInt() == 1) {
        if ((gpio = getSetting("cseGPIO", GPIO_NONE).toInt()) != GPIO_NONE) {

            CSE7766Sensor * sensor = new CSE7766Sensor();

            sensor->setRX(gpio);

            double value;
            value = getSetting("curRatio", 0).toFloat();
            if (value > 0) sensor->setCurrentRatio(value);
            value = getSetting("volRatio", 0).toFloat();
            if (value > 0) sensor->setVoltageRatio(value);
            value = getSetting("pwrRatio", 0).toFloat();
            if (value > 0) sensor->setPowerRatio(value);

            _sensors.push_back(sensor);

        }
    }
    #endif

    #if DALLAS_SUPPORT
    if (getSetting("dsEnabled", 0).toInt() == 1) {
        index = 0;
        while ((gpio = getSetting("dsGPIO", index, GPIO_NONE).toInt()) != GPIO_NONE) {
            DallasSensor * sensor = new DallasSensor();
            sensor->setGPIO(gpio);
            _sensors.push_back(sensor);
            index++;
        }
    }
    #endif

    #if DHT_SUPPORT
    if (getSetting("dhtEnabled", 0).toInt() == 1) {
        index = 0;
        while ((gpio = getSetting("dhtGPIO", index, GPIO_NONE).toInt()) != GPIO_NONE) {
            DHTSensor * sensor = new DHTSensor();
            sensor->setGPIO(gpio);
            sensor->setType(getSetting("dhtType", index, DHT_CHIP_DHT22).toInt());
            _sensors.push_back(sensor);
            index++;
        }
    }
    #endif

    #if DIGITAL_SUPPORT
    if (getSetting("digEnabled", 0).toInt() == 1) {
        index = 0;
        while ((gpio = getSetting("digGPIO", index, GPIO_NONE).toInt()) != GPIO_NONE) {
            DigitalSensor * sensor = new DigitalSensor();
            sensor->setGPIO(gpio);
            sensor->setMode(getSetting("digMode", index, DIGITAL_PIN_MODE).toInt());
            sensor->setDefault(getSetting("digDefault", index, DIGITAL_DEFAULT_STATE).toInt());
            _sensors.push_back(sensor);
            index++;
        }
    }
    #endif

    #if ECH1560_SUPPORT
    if (getSetting("echEnabled", 0).toInt() == 1) {
        ECH1560Sensor * sensor = new ECH1560Sensor();
        sensor->setCLK(getSetting("echCLKGPIO", ECH1560_CLK_PIN).toInt());
        sensor->setMISO(getSetting("echMISOGPIO", ECH1560_MISO_PIN).toInt());
        sensor->setInverted(getSetting("echLogic", ECH1560_INVERTED).toInt());
        _sensors.push_back(sensor);
    }
    #endif

    #if EMON_ADC121_SUPPORT || EMON_ADS1X15_SUPPORT || EMON_ANALOG_SUPPORT

    if (getSetting("emonEnabled", 0).toInt() == 1) {

        #if EMON_ADC121_SUPPORT
        if (getSetting("emonProvider", 0).toInt() == EMON_PROVIDER_ADC121) {
            EmonADC121Sensor * sensor = new EmonADC121Sensor();
            sensor->setAddress(getSetting("emonAddress", EMON_ADC121_I2C_ADDRESS).toInt());
            sensor->setReference(getSetting("emonReference", EMON_REFERENCE_VOLTAGE).toInt());
            sensor->setCurrentRatio(0, getSetting("curRatio", EMON_CURRENT_RATIO).toFloat());
            sensor->setVoltage(getSetting("volNominal", EMON_MAINS_VOLTAGE).toInt());
            _sensors.push_back(sensor);
        }
        #endif

        #if EMON_ADS1X15_SUPPORT
        if (getSetting("emonProvider", 0).toInt() == EMON_PROVIDER_ADS1X15) {
            EmonADS1X15Sensor * sensor = new EmonADS1X15Sensor();
            sensor->setAddress(getSetting("emonAddress", EMON_ADS1X15_I2C_ADDRESS).toInt());
            sensor->setType(getSetting("emonType", EMON_ADS1X15_TYPE).toInt());
            sensor->setMask(getSetting("emonMask", EMON_ADS1X15_MASK).toInt());
            sensor->setGain(getSetting("emonGain", EMON_ADS1X15_GAIN).toInt());
            sensor->setReference(getSetting("emonReference", EMON_REFERENCE_VOLTAGE).toInt());
            double curRatio = getSetting("curRatio", EMON_CURRENT_RATIO).toFloat();
            sensor->setCurrentRatio(0, getSetting("curRatio", 0, curRatio).toFloat());
            sensor->setCurrentRatio(1, getSetting("curRatio", 1, curRatio).toFloat());
            sensor->setCurrentRatio(2, getSetting("curRatio", 2, curRatio).toFloat());
            sensor->setCurrentRatio(3, getSetting("curRatio", 3, curRatio).toFloat());
            sensor->setVoltage(getSetting("volNominal", EMON_MAINS_VOLTAGE).toInt());
            _sensors.push_back(sensor);
        }
        #endif

        #if EMON_ANALOG_SUPPORT
        if (getSetting("emonProvider", 0).toInt() == EMON_PROVIDER_ANALOG) {
            EmonAnalogSensor * sensor = new EmonAnalogSensor();
            sensor->setReference(getSetting("emonReference", EMON_REFERENCE_VOLTAGE).toInt());
            sensor->setCurrentRatio(0, getSetting("curRatio", EMON_CURRENT_RATIO).toFloat());
            sensor->setVoltage(getSetting("volNominal", EMON_MAINS_VOLTAGE).toInt());
            _sensors.push_back(sensor);
        }
        #endif

    }

    #endif

    #if EVENTS_SUPPORT
    if (getSetting("evtEnabled", 0).toInt() == 1) {
        index = 0;
        while ((gpio = getSetting("evtGPIO", index, GPIO_NONE).toInt()) != GPIO_NONE) {
            EventSensor * sensor = new EventSensor();
            sensor->setGPIO(gpio);
            sensor->setTrigger(getSetting("evtTrigger", index, EVENTS_TRIGGER).toInt());
            sensor->setPinMode(getSetting("evtMode", index, EVENTS_PIN_MODE).toInt());
            sensor->setDebounceTime(getSetting("evtDebounce", index, EVENTS_DEBOUNCE).toInt());
            sensor->setInterruptMode(getSetting("evtIntMode", index, EVENTS_INTERRUPT_MODE).toInt());
            _sensors.push_back(sensor);
            index++;
        }
    }
    #endif

    #if GEIGER_SUPPORT
    if (getSetting("geiEnabled", 0).toInt() == 1) {
        if ((gpio = getSetting("geiGPIO", GPIO_NONE).toInt()) != GPIO_NONE) {
            GeigerSensor * sensor = new GeigerSensor();                                         // Create instance of the Geiger module.
            sensor->setGPIO(gpio);                                                              // Interrupt pin of the attached geiger counter board.
            sensor->setMode(getSetting("geiMode", GEIGER_PIN_MODE).toInt());                    // This pin is an input.
            sensor->setDebounceTime(getSetting("geiDebounce", GEIGER_DEBOUNCE).toInt());        // Debounce time 25ms, because https://github.com/Trickx/espurna/wiki/Geiger-counter
            sensor->setInterruptMode(getSetting("geiIntMode", GEIGER_INTERRUPT_MODE).toInt());  // Interrupt triggering: edge detection rising.
            sensor->setCPM2SievertFactor(getSetting("geiRatio", GEIGER_CPM2SIEVERT).toInt());   // Conversion factor from counts per minute to µSv/h
            _sensors.push_back(sensor);
        }
    }
    #endif

    #if GUVAS12SD_SUPPORT
    if (getSetting("guvEnabled", 0).toInt() == 1) {
        if ((gpio = getSetting("guvGPIO", GPIO_NONE).toInt()) != GPIO_NONE) {
            GUVAS12SDSensor * sensor = new GUVAS12SDSensor();
            sensor->setGPIO(gpio);
            _sensors.push_back(sensor);
        }
    }
    #endif

    #if SONAR_SUPPORT
    if (getSetting("sonEnabled", 0).toInt() == 1) {
        SonarSensor * sensor = new SonarSensor();
        sensor->setEcho(getSetting("sonEcho", SONAR_ECHO).toInt());
        sensor->setTrigger(getSetting("sonTrigger", SONAR_TRIGGER).toInt());
        sensor->setIterations(getSetting("sonIterations", SONAR_ITERATIONS).toInt());
        sensor->setMaxDistance(getSetting("sonMaxDist", SONAR_MAX_DISTANCE).toInt());
        _sensors.push_back(sensor);
    }
    #endif

    #if HLW8012_SUPPORT
    if (getSetting("hlwEnabled", 0).toInt() == 1) {

        HLW8012Sensor * sensor = new HLW8012Sensor();

        sensor->setSEL(getSetting("hlwSELGPIO", HLW8012_SEL_PIN).toInt());
        sensor->setCF(getSetting("hlwCFGPIO", HLW8012_CF_PIN).toInt());
        sensor->setCF1(getSetting("hlwCF1GPIO", HLW8012_CF1_PIN).toInt());
        sensor->setCurrentSEL(getSetting("hlwCurSel", HLW8012_SEL_CURRENT).toInt());
        sensor->setInterruptMode(getSetting("hlwIntMode", HLW8012_INTERRUPT_ON).toInt());
        sensor->setCurrentResistor(getSetting("hlwCurRes", HLW8012_CURRENT_R ).toFloat());
        sensor->setUpstreamResistor(getSetting("hlwVolResUp", HLW8012_VOLTAGE_R_UP).toFloat());
        sensor->setDownstreamResistor(getSetting("hlwVolResDw", HLW8012_VOLTAGE_R_DOWN).toFloat());

        double value;
        value = getSetting("curRatio", HLW8012_CURRENT_RATIO).toFloat();
        if (value > 0) sensor->setCurrentRatio(value);
        value = getSetting("volRatio", HLW8012_VOLTAGE_RATIO).toFloat();
        if (value > 0) sensor->setVoltageRatio(value);
        value = getSetting("pwrRatio", HLW8012_POWER_RATIO).toFloat();
        if (value > 0) sensor->setPowerRatio(value);

        _sensors.push_back(sensor);

    }
    #endif

    #if MHZ19_SUPPORT
    if (getSetting("mhzEnabled", 0).toInt() == 1) {
        MHZ19Sensor * sensor = new MHZ19Sensor();
        sensor->setRX(getSetting("mhzRX", MHZ19_RX_PIN).toInt());
        sensor->setTX(getSetting("mhzTX", MHZ19_TX_PIN).toInt());
        _sensors.push_back(sensor);
    }
    #endif

    #if NTC_SUPPORT
    if (getSetting("ntcEnabled", 0).toInt() == 1) {
        NTCSensor * sensor = new NTCSensor();
        sensor->setSamples(getSetting("ntcSamples", NTC_SAMPLES).toInt());
        sensor->setDelay(getSetting("ntcDelay", NTC_DELAY).toInt());
        sensor->setUpstreamResistor(getSetting("ntcResUp", NTC_R_UP).toInt());
        sensor->setDownstreamResistor(getSetting("ntcResDown", NTC_R_DOWN).toInt());
        sensor->setBeta(getSetting("ntcBeta", NTC_BETA).toInt());
        sensor->setR0(getSetting("ntcR0", NTC_R0).toInt());
        sensor->setT0(getSetting("ntcT0", NTC_T0).toFloat());
        _sensors.push_back(sensor);
    }
    #endif

    #if SENSEAIR_SUPPORT
    if (getSetting("airEnabled", 0).toInt() == 1) {
        SenseAirSensor * sensor = new SenseAirSensor();
        sensor->setRX(getSetting("airRX", SENSEAIR_RX_PIN).toInt());
        sensor->setTX(getSetting("airTX", SENSEAIR_TX_PIN).toInt());
        _sensors.push_back(sensor);
    }
    #endif

    #if PMSX003_SUPPORT
    if (getSetting("pmsEnabled", 0).toInt() == 1) {
        PMSX003Sensor * sensor = new PMSX003Sensor();
        if (getSetting("pmsSoft", PMS_USE_SOFT).toInt() == 1) {
            sensor->setRX(getSetting("pmsRX", PMS_RX_PIN).toInt());
            sensor->setTX(getSetting("pmsTX", PMS_TX_PIN).toInt());
        } else {
            sensor->setSerial(& PMS_HW_PORT);
        }
        sensor->setType(getSetting("pmsType", PMS_TYPE).toInt());
        _sensors.push_back(sensor);
    }
    #endif

    #if PZEM004T_SUPPORT
    if (getSetting("pzemEnabled", 0).toInt() == 1) {
        PZEM004TSensor * sensor = new PZEM004TSensor();
        if (getSetting("pzemSoft", PZEM004T_USE_SOFT).toInt() == 1) {
            sensor->setRX(getSetting("pzemRX", PZEM004T_RX_PIN).toInt());
            sensor->setTX(getSetting("pzemTX", PZEM004T_TX_PIN).toInt());
        } else {
            sensor->setSerial(& PZEM004T_HW_PORT);
        }
        _sensors.push_back(sensor);
    }
    #endif

    #if SHT3X_I2C_SUPPORT
    if (getSetting("shtEnabled", 0).toInt() == 1) {
         SHT3XI2CSensor * sensor = new SHT3XI2CSensor();
         sensor->setAddress(getSetting("shtAddress", SHT3X_I2C_ADDRESS).toInt());
         _sensors.push_back(sensor);
    }
    #endif

    #if SI7021_SUPPORT
    if (getSetting("si7021Enabled", 0).toInt() == 1) {
         SI7021Sensor * sensor = new SI7021Sensor();
         sensor->setAddress(getSetting("si7021Address", SI7021_ADDRESS).toInt());
         _sensors.push_back(sensor);
    }
    #endif

    #if TMP3X_SUPPORT
    if (getSetting("tmp3xEnabled", 0).toInt() == 1) {
         TMP3XSensor * sensor = new TMP3XSensor();
         sensor->setType(getSetting("tmp3xType", TMP3X_TYPE).toInt());
         _sensors.push_back(sensor);
    }
    #endif

    #if V9261F_SUPPORT
    if (getSetting("v92Enabled", 0).toInt() == 1) {
        if ((gpio = getSetting("v92GPIO", GPIO_NONE).toInt()) != GPIO_NONE) {
            V9261FSensor * sensor = new V9261FSensor();
            sensor->setRX(gpio);
            sensor->setInverted(getSetting("v92Inverse", V9261F_PIN_INVERSE).toInt());
            _sensors.push_back(sensor);
        }
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

            sensor_magnitude_t new_magnitude;
            new_magnitude.sensor = _sensors[i];
            new_magnitude.local = k;
            new_magnitude.type = type;
            new_magnitude.global = _counts[type];
            new_magnitude.current = 0;
            new_magnitude.filtered = 0;
            new_magnitude.reported = 0;
            new_magnitude.min_change = 0;
            if (type == MAGNITUDE_DIGITAL) {
                new_magnitude.filter = new MaxFilter();
            } else if (type == MAGNITUDE_COUNT || type == MAGNITUDE_GEIGER_CPM|| type == MAGNITUDE_GEIGER_SIEVERT) {  // For geiger counting moving average filter is the most appropriate if needed at all.
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

    }

}

void _sensorConfigure() {

    // General sensor settings
    _sensor_read_interval = 1000 * constrain(getSetting("snsRead", SENSOR_READ_INTERVAL).toInt(), SENSOR_READ_MIN_INTERVAL, SENSOR_READ_MAX_INTERVAL);
    _sensor_report_every = constrain(getSetting("snsReport", SENSOR_REPORT_EVERY).toInt(), SENSOR_REPORT_MIN_EVERY, SENSOR_REPORT_MAX_EVERY);
    _sensor_realtime = apiRealTime();
    _sensor_power_units = getSetting("pwrUnits", SENSOR_POWER_UNITS).toInt();
    _sensor_energy_units = getSetting("eneUnits", SENSOR_ENERGY_UNITS).toInt();
    _sensor_temperature_units = getSetting("tmpUnits", SENSOR_TEMPERATURE_UNITS).toInt();
    _sensor_temperature_correction = getSetting("tmpOffset", SENSOR_TEMPERATURE_CORRECTION).toFloat();
    _sensor_humidity_correction = getSetting("humOffset", SENSOR_HUMIDITY_CORRECTION).toFloat();

    // Specific sensor settings
    for (unsigned char i=0; i<_sensors.size(); i++) {

        #if EMON_ANALOG_SUPPORT

            if (_sensors[i]->getID() == SENSOR_EMON_ANALOG_ID) {

                double value;
                EmonAnalogSensor * sensor = (EmonAnalogSensor *) _sensors[i];

                if ((value = getSetting("pwrExpected", 0).toInt())) {
                    sensor->expectedPower(0, value);
                    setSetting("curRatio", sensor->getCurrentRatio(0));
                }

                if (getSetting("snsResetCalibrarion", 0).toInt() == 1) {
                    sensor->setCurrentRatio(0, EMON_CURRENT_RATIO);
                    delSetting("curRatio");
                }

                if (getSetting("eneReset", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    _sensorReset();
                }

                sensor->setVoltage(getSetting("volNominal", EMON_MAINS_VOLTAGE).toInt());

            }

        #endif // EMON_ANALOG_SUPPORT

        #if EMON_ADC121_SUPPORT
            if (_sensors[i]->getID() == SENSOR_EMON_ADC121_ID) {
                EmonADC121Sensor * sensor = (EmonADC121Sensor *) _sensors[i];
                if (getSetting("eneReset", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    _sensorReset();
                }
            }
        #endif

        #if EMON_ADS1X15_SUPPORT
            if (_sensors[i]->getID() == SENSOR_EMON_ADS1X15_ID) {
                EmonADS1X15Sensor * sensor = (EmonADS1X15Sensor *) _sensors[i];
                if (getSetting("eneReset", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    _sensorReset();
                }
            }
        #endif

        #if HLW8012_SUPPORT


            if (_sensors[i]->getID() == SENSOR_HLW8012_ID) {

                double value;
                HLW8012Sensor * sensor = (HLW8012Sensor *) _sensors[i];

                if (value = getSetting("curExpected", 0).toFloat()) {
                    sensor->expectedCurrent(value);
                    setSetting("curRatio", sensor->getCurrentRatio());
                }

                if (value = getSetting("volExpected", 0).toInt()) {
                    sensor->expectedVoltage(value);
                    setSetting("volRatio", sensor->getVoltageRatio());
                }

                if (value = getSetting("pwrExpected", 0).toInt()) {
                    sensor->expectedPower(value);
                    setSetting("pwrRatio", sensor->getPowerRatio());
                }

                if (getSetting("eneReset", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    _sensorReset();
                }

                if (getSetting("snsResetCalibrarion", 0).toInt() == 1) {
                    sensor->resetRatios();
                    delSetting("curRatio");
                    delSetting("volRatio");
                    delSetting("pwrRatio");
                }

            }

        #endif // HLW8012_SUPPORT

        #if CSE7766_SUPPORT

            if (_sensors[i]->getID() == SENSOR_CSE7766_ID) {

                double value;
                CSE7766Sensor * sensor = (CSE7766Sensor *) _sensors[i];

                if ((value = getSetting("curExpected", 0).toFloat())) {
                    sensor->expectedCurrent(value);
                    setSetting("curRatio", sensor->getCurrentRatio());
                }

                if ((value = getSetting("volExpected", 0).toInt())) {
                    sensor->expectedVoltage(value);
                    setSetting("volRatio", sensor->getVoltageRatio());
                }

                if ((value = getSetting("pwrExpected", 0).toInt())) {
                    sensor->expectedPower(value);
                    setSetting("pwrRatio", sensor->getPowerRatio());
                }

                if (getSetting("eneReset", 0).toInt() == 1) {
                    sensor->resetEnergy();
                    _sensorReset();
                }

                if (getSetting("snsResetCalibrarion", 0).toInt() == 1) {
                    sensor->resetRatios();
                    delSetting("curRatio");
                    delSetting("volRatio");
                    delSetting("pwrRatio");
                }

            }

        #endif // CSE7766_SUPPORT

    }

    // Update filter sizes
    for (unsigned char i=0; i<_magnitudes.size(); i++) {
        _magnitudes[i].filter->resize(_sensor_report_every);
    }

    // Save settings
    delSetting("pwrExpected");
    delSetting("curExpected");
    delSetting("volExpected");
    delSetting("snsResetCalibrarion");
    delSetting("eneReset");
    saveSettings();

}

bool _sensorKeyCheck(const char * key) {

    if (strncmp(key, "sns", 3) == 0) return true;

    if (strncmp(key, "pwr", 3) == 0) return true;
    if (strncmp(key, "ene", 3) == 0) return true;
    if (strncmp(key, "cur", 3) == 0) return true;
    if (strncmp(key, "vol", 3) == 0) return true;
    if (strncmp(key, "tmp", 3) == 0) return true;
    if (strncmp(key, "hum", 3) == 0) return true;

    if (strncmp(key, "air", 3) == 0) return true;
    if (strncmp(key, "am", 2) == 0) return true;
    if (strncmp(key, "ana", 3) == 0) return true;
    if (strncmp(key, "bh", 2) == 0) return true;
    if (strncmp(key, "bmx", 3) == 0) return true;
    if (strncmp(key, "cse", 3) == 0) return true;
    if (strncmp(key, "dht", 3) == 0) return true;
    if (strncmp(key, "dig", 3) == 0) return true;
    if (strncmp(key, "ds" , 2) == 0) return true;
    if (strncmp(key, "ech", 3) == 0) return true;
    if (strncmp(key, "emon", 4) == 0) return true;
    if (strncmp(key, "evt", 3) == 0) return true;
    if (strncmp(key, "gei", 3) == 0) return true;
    if (strncmp(key, "guv", 3) == 0) return true;
    if (strncmp(key, "hlw", 3) == 0) return true;
    if (strncmp(key, "mhz", 3) == 0) return true;
    if (strncmp(key, "ntc", 3) == 0) return true;
    if (strncmp(key, "pms", 3) == 0) return true;
    if (strncmp(key, "pzem", 4) == 0) return true;
    if (strncmp(key, "sht", 3) == 0) return true;
    if (strncmp(key, "son", 3) == 0) return true;
    if (strncmp(key, "tmp3x", 4) == 0) return true;
    if (strncmp(key, "v92", 3) == 0) return true;

    return false;

}

void _sensorBackwards() {
    moveSetting("powerUnits", "pwrUnits"); // 1.12.5 - 2018-04-03
    moveSetting("tmpCorrection", "tmpOffset"); // 1.14.0 - 2018-06-26
    moveSetting("humCorrection", "humOffset"); // 1.14.0 - 2018-06-26
    moveSetting("energyUnits", "eneUnits"); // 1.14.0 - 2018-06-26
    moveSetting("pwrRatioC", "curRatio"); // 1.14.0 - 2018-06-26
    moveSetting("pwrRatioP", "pwrRatio"); // 1.14.0 - 2018-06-26
    moveSetting("pwrRatioV", "volRatio"); // 1.14.0 - 2018-06-26
    moveSetting("pwrVoltage", "volNominal"); // 1.14.0 - 2018-06-26
    moveSetting("pwrExpectedP", "pwrExpected"); // 1.14.0 - 2018-06-26
    moveSetting("pwrExpectedC", "curExpected"); // 1.14.0 - 2018-06-26
    moveSetting("pwrExpectedV", "volExpected"); // 1.14.0 - 2018-06-26
    moveSetting("pwrResetCalibration", "snsResetCalibration"); // 1.14.0 - 2018-06-26
    moveSetting("pwrResetE", "eneReset"); // 1.14.0 - 2018-06-26

}

void _sensorReport(unsigned char index, double value) {

    sensor_magnitude_t magnitude = _magnitudes[index];
    unsigned char decimals = _magnitudeDecimals(magnitude.type);

    char buffer[10];
    dtostrf(value, 1-sizeof(buffer), decimals, buffer);

    #if BROKER_SUPPORT
        brokerPublish(magnitudeTopic(magnitude.type).c_str(), magnitude.local, buffer);
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

    #if INFLUXDB_SUPPORT
        if (SENSOR_USE_INDEX || (_counts[magnitude.type] > 1)) {
            idbSend(magnitudeTopic(magnitude.type).c_str(), magnitude.global, buffer);
        } else {
            idbSend(magnitudeTopic(magnitude.type).c_str(), buffer);
        }
    #endif // INFLUXDB_SUPPORT

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
    _sensorBackwards();

    // Load sensors
    _sensorLoad();
    _sensorInit();

    // Configure stored values
    _sensorConfigure();

    // Websockets
    #if WEB_SUPPORT
        wsOnSendRegister(_sensorWebSocketStart);
        wsOnSendRegister(_sensorWebSocketSendData);
        wsOnAfterParseRegister(_sensorConfigure);
    #endif

    // API
    #if API_SUPPORT
        _sensorAPISetup();
    #endif

    // Terminal
    #if TERMINAL_SUPPORT
        _sensorInitCommands();
    #endif

    settingsRegisterKeyCheck(_sensorKeyCheck);

    // Register loop
    espurnaRegisterLoop(sensorLoop);

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

        double current;
        double filtered;

        // Pre-read hook
        _sensorPre();

        // Get the first relay state
        #if SENSOR_POWER_CHECK_STATUS
            bool relay_off = (relayCount() > 0) && (relayStatus(0) == 0);
        #endif

        // Get readings
        for (unsigned char i=0; i<_magnitudes.size(); i++) {

            sensor_magnitude_t magnitude = _magnitudes[i];

            if (magnitude.sensor->status()) {

                current = magnitude.sensor->value(magnitude.local);

                // Completely remove spurious values if relay is OFF
                #if SENSOR_POWER_CHECK_STATUS
                    if (relay_off) {
                        if (magnitude.type == MAGNITUDE_POWER_ACTIVE ||
                            magnitude.type == MAGNITUDE_POWER_REACTIVE ||
                            magnitude.type == MAGNITUDE_POWER_APPARENT ||
                            magnitude.type == MAGNITUDE_CURRENT ||
                            magnitude.type == MAGNITUDE_ENERGY_DELTA
                        ) {
                            current = 0;
                        }
                    }
                #endif

                magnitude.filter->add(current);

                // Special case
                if (magnitude.type == MAGNITUDE_COUNT) {
                    current = magnitude.filter->result();
                }

                current = _magnitudeProcess(magnitude.type, current);
                _magnitudes[i].current = current;

                // Debug
                #if SENSOR_DEBUG
                {
                    char buffer[64];
                    dtostrf(current, 1-sizeof(buffer), _magnitudeDecimals(magnitude.type), buffer);
                    DEBUG_MSG_P(PSTR("[SENSOR] %s - %s: %s%s\n"),
                        magnitude.sensor->slot(magnitude.local).c_str(),
                        magnitudeTopic(magnitude.type).c_str(),
                        buffer,
                        magnitudeUnits(magnitude.type).c_str()
                    );
                }
                #endif // SENSOR_DEBUG

                // Time to report (we do it every _sensor_report_every readings)
                if (report_count == 0) {

                    filtered = magnitude.filter->result();
                    magnitude.filter->reset();
                    filtered = _magnitudeProcess(magnitude.type, filtered);
                    _magnitudes[i].filtered = filtered;

                    // Check if there is a minimum change threshold to report
                    if (fabs(filtered - magnitude.reported) >= magnitude.min_change) {

                        _magnitudes[i].reported = filtered;

                        _sensorReport(i, filtered);

                    } // if (fabs(filtered - magnitude.reported) >= magnitude.min_change)
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
