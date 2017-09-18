/*

V9261F MODULE
Support for V9261D-based power monitors

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

/*

#if V9261F_SUPPORT

#include <SoftwareSerial.h>
#include <ArduinoJson.h>

SoftwareSerial * _v9261f_uart;

bool _v9261f_enabled = false;
bool _v9261f_ready = false;
bool _v9261f_newdata = false;
int _v9261f_power = 0;
int _v9261f_rpower = 0;
int _v9261f_voltage = 0;
double _v9261f_current = 0;

unsigned char _v9261f_data[24];

// -----------------------------------------------------------------------------
// PRIVATE
// -----------------------------------------------------------------------------

void v9261fRead() {

    static unsigned char state = 0;
    static unsigned long last = 0;
    static bool found = false;
    static unsigned char index = 0;

    if (state == 0) {

        while (_v9261f_uart->available()) {
            _v9261f_uart->flush();
            found = true;
            last = millis();
        }

        if (found && (millis() - last > V9261F_SYNC_INTERVAL)) {
            _v9261f_uart->flush();
            index = 0;
            state = 1;
        }

    } else if (state == 1) {

        while (_v9261f_uart->available()) {
            _v9261f_uart->read();
            if (index++ >= 7) {
                _v9261f_uart->flush();
                index = 0;
                state = 2;
            }
        }

    } else if (state == 2) {

        while (_v9261f_uart->available()) {
            _v9261f_data[index] = _v9261f_uart->read();
            if (index++ >= 19) {
                _v9261f_uart->flush();
                last = millis();
                state = 3;
            }
        }

    } else if (state == 3) {

        if (checksumOK()) {

            _v9261f_power = (double) (
                (_v9261f_data[3]) +
                (_v9261f_data[4] << 8) +
                (_v9261f_data[5] << 16) +
                (_v9261f_data[6] << 24)
            ) / V9261F_POWER_FACTOR;

            _v9261f_rpower = (double) (
                (_v9261f_data[7]) +
                (_v9261f_data[8] <<  8) +
                (_v9261f_data[9] << 16) +
                (_v9261f_data[10] << 24)
            ) / V9261F_RPOWER_FACTOR;

            _v9261f_voltage = (double) (
                (_v9261f_data[11]) +
                (_v9261f_data[12] <<  8) +
                (_v9261f_data[13] << 16) +
                (_v9261f_data[14] << 24)
            ) / V9261F_VOLTAGE_FACTOR;

            _v9261f_current = (double) (
                (_v9261f_data[15]) +
                (_v9261f_data[16] <<  8) +
                (_v9261f_data[17] << 16) +
                (_v9261f_data[18] << 24)
            ) / V9261F_CURRENT_FACTOR;

            _v9261f_newdata = true;

        }

        last = millis();
        index = 0;
        state = 4;

    } else if (state == 4) {

        while (_v9261f_uart->available()) {
            _v9261f_uart->flush();
            last = millis();
        }

        if (millis() - last > V9261F_SYNC_INTERVAL) {
            state = 1;
        }

    }

}

boolean checksumOK() {
    unsigned char checksum = 0;
    for (unsigned char i = 0; i < 19; i++) {
        checksum = checksum + _v9261f_data[i];
    }
    checksum = ~checksum + 0x33;
    return checksum == _v9261f_data[19];
}

// -----------------------------------------------------------------------------
// HAL
// -----------------------------------------------------------------------------

unsigned int getActivePower() {
    return _v9261f_power;
}

unsigned int getReactivePower() {
    return _v9261f_rpower;
}

unsigned int getApparentPower() {
    return sqrt(_v9261f_rpower * _v9261f_rpower + _v9261f_power * _v9261f_power);
}

unsigned int getVoltage() {
    return _v9261f_voltage;
}

double getCurrent() {
    return _v9261f_current;
}

double getPowerFactor() {
    unsigned int apparent = getApparentPower();
    if (apparent > 0) return getActivePower() / getApparentPower();
    return 1;
}

// -----------------------------------------------------------------------------

void v9261fSetup() {

    _v9261f_uart = new SoftwareSerial(V9261F_PIN, SW_SERIAL_UNUSED_PIN, V9261F_PIN_INVERSE, 256);
    _v9261f_uart->begin(V9261F_BAUDRATE);

    // API definitions
    #if WEB_SUPPORT

        apiRegister(HLW8012_POWER_TOPIC, HLW8012_POWER_TOPIC, [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), _v9261f_power);
        });
        apiRegister(HLW8012_CURRENT_TOPIC, HLW8012_CURRENT_TOPIC, [](char * buffer, size_t len) {
            dtostrf(_v9261f_current, len-1, 3, buffer);
        });
        apiRegister(HLW8012_VOLTAGE_TOPIC, HLW8012_VOLTAGE_TOPIC, [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), _v9261f_voltage);
        });

    #endif // WEB_SUPPORT

}


void v9261fLoop() {

    static int sum_power = 0;
    static int sum_rpower = 0;
    static int sum_voltage = 0;
    static double sum_current = 0;
    static int count = 0;

    // Sniff data in the UART interface
    v9261fRead();

    // Do we have new data?
    if (_v9261f_newdata) {

        _v9261f_newdata = false;

        sum_power += getActivePower();
        sum_rpower += getReactivePower();
        sum_voltage += getVoltage();
        sum_current += getCurrent();
        count++;

        #if WEB_SUPPORT
        {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.createObject();

            char buf_current[10];
            dtostrf(getCurrent(), 6, 3, buf_current);

            root["powVisible"] = 1;
            root["powActivePower"] = getActivePower();
            root["powCurrent"] = String(ltrim(buf_current));
            root["powVoltage"] = getVoltage();
            root["powApparentPower"] = getApparentPower();
            root["powReactivePower"] = getReactivePower();
            root["powPowerFactor"] = 100 * getPowerFactor();

            String output;
            root.printTo(output);
            wsSend(output.c_str());
        }
        #endif

    }

    // Do we have to report?
    static unsigned long last = 0;
    if ((count == 0) || (millis() - last < V9261F_REPORT_INTERVAL)) return;
    last = millis();

    {

        unsigned int power = sum_power / count;
        unsigned int reactive = sum_rpower / count;
        unsigned int voltage = sum_voltage / count;
        double current = sum_current / count;
        char buf_current[10];
        dtostrf(current, 6, 3, buf_current);
        unsigned int apparent = sqrt(power * power + reactive * reactive);
        double energy_delta = (double) power * V9261F_REPORT_INTERVAL / 1000.0 / 3600.0;
        char buf_energy[10];
        dtostrf(energy_delta, 6, 3, buf_energy);
        unsigned int factor = 100 * ((double) power / apparent);

        // Report values to MQTT broker
        mqttSend(HLW8012_POWER_TOPIC, String(power).c_str());
        mqttSend(HLW8012_CURRENT_TOPIC, buf_current);
        mqttSend(HLW8012_VOLTAGE_TOPIC, String(voltage).c_str());
        mqttSend(HLW8012_ENERGY_TOPIC, buf_energy);
        mqttSend(HLW8012_APOWER_TOPIC, String(apparent).c_str());
        mqttSend(HLW8012_RPOWER_TOPIC, String(reactive).c_str());
        mqttSend(HLW8012_PFACTOR_TOPIC, String(factor).c_str());

        // Report values to Domoticz
        #if DOMOTICZ_SUPPORT
        {
            char buffer[20];
            snprintf_P(buffer, sizeof(buffer), PSTR("%d;%s"), power, buf_energy);
            domoticzSend("dczPowIdx", 0, buffer);
            snprintf_P(buffer, sizeof(buffer), PSTR("%s"), buf_energy);
            domoticzSend("dczEnergyIdx", 0, buffer);
            snprintf_P(buffer, sizeof(buffer), PSTR("%d"), voltage);
            domoticzSend("dczVoltIdx", 0, buffer);
            snprintf_P(buffer, sizeof(buffer), PSTR("%s"), buf_current);
            domoticzSend("dczCurrentIdx", 0, buffer);
        }
        #endif

        #if INFLUXDB_SUPPORT
        {
            influxDBSend(HLW8012_POWER_TOPIC, String(power).c_str());
            influxDBSend(HLW8012_CURRENT_TOPIC, buf_current);
            influxDBSend(HLW8012_VOLTAGE_TOPIC, String(voltage).c_str());
            influxDBSend(HLW8012_ENERGY_TOPIC, buf_energy);
            influxDBSend(HLW8012_APOWER_TOPIC, String(apparent).c_str());
            influxDBSend(HLW8012_RPOWER_TOPIC, String(reactive).c_str());
            influxDBSend(HLW8012_PFACTOR_TOPIC,  String(factor).c_str());
        }
        #endif

        // Reset counters
        sum_power = sum_rpower = sum_voltage = sum_current = count = 0;

    }

}

#endif

*/
