/*

UART_MQTT MODULE

Copyright (C) 2018 by Albert Weterings
Adapted by Xose Pérez <xose dot perez at gmail dot com>

*/

#if UART_MQTT_SUPPORT

#if UART_MQTT_USE_SOFT
    #include <SoftwareSerial.h>
    SoftwareSerial * _uartmqttSerial = NULL;
    #define UART_MQTT_PORT (*_uartmqttSerial)
#else
    #define UART_MQTT_PORT  UART_MQTT_HW_PORT
#endif

#define UART_MQTT_KEY_VISIBLE "uartmqttVisible"
#define UART_MQTT_KEY_PINS_VISIBLE "uartmqttPinsVisible"

#define UART_MQTT_KEY_ENABLED "uartmqttEnabled"
#define UART_MQTT_KEY_BAUDRATE "uartmqttBaudrate"
#define UART_MQTT_KEY_RX_PIN "uartmqttRxGPIO"
#define UART_MQTT_KEY_TX_PIN "uartmqttTxGPIO"
#define UART_MQTT_KEY_HEXLIFY "uartmqttHexlify"
#define UART_MQTT_KEY_TERMINATION "uartmqttTermination"

unsigned char _uartmqtt_buffer[UART_MQTT_BUFFER_SIZE];
size_t _uartmqtt_buffer_len = 0;

bool _uartmqtt_new_data = false;
bool _uartmqtt_active = false;
bool _uartmqtt_reconfigure = false;

bool _uartmqtt_enabled = false;
bool _uartmqtt_hexlify = false;

unsigned long _uartmqtt_baudrate = UART_MQTT_BAUDRATE;
unsigned char _uartmqtt_rx = UART_MQTT_RX_PIN;
unsigned char _uartmqtt_tx = UART_MQTT_TX_PIN;
int _uartmqtt_termination = -1;

// -----------------------------------------------------------------------------
// Utility
// -----------------------------------------------------------------------------

// "AABBCC" -> {0xAA, 0xBB, 0xCC} (out is half the length)
static bool _hexToByte(const char * in, byte * out, size_t bytes) {
    size_t _length = strlen(in);
    if (_length % 2) {
       return false;
    }

    _length /= 2;
    if ((_length > bytes)
        || (bytes > 0 && _length != bytes)) {
        return false;
    }

    char _hex[3] = {0,0,0};
    for (unsigned char b = 0; b < _length; b++) {
        memcpy(_hex, &in[b*2], 2);
        out[b] = strtol(_hex, NULL, 16);
    }

    return true;
}

// {0xAA, 0xBB, 0xCC} -> "AABBCC" (out is twice the length)
static void _byteToHex(byte * in, char * out, size_t bytes) {
    for (unsigned char c = 0; c < bytes; c++) {
        sprintf_P(&out[c*2], PSTR("%02X"), in[c]);
    }

    out[bytes*2+1] = '\0';
}

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------

bool _uartmqttActive() {
     #if UART_MQTT_USE_SOFT
         const bool has_serial = (_uartmqttSerial != NULL);
     #else
         const bool has_serial = (UART_MQTT_HW_PORT);
     #endif

     return _uartmqtt_active && has_serial;
}

void _uartmqttActive(bool active) {
    _uartmqtt_active = active;
}

bool uartmqttEnabled() {
    return _uartmqtt_enabled;
}

bool uartmqttEnabled(bool enabled) {
    _uartmqtt_enabled = enabled;
    setSetting(UART_MQTT_KEY_ENABLED, enabled ? 1 : 0);
    return _uartmqtt_enabled;
}

bool _uartmqttEnabled() {
    return (getSetting(UART_MQTT_KEY_ENABLED, UART_MQTT_ENABLED).toInt() == 1);
}

bool uartmqttHexlify() {
    return _uartmqtt_hexlify;
}

bool uartmqttHexlify(bool hexlify) {
    _uartmqtt_hexlify = hexlify;
    setSetting(UART_MQTT_KEY_HEXLIFY, hexlify ? 1 : 0);
    return _uartmqtt_hexlify;
}

bool _uartmqttHexlify() {
    return (getSetting(UART_MQTT_KEY_HEXLIFY, UART_MQTT_HEXLIFY).toInt() == 1);
}

int uartmqttTermination() {
    return _uartmqtt_termination;
}

