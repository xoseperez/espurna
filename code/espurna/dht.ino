/*

DHT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if DHT_SUPPORT

double _dhtTemperature = 0;
unsigned int _dhtHumidity = 0;

// -----------------------------------------------------------------------------
// HAL
// https://github.com/gosouth/DHT22/blob/master/main/DHT22.c
// -----------------------------------------------------------------------------

#define DHT_MAX_DATA                5
#define DHT_MAX_ERRORS              5
#define DHT_MIN_INTERVAL            2000
#define DHT_OK                      0
#define DHT_CHECKSUM_ERROR          -1
#define DHT_TIMEOUT_ERROR           -2

#define DHT11                       11
#define DHT22                       22
#define DHT21                       21
#define AM2301                      21

unsigned long _getSignalLevel(unsigned char gpio, int usTimeOut, bool state) {
	unsigned long uSec = 1;
	while (digitalRead(gpio) == state) {
        if (++uSec > usTimeOut) return 0;
        delayMicroseconds(1);
	}
	return uSec;
}

int readDHT(unsigned char gpio, unsigned char type) {

    static unsigned long last_ok = 0;
    if (millis() - last_ok < DHT_MIN_INTERVAL) return DHT_OK;

    unsigned long low = 0;
    unsigned long high = 0;

    static unsigned char errors = 0;
    uint8_t dhtData[DHT_MAX_DATA] = {0};
    uint8_t byteInx = 0;
    uint8_t bitInx = 7;

	// Send start signal to DHT sensor
	if (++errors > DHT_MAX_ERRORS) {
        errors = 0;
        digitalWrite(gpio, HIGH);
        delay(250);
    }
    pinMode(gpio, OUTPUT);
	digitalWrite(gpio, LOW);
	delay(20);
    noInterrupts();
    digitalWrite(gpio, HIGH);
    delayMicroseconds(40);
    pinMode(gpio, INPUT_PULLUP);
    delayMicroseconds(10);

	// DHT will keep the line low for 80 us and then high for 80us
	low = _getSignalLevel(gpio, 85, LOW);
	if (low==0) return DHT_TIMEOUT_ERROR;
	high = _getSignalLevel(gpio, 85, HIGH);
    if (high==0) return DHT_TIMEOUT_ERROR;

	// No errors, read the 40 data bits
	for( int k = 0; k < 40; k++ ) {

		// Starts new data transmission with >50us low signal
		low = _getSignalLevel(gpio, 56, LOW);
		if (low==0) return DHT_TIMEOUT_ERROR;

		// Check to see if after >70us rx data is a 0 or a 1
		high = _getSignalLevel(gpio, 75, HIGH);
        if (high==0) return DHT_TIMEOUT_ERROR;

		// add the current read to the output data
		// since all dhtData array where set to 0 at the start,
		// only look for "1" (>28us us)
		if (high > low) dhtData[byteInx] |= (1 << bitInx);

		// index to next byte
		if (bitInx == 0) {
            bitInx = 7;
            ++byteInx;
        } else {
    		--bitInx;
        }

	}

    interrupts();

    // Verify checksum
    if (dhtData[4] != ((dhtData[0] + dhtData[1] + dhtData[2] + dhtData[3]) & 0xFF)) {
        return DHT_CHECKSUM_ERROR;
    }

	// Get humidity from Data[0] and Data[1]
    if (type == DHT11) {
        _dhtHumidity = dhtData[0];
    } else {
	   _dhtHumidity = dhtData[0] * 256 + dhtData[1];
	   _dhtHumidity /= 10;
    }

	// Get temp from Data[2] and Data[3]
    if (type == DHT11) {
        _dhtTemperature = dhtData[2];
    } else {
        _dhtTemperature = (dhtData[2] & 0x7F) * 256 + dhtData[3];
        _dhtTemperature /= 10;
        if (dhtData[2] & 0x80) _dhtTemperature *= -1;
    }

    last_ok = millis();
    errors = 0;
	return DHT_OK;

}

int readDHT() {
    return readDHT(DHT_PIN, DHT_TYPE);
}

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

void _dhtWebSocketOnSend(JsonObject& root) {
    root["dhtVisible"] = 1;
    root["dhtTmp"] = getDHTTemperature();
    root["dhtHum"] = getDHTHumidity();
}

// -----------------------------------------------------------------------------
// Values
// -----------------------------------------------------------------------------

double getDHTTemperature(bool celsius) {
    double value = celsius ? _dhtTemperature : _dhtTemperature * 1.8 + 32;
    double correction = getSetting("tmpCorrection", TEMPERATURE_CORRECTION).toFloat();
    return roundTo(value + correction, DHT_TEMPERATURE_DECIMALS);
}

double getDHTTemperature() {
    return getDHTTemperature(true);
}

unsigned int getDHTHumidity() {
    return _dhtHumidity;
}

void dhtSetup() {

    #if WEB_SUPPORT

        // Websockets
        wsOnSendRegister(_dhtWebSocketOnSend);

        apiRegister(DHT_TEMPERATURE_TOPIC, DHT_TEMPERATURE_TOPIC, [](char * buffer, size_t len) {
            dtostrf(_dhtTemperature, 1-len, 1, buffer);
        });
        apiRegister(DHT_HUMIDITY_TOPIC, DHT_HUMIDITY_TOPIC, [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), _dhtHumidity);
        });

    #endif

}

void dhtLoop() {

    // Check if we should read new data
    static unsigned long last_update = 0;
    if ((millis() - last_update > DHT_UPDATE_INTERVAL) || (last_update == 0)) {
        last_update = millis();

        // Read sensor data
        if (readDHT(DHT_PIN, DHT_TYPE) == DHT_OK) {

            unsigned char tmpUnits = getSetting("tmpUnits", TMP_UNITS).toInt();
            double t = getDHTTemperature(tmpUnits == TMP_CELSIUS);
            unsigned int h = getDHTHumidity();

            char temperature[6];
            char humidity[6];
            dtostrf(t, 1-sizeof(temperature), 1, temperature);
            itoa((unsigned int) h, humidity, 10);

            DEBUG_MSG_P(PSTR("[DHT] Temperature: %s%s\n"), temperature, (tmpUnits == TMP_CELSIUS) ? "ºC" : "ºF");
            DEBUG_MSG_P(PSTR("[DHT] Humidity: %s\n"), humidity);

            // Send MQTT messages
            mqttSend(getSetting("dhtTmpTopic", DHT_TEMPERATURE_TOPIC).c_str(), temperature);
            mqttSend(getSetting("dhtHumTopic", DHT_HUMIDITY_TOPIC).c_str(), humidity);

            // Send to Domoticz
            #if DOMOTICZ_SUPPORT
            {
                domoticzSend("dczTmpIdx", 0, temperature);
                int status;
                if (h > 70) {
                    status = HUMIDITY_WET;
                } else if (h > 45) {
                    status = HUMIDITY_COMFORTABLE;
                } else if (h > 30) {
                    status = HUMIDITY_NORMAL;
                } else {
                    status = HUMIDITY_DRY;
                }
                char buffer[2];
                snprintf_P(buffer, sizeof(buffer), PSTR("%d"), status);
                domoticzSend("dczHumIdx", humidity, buffer);
            }
            #endif

            #if INFLUXDB_SUPPORT
                idbSend(getSetting("dhtTmpTopic", DHT_TEMPERATURE_TOPIC).c_str(), temperature);
                idbSend(getSetting("dhtHumTopic", DHT_HUMIDITY_TOPIC).c_str(), humidity);
            #endif

            // Update websocket clients
            #if WEB_SUPPORT
                char buffer[100];
                snprintf_P(buffer, sizeof(buffer), PSTR("{\"dhtVisible\": 1, \"dhtTmp\": %s, \"dhtHum\": %s, \"tmpUnits\": %d}"), temperature, humidity, tmpUnits);
                wsSend(buffer);
            #endif

        }

    }

}

#endif