int _uartmqttTermination() {
    String setting = getSetting(UART_MQTT_KEY_TERMINATION, (int)UART_MQTT_TERMINATION);
    if (setting.length() == 0) {
        return -1;
    }

    int value = setting.toInt();
    if (value != constrain(value, 0, 255)) {
        return -1;
    }

    return value;
}

unsigned char uartmqttTx() {
    return _uartmqtt_tx;
}

void uartmqttTx(unsigned char tx) {
    _uartmqtt_tx = tx;
    setSetting(UART_MQTT_KEY_TX_PIN, tx);
}

unsigned char _uartmqttTx() {
    return getSetting(UART_MQTT_KEY_TX_PIN, UART_MQTT_TX_PIN).toInt();
}

unsigned char uartmqttRx() {
    return _uartmqtt_rx;
}

void uartmqttRx(unsigned char rx) {
    _uartmqtt_rx = rx;
    setSetting(UART_MQTT_KEY_RX_PIN, rx);
}

unsigned char _uartmqttRx() {
    return getSetting(UART_MQTT_KEY_RX_PIN, UART_MQTT_RX_PIN).toInt();
}

unsigned int uartmqttBaudrate() {
    return _uartmqtt_baudrate;
}

void uartmqttBaudrate(unsigned int baudrate) {
    _uartmqtt_baudrate = baudrate;
    setSetting(UART_MQTT_KEY_BAUDRATE, baudrate);
}

unsigned int _uartmqttBaudrate() {
    return getSetting(UART_MQTT_KEY_BAUDRATE, UART_MQTT_BAUDRATE).toInt();
}

void _uartmqttConfigure() {
    _uartmqtt_enabled = _uartmqttEnabled();
    _uartmqtt_baudrate = _uartmqttBaudrate();
    _uartmqtt_rx = _uartmqttRx();
    _uartmqtt_tx = _uartmqttTx();
    _uartmqtt_hexlify = _uartmqttHexlify();
    _uartmqtt_termination = _uartmqttTermination();
}

void _uartmqttWebConfigure() {
    if ((_uartmqtt_baudrate != _uartmqttBaudrate())
        || (_uartmqtt_rx != _uartmqttRx())
        || (_uartmqtt_tx != _uartmqttTx())) {
        _uartmqtt_reconfigure = true;
    }

    _uartmqttConfigure();
}

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

void _uartmqttReceiveUART() {
    const unsigned char stop_byte = _uartmqtt_termination;
    const bool stop = (_uartmqtt_termination >= 0);

    while (UART_MQTT_PORT.available() && !_uartmqtt_new_data) {
        if (_uartmqtt_buffer_len >= (UART_MQTT_BUFFER_SIZE - 1)) {
            // just wait for next data loop
            if (stop) {
                _uartmqtt_buffer_len = 0;
            }
            break;
        }

        int c = UART_MQTT_PORT.read();
        if (c < 0) {
            break;
        }

        _uartmqtt_buffer[_uartmqtt_buffer_len] = (unsigned char) c;
        _uartmqtt_buffer_len += 1;

        if (stop && (c == stop_byte)) {
           _uartmqtt_new_data = true;
           break;
        }
    }

    if (!stop && !_uartmqtt_new_data && (_uartmqtt_buffer_len > 0)) {
        _uartmqtt_new_data = true;
    }
}

void _uartmqttSendMQTT() {
    if (!_uartmqtt_new_data) {
        return;
    }

    if (_uartmqtt_hexlify) {
        size_t _len = (_uartmqtt_buffer_len * 2) + 1;
        char buffer[_len];
        _byteToHex(_uartmqtt_buffer, buffer, _uartmqtt_buffer_len);
        mqttSend(MQTT_TOPIC_UARTIN, buffer, false, false);
    } else {
        char *sbuffer = (char*)_uartmqtt_buffer;
        sbuffer[_uartmqtt_buffer_len - 1] = '\0';
        mqttSend(MQTT_TOPIC_UARTIN, sbuffer, false, false);
    }

    _uartmqtt_new_data = false;
    _uartmqtt_buffer_len = 0;
}

void _uartmqttSendUART(const char * message) {
    if (!_uartmqttActive()) {
        return;
    }

    if (_uartmqtt_hexlify) {
        size_t _len = strlen(message);
        size_t _byte_len = _len/2;
        byte _raw_message[_byte_len];

        if (!_hexToByte(message, _raw_message, _byte_len)) {
            return;
        }

        for (unsigned char n = 0; n < _byte_len; n++) {
            UART_MQTT_PORT.write(_raw_message[n]);
        }
    } else {
        UART_MQTT_PORT.print(message);
    }
}

void _uartmqttMQTTCallback(unsigned int type, const char * topic, const char * payload) {
    if (!uartmqttEnabled()) {
        return;
    }

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_UARTOUT);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        String t = mqttMagnitude((char *) topic);
        if (t.equals(MQTT_TOPIC_UARTOUT)) {
            _uartmqttSendUART(payload);
        }

    }

}

// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

void _uartmqttLoop() {
    if (!uartmqttEnabled()) {
        if (_uartmqttActive()) {
            DEBUG_MSG_P(PSTR("[UART_MQTT] Stopping serial connection\n"));
            _uartmqttStopSerial();
        }
        return;
    }

    if (_uartmqtt_reconfigure) {
        DEBUG_MSG_P(PSTR("[UART_MQTT] Reconfiguring serial connection\n"));
        _uartmqtt_reconfigure = false;
        _uartmqttStopSerial();
    }

    if (!_uartmqttActive()) {
        DEBUG_MSG_P(PSTR("[UART_MQTT] Starting serial connection\n"));
        _uartmqttStartSerial();
    }

    _uartmqttReceiveUART();
    _uartmqttSendMQTT();
}

void _uartmqttStopSerial() {
    // Stop callbacks
    _uartmqttActive(false);

    #if UART_MQTT_USE_SOFT
        if (_uartmqttSerial != NULL) {
            UART_MQTT_PORT.end();
            delete _uartmqttSerial;
            _uartmqttSerial = NULL;
        }
    #else
        UART_MQTT_PORT.end();
    #endif
}

void _uartmqttStartSerial() {
    #if UART_MQTT_USE_SOFT
        if (_uartmqttSerial == NULL) {
            _uartmqttSerial = new SoftwareSerial(uartmqttRx(), uartmqttTx(), false, UART_MQTT_RX_BUFFER_SIZE);
        }
    #else
        // hardcoded value in cores/esp8266/HardwareSerial.cpp
        if (UART_MQTT_RX_BUFFER_SIZE > 256) {
            UART_MQTT_PORT.setRxBufferSize(UART_MQTT_RX_BUFFER_SIZE);
        }
    #endif

    UART_MQTT_PORT.begin(uartmqttBaudrate());

    // Allow callbacks to work
    _uartmqttActive(true);
}

void uartmqttSetup() {
    _uartmqttConfigure();

    mqttRegister(_uartmqttMQTTCallback);
    espurnaRegisterLoop(_uartmqttLoop);

    #if TERMINAL_SUPPORT
        _uartmqttInitCommands();
    #endif

    #if WEB_SUPPORT
        wsOnSendRegister(_uartmqttWebSocketOnSend);
        wsOnAfterParseRegister(_uartmqttWebConfigure);
        wsOnReceiveRegister(_uartmqttWebSocketOnReceive);
    #endif
}

#if TERMINAL_SUPPORT

void _uartmqttInitCommands() {
    settingsRegisterCommand(F("UARTMQTT.RESET"), [](Embedis* e) {
        _uartmqttConfigure();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("UARTMQTT.START"), [](Embedis* e) {
        _uartmqttConfigure();
        _uartmqtt_enabled = true;
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("UARTMQTT.STOP"), [](Embedis* e) {
        _uartmqtt_enabled = false;
        DEBUG_MSG_P(PSTR("+OK\n"));
    });
}

#endif

#if WEB_SUPPORT

void _uartmqttWebSocketOnSend(JsonObject& root) {
    root[UART_MQTT_KEY_VISIBLE] = 1;
    root[UART_MQTT_KEY_ENABLED] = uartmqttEnabled();
    root[UART_MQTT_KEY_BAUDRATE] = uartmqttBaudrate();
    root[UART_MQTT_KEY_HEXLIFY] = uartmqttHexlify();
    root[UART_MQTT_KEY_TERMINATION] = uartmqttTermination();
    #if UART_MQTT_USE_SOFT
        root[UART_MQTT_KEY_PINS_VISIBLE] = 1;
        root[UART_MQTT_KEY_RX_PIN] = uartmqttRx();
        root[UART_MQTT_KEY_TX_PIN] = uartmqttTx();
    #endif
}

bool _uartmqttWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "uartmqtt", 8) == 0);
}

#endif

#endif // UART_MQTT_SUPPORT
