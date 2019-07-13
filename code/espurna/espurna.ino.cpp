# 1 "/var/folders/1j/x8lsqxdj70535rn3mx9r1fjm0000gp/T/tmp6k6N7G"
#include <Arduino.h>
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/espurna.ino"
# 22 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/espurna.ino"
#include "config/all.h"
#include <vector>

#include "libs/HeapStats.h"

std::vector<void (*)()> _loop_callbacks;
std::vector<void (*)()> _reload_callbacks;

bool _reload_config = false;
unsigned long _loop_delay = 0;





void espurnaRegisterLoop(void (*callback)()) {
    _loop_callbacks.push_back(callback);
}

void espurnaRegisterReload(void (*callback)()) {
    _reload_callbacks.push_back(callback);
}
void espurnaReload();
void _espurnaReload();
unsigned long espurnaLoopDelay();
void setup();
void loop();
bool _alexaWebSocketOnReceive(const char * key, JsonVariant& value);
void _alexaWebSocketOnSend(JsonObject& root);
void _alexaConfigure();
void _alexaBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload);
bool alexaEnabled();
void alexaSetup();
void alexaLoop();
bool _apiWebSocketOnReceive(const char * key, JsonVariant& value);
void _apiWebSocketOnSend(JsonObject& root);
void _apiConfigure();
bool _authAPI(AsyncWebServerRequest *request);
bool _asJson(AsyncWebServerRequest *request);
void _onAPIs(AsyncWebServerRequest *request);
void _onRPC(AsyncWebServerRequest *request);
bool _apiRequestCallback(AsyncWebServerRequest *request);
void apiRegister(const char * key, api_get_callback_f getFn, api_put_callback_f putFn);
void apiSetup();
void brokerPublish(const unsigned char type, const char * topic, unsigned char id, const char * message);
void brokerPublish(const unsigned char type, const char * topic, const char * message);
void buttonMQTT(unsigned char id, uint8_t event);
bool _buttonWebSocketOnReceive(const char * key, JsonVariant& value);
int buttonFromRelay(unsigned int relayID);
bool buttonState(unsigned char id);
unsigned char buttonAction(unsigned char id, unsigned char event);
unsigned long buttonStore(unsigned long pressed, unsigned long click, unsigned long dblclick, unsigned long lngclick, unsigned long lnglngclick, unsigned long tripleclick);
uint8_t mapEvent(uint8_t event, uint8_t count, uint16_t length);
void buttonEvent(unsigned int id, unsigned char event);
void buttonSetup();
void buttonLoop();
void crashClear();
void crashDump();
void crashSetup();
void debugSendImpl(const char * message);
void debugWebSetup();
void debugSetup();
int _domoticzRelay(unsigned int idx);
void _domoticzMqttSubscribe(bool value);
bool _domoticzStatus(unsigned char id);
void _domoticzStatus(unsigned char id, bool status);
void _domoticzLight(unsigned int idx, const JsonObject& root);
void _domoticzMqtt(unsigned int type, const char * topic, const char * payload);
void _domoticzBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload);
bool _domoticzWebSocketOnReceive(const char * key, JsonVariant& value);
void _domoticzWebSocketOnSend(JsonObject& root);
void _domoticzConfigure();
template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue);
template<typename T> void domoticzSend(const char * key, T nvalue);
void domoticzSendRelay(unsigned char relayID, bool status);
void domoticzSendRelays();
unsigned int domoticzIdx(unsigned char relayID);
void domoticzSetup();
bool domoticzEnabled();
void eepromRotate(bool value);
uint32_t eepromCurrent();
String eepromSectors();
void eepromSectorsDebug();
bool _eepromCommit();
void eepromCommit();
void _eepromInitCommands();
void eepromLoop();
void eepromSetup();
void _encoderConfigure();
void _encoderLoop();
void encoderSetup();
bool gpioValid(unsigned char gpio);
bool gpioGetLock(unsigned char gpio);
bool gpioReleaseLock(unsigned char gpio);
String _haFixName(String name);
void _haSendMagnitude(unsigned char i, JsonObject& config);
void _haSendMagnitudes(const JsonObject& deviceConfig);
void _haSendSwitch(unsigned char i, JsonObject& config);
void _haSendSwitches(const JsonObject& deviceConfig);
void _haGetDeviceConfig(JsonObject& config);
void _haSend();
void _haConfigure();
bool _haWebSocketOnReceive(const char * key, JsonVariant& value);
void _haWebSocketOnSend(JsonObject& root);
void _haWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data);
void _haInitCommands();
void _haLoop();
void haSetup();
int _i2cClearbus(int sda, int scl);
void i2c_wakeup(uint8_t address);
uint8_t i2c_write_uint8(uint8_t address, uint8_t value);
uint8_t i2c_write_buffer(uint8_t address, uint8_t * buffer, size_t len);
uint8_t i2c_read_uint8(uint8_t address);
uint8_t i2c_read_uint8(uint8_t address, uint8_t reg);
uint16_t i2c_read_uint16(uint8_t address);
uint16_t i2c_read_uint16(uint8_t address, uint8_t reg);
void i2c_read_buffer(uint8_t address, uint8_t * buffer, size_t len);
void i2c_wakeup(uint8_t address);
uint8_t i2c_write_uint8(uint8_t address, uint8_t value);
uint8_t i2c_write_buffer(uint8_t address, uint8_t * buffer, size_t len);
uint8_t i2c_read_uint8(uint8_t address);
uint8_t i2c_read_uint8(uint8_t address, uint8_t reg);
uint16_t i2c_read_uint16(uint8_t address);
uint16_t i2c_read_uint16(uint8_t address, uint8_t reg);
void i2c_read_buffer(uint8_t address, uint8_t * buffer, size_t len);
uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value);
uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value1, uint8_t value2);
uint8_t i2c_write_uint16(uint8_t address, uint8_t reg, uint16_t value);
uint8_t i2c_write_uint16(uint8_t address, uint16_t value);
uint16_t i2c_read_uint16_le(uint8_t address, uint8_t reg);
int16_t i2c_read_int16(uint8_t address, uint8_t reg);
int16_t i2c_read_int16_le(uint8_t address, uint8_t reg);
void i2cClearBus();
bool i2cCheck(unsigned char address);
bool i2cGetLock(unsigned char address);
bool i2cReleaseLock(unsigned char address);
unsigned char i2cFind(size_t size, unsigned char * addresses, unsigned char &start);
unsigned char i2cFind(size_t size, unsigned char * addresses);
unsigned char i2cFindAndLock(size_t size, unsigned char * addresses);
void i2cScan();
void i2cCommands();
void i2cSetup();
bool _idbWebSocketOnReceive(const char * key, JsonVariant& value);
void _idbWebSocketOnSend(JsonObject& root);
void _idbConfigure();
void _idbBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload);
bool idbSend(const char * topic, const char * payload);
bool idbSend(const char * topic, unsigned char id, const char * payload);
bool idbEnabled();
void idbSetup();
void _irMqttCallback(unsigned int type, const char * topic, const char * payload);
void _irTXLoop();
void _irProcess(unsigned char type, unsigned long code);
void _irRXLoop();
void _irLoop();
void irSetup();
bool _ledStatus(unsigned char id);
bool _ledStatus(unsigned char id, bool status);
bool _ledToggle(unsigned char id);
unsigned char _ledMode(unsigned char id);
void _ledMode(unsigned char id, unsigned char mode);
unsigned char _ledRelay(unsigned char id);
void _ledRelay(unsigned char id, unsigned char relay);
void _ledBlink(unsigned char id, unsigned long delayOff, unsigned long delayOn);
bool _ledWebSocketOnReceive(const char * key, JsonVariant& value);
void _ledWebSocketOnSend(JsonObject& root);
void _ledBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload);
void _ledMQTTCallback(unsigned int type, const char * topic, const char * payload);
unsigned char _ledCount();
void _ledConfigure();
void ledUpdate(bool value);
void ledSetup();
void ledLoop();
void _setRGBInputValue(unsigned char red, unsigned char green, unsigned char blue);
void _setCCTInputValue(unsigned char warm, unsigned char cold);
void _generateBrightness();
void _fromLong(unsigned long value, bool brightness);
void _fromRGB(const char * rgb);
void _fromHSV(const char * hsv);
void _fromKelvin(unsigned long kelvin);
void _fromMireds(unsigned long mireds);
void _toRGB(char * rgb, size_t len, bool target);
void _toRGB(char * rgb, size_t len);
void _toHSV(char * hsv, size_t len, bool target);
void _toHSV(char * hsv, size_t len);
void _toLong(char * color, size_t len, bool target);
void _toLong(char * color, size_t len);
void _toCSV(char * buffer, size_t len, bool applyBrightness, bool target);
void _toCSV(char * buffer, size_t len, bool applyBrightness);
unsigned int _toPWM(unsigned long value, bool gamma, bool reverse);
unsigned int _toPWM(unsigned char id);
void _transition();
void _lightProviderUpdate();
void _lightSaveRtcmem();
void _lightRestoreRtcmem();
void _lightSaveSettings();
void _lightRestoreSettings();
void _lightMQTTCallback(unsigned int type, const char * topic, const char * payload);
void lightMQTT();
void lightMQTTGroup();
void lightBroker();
unsigned char lightChannels();
bool lightHasColor();
bool lightUseCCT();
void _lightComms(unsigned char mask);
void lightUpdate(bool save, bool forward, bool group_forward);
void lightUpdate(bool save, bool forward);
void lightSave();
void lightState(unsigned char i, bool state);
bool lightState(unsigned char i);
void lightState(bool state);
bool lightState();
void lightColor(const char * color, bool rgb);
void lightColor(const char * color);
void lightColor(unsigned long color);
String lightColor(bool rgb);
String lightColor();
unsigned int lightChannel(unsigned char id);
void lightChannel(unsigned char id, int value);
void lightChannelStep(unsigned char id, int steps);
unsigned int lightBrightness();
void lightBrightness(int b);
void lightBrightnessStep(int steps);
unsigned long lightTransitionTime();
void lightTransitionTime(unsigned long m);
bool _lightWebSocketOnReceive(const char * key, JsonVariant& value);
void _lightWebSocketStatus(JsonObject& root);
void _lightWebSocketOnSend(JsonObject& root);
void _lightWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data);
void _lightAPISetup();
void _lightInitCommands();
unsigned long getIOMux(unsigned long gpio);
unsigned long getIOFunc(unsigned long gpio);
void _lightConfigure();
void lightSetup();
void lightfoxLearn();
void lightfoxClear();
void _lightfoxWebSocketOnSend(JsonObject& root);
void _lightfoxWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data);
void _lightfoxInitCommands();
void lightfoxSetup();
void llmnrSetup();
void _mdnsFindMQTT();
void _mdnsServerStart();
void mdnsServerSetup();
String mdnsResolve(char * name);
String mdnsResolve(String name);
void mdnsClientSetup();
void mdnsClientLoop();
void migrate();
void _mqttConnect();
void _mqttPlaceholders(String& text);
template<typename T>
void _mqttApplySetting(T& current, T& updated);
template<typename T>
void _mqttApplySetting(T& current, const T& updated);
template<typename T>
void _mqttApplyTopic(T& current, const char* magnitude);
void _mqttConfigure();
void _mqttBackwards();
void _mqttInfo();
bool _mqttWebSocketOnReceive(const char * key, JsonVariant& value);
void _mqttWebSocketOnSend(JsonObject& root);
void _mqttInitCommands();
void _mqttCallback(unsigned int type, const char * topic, const char * payload);
void _mqttOnConnect();
void _mqttOnDisconnect();
void _mqttOnMessage(char* topic, char* payload, unsigned int len);
String mqttMagnitude(char * topic);
String mqttTopic(const char * magnitude, bool is_set);
String mqttTopic(const char * magnitude, unsigned int index, bool is_set);
void mqttSendRaw(const char * topic, const char * message, bool retain);
void mqttSendRaw(const char * topic, const char * message);
void mqttSend(const char * topic, const char * message, bool force, bool retain);
void mqttSend(const char * topic, const char * message, bool force);
void mqttSend(const char * topic, const char * message);
void mqttSend(const char * topic, unsigned int index, const char * message, bool force, bool retain);
void mqttSend(const char * topic, unsigned int index, const char * message, bool force);
void mqttSend(const char * topic, unsigned int index, const char * message);
unsigned char _mqttBuildTree(JsonObject& root, char parent);
void mqttFlush();
int8_t mqttEnqueue(const char * topic, const char * message, unsigned char parent);
int8_t mqttEnqueue(const char * topic, const char * message);
void mqttSubscribeRaw(const char * topic);
void mqttSubscribe(const char * topic);
void mqttUnsubscribeRaw(const char * topic);
void mqttUnsubscribe(const char * topic);
void mqttEnabled(bool status);
bool mqttEnabled();
bool mqttConnected();
void mqttDisconnect();
bool mqttForward();
void mqttRegister(mqtt_callback_f callback);
void mqttSetBroker(IPAddress ip, unsigned int port);
void mqttSetBrokerIfNone(IPAddress ip, unsigned int port);
void mqttSetup();
void mqttLoop();
bool mqttForward();
void netbiosSetup();
bool _nofussWebSocketOnReceive(const char * key, JsonVariant& value);
void _nofussWebSocketOnSend(JsonObject& root);
void _nofussConfigure();
void _nofussInitCommands();
void nofussRun();
void nofussSetup();
void nofussLoop();
bool _ntpWebSocketOnReceive(const char * key, JsonVariant& value);
void _ntpWebSocketOnSend(JsonObject& root);
time_t _ntpSyncProvider();
void _ntpWantSync();
int inline _ntpSyncInterval();
int inline _ntpUpdateInterval();
void _ntpStart();
void _ntpConfigure();
void _ntpReport();
void inline _ntpBroker();
void _ntpLoop();
void _ntpBackwards();
bool ntpSynced();
String ntpDateTime(time_t t);
String ntpDateTime();
time_t ntpLocal2UTC(time_t local);
void ntpSetup();
void _otaConfigure();
void _otaLoop();
void _otaFrom(const char * host, unsigned int port, const char * url);
void _otaFrom(String url);
void _otaInitCommands();
void _otaMQTTCallback(unsigned int type, const char * topic, const char * payload);
void otaSetup();
void _relayProviderStatus(unsigned char id, bool status);
void _relayProcess(bool mode);
unsigned char getSpeed();
void setSpeed(unsigned char speed);
void _relayMaskRtcmem(uint32_t mask);
uint32_t _relayMaskRtcmem();
void relayPulse(unsigned char id);
bool relayStatus(unsigned char id, bool status, bool report, bool group_report);
bool relayStatus(unsigned char id, bool status);
bool relayStatus(unsigned char id);
void relaySync(unsigned char id);
void relaySave(bool eeprom);
void relaySave();
void relayToggle(unsigned char id, bool report, bool group_report);
void relayToggle(unsigned char id);
unsigned char relayCount();
unsigned char relayParsePayload(const char * payload);
void _relayBackwards();
void _relayBoot();
void _relayConfigure();
bool _relayWebSocketOnReceive(const char * key, JsonVariant& value);
void _relayWebSocketUpdate(JsonObject& root);
String _relayFriendlyName(unsigned char i);
void _relayWebSocketSendRelays();
void _relayWebSocketOnStart(JsonObject& root);
void _relayWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data);
void relaySetupWS();
void relaySetupAPI();
void _relayMQTTGroup(unsigned char id);
void relayMQTT(unsigned char id);
void relayMQTT();
void relayStatusWrap(unsigned char id, unsigned char value, bool is_group_topic);
void relayMQTTCallback(unsigned int type, const char * topic, const char * payload);
void relaySetupMQTT();
void _relayInitCommands();
void _relayLoop();
void relaySetup();
void _rfbWebSocketSendCodeArray(unsigned char start, unsigned char size);
void _rfbWebSocketSendCode(unsigned char id);
void _rfbWebSocketSendCodes();
void _rfbWebSocketOnSend(JsonObject& root);
void _rfbWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data);
bool _rfbWebSocketOnReceive(const char * key, JsonVariant& value);
void _rfbSend();
void _rfbSend(byte * code, unsigned char times);
void _rfbSendRawOnce(byte *code, unsigned char length);
void _rfbDecode();
bool _rfbCompare(const char * code1, const char * code2);
bool _rfbSameOnOff(unsigned char id);
void _rfbParseRaw(char * raw);
void _rfbParseCode(char * code);
void _rfbAck();
void _rfbLearnImpl();
void _rfbSend(byte * message);
void _rfbReceive();
void _rfbAck();
void _rfbLearnImpl();
void _rfbSend(byte * message);
void _rfbReceive();
void _rfbLearn();
void _rfbMqttCallback(unsigned int type, const char * topic, const char * payload);
void _rfbAPISetup();
void _rfbInitCommands();
void rfbStore(unsigned char id, bool status, const char * code);
String rfbRetrieve(unsigned char id, bool status);
void rfbStatus(unsigned char id, bool status);
void rfbLearn(unsigned char id, bool status);
void rfbForget(unsigned char id, bool status);
void rfbSetup();
void rfbLoop();
void _rfm69WebSocketOnSend(JsonObject& root);
bool _rfm69WebSocketOnReceive(const char * key, JsonVariant& value);
void _rfm69WebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data);
void _rfm69CleanNodes(unsigned char num);
void _rfm69Configure();
void _rfm69Debug(const char * level, packet_t * data);
void _rfm69Process(packet_t * data);
void _rfm69Loop();
void _rfm69Clear();
void rfm69Setup();
void _rtcmemErase();
void _rtcmemInit();
bool _rtcmemStatus();
void _rtcmemInitCommands();
bool rtcmemStatus();
void rtcmemSetup();
bool _schWebSocketOnReceive(const char * key, JsonVariant& value);
void _schWebSocketOnSend(JsonObject &root);
void _schConfigure();
bool _schIsThisWeekday(time_t t, String weekdays);
int _schMinutesLeft(time_t t, unsigned char schedule_hour, unsigned char schedule_minute);
void _schCheck();
void _schLoop();
void schSetup();
unsigned char _magnitudeDecimals(unsigned char type);
double _magnitudeProcess(unsigned char type, unsigned char decimals, double value);
template<typename T> void _sensorWebSocketMagnitudes(JsonObject& root, T prefix);
bool _sensorWebSocketOnReceive(const char * key, JsonVariant& value);
void _sensorWebSocketSendData(JsonObject& root);
void _sensorWebSocketStart(JsonObject& root);
void _sensorAPISetup();
void _sensorInitCommands();
void _sensorTick();
void _sensorPre();
void _sensorPost();
void _sensorResetTS();
double _sensorEnergyTotal();
void _sensorEnergyTotal(double value);
void _sensorLoad();
void _sensorCallback(unsigned char i, unsigned char type, double value);
void _sensorInit();
void _sensorConfigure();
void _sensorReport(unsigned char index, double value);
unsigned char sensorCount();
unsigned char magnitudeCount();
String magnitudeName(unsigned char index);
unsigned char magnitudeType(unsigned char index);
double magnitudeValue(unsigned char index);
unsigned char magnitudeIndex(unsigned char index);
String magnitudeTopic(unsigned char type);
String magnitudeTopicIndex(unsigned char index);
String magnitudeUnits(unsigned char type);
void sensorSetup();
void sensorLoop();
unsigned long settingsSize();
unsigned int settingsKeyCount();
String settingsKeyName(unsigned int index);
void moveSetting(const char * from, const char * to);
void moveSetting(const char * from, const char * to, unsigned int index);
void moveSettings(const char * from, const char * to);
template<typename T> String getSetting(const String& key, T defaultValue);
template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue);
String getSetting(const String& key);
template<typename T> bool setSetting(const String& key, T value);
template<typename T> bool setSetting(const String& key, unsigned int index, T value);
bool delSetting(const String& key);
bool delSetting(const String& key, unsigned int index);
bool hasSetting(const String& key);
bool hasSetting(const String& key, unsigned int index);
void saveSettings();
void resetSettings();
size_t settingsMaxSize();
bool settingsRestoreJson(JsonObject& data);
void settingsGetJson(JsonObject& root);
void settingsSetup();
void ssdpSetup();
uint8_t systemStabilityCounter();
void systemStabilityCounter(uint8_t counter);
uint8_t _systemResetReason();
void _systemResetReason(uint8_t reason);
void systemCheck(bool stable);
bool systemCheck();
void systemCheckLoop();
uint32_t systemResetReason();
void customResetReason(unsigned char reason);
unsigned char customResetReason();
void reset();
void deferredReset(unsigned long delay, unsigned char reason);
bool checkNeedsReset();
void systemSendHeartbeat();
bool systemGetHeartbeat();
unsigned long systemLoadAverage();
void _systemSetupHeartbeat();
void systemLoop();
void _systemSetupSpecificHardware();
void systemSetup();
bool _telnetWebSocketOnReceive(const char * key, JsonVariant& value);
void _telnetWebSocketOnSend(JsonObject& root);
void _telnetDisconnect(unsigned char clientId);
bool _telnetWrite(unsigned char clientId, const char *data, size_t len);
unsigned char _telnetWrite(const char *data, size_t len);
unsigned char _telnetWrite(const char *data);
bool _telnetWrite(unsigned char clientId, const char * message);
void _telnetData(unsigned char clientId, void *data, size_t len);
void _telnetNotifyConnected(unsigned char i);
void _telnetLoop();
void _telnetNewClient(AsyncClient* client);
bool telnetConnected();
unsigned char telnetWrite(unsigned char ch);
void _telnetConfigure();
void telnetSetup();
void _terminalHelpCommand();
void _terminalKeysCommand();
void _terminalInitCommand();
void _terminalLoop();
void terminalInject(void *data, size_t len);
Stream & terminalSerial();
void terminalOK();
void terminalError(const String& error);
void terminalSetup();
void thermostatEnabled(bool enabled);
bool thermostatEnabled();
void thermostatModeCooler(bool cooler);
bool thermostatModeCooler();
void thermostatRegister(thermostat_callback_f callback);
void updateOperationMode();
void updateRemoteTemp(bool remote_temp_actual);
void thermostatMQTTCallback(unsigned int type, const char * topic, const char * payload);
void thermostatSetupMQTT();
void notifyRangeChanged(bool min);
void commonSetup();
void thermostatConfigure();
void _thermostatReload();
void _thermostatWebSocketOnSend(JsonObject& root);
bool _thermostatWebSocketOnReceive(const char * key, JsonVariant& value);
void _thermostatWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data);
void thermostatSetup();
void sendTempRangeRequest();
void setThermostatState(bool state);
void debugPrintSwitch(bool state, double temp);
inline bool lastSwitchEarlierThan(unsigned int comparing_time);
inline void switchThermostat(bool state, double temp);
void checkTempAndAdjustRelay(double temp);
void updateCounters();
double getLocalTemperature();
double getLocalHumidity();
void thermostatLoop(void);
String getBurnTimeStr(unsigned int burn_time);
void resetBurnCounters();
void display_wifi_status(bool on);
void display_mqtt_status(bool on);
void display_server_status(bool on);
void display_remote_temp_status(bool on);
void display_temp_range();
void display_remote_temp();
void display_local_temp();
void display_local_humidity();
void displaySetup();
void displayLoop();
void _tspkBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload);
bool _tspkWebSocketOnReceive(const char * key, JsonVariant& value);
void _tspkWebSocketOnSend(JsonObject& root);
void _tspkConfigure();
void _tspkInitClient();
void _tspkPost();
void _tspkPost();
void _tspkEnqueue(unsigned char index, char * payload);
void _tspkClearQueue();
void _tspkFlush();
bool tspkEnqueueRelay(unsigned char index, char * payload);
bool tspkEnqueueMeasurement(unsigned char index, char * payload);
void tspkFlush();
bool tspkEnabled();
void tspkSetup();
void tspkLoop();
void _uartmqttReceiveUART();
void _uartmqttSendMQTT();
void _uartmqttSendUART(const char * message);
void _uartmqttMQTTCallback(unsigned int type, const char * topic, const char * payload);
void _uartmqttLoop();
void uartmqttSetup();
String getIdentifier();
void setDefaultHostname();
void setBoardName();
String getBoardName();
String getAdminPass();
String getCoreVersion();
String getCoreRevision();
unsigned char getHeartbeatMode();
unsigned char getHeartbeatInterval();
String getEspurnaModules();
String getEspurnaSensors();
String getEspurnaWebUI();
String buildTime();
unsigned long getUptime();
void heartbeat();
unsigned int info_bytes2sectors(size_t size);
unsigned long info_ota_space();
unsigned long info_filesystem_space();
unsigned long info_eeprom_space();
void _info_print_memory_layout_line(const char * name, unsigned long bytes, bool reset);
void _info_print_memory_layout_line(const char * name, unsigned long bytes);
void infoMemory(const char * name, unsigned int total_memory, unsigned int free_memory);
const char* _info_wifi_sleep_mode(WiFiSleepType_t type);
void info();
bool sslCheckFingerPrint(const char * fingerprint);
bool sslFingerPrintArray(const char * fingerprint, unsigned char * bytearray);
bool sslFingerPrintChar(const char * fingerprint, char * destination);
bool eraseSDKConfig();
double roundTo(double num, unsigned char positions);
void nice_delay(unsigned long ms);
int __get_adc_mode();
bool isNumber(const char * s);
char* strnstr(const char* buffer, const char* token, size_t n);
void _onReset(AsyncWebServerRequest *request);
void _onDiscover(AsyncWebServerRequest *request);
void _onGetConfig(AsyncWebServerRequest *request);
void _onPostConfig(AsyncWebServerRequest *request);
void _onPostConfigData(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void _onHome(AsyncWebServerRequest *request);
int _onCertificate(void * arg, const char *filename, uint8_t **buf);
void _onUpgrade(AsyncWebServerRequest *request);
void _onUpgradeFile(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
bool _onAPModeRequest(AsyncWebServerRequest *request);
void _onRequest(AsyncWebServerRequest *request);
void _onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
bool webAuthenticate(AsyncWebServerRequest *request);
void webBodyRegister(web_body_callback_f callback);
void webRequestRegister(web_request_callback_f callback);
unsigned int webPort();
void webLog(AsyncWebServerRequest *request);
void webSetup();
void _wifiCheckAP();
void _wifiConfigure();
bool _wifiClean(unsigned char num);
void _wifiInject();
void _wifiCallback(justwifi_messages_t code, char * parameter);
void _wifiCaptivePortal(justwifi_messages_t code, char * parameter);
void _wifiDebugCallback(justwifi_messages_t code, char * parameter);
void _wifiInitCommands();
bool _wifiWebSocketOnReceive(const char * key, JsonVariant& value);
void _wifiWebSocketOnSend(JsonObject& root);
void _wifiWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data);
void wifiDebug(WiFiMode_t modes);
void wifiDebug();
String getIP();
String getNetwork();
bool wifiConnected();
void wifiDisconnect();
void wifiStartAP(bool only);
void wifiStartAP();
void wifiStartWPS();
void wifiStartSmartConfig();
void wifiReconnectCheck();
uint8_t wifiState();
void wifiRegister(wifi_callback_f callback);
void wifiSetup();
void wifiLoop();
void _onAuth(AsyncWebServerRequest *request);
bool _wsAuth(AsyncWebSocketClient * client);
bool wsDebugSend(const char* prefix, const char* message);
void _wsMQTTCallback(unsigned int type, const char * topic, const char * payload);
bool _wsStore(String key, String value);
bool _wsStore(String key, JsonArray& value);
void _wsParse(AsyncWebSocketClient *client, uint8_t * payload, size_t length);
void _wsUpdate(JsonObject& root);
bool _wsOnReceive(const char * key, JsonVariant& value);
void _wsOnStart(JsonObject& root);
void wsSend(JsonObject& root);
void wsSend(uint32_t client_id, JsonObject& root);
void _wsStart(uint32_t client_id);
void _wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void _wsLoop();
bool wsConnected();
bool wsConnected(uint32_t client_id);
void wsOnSendRegister(ws_on_send_callback_f callback);
void wsOnReceiveRegister(ws_on_receive_callback_f callback);
void wsOnActionRegister(ws_on_action_callback_f callback);
void wsSend(ws_on_send_callback_f callback);
void wsSend(const char * payload);
void wsSend_P(PGM_P payload);
void wsSend(uint32_t client_id, ws_on_send_callback_f callback);
void wsSend(uint32_t client_id, const char * payload);
void wsSend_P(uint32_t client_id, PGM_P payload);
void wsSetup();
#line 45 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/espurna.ino"
void espurnaReload() {
    _reload_config = true;
}

void _espurnaReload() {
    for (unsigned char i = 0; i < _reload_callbacks.size(); i++) {
        (_reload_callbacks[i])();
    }
}

unsigned long espurnaLoopDelay() {
    return _loop_delay;
}





void setup() {






    setInitialFreeHeap();


    #if DEBUG_SUPPORT
        debugSetup();
    #endif


    rtcmemSetup();


    eepromSetup();


    settingsSetup();


    #if DEBUG_SUPPORT
        crashSetup();
    #endif



    wtfHeap(getSetting("wtfHeap", 0).toInt());


    systemSetup();


    #if TERMINAL_SUPPORT
        terminalSetup();
    #endif


    if (getSetting("hostname").length() == 0) {
        setDefaultHostname();
    }
    setBoardName();


    info();

    wifiSetup();
    otaSetup();
    #if TELNET_SUPPORT
        telnetSetup();
    #endif





    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) return;
    #endif






    #if WEB_SUPPORT
        webSetup();
        wsSetup();
        #if DEBUG_WEB_SUPPORT
            debugWebSetup();
        #endif
    #endif
    #if API_SUPPORT
        apiSetup();
    #endif


    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightSetup();
    #endif
    relaySetup();
    #if BUTTON_SUPPORT
        buttonSetup();
    #endif
    #if ENCODER_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
        encoderSetup();
    #endif
    #if LED_SUPPORT
        ledSetup();
    #endif

    #if MQTT_SUPPORT
        mqttSetup();
    #endif
    #if MDNS_SERVER_SUPPORT
        mdnsServerSetup();
    #endif
    #if MDNS_CLIENT_SUPPORT
        mdnsClientSetup();
    #endif
    #if LLMNR_SUPPORT
        llmnrSetup();
    #endif
    #if NETBIOS_SUPPORT
        netbiosSetup();
    #endif
    #if SSDP_SUPPORT
        ssdpSetup();
    #endif
    #if NTP_SUPPORT
        ntpSetup();
    #endif
    #if I2C_SUPPORT
        i2cSetup();
    #endif
    #if RF_SUPPORT
        rfbSetup();
    #endif
    #if ALEXA_SUPPORT
        alexaSetup();
    #endif
    #if NOFUSS_SUPPORT
        nofussSetup();
    #endif
    #if INFLUXDB_SUPPORT
        idbSetup();
    #endif
    #if THINGSPEAK_SUPPORT
        tspkSetup();
    #endif
    #if RFM69_SUPPORT
        rfm69Setup();
    #endif
    #if IR_SUPPORT
        irSetup();
    #endif
    #if DOMOTICZ_SUPPORT
        domoticzSetup();
    #endif
    #if HOMEASSISTANT_SUPPORT
        haSetup();
    #endif
    #if SENSOR_SUPPORT
        sensorSetup();
    #endif
    #if SCHEDULER_SUPPORT
        schSetup();
    #endif
    #if UART_MQTT_SUPPORT
        uartmqttSetup();
    #endif
    #ifdef FOXEL_LIGHTFOX_DUAL
        lightfoxSetup();
    #endif
    #if THERMOSTAT_SUPPORT
        thermostatSetup();
    #endif
    #if THERMOSTAT_DISPLAY_SUPPORT
        displaySetup();
    #endif



    #if USE_EXTRA
        extraSetup();
    #endif


    migrate();



    _loop_delay = atol(getSetting("loopDelay", LOOP_DELAY_TIME).c_str());
    _loop_delay = constrain(_loop_delay, 0, 300);

    saveSettings();

}

void loop() {


    if (_reload_config) {
        _espurnaReload();
        _reload_config = false;
    }


    for (unsigned char i = 0; i < _loop_callbacks.size(); i++) {
        (_loop_callbacks[i])();
    }


    if (_loop_delay) delay(_loop_delay);

}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/alexa.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/alexa.ino"
#if ALEXA_SUPPORT

#include <fauxmoESP.h>
fauxmoESP alexa;

#include <queue>
typedef struct {
    unsigned char device_id;
    bool state;
    unsigned char value;
} alexa_queue_element_t;
static std::queue<alexa_queue_element_t> _alexa_queue;





bool _alexaWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "alexa", 5) == 0);
}

void _alexaWebSocketOnSend(JsonObject& root) {
    root["alexaVisible"] = 1;
    root["alexaEnabled"] = alexaEnabled();
    root["alexaName"] = getSetting("alexaName");
}

void _alexaConfigure() {
    alexa.enable(wifiConnected() && alexaEnabled());
}

#if WEB_SUPPORT
    bool _alexaBodyCallback(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        return alexa.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data));
    }

    bool _alexaRequestCallback(AsyncWebServerRequest *request) {
        String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
        return alexa.process(request->client(), request->method() == HTTP_GET, request->url(), body);
    }
#endif

#if BROKER_SUPPORT
void _alexaBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload) {


    if (BROKER_MSG_TYPE_STATUS != type) return;

    unsigned char value = atoi(payload);

    if (strcmp(MQTT_TOPIC_CHANNEL, topic) == 0) {
        alexa.setState(id+1, value > 0, value);
    }

    if (strcmp(MQTT_TOPIC_RELAY, topic) == 0) {
        #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT
            if (id > 0) return;
        #endif
        alexa.setState(id, value, value > 0 ? 255 : 0);
    }

}
#endif



bool alexaEnabled() {
    return (getSetting("alexaEnabled", ALEXA_ENABLED).toInt() == 1);
}

void alexaSetup() {


    moveSetting("fauxmoEnabled", "alexaEnabled");


    alexa.createServer(!WEB_SUPPORT);
    alexa.setPort(80);


    String hostname = getSetting("alexaName", ALEXA_HOSTNAME);
    if (hostname.length() == 0) {
        hostname = getSetting("hostname");
    }


    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT


        alexa.addDevice(hostname.c_str());


        for (unsigned char i = 1; i <= lightChannels(); i++) {
            alexa.addDevice((hostname + " " + i).c_str());
        }


    #else

        unsigned int relays = relayCount();
        if (relays == 1) {
            alexa.addDevice(hostname.c_str());
        } else {
            for (unsigned int i=1; i<=relays; i++) {
                alexa.addDevice((hostname + " " + i).c_str());
            }
        }

    #endif


    _alexaConfigure();


    #if WEB_SUPPORT
        webBodyRegister(_alexaBodyCallback);
        webRequestRegister(_alexaRequestCallback);
        wsOnSendRegister(_alexaWebSocketOnSend);
        wsOnReceiveRegister(_alexaWebSocketOnReceive);
    #endif


    wifiRegister([](justwifi_messages_t code, char * parameter) {
        if ((MESSAGE_CONNECTED == code) || (MESSAGE_DISCONNECTED == code)) {
            _alexaConfigure();
        }
    });


    alexa.onSetState([&](unsigned char device_id, const char * name, bool state, unsigned char value) {
        alexa_queue_element_t element;
        element.device_id = device_id;
        element.state = state;
        element.value = value;
        _alexa_queue.push(element);
    });


    #if BROKER_SUPPORT
        brokerRegister(_alexaBrokerCallback);
    #endif
    espurnaRegisterReload(_alexaConfigure);
    espurnaRegisterLoop(alexaLoop);

}

void alexaLoop() {

    alexa.handle();

    while (!_alexa_queue.empty()) {

        alexa_queue_element_t element = _alexa_queue.front();
        DEBUG_MSG_P(PSTR("[ALEXA] Device #%u state: %s value: %d\n"), element.device_id, element.state ? "ON" : "OFF", element.value);

        #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT
            if (0 == element.device_id) {
                relayStatus(0, element.state);
            } else {
                lightState(element.device_id - 1, element.state);
                lightChannel(element.device_id - 1, element.value);
                lightUpdate(true, true);
            }
        #else
            relayStatus(element.device_id, element.state);
        #endif

        _alexa_queue.pop();
    }

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/api.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/api.ino"
#if API_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <vector>

typedef struct {
    char * key;
    api_get_callback_f getFn = NULL;
    api_put_callback_f putFn = NULL;
} web_api_t;
std::vector<web_api_t> _apis;



bool _apiWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "api", 3) == 0);
}

void _apiWebSocketOnSend(JsonObject& root) {
    root["apiVisible"] = 1;
    root["apiEnabled"] = getSetting("apiEnabled", API_ENABLED).toInt() == 1;
    root["apiKey"] = getSetting("apiKey");
    root["apiRealTime"] = getSetting("apiRealTime", API_REAL_TIME_VALUES).toInt() == 1;
    root["apiRestFul"] = getSetting("apiRestFul", API_RESTFUL).toInt() == 1;
}

void _apiConfigure() {

}





bool _authAPI(AsyncWebServerRequest *request) {

    if (getSetting("apiEnabled", API_ENABLED).toInt() == 0) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] HTTP API is not enabled\n"));
        request->send(403);
        return false;
    }

    if (!request->hasParam("apikey", (request->method() == HTTP_PUT))) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] Missing apikey parameter\n"));
        request->send(403);
        return false;
    }

    AsyncWebParameter* p = request->getParam("apikey", (request->method() == HTTP_PUT));
    if (!p->value().equals(getSetting("apiKey"))) {
        DEBUG_MSG_P(PSTR("[WEBSERVER] Wrong apikey parameter\n"));
        request->send(403);
        return false;
    }

    return true;

}

bool _asJson(AsyncWebServerRequest *request) {
    bool asJson = false;
    if (request->hasHeader("Accept")) {
        AsyncWebHeader* h = request->getHeader("Accept");
        asJson = h->value().equals("application/json");
    }
    return asJson;
}

void _onAPIs(AsyncWebServerRequest *request) {

    webLog(request);
    if (!_authAPI(request)) return;

    bool asJson = _asJson(request);

    char buffer[40];

    String output;
    if (asJson) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        for (unsigned int i=0; i < _apis.size(); i++) {
            snprintf_P(buffer, sizeof(buffer), PSTR("/api/%s"), _apis[i].key);
            root[_apis[i].key] = String(buffer);
        }
        root.printTo(output);
        jsonBuffer.clear();
        request->send(200, "application/json", output);

    } else {
        for (unsigned int i=0; i < _apis.size(); i++) {
            snprintf_P(buffer, sizeof(buffer), PSTR("/api/%s"), _apis[i].key);
            output += _apis[i].key + String(" -> ") + String(buffer) + String("\n");
        }
        request->send(200, "text/plain", output);
    }

}

void _onRPC(AsyncWebServerRequest *request) {

    webLog(request);
    if (!_authAPI(request)) return;


    int response = 404;

    if (request->hasParam("action")) {

        AsyncWebParameter* p = request->getParam("action");
        String action = p->value();
        DEBUG_MSG_P(PSTR("[RPC] Action: %s\n"), action.c_str());

        if (action.equals("reboot")) {
            response = 200;
            deferredReset(100, CUSTOM_RESET_RPC);
        }

    }

    request->send(response);

}

bool _apiRequestCallback(AsyncWebServerRequest *request) {

    String url = request->url();


    if (url.equals("/api") || url.equals("/apis")) {
        _onAPIs(request);
        return true;
    }


    if (url.equals("/rpc")) {
        _onRPC(request);
        return true;
    }


    if (!url.startsWith("/api/")) return false;

    for (unsigned char i=0; i < _apis.size(); i++) {


        web_api_t api = _apis[i];
        if (!url.endsWith(api.key)) continue;


        webLog(request);
        if (!_authAPI(request)) return false;


        if (api.putFn != NULL) {
            if ((getSetting("apiRestFul", API_RESTFUL).toInt() != 1) || (request->method() == HTTP_PUT)) {
                if (request->hasParam("value", request->method() == HTTP_PUT)) {
                    AsyncWebParameter* p = request->getParam("value", request->method() == HTTP_PUT);
                    (api.putFn)((p->value()).c_str());
                }
            }
        }


        char value[API_BUFFER_SIZE] = {0};
        (api.getFn)(value, API_BUFFER_SIZE);


        if (0 == value[0]) {
            DEBUG_MSG_P(PSTR("[API] Sending 404 response\n"));
            request->send(404);
            return false;
        }

        DEBUG_MSG_P(PSTR("[API] Sending response '%s'\n"), value);


        if (_asJson(request)) {
            char buffer[64];
            if (isNumber(value)) {
                snprintf_P(buffer, sizeof(buffer), PSTR("{ \"%s\": %s }"), api.key, value);
            } else {
                snprintf_P(buffer, sizeof(buffer), PSTR("{ \"%s\": \"%s\" }"), api.key, value);
            }
            request->send(200, "application/json", buffer);
        } else {
            request->send(200, "text/plain", value);
        }

        return true;

    }

    return false;

}



void apiRegister(const char * key, api_get_callback_f getFn, api_put_callback_f putFn) {


    web_api_t api;
    api.key = strdup(key);
    api.getFn = getFn;
    api.putFn = putFn;
    _apis.push_back(api);

}

void apiSetup() {
    _apiConfigure();
    wsOnSendRegister(_apiWebSocketOnSend);
    wsOnReceiveRegister(_apiWebSocketOnReceive);
    webRequestRegister(_apiRequestCallback);
    espurnaRegisterReload(_apiConfigure);
}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/broker.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/broker.ino"
#if BROKER_SUPPORT

#include <vector>

std::vector<void (*)(const unsigned char, const char *, unsigned char, const char *)> _broker_callbacks;



void brokerRegister(void (*callback)(const unsigned char, const char *, unsigned char, const char *)) {
    _broker_callbacks.push_back(callback);
}

void brokerPublish(const unsigned char type, const char * topic, unsigned char id, const char * message) {

    for (unsigned char i=0; i<_broker_callbacks.size(); i++) {
        (_broker_callbacks[i])(type, topic, id, message);
    }
}

void brokerPublish(const unsigned char type, const char * topic, const char * message) {
    brokerPublish(type, topic, 0, message);
}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/button.ino"
# 13 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/button.ino"
#if BUTTON_SUPPORT

#include <DebounceEvent.h>
#include <vector>

typedef struct {
    DebounceEvent * button;
    unsigned long actions;
    unsigned int relayID;
} button_t;

std::vector<button_t> _buttons;

#if MQTT_SUPPORT

void buttonMQTT(unsigned char id, uint8_t event) {
    if (id >= _buttons.size()) return;
    char payload[2];
    itoa(event, payload, 10);
    mqttSend(MQTT_TOPIC_BUTTON, id, payload, false, false);
}

#endif

#if WEB_SUPPORT

bool _buttonWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "btn", 3) == 0);
}

#endif

int buttonFromRelay(unsigned int relayID) {
    for (unsigned int i=0; i < _buttons.size(); i++) {
        if (_buttons[i].relayID == relayID) return i;
    }
    return -1;
}

bool buttonState(unsigned char id) {
    if (id >= _buttons.size()) return false;
    return _buttons[id].button->pressed();
}

unsigned char buttonAction(unsigned char id, unsigned char event) {
    if (id >= _buttons.size()) return BUTTON_MODE_NONE;
    unsigned long actions = _buttons[id].actions;
    if (event == BUTTON_EVENT_PRESSED) return (actions) & 0x0F;
    if (event == BUTTON_EVENT_CLICK) return (actions >> 4) & 0x0F;
    if (event == BUTTON_EVENT_DBLCLICK) return (actions >> 8) & 0x0F;
    if (event == BUTTON_EVENT_LNGCLICK) return (actions >> 12) & 0x0F;
    if (event == BUTTON_EVENT_LNGLNGCLICK) return (actions >> 16) & 0x0F;
    if (event == BUTTON_EVENT_TRIPLECLICK) return (actions >> 20) & 0x0F;
    return BUTTON_MODE_NONE;
}

unsigned long buttonStore(unsigned long pressed, unsigned long click, unsigned long dblclick, unsigned long lngclick, unsigned long lnglngclick, unsigned long tripleclick) {
    unsigned int value;
    value = pressed;
    value += click << 4;
    value += dblclick << 8;
    value += lngclick << 12;
    value += lnglngclick << 16;
    value += tripleclick << 20;
    return value;
}

uint8_t mapEvent(uint8_t event, uint8_t count, uint16_t length) {
    if (event == EVENT_PRESSED) return BUTTON_EVENT_PRESSED;
    if (event == EVENT_CHANGED) return BUTTON_EVENT_CLICK;
    if (event == EVENT_RELEASED) {
        if (1 == count) {
            if (length > BUTTON_LNGLNGCLICK_DELAY) return BUTTON_EVENT_LNGLNGCLICK;
            if (length > BUTTON_LNGCLICK_DELAY) return BUTTON_EVENT_LNGCLICK;
            return BUTTON_EVENT_CLICK;
        }
        if (2 == count) return BUTTON_EVENT_DBLCLICK;
        if (3 == count) return BUTTON_EVENT_TRIPLECLICK;
    }
    return BUTTON_EVENT_NONE;
}

void buttonEvent(unsigned int id, unsigned char event) {

    DEBUG_MSG_P(PSTR("[BUTTON] Button #%u event %u\n"), id, event);
    if (event == 0) return;

    unsigned char action = buttonAction(id, event);

    #if MQTT_SUPPORT
       if (action != BUTTON_MODE_NONE || BUTTON_MQTT_SEND_ALL_EVENTS) {
           buttonMQTT(id, event);
       }
    #endif

    if (BUTTON_MODE_TOGGLE == action) {
        if (_buttons[id].relayID > 0) {
            relayToggle(_buttons[id].relayID - 1);
        }
    }

    if (BUTTON_MODE_ON == action) {
        if (_buttons[id].relayID > 0) {
            relayStatus(_buttons[id].relayID - 1, true);
        }
    }

    if (BUTTON_MODE_OFF == action) {
        if (_buttons[id].relayID > 0) {
            relayStatus(_buttons[id].relayID - 1, false);
        }
    }

    if (BUTTON_MODE_AP == action) {
        wifiStartAP();
    }

    if (BUTTON_MODE_RESET == action) {
        deferredReset(100, CUSTOM_RESET_HARDWARE);
    }

    if (BUTTON_MODE_FACTORY == action) {
        DEBUG_MSG_P(PSTR("\n\nFACTORY RESET\n\n"));
        resetSettings();
        deferredReset(100, CUSTOM_RESET_FACTORY);
    }

    #if defined(JUSTWIFI_ENABLE_WPS)
        if (BUTTON_MODE_WPS == action) {
            wifiStartWPS();
        }
    #endif

    #if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
        if (BUTTON_MODE_SMART_CONFIG == action) {
            wifiStartSmartConfig();
        }
    #endif

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    if (BUTTON_MODE_DIM_UP == action) {
        lightBrightnessStep(1);
        lightUpdate(true, true);
    }
    if (BUTTON_MODE_DIM_DOWN == action) {
        lightBrightnessStep(-1);
        lightUpdate(true, true);
    }
    #endif

}

void buttonSetup() {

    #if defined(ITEAD_SONOFF_DUAL)

        unsigned int actions = buttonStore(BUTTON_MODE_NONE, BUTTON_MODE_TOGGLE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE);
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, 1});
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, 2});
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, BUTTON3_RELAY});

    #elif defined(FOXEL_LIGHTFOX_DUAL)

        unsigned int actions = buttonStore(BUTTON_MODE_NONE, BUTTON_MODE_TOGGLE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE, BUTTON_MODE_NONE);
        unsigned int btn1Relay = getSetting("btnRelay", 0, BUTTON1_RELAY - 1).toInt() + 1;
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, btn1Relay});
        unsigned int btn2Relay = getSetting("btnRelay", 1, BUTTON2_RELAY - 1).toInt() + 1;
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, btn2Relay});
        unsigned int btn3Relay = getSetting("btnRelay", 2, BUTTON3_RELAY - 1).toInt() + 1;
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, btn3Relay});
        unsigned int btn4Relay = getSetting("btnRelay", 3, BUTTON4_RELAY - 1).toInt() + 1;
        _buttons.push_back({new DebounceEvent(0, BUTTON_PUSHBUTTON), actions, btn4Relay});

    #else

        unsigned long btnDelay = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();
        UNUSED(btnDelay);

        #if BUTTON1_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON1_PRESS, BUTTON1_CLICK, BUTTON1_DBLCLICK, BUTTON1_LNGCLICK, BUTTON1_LNGLNGCLICK, BUTTON1_TRIPLECLICK);
            _buttons.push_back({new DebounceEvent(BUTTON1_PIN, BUTTON1_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON1_RELAY});
        }
        #endif
        #if BUTTON2_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON2_PRESS, BUTTON2_CLICK, BUTTON2_DBLCLICK, BUTTON2_LNGCLICK, BUTTON2_LNGLNGCLICK, BUTTON2_TRIPLECLICK);
            _buttons.push_back({new DebounceEvent(BUTTON2_PIN, BUTTON2_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON2_RELAY});
        }
        #endif
        #if BUTTON3_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON3_PRESS, BUTTON3_CLICK, BUTTON3_DBLCLICK, BUTTON3_LNGCLICK, BUTTON3_LNGLNGCLICK, BUTTON3_TRIPLECLICK);
            _buttons.push_back({new DebounceEvent(BUTTON3_PIN, BUTTON3_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON3_RELAY});
        }
        #endif
        #if BUTTON4_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON4_PRESS, BUTTON4_CLICK, BUTTON4_DBLCLICK, BUTTON4_LNGCLICK, BUTTON4_LNGLNGCLICK, BUTTON4_TRIPLECLICK);
            _buttons.push_back({new DebounceEvent(BUTTON4_PIN, BUTTON4_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON4_RELAY});
        }
        #endif
        #if BUTTON5_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON5_PRESS, BUTTON5_CLICK, BUTTON5_DBLCLICK, BUTTON5_LNGCLICK, BUTTON5_LNGLNGCLICK, BUTTON5_TRIPLECLICK);
            _buttons.push_back({new DebounceEvent(BUTTON5_PIN, BUTTON5_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON5_RELAY});
        }
        #endif
        #if BUTTON6_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON6_PRESS, BUTTON6_CLICK, BUTTON6_DBLCLICK, BUTTON6_LNGCLICK, BUTTON6_LNGLNGCLICK, BUTTON6_TRIPLECLICK);
            _buttons.push_back({new DebounceEvent(BUTTON6_PIN, BUTTON6_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON6_RELAY});
        }
        #endif
        #if BUTTON7_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON7_PRESS, BUTTON7_CLICK, BUTTON7_DBLCLICK, BUTTON7_LNGCLICK, BUTTON7_LNGLNGCLICK, BUTTON7_TRIPLECLICK);
            _buttons.push_back({new DebounceEvent(BUTTON7_PIN, BUTTON7_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON7_RELAY});
        }
        #endif
        #if BUTTON8_PIN != GPIO_NONE
        {
            unsigned int actions = buttonStore(BUTTON8_PRESS, BUTTON8_CLICK, BUTTON8_DBLCLICK, BUTTON8_LNGCLICK, BUTTON8_LNGLNGCLICK, BUTTON8_TRIPLECLICK);
            _buttons.push_back({new DebounceEvent(BUTTON8_PIN, BUTTON8_MODE, BUTTON_DEBOUNCE_DELAY, btnDelay), actions, BUTTON8_RELAY});
        }
        #endif

    #endif

    DEBUG_MSG_P(PSTR("[BUTTON] Number of buttons: %u\n"), _buttons.size());


    #if WEB_SUPPORT
        wsOnReceiveRegister(_buttonWebSocketOnReceive);
    #endif


    espurnaRegisterLoop(buttonLoop);

}

void buttonLoop() {

    #if defined(ITEAD_SONOFF_DUAL)

        if (Serial.available() >= 4) {
            if (Serial.read() == 0xA0) {
                if (Serial.read() == 0x04) {
                    unsigned char value = Serial.read();
                    if (Serial.read() == 0xA1) {






                        if ((value & 4) == 4) {
                            buttonEvent(2, BUTTON_EVENT_CLICK);
                            return;
                        }






                        for (unsigned int i=0; i<relayCount(); i++) {

                            bool status = (value & (1 << i)) > 0;


                            if (relayStatus(i) != status) {
                                buttonEvent(i, BUTTON_EVENT_CLICK);
                                break;
                            }

                        }

                    }
                }
            }
        }

    #elif defined(FOXEL_LIGHTFOX_DUAL)

        if (Serial.available() >= 4) {
            if (Serial.read() == 0xA0) {
                if (Serial.read() == 0x04) {
                    unsigned char value = Serial.read();
                    if (Serial.read() == 0xA1) {

                        DEBUG_MSG_P(PSTR("[BUTTON] [LIGHTFOX] Received buttons mask: %d\n"), value);

                        for (unsigned int i=0; i<_buttons.size(); i++) {

                            bool clicked = (value & (1 << i)) > 0;

                            if (clicked) {
                                buttonEvent(i, BUTTON_EVENT_CLICK);
                            }
                        }
                    }
                }
            }
        }

    #else

        for (unsigned int i=0; i < _buttons.size(); i++) {
            if (unsigned char event = _buttons[i].button->loop()) {
                unsigned char count = _buttons[i].button->getEventCount();
                unsigned long length = _buttons[i].button->getEventLength();
                unsigned char mapped = mapEvent(event, count, length);
                buttonEvent(i, mapped);
            }
       }

    #endif

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/crash.ino"






#if DEBUG_SUPPORT

#include <stdio.h>
#include <stdarg.h>
#include <EEPROM_Rotate.h>

extern "C" {
    #include "user_interface.h"
}

#define SAVE_CRASH_EEPROM_OFFSET 0x0100
# 36 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/crash.ino"
#define SAVE_CRASH_CRASH_TIME 0x00
#define SAVE_CRASH_RESTART_REASON 0x04
#define SAVE_CRASH_EXCEPTION_CAUSE 0x05
#define SAVE_CRASH_EPC1 0x06
#define SAVE_CRASH_EPC2 0x0A
#define SAVE_CRASH_EPC3 0x0E
#define SAVE_CRASH_EXCVADDR 0x12
#define SAVE_CRASH_DEPC 0x16
#define SAVE_CRASH_STACK_START 0x1A
#define SAVE_CRASH_STACK_END 0x1E
#define SAVE_CRASH_STACK_SIZE 0x22
#define SAVE_CRASH_STACK_TRACE 0x24

#define SAVE_CRASH_STACK_TRACE_MAX 0x80

uint16_t _save_crash_stack_trace_max = SAVE_CRASH_STACK_TRACE_MAX;
uint16_t _save_crash_enabled = true;







extern "C" void custom_crash_callback(struct rst_info * rst_info, uint32_t stack_start, uint32_t stack_end ) {


    if (checkNeedsReset()) {
        return;
    }


    if (!_save_crash_enabled) {
        return;
    }


    uint32_t crash_time = millis();
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_CRASH_TIME, crash_time);


    EEPROMr.write(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_RESTART_REASON, rst_info->reason);
    EEPROMr.write(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCEPTION_CAUSE, rst_info->exccause);


    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC1, rst_info->epc1);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC2, rst_info->epc2);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC3, rst_info->epc3);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCVADDR, rst_info->excvaddr);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_DEPC, rst_info->depc);




    const uint16_t stack_size = constrain((stack_end - stack_start), 0, _save_crash_stack_trace_max);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_START, stack_start);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_END, stack_end);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_SIZE, stack_size);


    const uint16_t settings_start = (
        ((SPI_FLASH_SEC_SIZE - settingsSize() + 31) & -32) - 0x20);


    int16_t eeprom_addr = SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_TRACE;
    for (uint32_t* addr = (uint32_t*)stack_start; addr < (uint32_t*)(stack_start + stack_size); addr++) {
        if (eeprom_addr >= settings_start) break;
        EEPROMr.put(eeprom_addr, *addr);
        eeprom_addr += sizeof(uint32_t);
    }

    EEPROMr.commit();

}




void crashClear() {
    uint32_t crash_time = 0xFFFFFFFF;
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_CRASH_TIME, crash_time);
    EEPROMr.commit();
}




void crashDump() {

    uint32_t crash_time;
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_CRASH_TIME, crash_time);
    if ((crash_time == 0) || (crash_time == 0xFFFFFFFF)) {
        DEBUG_MSG_P(PSTR("[DEBUG] No crash info\n"));
        return;
    }

    DEBUG_MSG_P(PSTR("[DEBUG] Latest crash was at %lu ms after boot\n"), crash_time);
    DEBUG_MSG_P(PSTR("[DEBUG] Reason of restart: %u\n"), EEPROMr.read(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_RESTART_REASON));
    DEBUG_MSG_P(PSTR("[DEBUG] Exception cause: %u\n"), EEPROMr.read(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCEPTION_CAUSE));

    uint32_t epc1, epc2, epc3, excvaddr, depc;
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC1, epc1);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC2, epc2);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC3, epc3);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCVADDR, excvaddr);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_DEPC, depc);

    DEBUG_MSG_P(PSTR("[DEBUG] epc1=0x%08x epc2=0x%08x epc3=0x%08x\n"), epc1, epc2, epc3);
    DEBUG_MSG_P(PSTR("[DEBUG] excvaddr=0x%08x depc=0x%08x\n"), excvaddr, depc);

    uint32_t stack_start, stack_end;
    uint16_t stack_size;

    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_START, stack_start);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_END, stack_end);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_SIZE, stack_size);

    DEBUG_MSG_P(PSTR("sp=0x%08x end=0x%08x saved=0x%04x\n\n"), stack_start, stack_end, stack_size);
    if (0xFFFF == stack_size) return;

    int16_t current_address = SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_TRACE;

    uint32_t stack_trace;

    DEBUG_MSG_P(PSTR("[DEBUG] >>>stack>>>\n[DEBUG] "));

    for (int16_t i = 0; i < stack_size; i += 0x10) {
        DEBUG_MSG_P(PSTR("%08x: "), stack_start + i);
        for (byte j = 0; j < 4; j++) {
            EEPROMr.get(current_address, stack_trace);
            DEBUG_MSG_P(PSTR("%08x "), stack_trace);
            current_address += 4;
        }
        DEBUG_MSG_P(PSTR("\n[DEBUG] "));
    }
    DEBUG_MSG_P(PSTR("<<<stack<<<\n"));

}

void crashSetup() {

    #if TERMINAL_SUPPORT
        terminalRegisterCommand(F("CRASH"), [](Embedis* e) {
            crashDump();
            crashClear();
            terminalOK();
        });
    #endif


    _save_crash_stack_trace_max = getSetting("sysTraceMax", SAVE_CRASH_STACK_TRACE_MAX).toInt();
    _save_crash_stack_trace_max = (_save_crash_stack_trace_max + 15) & -16;
    setSetting("sysScTraceMax", _save_crash_stack_trace_max);

    _save_crash_enabled = getSetting("sysCrashSave", 1).toInt() == 1;

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/debug.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/debug.ino"
#if DEBUG_SUPPORT

#include "libs/DebugSend.h"

#if DEBUG_UDP_SUPPORT
#include <WiFiUdp.h>
WiFiUDP _udp_debug;
#if DEBUG_UDP_PORT == 514
char _udp_syslog_header[40] = {0};
#endif
#endif

#if DEBUG_SERIAL_SUPPORT
    void _debugSendSerial(const char* prefix, const char* data) {
        if (prefix && (prefix[0] != '\0')) {
            DEBUG_PORT.print(prefix);
        }
        DEBUG_PORT.print(data);

    }
#endif

#if DEBUG_TELNET_SUPPORT
    void _debugSendTelnet(const char* prefix, const char* data) {
        if (prefix && (prefix[0] != '\0')) {
            _telnetWrite(prefix);
        }
        _telnetWrite(data);

    }
#endif

void debugSendImpl(const char * message) {

    const size_t msg_len = strlen(message);

    bool pause = false;
    char timestamp[10] = {0};

    #if DEBUG_ADD_TIMESTAMP
        static bool add_timestamp = true;
        if (add_timestamp) {
            snprintf(timestamp, sizeof(timestamp), "[%06lu] ", millis() % 1000000);
        }
        add_timestamp = (message[msg_len - 1] == 10) || (message[msg_len - 1] == 13);
    #endif

    #if DEBUG_SERIAL_SUPPORT
        _debugSendSerial(timestamp, message);
    #endif

    #if DEBUG_UDP_SUPPORT
        #if SYSTEM_CHECK_ENABLED
        if (systemCheck()) {
        #endif
            _udp_debug.beginPacket(DEBUG_UDP_IP, DEBUG_UDP_PORT);
            #if DEBUG_UDP_PORT == 514
                _udp_debug.write(_udp_syslog_header);
            #endif
            _udp_debug.write(message);
            _udp_debug.endPacket();
            pause = true;
        #if SYSTEM_CHECK_ENABLED
        }
        #endif
    #endif

    #if DEBUG_TELNET_SUPPORT
        _debugSendTelnet(timestamp, message);
        pause = true;
    #endif

    #if DEBUG_WEB_SUPPORT
        wsDebugSend(timestamp, message);
        pause = true;
    #endif

    if (pause) optimistic_yield(100);

}


#if DEBUG_WEB_SUPPORT

void debugWebSetup() {

    wsOnSendRegister([](JsonObject& root) {
        root["dbgVisible"] = 1;
    });

    wsOnActionRegister([](uint32_t client_id, const char * action, JsonObject& data) {

        #if TERMINAL_SUPPORT
            if (strcmp(action, "dbgcmd") == 0) {
                const char* command = data.get<const char*>("command");
                char buffer[strlen(command) + 2];
                snprintf(buffer, sizeof(buffer), "%s\n", command);
                terminalInject((void*) buffer, strlen(buffer));
            }
        #endif

    });

    #if DEBUG_UDP_SUPPORT
    #if DEBUG_UDP_PORT == 514
        snprintf_P(_udp_syslog_header, sizeof(_udp_syslog_header), PSTR("<%u>%s ESPurna[0]: "), DEBUG_UDP_FAC_PRI, getSetting("hostname").c_str());
    #endif
    #endif


}

#endif



void debugSetup() {

    #if DEBUG_SERIAL_SUPPORT
        DEBUG_PORT.begin(SERIAL_BAUDRATE);
        #if DEBUG_ESP_WIFI
            DEBUG_PORT.setDebugOutput(true);
        #endif
    #endif

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/domoticz.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/domoticz.ino"
#if DOMOTICZ_SUPPORT

#include <ArduinoJson.h>

bool _dcz_enabled = false;
std::vector<bool> _dcz_relay_state;





int _domoticzRelay(unsigned int idx) {
    for (unsigned char relayID=0; relayID<relayCount(); relayID++) {
        if (domoticzIdx(relayID) == idx) {
            return relayID;
        }
    }
    return -1;
}

void _domoticzMqttSubscribe(bool value) {

    String dczTopicOut = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);
    if (value) {
        mqttSubscribeRaw(dczTopicOut.c_str());
    } else {
        mqttUnsubscribeRaw(dczTopicOut.c_str());
    }

}

bool _domoticzStatus(unsigned char id) {
    return _dcz_relay_state[id];
}

void _domoticzStatus(unsigned char id, bool status) {
    _dcz_relay_state[id] = status;
    relayStatus(id, status);
}

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

void _domoticzLight(unsigned int idx, const JsonObject& root) {

    if (!lightHasColor()) return;

    JsonObject& color = root["Color"];
    if (!color.success()) return;


    unsigned int cmode = color["m"];

    if (cmode == 3 || cmode == 4) {

        lightChannel(0, color["r"]);
        lightChannel(1, color["g"]);
        lightChannel(2, color["b"]);



        if (lightChannels() > 3) {
            lightChannel(3, color["ww"]);
        }

        if (lightChannels() > 4) {
            lightChannel(4, color["cw"]);
        }


        unsigned int brightness = (root["Level"].as<uint8_t>() / 100.0) * LIGHT_MAX_BRIGHTNESS;
        lightBrightness(brightness);

        DEBUG_MSG_P(PSTR("[DOMOTICZ] Received rgb:%u,%u,%u ww:%u,cw:%u brightness:%u for IDX %u\n"),
            color["r"].as<uint8_t>(),
            color["g"].as<uint8_t>(),
            color["b"].as<uint8_t>(),
            color["ww"].as<uint8_t>(),
            color["cw"].as<uint8_t>(),
            brightness,
            idx
        );

        lightUpdate(true, mqttForward());

    }

}

#endif

void _domoticzMqtt(unsigned int type, const char * topic, const char * payload) {

    if (!_dcz_enabled) return;

    String dczTopicOut = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

    if (type == MQTT_CONNECT_EVENT) {


        mqttSubscribeRaw(dczTopicOut.c_str());


        domoticzSendRelays();

    }

    if (type == MQTT_MESSAGE_EVENT) {


        if (dczTopicOut.equals(topic)) {


            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject((char *) payload);
            if (!root.success()) {
                DEBUG_MSG_P(PSTR("[DOMOTICZ] Error parsing data\n"));
                return;
            }


            unsigned int idx = root["idx"];
            String stype = root["stype"];

            #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
                if (stype.startsWith("RGB") && (domoticzIdx(0) == idx)) {
                    _domoticzLight(idx, root);
                }
            #endif

            int relayID = _domoticzRelay(idx);
            if (relayID >= 0) {
                unsigned char value = root["nvalue"];
                DEBUG_MSG_P(PSTR("[DOMOTICZ] Received value %u for IDX %u\n"), value, idx);
                _domoticzStatus(relayID, value >= 1);
            }

        }

    }

};

#if BROKER_SUPPORT
void _domoticzBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload) {


    if (BROKER_MSG_TYPE_STATUS != type) return;

    if (strcmp(MQTT_TOPIC_RELAY, topic) == 0) {
        bool status = atoi(payload) == 1;
        if (_domoticzStatus(id) == status) return;
        _dcz_relay_state[id] = status;
        domoticzSendRelay(id, status);
    }

}
#endif

#if WEB_SUPPORT

bool _domoticzWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "dcz", 3) == 0);
}

void _domoticzWebSocketOnSend(JsonObject& root) {

    unsigned char visible = 0;
    root["dczEnabled"] = getSetting("dczEnabled", DOMOTICZ_ENABLED).toInt() == 1;
    root["dczTopicIn"] = getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC);
    root["dczTopicOut"] = getSetting("dczTopicOut", DOMOTICZ_OUT_TOPIC);

    JsonArray& relays = root.createNestedArray("dczRelays");
    for (unsigned char i=0; i<relayCount(); i++) {
        relays.add(domoticzIdx(i));
    }
    visible = (relayCount() > 0);

    #if SENSOR_SUPPORT
        _sensorWebSocketMagnitudes(root, "dcz");
        visible = visible || (magnitudeCount() > 0);
    #endif

    root["dczVisible"] = visible;

}

#endif

void _domoticzConfigure() {
    bool enabled = getSetting("dczEnabled", DOMOTICZ_ENABLED).toInt() == 1;
    if (enabled != _dcz_enabled) _domoticzMqttSubscribe(enabled);

    _dcz_relay_state.reserve(relayCount());
    for (size_t n = 0; n < relayCount(); ++n) {
        _dcz_relay_state[n] = relayStatus(n);
    }

    _dcz_enabled = enabled;
}





template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue) {
    if (!_dcz_enabled) return;
    unsigned int idx = getSetting(key).toInt();
    if (idx > 0) {
        char payload[128];
        snprintf(payload, sizeof(payload), "{\"idx\": %u, \"nvalue\": %s, \"svalue\": \"%s\"}", idx, String(nvalue).c_str(), svalue);
        mqttSendRaw(getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC).c_str(), payload);
    }
}

template<typename T> void domoticzSend(const char * key, T nvalue) {
    domoticzSend(key, nvalue, "");
}

void domoticzSendRelay(unsigned char relayID, bool status) {
    if (!_dcz_enabled) return;
    char buffer[15];
    snprintf_P(buffer, sizeof(buffer), PSTR("dczRelayIdx%u"), relayID);
    domoticzSend(buffer, status ? "1" : "0");
}

void domoticzSendRelays() {
    for (uint8_t relayID=0; relayID < relayCount(); relayID++) {
        domoticzSendRelay(relayID, relayStatus(relayID));
    }
}

unsigned int domoticzIdx(unsigned char relayID) {
    char buffer[15];
    snprintf_P(buffer, sizeof(buffer), PSTR("dczRelayIdx%u"), relayID);
    return getSetting(buffer).toInt();
}

void domoticzSetup() {

    _domoticzConfigure();

    #if WEB_SUPPORT
        wsOnSendRegister(_domoticzWebSocketOnSend);
        wsOnReceiveRegister(_domoticzWebSocketOnReceive);
    #endif

    #if BROKER_SUPPORT
        brokerRegister(_domoticzBrokerCallback);
    #endif


    mqttRegister(_domoticzMqtt);
    espurnaRegisterReload(_domoticzConfigure);

}

bool domoticzEnabled() {
    return _dcz_enabled;
}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/eeprom.ino"






#include <EEPROM_Rotate.h>



bool _eeprom_commit = false;

uint32_t _eeprom_commit_count = 0;
bool _eeprom_last_commit_result = false;

void eepromRotate(bool value) {


    if (EEPROMr.size() > EEPROMr.reserved()) {
        if (value) {
            DEBUG_MSG_P(PSTR("[EEPROM] Reenabling EEPROM rotation\n"));
        } else {
            DEBUG_MSG_P(PSTR("[EEPROM] Disabling EEPROM rotation\n"));
        }
        EEPROMr.rotate(value);
    }
}

uint32_t eepromCurrent() {
    return EEPROMr.current();
}

String eepromSectors() {
    String response;
    for (uint32_t i = 0; i < EEPROMr.size(); i++) {
        if (i > 0) response = response + String(", ");
        response = response + String(EEPROMr.base() - i);
    }
    return response;
}

void eepromSectorsDebug() {
    DEBUG_MSG_P(PSTR("[MAIN] EEPROM sectors: %s\n"), (char *) eepromSectors().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] EEPROM current: %lu\n"), eepromCurrent());
}

bool _eepromCommit() {
    _eeprom_commit_count++;
    _eeprom_last_commit_result = EEPROMr.commit();
    return _eeprom_last_commit_result;
}

void eepromCommit() {
    _eeprom_commit = true;
}

#if TERMINAL_SUPPORT

void _eepromInitCommands() {

    terminalRegisterCommand(F("EEPROM"), [](Embedis* e) {
        infoMemory("EEPROM", SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE - settingsSize());
        eepromSectorsDebug();
        if (_eeprom_commit_count > 0) {
            DEBUG_MSG_P(PSTR("[MAIN] Commits done: %lu\n"), _eeprom_commit_count);
            DEBUG_MSG_P(PSTR("[MAIN]  Last result: %s\n"), _eeprom_last_commit_result ? "OK" : "ERROR");
        }
        terminalOK();
    });

    terminalRegisterCommand(F("EEPROM.COMMIT"), [](Embedis* e) {
        const bool res = _eepromCommit();
        if (res) {
            terminalOK();
        } else {
            DEBUG_MSG_P(PSTR("-ERROR\n"));
        }
    });

    terminalRegisterCommand(F("EEPROM.DUMP"), [](Embedis* e) {
        EEPROMr.dump(terminalSerial());
        terminalOK();
    });

    terminalRegisterCommand(F("FLASH.DUMP"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("Wrong arguments"));
            return;
        }
        uint32_t sector = String(e->argv[1]).toInt();
        uint32_t max = ESP.getFlashChipSize() / SPI_FLASH_SEC_SIZE;
        if (sector >= max) {
            terminalError(F("Sector out of range"));
            return;
        }
        EEPROMr.dump(terminalSerial(), sector);
        terminalOK();
    });

}

#endif



void eepromLoop() {
    if (_eeprom_commit) {
        _eepromCommit();
        _eeprom_commit = false;
    }
}

void eepromSetup() {

    #ifdef EEPROM_ROTATE_SECTORS
        EEPROMr.size(EEPROM_ROTATE_SECTORS);
    #else


        if (EEPROMr.size() == 1) {
            if (EEPROMr.last() > 1000) {
                EEPROMr.size(4);
            } else if (EEPROMr.last() > 250) {
                EEPROMr.size(2);
            }
        }
    #endif

    EEPROMr.offset(EEPROM_ROTATE_DATA);
    EEPROMr.begin(EEPROM_SIZE);

    #if TERMINAL_SUPPORT
        _eepromInitCommands();
    #endif

    espurnaRegisterLoop(eepromLoop);

}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/encoder.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/encoder.ino"
#if ENCODER_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)

#include "libs/Encoder.h"
#include <vector>

typedef struct {
    Encoder * encoder;
    unsigned char button_pin;
    unsigned char button_logic;
    unsigned char button_mode;
    unsigned char mode;
    unsigned char channel1;
    unsigned char channel2;
} encoder_t;

std::vector<encoder_t> _encoders;
unsigned long _encoder_min_delta = 1;

void _encoderConfigure() {


    for (unsigned char i=0; i<_encoders.size(); i++) {
        free(_encoders[i].encoder);
    }
    _encoders.clear();


    #if (ENCODER1_PIN1 != GPIO_NONE) && (ENCODER1_PIN2 != GPIO_NONE)
    {
        _encoders.push_back({
            new Encoder(ENCODER1_PIN1, ENCODER1_PIN2),
            ENCODER1_BUTTON_PIN, ENCODER1_BUTTON_LOGIC, ENCODER1_BUTTON_MODE, ENCODER1_MODE,
            ENCODER1_CHANNEL1, ENCODER1_CHANNEL2
        });
    }
    #endif
    #if (ENCODER2_PIN1 != GPIO_NONE) && (ENCODER2_PIN2 != GPIO_NONE)
    {
        _encoders.push_back({
            new Encoder(ENCODER2_PIN1, ENCODER2_PIN2),
            ENCODER2_BUTTON_PIN, ENCODER2_BUTTON_LOGIC, ENCODER2_BUTTON_MODE, ENCODER2_MODE,
            ENCODER2_CHANNEL1, ENCODER2_CHANNEL2
        });
    }
    #endif
    #if (ENCODER3_PIN1 != GPIO_NONE) && (ENCODER3_PIN2 != GPIO_NONE)
    {
        _encoders.push_back({
            new Encoder(ENCODER3_PIN1, ENCODER3_PIN2),
            ENCODER3_BUTTON_PIN, ENCODER3_BUTTON_LOGIC, ENCODER3_BUTTON_MODE, ENCODER3_MODE,
            ENCODER3_CHANNEL1, ENCODER3_CHANNEL2
        });
    }
    #endif
    #if (ENCODER4_PIN1 != GPIO_NONE) && (ENCODER4_PIN2 != GPIO_NONE)
    {
        _encoders.push_back({
            new Encoder(ENCODER4_PIN1, ENCODER4_PIN2),
            ENCODER4_BUTTON_PIN, ENCODER4_BUTTON_LOGIC, ENCODER4_BUTTON_MODE, ENCODER4_MODE,
            ENCODER4_CHANNEL1, ENCODER4_CHANNEL2
        });
    }
    #endif
    #if (ENCODER5_PIN1 != GPIO_NONE) && (ENCODER5_PIN2 != GPIO_NONE)
    {
        _encoders.push_back({
            new Encoder(ENCODER5_PIN1, ENCODER5_PIN2),
            ENCODER5_BUTTON_PIN, ENCODER5_BUTTON_LOGIC, ENCODER5_BUTTON_MODE, ENCODER5_MODE,
            ENCODER5_CHANNEL1, ENCODER5_CHANNEL2
        });
    }
    #endif


    for (unsigned char i=0; i<_encoders.size(); i++) {
        if (GPIO_NONE != _encoders[i].button_pin) {
            pinMode(_encoders[i].button_pin, _encoders[i].button_mode);
        }
    }

    _encoder_min_delta = getSetting("encMinDelta", ENCODER_MINIMUM_DELTA).toInt();
    if (!_encoder_min_delta) _encoder_min_delta = 1;

}

void _encoderLoop() {


    for (unsigned char i=0; i<_encoders.size(); i++) {

        encoder_t encoder = _encoders[i];

        long delta = encoder.encoder->read();
        encoder.encoder->write(0);
        if ((0 == delta) || (_encoder_min_delta > abs(delta))) continue;

        if (encoder.button_pin == GPIO_NONE) {


            lightChannelStep(encoder.channel1, delta);

        } else {


            bool pressed = (digitalRead(encoder.button_pin) != encoder.button_logic);

            if (ENCODER_MODE_CHANNEL == encoder.mode) {


                lightChannelStep(pressed ? encoder.channel2 : encoder.channel1, delta);

            } if (ENCODER_MODE_RATIO == encoder.mode) {


                if (pressed) {
                    lightChannelStep(encoder.channel1, delta);
                    lightChannelStep(encoder.channel2, -delta);
                } else {
                    lightBrightnessStep(delta);
                }

            }

        }

        lightUpdate(true, true);

    }

}



void encoderSetup() {


    _encoderConfigure();


    espurnaRegisterLoop(_encoderLoop);
    espurnaRegisterReload(_encoderConfigure);

    DEBUG_MSG_P(PSTR("[ENCODER] Number of encoders: %u\n"), _encoders.size());

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/gpio.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/gpio.ino"
unsigned int _gpio_locked = 0;

bool gpioValid(unsigned char gpio) {
    if (gpio <= 5) return true;
    if (12 <= gpio && gpio <= 15) return true;
    return false;
}

bool gpioGetLock(unsigned char gpio) {
    if (gpioValid(gpio)) {
        unsigned int mask = 1 << gpio;
        if ((_gpio_locked & mask) == 0) {
            _gpio_locked |= mask;
            DEBUG_MSG_P(PSTR("[GPIO] GPIO%u locked\n"), gpio);
            return true;
        }
    }
    DEBUG_MSG_P(PSTR("[GPIO] Failed getting lock for GPIO%u\n"), gpio);
    return false;
}

bool gpioReleaseLock(unsigned char gpio) {
    if (gpioValid(gpio)) {
        unsigned int mask = 1 << gpio;
        _gpio_locked &= ~mask;
        DEBUG_MSG_P(PSTR("[GPIO] GPIO%u lock released\n"), gpio);
        return true;
    }
    DEBUG_MSG_P(PSTR("[GPIO] Failed releasing lock for GPIO%u\n"), gpio);
    return false;
}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/homeassistant.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/homeassistant.ino"
#if HOMEASSISTANT_SUPPORT

#include <ArduinoJson.h>
#include <queue>

bool _haEnabled = false;
bool _haSendFlag = false;





String _haFixName(String name) {
    for (unsigned char i=0; i<name.length(); i++) {
        if (!isalnum(name.charAt(i))) name.setCharAt(i, '_');
    }
    return name;
}





#if SENSOR_SUPPORT

void _haSendMagnitude(unsigned char i, JsonObject& config) {

    unsigned char type = magnitudeType(i);
    config["name"] = _haFixName(getSetting("hostname") + String(" ") + magnitudeTopic(type));
    config.set("platform", "mqtt");
    config["state_topic"] = mqttTopic(magnitudeTopicIndex(i).c_str(), false);
    config["unit_of_measurement"] = magnitudeUnits(type);
}

void _haSendMagnitudes(const JsonObject& deviceConfig) {

    for (unsigned char i=0; i<magnitudeCount(); i++) {

        String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX) +
            "/sensor/" +
            getSetting("hostname") + "_" + String(i) +
            "/config";

        String output;
        if (_haEnabled) {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& config = jsonBuffer.createObject();
            _haSendMagnitude(i, config);
            config["uniq_id"] = getIdentifier() + "_" + magnitudeTopic(magnitudeType(i)) + "_" + String(i);
            config["device"] = deviceConfig;

            config.printTo(output);
            jsonBuffer.clear();
        }

        mqttSendRaw(topic.c_str(), output.c_str());
        mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

    }

}

#endif





void _haSendSwitch(unsigned char i, JsonObject& config) {

    String name = getSetting("hostname");
    if (relayCount() > 1) {
        name += String("_") + String(i);
    }

    config.set("name", _haFixName(name));
    config.set("platform", "mqtt");

    if (relayCount()) {
        config["state_topic"] = mqttTopic(MQTT_TOPIC_RELAY, i, false);
        config["command_topic"] = mqttTopic(MQTT_TOPIC_RELAY, i, true);
        config["payload_on"] = String(HOMEASSISTANT_PAYLOAD_ON);
        config["payload_off"] = String(HOMEASSISTANT_PAYLOAD_OFF);
        config["availability_topic"] = mqttTopic(MQTT_TOPIC_STATUS, false);
        config["payload_available"] = String(HOMEASSISTANT_PAYLOAD_AVAILABLE);
        config["payload_not_available"] = String(HOMEASSISTANT_PAYLOAD_NOT_AVAILABLE);
    }

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

        if (i == 0) {

            config["brightness_state_topic"] = mqttTopic(MQTT_TOPIC_BRIGHTNESS, false);
            config["brightness_command_topic"] = mqttTopic(MQTT_TOPIC_BRIGHTNESS, true);

            if (lightHasColor()) {
                config["rgb_state_topic"] = mqttTopic(MQTT_TOPIC_COLOR_RGB, false);
                config["rgb_command_topic"] = mqttTopic(MQTT_TOPIC_COLOR_RGB, true);
            }
            if (lightUseCCT()) {
                config["color_temp_command_topic"] = mqttTopic(MQTT_TOPIC_MIRED, true);
            }

            if (lightChannels() > 3) {
                config["white_value_state_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, false);
                config["white_value_command_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, true);
            }

        }

    #endif

}

void _haSendSwitches(const JsonObject& deviceConfig) {

    #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE) || (defined(ITEAD_SLAMPHER))
        String type = String("light");
    #else
        String type = String("switch");
    #endif

    for (unsigned char i=0; i<relayCount(); i++) {

        String topic = getSetting("haPrefix", HOMEASSISTANT_PREFIX) +
            "/" + type +
            "/" + getSetting("hostname") + "_" + String(i) +
            "/config";

        String output;
        if (_haEnabled) {
            DynamicJsonBuffer jsonBuffer;
            JsonObject& config = jsonBuffer.createObject();
            _haSendSwitch(i, config);
            config["uniq_id"] = getIdentifier() + "_" + type + "_" + String(i);
            config["device"] = deviceConfig;

            config.printTo(output);
            jsonBuffer.clear();
        }

        mqttSendRaw(topic.c_str(), output.c_str());
        mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

    }

}



void _haDumpConfig(std::function<void(String&)> printer, bool wrapJson = false) {

    #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE) || (defined(ITEAD_SLAMPHER))
        String type = String("light");
    #else
        String type = String("switch");
    #endif

    for (unsigned char i=0; i<relayCount(); i++) {

        DynamicJsonBuffer jsonBuffer;
        JsonObject& config = jsonBuffer.createObject();
        _haSendSwitch(i, config);

        String output;
        output.reserve(config.measureLength() + 32);

        if (wrapJson) {
            output += "{\"haConfig\": \"";
        }

        output += "\n\n" + type + ":\n";
        bool first = true;

        for (auto kv : config) {
            if (first) {
                output += "  - ";
                first = false;
            } else {
                output += "    ";
            }
            output += kv.key;
            output += ": ";
            output += kv.value.as<String>();
            output += "\n";
        }
        output += " ";

        if (wrapJson) {
            output += "\"}";
        }

        jsonBuffer.clear();

        printer(output);

    }

    #if SENSOR_SUPPORT

        for (unsigned char i=0; i<magnitudeCount(); i++) {

            DynamicJsonBuffer jsonBuffer;
            JsonObject& config = jsonBuffer.createObject();
            _haSendMagnitude(i, config);

            String output;
            output.reserve(config.measureLength() + 32);

            if (wrapJson) {
                output += "{\"haConfig\": \"";
            }

            output += "\n\nsensor:\n";
            bool first = true;

            for (auto kv : config) {
                if (first) {
                    output += "  - ";
                    first = false;
                } else {
                    output += "    ";
                }
                String value = kv.value.as<String>();
                value.replace("%", "'%'");
                output += kv.key;
                output += ": ";
                output += value;
                output += "\n";
            }
            output += " ";

            if (wrapJson) {
                output += "\"}";
            }

            jsonBuffer.clear();

            printer(output);

        }

    #endif
}

void _haGetDeviceConfig(JsonObject& config) {
    String identifier = getIdentifier();

    config.createNestedArray("identifiers").add(identifier);
    config["name"] = getSetting("desc", getSetting("hostname"));
    config["manufacturer"] = String(MANUFACTURER);
    config["model"] = String(DEVICE);
    config["sw_version"] = String(APP_NAME) + " " + String(APP_VERSION) + " (" + getCoreVersion() + ")";
}

void _haSend() {


    if (!_haSendFlag) return;


    if (!mqttConnected()) return;

    DEBUG_MSG_P(PSTR("[HA] Sending autodiscovery MQTT message\n"));


    DynamicJsonBuffer jsonBuffer;
    JsonObject& deviceConfig = jsonBuffer.createObject();
    _haGetDeviceConfig(deviceConfig);


    _haSendSwitches(deviceConfig);
    #if SENSOR_SUPPORT
        _haSendMagnitudes(deviceConfig);
    #endif

    jsonBuffer.clear();
    _haSendFlag = false;

}

void _haConfigure() {
    bool enabled = getSetting("haEnabled", HOMEASSISTANT_ENABLED).toInt() == 1;
    _haSendFlag = (enabled != _haEnabled);
    _haEnabled = enabled;
    _haSend();
}

#if WEB_SUPPORT

std::queue<uint32_t> _ha_send_config;

bool _haWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "ha", 2) == 0);
}

void _haWebSocketOnSend(JsonObject& root) {
    root["haVisible"] = 1;
    root["haPrefix"] = getSetting("haPrefix", HOMEASSISTANT_PREFIX);
    root["haEnabled"] = getSetting("haEnabled", HOMEASSISTANT_ENABLED).toInt() == 1;
}

void _haWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "haconfig") == 0) {
        _ha_send_config.push(client_id);
    }
}

#endif

#if TERMINAL_SUPPORT

void _haInitCommands() {

    terminalRegisterCommand(F("HA.CONFIG"), [](Embedis* e) {
        _haDumpConfig([](String& data) {
            DEBUG_MSG(data.c_str());
        });
        DEBUG_MSG("\n");
        terminalOK();
    });

    terminalRegisterCommand(F("HA.SEND"), [](Embedis* e) {
        setSetting("haEnabled", "1");
        _haConfigure();
        #if WEB_SUPPORT
            wsSend(_haWebSocketOnSend);
        #endif
        terminalOK();
    });

    terminalRegisterCommand(F("HA.CLEAR"), [](Embedis* e) {
        setSetting("haEnabled", "0");
        _haConfigure();
        #if WEB_SUPPORT
            wsSend(_haWebSocketOnSend);
        #endif
        terminalOK();
    });

}

#endif



#if WEB_SUPPORT
void _haLoop() {
    if (_ha_send_config.empty()) return;

    uint32_t client_id = _ha_send_config.front();
    _ha_send_config.pop();

    if (!wsConnected(client_id)) return;


    _haDumpConfig([client_id](String& output) {
        wsSend(client_id, output.c_str());
        yield();
    }, true);
}
#endif

void haSetup() {

    _haConfigure();

    #if WEB_SUPPORT
        wsOnSendRegister(_haWebSocketOnSend);
        wsOnActionRegister(_haWebSocketOnAction);
        wsOnReceiveRegister(_haWebSocketOnReceive);
        espurnaRegisterLoop(_haLoop);
    #endif

    #if TERMINAL_SUPPORT
        _haInitCommands();
    #endif


    mqttRegister([](unsigned int type, const char * topic, const char * payload) {
        if (type == MQTT_CONNECT_EVENT) _haSend();
    });


    espurnaRegisterReload(_haConfigure);

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/i2c.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/i2c.ino"
#if I2C_SUPPORT

unsigned int _i2c_locked[16] = {0};

#if I2C_USE_BRZO
#include "brzo_i2c.h"
unsigned long _i2c_scl_frequency = 0;
#else
#include "Wire.h"
#endif





int _i2cClearbus(int sda, int scl) {

    #if defined(TWCR) && defined(TWEN)

        TWCR &= ~(_BV(TWEN));
    #endif


    pinMode(sda, INPUT_PULLUP);
    pinMode(scl, INPUT_PULLUP);

    nice_delay(2500);
# 44 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/i2c.ino"
    boolean scl_low = (digitalRead(scl) == LOW);
    if (scl_low) return 1;


    boolean sda_low = (digitalRead(sda) == LOW);
    int clockCount = 20;


    while (sda_low && (clockCount > 0)) {

        clockCount--;


        pinMode(scl, INPUT);
        pinMode(scl, OUTPUT);
        delayMicroseconds(10);
        pinMode(scl, INPUT);
        pinMode(scl, INPUT_PULLUP);


        delayMicroseconds(10);


        scl_low = (digitalRead(scl) == LOW);
        int counter = 20;
        while (scl_low && (counter > 0)) {
            counter--;
            nice_delay(100);
            scl_low = (digitalRead(scl) == LOW);
        }



        if (scl_low) return 2;

        sda_low = (digitalRead(sda) == LOW);

    }



    if (sda_low) return 3;


    pinMode(sda, INPUT);
    pinMode(sda, OUTPUT);




    delayMicroseconds(10);
    pinMode(sda, INPUT);
    pinMode(sda, INPUT_PULLUP);

    delayMicroseconds(10);
    pinMode(sda, INPUT);
    pinMode(scl, INPUT);


    return 0;

}





#if I2C_USE_BRZO

void i2c_wakeup(uint8_t address) {
    brzo_i2c_start_transaction(_address, _i2c_scl_frequency);
    brzo_i2c_end_transaction();
}

uint8_t i2c_write_uint8(uint8_t address, uint8_t value) {
    uint8_t buffer[1] = {value};
    brzo_i2c_start_transaction(_address, _i2c_scl_frequency);
    brzo_i2c_write_uint8(buffer, 1, false);
    return brzo_i2c_end_transaction();
}

uint8_t i2c_write_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    brzo_i2c_start_transaction(_address, _i2c_scl_frequency);
    brzo_i2c_write_uint8(buffer, len, false);
    return brzo_i2c_end_transaction();
}

uint8_t i2c_read_uint8(uint8_t address) {
    uint8_t buffer[1] = {reg};
    brzo_i2c_start_transaction(_address, _i2c_scl_frequency);
    brzo_i2c_read(buffer, 1, false);
    brzo_i2c_end_transaction();
    return buffer[0];
};

uint8_t i2c_read_uint8(uint8_t address, uint8_t reg) {
    uint8_t buffer[1] = {reg};
    brzo_i2c_start_transaction(_address, _i2c_scl_frequency);
    brzo_i2c_write_uint8(buffer, 1, false);
    brzo_i2c_read(buffer, 1, false);
    brzo_i2c_end_transaction();
    return buffer[0];
};

uint16_t i2c_read_uint16(uint8_t address) {
    uint8_t buffer[2] = {reg, 0};
    brzo_i2c_start_transaction(_address, _i2c_scl_frequency);
    brzo_i2c_read(buffer, 2, false);
    brzo_i2c_end_transaction();
    return (buffer[0] * 256) | buffer[1];
};

uint16_t i2c_read_uint16(uint8_t address, uint8_t reg) {
    uint8_t buffer[2] = {reg, 0};
    brzo_i2c_start_transaction(_address, _i2c_scl_frequency);
    brzo_i2c_write_uint8(buffer, 1, false);
    brzo_i2c_read(buffer, 2, false);
    brzo_i2c_end_transaction();
    return (buffer[0] * 256) | buffer[1];
};

void i2c_read_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    brzo_i2c_start_transaction(address, _i2c_scl_frequency);
    brzo_i2c_read(buffer, len, false);
    brzo_i2c_end_transaction();
}

#else

void i2c_wakeup(uint8_t address) {
    Wire.beginTransmission((uint8_t) address);
    Wire.endTransmission();
}

uint8_t i2c_write_uint8(uint8_t address, uint8_t value) {
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) value);
    return Wire.endTransmission();
}

uint8_t i2c_write_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    Wire.beginTransmission((uint8_t) address);
    Wire.write(buffer, len);
    return Wire.endTransmission();
}

uint8_t i2c_read_uint8(uint8_t address) {
    uint8_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.requestFrom((uint8_t) address, (uint8_t) 1);
    value = Wire.read();
    Wire.endTransmission();
    return value;
};

uint8_t i2c_read_uint8(uint8_t address, uint8_t reg) {
    uint8_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) address, (uint8_t) 1);
    value = Wire.read();
    Wire.endTransmission();
    return value;
};

uint16_t i2c_read_uint16(uint8_t address) {
    uint16_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.requestFrom((uint8_t) address, (uint8_t) 2);
    value = (Wire.read() * 256) | Wire.read();
    Wire.endTransmission();
    return value;
};

uint16_t i2c_read_uint16(uint8_t address, uint8_t reg) {
    uint16_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) address, (uint8_t) 2);
    value = (Wire.read() * 256) | Wire.read();
    Wire.endTransmission();
    return value;
};

void i2c_read_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    Wire.beginTransmission((uint8_t) address);
    Wire.requestFrom(address, (uint8_t) len);
    for (size_t i=0; i<len; i++) buffer[i] = Wire.read();
    Wire.endTransmission();
}

#endif

uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    return i2c_write_buffer(address, buffer, 2);
}

uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value1, uint8_t value2) {
    uint8_t buffer[3] = {reg, value1, value2};
    return i2c_write_buffer(address, buffer, 3);
}

uint8_t i2c_write_uint16(uint8_t address, uint8_t reg, uint16_t value) {
    uint8_t buffer[3];
    buffer[0] = reg;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 0) & 0xFF;
    return i2c_write_buffer(address, buffer, 3);
}

uint8_t i2c_write_uint16(uint8_t address, uint16_t value) {
    uint8_t buffer[2];
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = (value >> 0) & 0xFF;
    return i2c_write_buffer(address, buffer, 2);
}

uint16_t i2c_read_uint16_le(uint8_t address, uint8_t reg) {
    uint16_t temp = i2c_read_uint16(address, reg);
    return (temp / 256) | (temp * 256);
};

int16_t i2c_read_int16(uint8_t address, uint8_t reg) {
    return (int16_t) i2c_read_uint16(address, reg);
};

int16_t i2c_read_int16_le(uint8_t address, uint8_t reg) {
    return (int16_t) i2c_read_uint16_le(address, reg);
};





void i2cClearBus() {
    unsigned char sda = getSetting("i2cSDA", I2C_SDA_PIN).toInt();
    unsigned char scl = getSetting("i2cSCL", I2C_SCL_PIN).toInt();
    DEBUG_MSG_P(PSTR("[I2C] Clear bus (response: %d)\n"), _i2cClearbus(sda, scl));
}

bool i2cCheck(unsigned char address) {
    #if I2C_USE_BRZO
        brzo_i2c_start_transaction(address, _i2c_scl_frequency);
        brzo_i2c_ACK_polling(1000);
        return brzo_i2c_end_transaction();
    #else
        Wire.beginTransmission(address);
        return Wire.endTransmission();
    #endif
}

bool i2cGetLock(unsigned char address) {
    unsigned char index = address / 8;
    unsigned char mask = 1 << (address % 8);
    if (_i2c_locked[index] & mask) return false;
    _i2c_locked[index] = _i2c_locked[index] | mask;
    DEBUG_MSG_P(PSTR("[I2C] Address 0x%02X locked\n"), address);
    return true;
}

bool i2cReleaseLock(unsigned char address) {
    unsigned char index = address / 8;
    unsigned char mask = 1 << (address % 8);
    if (_i2c_locked[index] & mask) {
        _i2c_locked[index] = _i2c_locked[index] & ~mask;
        return true;
    }
    return false;
}

unsigned char i2cFind(size_t size, unsigned char * addresses, unsigned char &start) {
    for (unsigned char i=start; i<size; i++) {
        if (i2cCheck(addresses[i]) == 0) {
            start = i;
            return addresses[i];
        }
    }
    return 0;
}

unsigned char i2cFind(size_t size, unsigned char * addresses) {
    unsigned char start = 0;
    return i2cFind(size, addresses, start);
}

unsigned char i2cFindAndLock(size_t size, unsigned char * addresses) {
    unsigned char start = 0;
    unsigned char address = 0;
    while ((address = i2cFind(size, addresses, start))) {
        if (i2cGetLock(address)) break;
        start++;
    }
    return address;
}

void i2cScan() {
    unsigned char nDevices = 0;
    for (unsigned char address = 1; address < 127; address++) {
        unsigned char error = i2cCheck(address);
        if (error == 0) {
            DEBUG_MSG_P(PSTR("[I2C] Device found at address 0x%02X\n"), address);
            nDevices++;
        }
    }
    if (nDevices == 0) DEBUG_MSG_P(PSTR("[I2C] No devices found\n"));
}

void i2cCommands() {

    terminalRegisterCommand(F("I2C.SCAN"), [](Embedis* e) {
        i2cScan();
        terminalOK();
    });

    terminalRegisterCommand(F("I2C.CLEAR"), [](Embedis* e) {
        i2cClearBus();
        terminalOK();
    });

}

void i2cSetup() {

    unsigned char sda = getSetting("i2cSDA", I2C_SDA_PIN).toInt();
    unsigned char scl = getSetting("i2cSCL", I2C_SCL_PIN).toInt();

    #if I2C_USE_BRZO
        unsigned long cst = getSetting("i2cCST", I2C_CLOCK_STRETCH_TIME).toInt();
        _i2c_scl_frequency = getSetting("i2cFreq", I2C_SCL_FREQUENCY).toInt();
        brzo_i2c_setup(sda, scl, cst);
    #else
        Wire.begin(sda, scl);
    #endif

    DEBUG_MSG_P(PSTR("[I2C] Using GPIO%u for SDA and GPIO%u for SCL\n"), sda, scl);

    #if TERMINAL_SUPPORT
        i2cCommands();
    #endif

    #if I2C_CLEAR_BUS
        i2cClearBus();
    #endif

    #if I2C_PERFORM_SCAN
        i2cScan();
    #endif

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/influxdb.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/influxdb.ino"
#if INFLUXDB_SUPPORT

#include "ESPAsyncTCP.h"

#include "libs/SyncClientWrap.h"

bool _idb_enabled = false;
SyncClientWrap * _idb_client;



bool _idbWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "idb", 3) == 0);
}

void _idbWebSocketOnSend(JsonObject& root) {
    root["idbVisible"] = 1;
    root["idbEnabled"] = getSetting("idbEnabled", INFLUXDB_ENABLED).toInt() == 1;
    root["idbHost"] = getSetting("idbHost", INFLUXDB_HOST);
    root["idbPort"] = getSetting("idbPort", INFLUXDB_PORT).toInt();
    root["idbDatabase"] = getSetting("idbDatabase", INFLUXDB_DATABASE);
    root["idbUsername"] = getSetting("idbUsername", INFLUXDB_USERNAME);
    root["idbPassword"] = getSetting("idbPassword", INFLUXDB_PASSWORD);
}

void _idbConfigure() {
    _idb_enabled = getSetting("idbEnabled", INFLUXDB_ENABLED).toInt() == 1;
    if (_idb_enabled && (getSetting("idbHost", INFLUXDB_HOST).length() == 0)) {
        _idb_enabled = false;
        setSetting("idbEnabled", 0);
    }
}

#if BROKER_SUPPORT
void _idbBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload) {


    if ((BROKER_MSG_TYPE_STATUS == type) || (BROKER_MSG_TYPE_SENSOR == type)) {
        idbSend(topic, id, (char *) payload);
    }

}
#endif



bool idbSend(const char * topic, const char * payload) {

    if (!_idb_enabled) return true;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return true;

    String h = getSetting("idbHost", INFLUXDB_HOST);
    #if MDNS_CLIENT_SUPPORT
        h = mdnsResolve(h);
    #endif
    char * host = strdup(h.c_str());
    unsigned int port = getSetting("idbPort", INFLUXDB_PORT).toInt();
    DEBUG_MSG_P(PSTR("[INFLUXDB] Sending to %s:%u\n"), host, port);

    bool success = false;

    _idb_client->setTimeout(2);
    if (_idb_client->connect((const char *) host, (unsigned int) port)) {

        char data[128];
        snprintf(data, sizeof(data), "%s,device=%s value=%s", topic, getSetting("hostname").c_str(), String(payload).c_str());
        DEBUG_MSG_P(PSTR("[INFLUXDB] Data: %s\n"), data);

        char request[256];
        snprintf(request, sizeof(request), "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: %s:%u\r\nContent-Length: %d\r\n\r\n%s",
            getSetting("idbDatabase", INFLUXDB_DATABASE).c_str(),
            getSetting("idbUsername", INFLUXDB_USERNAME).c_str(), getSetting("idbPassword", INFLUXDB_PASSWORD).c_str(),
            host, port, strlen(data), data);

        if (_idb_client->printf(request) > 0) {
            while (_idb_client->connected() && _idb_client->available() == 0) delay(1);
            while (_idb_client->available()) _idb_client->read();
            if (_idb_client->connected()) _idb_client->stop();
            success = true;
        } else {
            DEBUG_MSG_P(PSTR("[INFLUXDB] Sent failed\n"));
        }

        _idb_client->stop();
        while (_idb_client->connected()) yield();

    } else {
        DEBUG_MSG_P(PSTR("[INFLUXDB] Connection failed\n"));
    }

    free(host);
    return success;

}

bool idbSend(const char * topic, unsigned char id, const char * payload) {
    char measurement[64];
    snprintf(measurement, sizeof(measurement), "%s,id=%d", topic, id);
    return idbSend(measurement, payload);
}

bool idbEnabled() {
    return _idb_enabled;
}

void idbSetup() {

    _idb_client = new SyncClientWrap();

    _idbConfigure();

    #if WEB_SUPPORT
        wsOnSendRegister(_idbWebSocketOnSend);
        wsOnReceiveRegister(_idbWebSocketOnReceive);
    #endif

    #if BROKER_SUPPORT
        brokerRegister(_idbBrokerCallback);
    #endif


    espurnaRegisterReload(_idbConfigure);

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ir.ino"
# 49 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ir.ino"
#if IR_SUPPORT

#include <IRremoteESP8266.h>

#if defined(IR_RX_PIN)

    #include <IRrecv.h>
    IRrecv _ir_receiver(IR_RX_PIN, IR_BUFFER_SIZE, IR_TIMEOUT, true);

    decode_results _ir_results;

#endif

#if defined(IR_TX_PIN)

    #include <IRsend.h>
    IRsend _ir_sender(IR_TX_PIN);

    #if IR_USE_RAW
        uint16_t _ir_freq = 38;
        uint8_t _ir_repeat_size = 0;
        uint16_t * _ir_raw;
    #else
        uint8_t _ir_type = 0;
        uint64_t _ir_code = 0;
        uint16_t _ir_bits = 0;
    #endif

    uint8_t _ir_repeat = 0;
    uint32_t _ir_delay = IR_DELAY;

#endif


#if MQTT_SUPPORT && defined(IR_TX_PIN)

void _irMqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_IROUT);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        String t = mqttMagnitude((char *) topic);


        if (t.equals(MQTT_TOPIC_IROUT)) {

            String data = String(payload);
            unsigned int len = data.length();
            int col = data.indexOf(":");

            #if IR_USE_RAW

                unsigned char count = 1;

                if (col > 2) {

                    _ir_repeat_size = 1;


                    for(unsigned int i = col+1; i < len; i++) {
                        if (i < len-1) {
                            if ( payload[i] == ',' && isDigit(payload[i+1]) && i>0 ) {
                                _ir_repeat_size++;
                            } else if (!isDigit(payload[i])) {



                                DEBUG_MSG_P(PSTR("[IR] Error in repeat code.\n"));
                                return;
                            }
                        }
                    }

                    len = col;

                }


                for(unsigned int i = 0; i < len; i++) {
                    if (i<len-1) {
                        if ( payload[i] == ',' && isDigit(payload[i+1]) && i>0 ) {
                            count++;
                        } else if (!isDigit(payload[i])) {



                            DEBUG_MSG_P(PSTR("[IR] Error in main code.\n"));
                            return;
                        }
                    }

                }

                _ir_raw = (uint16_t*)calloc(count, sizeof(uint16_t));
                String value = "";
                int j = 0;


                for (unsigned int i = 0; i < len; i++) {
                    if (payload[i] != ',') {
                        value = value + data[i];
                    }
                    if ((payload[i] == ',') || (i == len - 1)) {
                        _ir_raw[j]= value.toInt();
                        value = "";
                        j++;
                    }
                }


                _ir_repeat=0;
                if (count>3) {
                    if (_ir_raw[count-2] <= 120) {
                        _ir_freq = _ir_raw[count-1];
                        _ir_repeat = _ir_raw[count-2];
                        _ir_delay = _ir_raw[count-3];
                        count = count - 3;
                    }
                }

                DEBUG_MSG_P(PSTR("[IR] Raw IR output %d codes, repeat %d times on %d(k)Hz freq.\n"), count, _ir_repeat, _ir_freq);

                #if defined(IR_RX_PIN)
                    _ir_receiver.disableIRIn();
                #endif
                _ir_sender.sendRaw(_ir_raw, count, _ir_freq);

                if (_ir_repeat==0) {
                    free(_ir_raw);
                    #if defined(IR_RX_PIN)
                        _ir_receiver.enableIRIn();
                    #endif
                } else if (col>2) {

                    DEBUG_MSG_P(PSTR("[IR] Repeat codes count: %d\n"), _ir_repeat_size);

                    free(_ir_raw);
                    _ir_raw = (uint16_t*)calloc(_ir_repeat_size, sizeof(uint16_t));

                    String value = "";
                    int j = 0;
                    len = data.length();


                    for (unsigned int i = col+1; i < len; i++) {
                        value = value + data[i];
                        if ((payload[i] == ',') || (i == len - 1)) {
                            _ir_raw[j]= value.toInt();
                            value = "";
                            j++;
                        }
                    }
                } else {
                    _ir_repeat_size = count;
                }

            #else

                _ir_repeat = 0;

                if (col > 0) {

                    _ir_type = data.toInt();
                    _ir_code = strtoul(data.substring(col+1).c_str(), NULL, 10);

                    col = data.indexOf(":", col+1);
                    if (col > 0) {
                        _ir_bits = data.substring(col+1).toInt();
                        col = data.indexOf(":", col+1);
                        if (col > 2) {
                            _ir_repeat = data.substring(col+1).toInt();
                        } else {
                            _ir_repeat = IR_REPEAT;
                        }
                    }
                }

                if (_ir_repeat > 0) {
                    DEBUG_MSG_P(PSTR("[IR] IROUT: %d:%lu:%d:%d\n"), _ir_type, (unsigned long) _ir_code, _ir_bits, _ir_repeat);
                } else {
                    DEBUG_MSG_P(PSTR("[IR] Wrong MQTT payload format (%s)\n"), payload);
                }

            #endif

        }

    }

}

void _irTXLoop() {

    static uint32_t last = 0;
    if ((_ir_repeat > 0) && (millis() - last > _ir_delay)) {
        last = millis();


        #if IR_USE_RAW
            _ir_sender.sendRaw(_ir_raw, _ir_repeat_size, _ir_freq);
        #else
            _ir_sender.send(_ir_type, _ir_code, _ir_bits);
        #endif


        --_ir_repeat;
        if (0 == _ir_repeat) {
            #if IR_USE_RAW
                free(_ir_raw);
            #endif
            #if defined(IR_RX_PIN)
                _ir_receiver.enableIRIn();
            #endif
        }

    }

}

#endif



#if defined(IR_RX_PIN)

void _irProcess(unsigned char type, unsigned long code) {

    #if IR_BUTTON_SET > 0

        boolean found = false;

        for (unsigned char i = 0; i < IR_BUTTON_COUNT ; i++) {

            uint32_t button_code = pgm_read_dword(&IR_BUTTON[i][0]);
            if (code == button_code) {

                unsigned long button_mode = pgm_read_dword(&IR_BUTTON[i][1]);
                unsigned long button_value = pgm_read_dword(&IR_BUTTON[i][2]);

                if (button_mode == IR_BUTTON_MODE_STATE) {
                    relayStatus(0, button_value);
                }

                if (button_mode == IR_BUTTON_MODE_TOGGLE) {
                    relayToggle(button_value);
                }

                #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

                    if (button_mode == IR_BUTTON_MODE_BRIGHTER) {
                        lightBrightnessStep(button_value ? 1 : -1);
                        nice_delay(150);
                    }

                    if (button_mode == IR_BUTTON_MODE_RGB) {
                        lightColor(button_value);
                    }
# 323 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ir.ino"
                    lightUpdate(true, true);

                #endif

                found = true;
                break;

      }

     }

     if (!found) {
      DEBUG_MSG_P(PSTR("[IR] Code does not match any action\n"));
     }

    #endif

}

void _irRXLoop() {

    if (_ir_receiver.decode(&_ir_results)) {

        _ir_receiver.resume();


        static unsigned long last_time = 0;
        if (millis() - last_time < IR_DEBOUNCE) return;
        last_time = millis();

        #if IR_USE_RAW

            if (_ir_results.rawlen < 1) return;
            char * payload;
            String value = "";
            for (int i = 1; i < _ir_results.rawlen; i++) {
                if (i>1) value = value + ",";
                value = value + String(_ir_results.rawbuf[i] * RAWTICK);
            }
            payload = const_cast<char*>(value.c_str());
        #else

            if (_ir_results.value < 1) return;
            if (_ir_results.decode_type < 1) return;
            if (_ir_results.bits < 1) return;
            char payload[32];
            snprintf_P(payload, sizeof(payload), PSTR("%u:%lu:%u"), _ir_results.decode_type, (unsigned long) _ir_results.value, _ir_results.bits);
        #endif

        DEBUG_MSG_P(PSTR("[IR] IRIN: %s\n"), payload);

        #if not IR_USE_RAW
            _irProcess(_ir_results.decode_type, (unsigned long) _ir_results.value);
        #endif

        #if MQTT_SUPPORT
            if (strlen(payload)>0) {
                mqttSend(MQTT_TOPIC_IRIN, (const char *) payload);
            }
        #endif

    }

}

#endif



void _irLoop() {
    #if defined(IR_RX_PIN)
        _irRXLoop();
    #endif
    #if MQTT_SUPPORT && defined(IR_TX_PIN)
        _irTXLoop();
    #endif
}

void irSetup() {

    #if defined(IR_RX_PIN)
        _ir_receiver.enableIRIn();
        DEBUG_MSG_P(PSTR("[IR] Receiver initialized \n"));
    #endif

    #if MQTT_SUPPORT && defined(IR_TX_PIN)
        _ir_sender.begin();
        mqttRegister(_irMqttCallback);
        DEBUG_MSG_P(PSTR("[IR] Transmitter initialized \n"));
    #endif

    espurnaRegisterLoop(_irLoop);

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/led.ino"
# 13 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/led.ino"
#if LED_SUPPORT

typedef struct {
    unsigned char pin;
    bool reverse;
    unsigned char mode;
    unsigned char relay;
} led_t;

std::vector<led_t> _leds;
bool _led_update = false;



bool _ledStatus(unsigned char id) {
    if (id >= _ledCount()) return false;
    bool status = digitalRead(_leds[id].pin);
    return _leds[id].reverse ? !status : status;
}

bool _ledStatus(unsigned char id, bool status) {
    if (id >=_ledCount()) return false;
    digitalWrite(_leds[id].pin, _leds[id].reverse ? !status : status);
    return status;
}

bool _ledToggle(unsigned char id) {
    if (id >= _ledCount()) return false;
    return _ledStatus(id, !_ledStatus(id));
}

unsigned char _ledMode(unsigned char id) {
    if (id >= _ledCount()) return false;
    return _leds[id].mode;
}

void _ledMode(unsigned char id, unsigned char mode) {
    if (id >= _ledCount()) return;
    _leds[id].mode = mode;
}

unsigned char _ledRelay(unsigned char id) {
    if (id >= _ledCount()) return false;
    return _leds[id].relay;
}

void _ledRelay(unsigned char id, unsigned char relay) {
    if (id >= _ledCount()) return;
    _leds[id].relay = relay;
}

void _ledBlink(unsigned char id, unsigned long delayOff, unsigned long delayOn) {
    if (id >= _ledCount()) return;
    static unsigned long next = millis();
    if (next < millis()) {
        next += (_ledToggle(id) ? delayOn : delayOff);
    }
}

#if WEB_SUPPORT

bool _ledWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "led", 3) == 0);
}

void _ledWebSocketOnSend(JsonObject& root) {
    if (_ledCount() == 0) return;
    root["ledVisible"] = 1;
    JsonArray& leds = root.createNestedArray("ledConfig");
    for (byte i=0; i<_ledCount(); i++) {
        JsonObject& led = leds.createNestedObject();
        led["mode"] = getSetting("ledMode", i, "").toInt();
        led["relay"] = getSetting("ledRelay", i, "").toInt();
    }
}

#endif

#if BROKER_SUPPORT
void _ledBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload) {


    if (BROKER_MSG_TYPE_STATUS != type) return;

    if (strcmp(MQTT_TOPIC_RELAY, topic) == 0) {
        ledUpdate(true);
    }

}
#endif

#if MQTT_SUPPORT
void _ledMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        char buffer[strlen(MQTT_TOPIC_LED) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_LED);
        mqttSubscribe(buffer);
    }

    if (type == MQTT_MESSAGE_EVENT) {


        String t = mqttMagnitude((char *) topic);
        if (!t.startsWith(MQTT_TOPIC_LED)) return;


        unsigned int ledID = t.substring(strlen(MQTT_TOPIC_LED)+1).toInt();
        if (ledID >= _ledCount()) {
            DEBUG_MSG_P(PSTR("[LED] Wrong ledID (%d)\n"), ledID);
            return;
        }


        if (_ledMode(ledID) != LED_MODE_MQTT) return;


        unsigned char value = relayParsePayload(payload);


        if (value == 2) {
            _ledToggle(ledID);
        } else {
            _ledStatus(ledID, value == 1);
        }

    }

}
#endif

unsigned char _ledCount() {
    return _leds.size();
}

void _ledConfigure() {
    for (unsigned int i=0; i < _leds.size(); i++) {
        _ledMode(i, getSetting("ledMode", i, _ledMode(i)).toInt());
        _ledRelay(i, getSetting("ledRelay", i, _ledRelay(i)).toInt());
    }
    _led_update = true;
}



void ledUpdate(bool value) {
    _led_update = value;
}

void ledSetup() {

    #if LED1_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED1_PIN, LED1_PIN_INVERSE, LED1_MODE, LED1_RELAY - 1 });
    #endif
    #if LED2_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED2_PIN, LED2_PIN_INVERSE, LED2_MODE, LED2_RELAY - 1 });
    #endif
    #if LED3_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED3_PIN, LED3_PIN_INVERSE, LED3_MODE, LED3_RELAY - 1 });
    #endif
    #if LED4_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED4_PIN, LED4_PIN_INVERSE, LED4_MODE, LED4_RELAY - 1 });
    #endif
    #if LED5_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED5_PIN, LED5_PIN_INVERSE, LED5_MODE, LED5_RELAY - 1 });
    #endif
    #if LED6_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED6_PIN, LED6_PIN_INVERSE, LED6_MODE, LED6_RELAY - 1 });
    #endif
    #if LED7_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED7_PIN, LED7_PIN_INVERSE, LED7_MODE, LED7_RELAY - 1 });
    #endif
    #if LED8_PIN != GPIO_NONE
        _leds.push_back((led_t) { LED8_PIN, LED8_PIN_INVERSE, LED8_MODE, LED8_RELAY - 1 });
    #endif

    for (unsigned int i=0; i < _leds.size(); i++) {
        if (!hasSetting("ledMode", i)) setSetting("ledMode", i, _leds[i].mode);
        if (!hasSetting("ledRelay", i)) setSetting("ledRelay", i, _leds[i].relay);
        pinMode(_leds[i].pin, OUTPUT);
        _ledStatus(i, false);
    }

    _ledConfigure();

    #if MQTT_SUPPORT
        mqttRegister(_ledMQTTCallback);
    #endif

    #if WEB_SUPPORT
        wsOnSendRegister(_ledWebSocketOnSend);
        wsOnReceiveRegister(_ledWebSocketOnReceive);
    #endif

    #if BROKER_SUPPORT
        brokerRegister(_ledBrokerCallback);
    #endif


    DEBUG_MSG_P(PSTR("[LED] Number of leds: %d\n"), _leds.size());


    espurnaRegisterLoop(ledLoop);
    espurnaRegisterReload(_ledConfigure);


}

void ledLoop() {

    uint8_t wifi_state = wifiState();

    for (unsigned char i=0; i<_leds.size(); i++) {

        if (_ledMode(i) == LED_MODE_WIFI) {

            if (wifi_state & WIFI_STATE_WPS || wifi_state & WIFI_STATE_SMARTCONFIG) {
                _ledBlink(i, 100, 100);
            } else if (wifi_state & WIFI_STATE_STA) {
                _ledBlink(i, 4900, 100);
            } else if (wifi_state & WIFI_STATE_AP) {
                _ledBlink(i, 900, 100);
            } else {
                _ledBlink(i, 500, 500);
            }

        }

        if (_ledMode(i) == LED_MODE_FINDME_WIFI) {

            if (wifi_state & WIFI_STATE_WPS || wifi_state & WIFI_STATE_SMARTCONFIG) {
                _ledBlink(i, 100, 100);
            } else if (wifi_state & WIFI_STATE_STA) {
                if (relayStatus(_leds[i].relay)) {
                    _ledBlink(i, 4900, 100);
                } else {
                    _ledBlink(i, 100, 4900);
                }
            } else if (wifi_state & WIFI_STATE_AP) {
                if (relayStatus(_leds[i].relay)) {
                    _ledBlink(i, 900, 100);
                } else {
                    _ledBlink(i, 100, 900);
                }
            } else {
                _ledBlink(i, 500, 500);
            }

        }

        if (_ledMode(i) == LED_MODE_RELAY_WIFI) {

            if (wifi_state & WIFI_STATE_WPS || wifi_state & WIFI_STATE_SMARTCONFIG) {
                _ledBlink(i, 100, 100);
            } else if (wifi_state & WIFI_STATE_STA) {
                if (relayStatus(_leds[i].relay)) {
                    _ledBlink(i, 100, 4900);
                } else {
                    _ledBlink(i, 4900, 100);
                }
            } else if (wifi_state & WIFI_STATE_AP) {
                if (relayStatus(_leds[i].relay)) {
                    _ledBlink(i, 100, 900);
                } else {
                    _ledBlink(i, 900, 100);
                }
            } else {
                _ledBlink(i, 500, 500);
            }

        }


        if (!_led_update) continue;

        if (_ledMode(i) == LED_MODE_FOLLOW) {
            _ledStatus(i, relayStatus(_leds[i].relay));
        }

        if (_ledMode(i) == LED_MODE_FOLLOW_INVERSE) {
            _ledStatus(i, !relayStatus(_leds[i].relay));
        }

        if (_ledMode(i) == LED_MODE_FINDME) {
            bool status = true;
            for (unsigned char k=0; k<relayCount(); k++) {
                if (relayStatus(k)) {
                    status = false;
                    break;
                }
            }
            _ledStatus(i, status);
        }

        if (_ledMode(i) == LED_MODE_RELAY) {
            bool status = false;
            for (unsigned char k=0; k<relayCount(); k++) {
                if (relayStatus(k)) {
                    status = true;
                    break;
                }
            }
            _ledStatus(i, status);
        }

        if (_ledMode(i) == LED_MODE_ON) {
            _ledStatus(i, true);
        }

        if (_ledMode(i) == LED_MODE_OFF) {
            _ledStatus(i, false);
        }

    }

    _led_update = false;

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/light.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/light.ino"
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#include <Ticker.h>
#include <ArduinoJson.h>
#include <vector>

extern "C" {
    #include "libs/fs_math.h"
}

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
#define PWM_CHANNEL_NUM_MAX LIGHT_CHANNELS
extern "C" {
    #include "libs/pwm.h"
}
#endif



Ticker _light_comms_ticker;
Ticker _light_save_ticker;
Ticker _light_transition_ticker;

typedef struct {
    unsigned char pin;
    bool reverse;
    bool state;
    unsigned char inputValue;
    unsigned char value;
    unsigned char target;
    double current;
} channel_t;
std::vector<channel_t> _light_channel;

bool _light_state = false;
bool _light_use_transitions = false;
unsigned int _light_transition_time = LIGHT_TRANSITION_TIME;
bool _light_has_color = false;
bool _light_use_white = false;
bool _light_use_cct = false;
bool _light_use_gamma = false;
unsigned long _light_steps_left = 1;
unsigned char _light_brightness = LIGHT_MAX_BRIGHTNESS;
unsigned int _light_mireds = round((LIGHT_COLDWHITE_MIRED+LIGHT_WARMWHITE_MIRED)/2);

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
#include <my92xx.h>
my92xx * _my92xx;
ARRAYINIT(unsigned char, _light_channel_map, MY92XX_MAPPING);
#endif



const unsigned char _light_gamma_table[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6,
    6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11, 11,
    12, 12, 13, 13, 14, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19,
    19, 20, 20, 21, 22, 22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 28,
    29, 30, 30, 31, 32, 33, 33, 34, 35, 35, 36, 37, 38, 39, 39, 40,
    41, 42, 43, 43, 44, 45, 46, 47, 48, 49, 50, 50, 51, 52, 53, 54,
    55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 71,
    72, 73, 74, 75, 76, 77, 78, 80, 81, 82, 83, 84, 86, 87, 88, 89,
    91, 92, 93, 94, 96, 97, 98, 100, 101, 102, 104, 105, 106, 108, 109, 110,
    112, 113, 115, 116, 118, 119, 121, 122, 123, 125, 126, 128, 130, 131, 133, 134,
    136, 137, 139, 140, 142, 144, 145, 147, 149, 150, 152, 154, 155, 157, 159, 160,
    162, 164, 166, 167, 169, 171, 173, 175, 176, 178, 180, 182, 184, 186, 187, 189,
    191, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
    223, 225, 227, 229, 231, 233, 235, 238, 240, 242, 244, 246, 248, 251, 253, 255
};





void _setRGBInputValue(unsigned char red, unsigned char green, unsigned char blue) {
    _light_channel[0].inputValue = constrain(red, 0, LIGHT_MAX_VALUE);
    _light_channel[1].inputValue = constrain(green, 0, LIGHT_MAX_VALUE);;
    _light_channel[2].inputValue = constrain(blue, 0, LIGHT_MAX_VALUE);;
}

void _setCCTInputValue(unsigned char warm, unsigned char cold) {
    _light_channel[0].inputValue = constrain(warm, 0, LIGHT_MAX_VALUE);
    _light_channel[1].inputValue = constrain(cold, 0, LIGHT_MAX_VALUE);
}

void _generateBrightness() {

    double brightness = (double) _light_brightness / LIGHT_MAX_BRIGHTNESS;


    if (_light_has_color && _light_use_white) {


        unsigned char white = std::min(_light_channel[0].inputValue, std::min(_light_channel[1].inputValue, _light_channel[2].inputValue));
        for (unsigned int i=0; i < 3; i++) {
            _light_channel[i].value = _light_channel[i].inputValue - white;
        }


        if (_light_use_cct) {


          double miredFactor = ((double) _light_mireds - (double) LIGHT_COLDWHITE_MIRED)/((double) LIGHT_WARMWHITE_MIRED - (double) LIGHT_COLDWHITE_MIRED);


          _light_channel[3].inputValue = 0;
          _light_channel[3].value = round(((double) 1.0 - miredFactor) * white);


          _light_channel[4].inputValue = 0;
          _light_channel[4].value = round(miredFactor * white);
        } else {
          _light_channel[3].inputValue = 0;
          _light_channel[3].value = white;
        }


        unsigned char max_in = std::max(_light_channel[0].inputValue, std::max(_light_channel[1].inputValue, _light_channel[2].inputValue));
        unsigned char max_out = std::max(std::max(_light_channel[0].value, _light_channel[1].value), std::max(_light_channel[2].value, _light_channel[3].value));
        unsigned char channelSize = _light_use_cct ? 5 : 4;

        if (_light_use_cct) {
          max_out = std::max(max_out, _light_channel[4].value);
        }

        double factor = (max_out > 0) ? (double) (max_in / max_out) : 0;
        for (unsigned char i=0; i < channelSize; i++) {
            _light_channel[i].value = round((double) _light_channel[i].value * factor * brightness);
        }


        for (unsigned char i=3; i < channelSize; i++) {
            _light_channel[i].value = constrain(_light_channel[i].value * LIGHT_WHITE_FACTOR, 0, LIGHT_MAX_BRIGHTNESS);
        }



        for (unsigned char i=channelSize; i < _light_channel.size(); i++) {
            _light_channel[i].value = _light_channel[i].inputValue;
        }

    } else {


        for (unsigned char i=0; i < _light_channel.size(); i++) {
            _light_channel[i].value = _light_channel[i].inputValue * brightness;
        }

    }

}





void _fromLong(unsigned long value, bool brightness) {
    if (brightness) {
        _setRGBInputValue((value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF);
        _light_brightness = (value & 0xFF) * LIGHT_MAX_BRIGHTNESS / 255;
    } else {
        _setRGBInputValue((value >> 16) & 0xFF, (value >> 8) & 0xFF, (value) & 0xFF);
    }
}

void _fromRGB(const char * rgb) {
    char * p = (char *) rgb;
    if (strlen(p) == 0) return;

    switch (p[0]) {
      case '#':
        if (_light_has_color) {
            ++p;
            unsigned long value = strtoul(p, NULL, 16);

            _fromLong(value, strlen(p) > 7);
        }
        break;
      case 'M':
          _fromMireds(atol(p + 1));
        break;
      case 'K':
          _fromKelvin(atol(p + 1));
        break;
      default:
        char * tok;
        unsigned char count = 0;
        unsigned char channels = _light_channel.size();

        tok = strtok(p, ",");
        while (tok != NULL) {
            _light_channel[count].inputValue = atoi(tok);
            if (++count == channels) break;
            tok = strtok(NULL, ",");
        }


        if (_light_has_color && (count < 3)) {

          for (int i = 1; i <= 2; i++) {
            if (count < (i+1)) {
              _light_channel[i].inputValue = 0;
            }
          }
        }
        break;
    }
}





void _fromHSV(const char * hsv) {

    char * ptr = (char *) hsv;
    if (strlen(ptr) == 0) return;
    if (!_light_has_color) return;

    char * tok;
    unsigned char count = 0;
    unsigned int value[3] = {0};

    tok = strtok(ptr, ",");
    while (tok != NULL) {
        value[count] = atoi(tok);
        if (++count == 3) break;
        tok = strtok(NULL, ",");
    }
    if (count != 3) return;







    double h = (value[0] == 360) ? 0 : (double) value[0] / 60.0;
    double f = (h - floor(h));
    double s = (double) value[1] / 100.0;

    _light_brightness = round((double) value[2] * 2.55);
    unsigned char p = round(255 * (1.0 - s));
    unsigned char q = round(255 * (1.0 - s * f));
    unsigned char t = round(255 * (1.0 - s * (1.0 - f)));

    switch (int(h)) {
        case 0:
            _setRGBInputValue(255, t, p);
            break;
        case 1:
            _setRGBInputValue(q, 255, p);
            break;
        case 2:
            _setRGBInputValue(p, 255, t);
            break;
        case 3:
            _setRGBInputValue(p, q, 255);
            break;
        case 4:
            _setRGBInputValue(t, p, 255);
            break;
        case 5:
            _setRGBInputValue(255, p, q);
            break;
        default:
            _setRGBInputValue(0, 0, 0);
            break;
    }
}



void _fromKelvin(unsigned long kelvin) {

    if (!_light_has_color) {

      if(!_light_use_cct) return;

      _light_mireds = constrain(round(1000000UL / kelvin), LIGHT_MIN_MIREDS, LIGHT_MAX_MIREDS);


      double factor = ((double) _light_mireds - (double) LIGHT_COLDWHITE_MIRED)/((double) LIGHT_WARMWHITE_MIRED - (double) LIGHT_COLDWHITE_MIRED);
      unsigned char warm = round(factor * LIGHT_MAX_VALUE);
      unsigned char cold = round(((double) 1.0 - factor) * LIGHT_MAX_VALUE);

      _setCCTInputValue(warm, cold);

      return;
    }

    _light_mireds = constrain(round(1000000UL / kelvin), LIGHT_MIN_MIREDS, LIGHT_MAX_MIREDS);

    if (_light_use_cct) {
      _setRGBInputValue(LIGHT_MAX_VALUE, LIGHT_MAX_VALUE, LIGHT_MAX_VALUE);
      return;
    }


    kelvin /= 100;
    unsigned int red = (kelvin <= 66)
        ? LIGHT_MAX_VALUE
        : 329.698727446 * fs_pow((double) (kelvin - 60), -0.1332047592);
    unsigned int green = (kelvin <= 66)
        ? 99.4708025861 * fs_log(kelvin) - 161.1195681661
        : 288.1221695283 * fs_pow((double) kelvin, -0.0755148492);
    unsigned int blue = (kelvin >= 66)
        ? LIGHT_MAX_VALUE
        : ((kelvin <= 19)
            ? 0
            : 138.5177312231 * fs_log(kelvin - 10) - 305.0447927307);

    _setRGBInputValue(red, green, blue);

}


void _fromMireds(unsigned long mireds) {
    unsigned long kelvin = constrain(1000000UL / mireds, 1000, 40000);
    _fromKelvin(kelvin);
}





void _toRGB(char * rgb, size_t len, bool target) {
    unsigned long value = 0;

    value += target ? _light_channel[0].target : _light_channel[0].inputValue;
    value <<= 8;
    value += target ? _light_channel[1].target : _light_channel[1].inputValue;
    value <<= 8;
    value += target ? _light_channel[2].target : _light_channel[2].inputValue;

    snprintf_P(rgb, len, PSTR("#%06X"), value);
}

void _toRGB(char * rgb, size_t len) {
    _toRGB(rgb, len, false);
}

void _toHSV(char * hsv, size_t len, bool target) {
    double h, s, v;
    double brightness = (double) _light_brightness / LIGHT_MAX_BRIGHTNESS;

    double r = (double) ((target ? _light_channel[0].target : _light_channel[0].inputValue) * brightness) / 255.0;
    double g = (double) ((target ? _light_channel[1].target : _light_channel[1].inputValue) * brightness) / 255.0;
    double b = (double) ((target ? _light_channel[2].target : _light_channel[2].inputValue) * brightness) / 255.0;

    double min = std::min(r, std::min(g, b));
    double max = std::max(r, std::max(g, b));

    v = 100.0 * max;
    if (v == 0) {
        h = s = 0;
    } else {
        s = 100.0 * (max - min) / max;
        if (s == 0) {
            h = 0;
        } else {
            if (max == r) {
                if (g >= b) {
                    h = 0.0 + 60.0 * (g - b) / (max - min);
                } else {
                    h = 360.0 + 60.0 * (g - b) / (max - min);
                }
            } else if (max == g) {
                h = 120.0 + 60.0 * (b - r) / (max - min);
            } else {
                h = 240.0 + 60.0 * (r - g) / (max - min);
            }
        }
    }


    snprintf_P(hsv, len, PSTR("%d,%d,%d"), round(h), round(s), round(v));
}

void _toHSV(char * hsv, size_t len) {
    _toHSV(hsv, len, false);
}

void _toLong(char * color, size_t len, bool target) {

    if (!_light_has_color) return;

    snprintf_P(color, len, PSTR("%d,%d,%d"),
        (int) (target ? _light_channel[0].target : _light_channel[0].inputValue),
        (int) (target ? _light_channel[1].target : _light_channel[1].inputValue),
        (int) (target ? _light_channel[2].target : _light_channel[2].inputValue)
    );

}

void _toLong(char * color, size_t len) {
    _toLong(color, len, false);
}

void _toCSV(char * buffer, size_t len, bool applyBrightness, bool target) {
    char num[10];
    float b = applyBrightness ? (float) _light_brightness / LIGHT_MAX_BRIGHTNESS : 1;
    for (unsigned char i=0; i<_light_channel.size(); i++) {
        itoa((target ? _light_channel[i].target : _light_channel[i].inputValue) * b, num, 10);
        if (i>0) strncat(buffer, ",", len--);
        strncat(buffer, num, len);
        len = len - strlen(num);
    }
}

void _toCSV(char * buffer, size_t len, bool applyBrightness) {
    _toCSV(buffer, len, applyBrightness, false);
}





unsigned int _toPWM(unsigned long value, bool gamma, bool reverse) {
    value = constrain(value, 0, LIGHT_MAX_VALUE);
    if (gamma) value = _light_gamma_table[value];
    if (LIGHT_MAX_VALUE != LIGHT_LIMIT_PWM) value = map(value, 0, LIGHT_MAX_VALUE, 0, LIGHT_LIMIT_PWM);
    if (reverse) value = LIGHT_LIMIT_PWM - value;
    return value;
}


unsigned int _toPWM(unsigned char id) {
    bool useGamma = _light_use_gamma && _light_has_color && (id < 3);
    return _toPWM(_light_channel[id].current, useGamma, _light_channel[id].reverse);
}

void _transition() {


    _light_steps_left--;
    if (_light_steps_left == 0) _light_transition_ticker.detach();


    for (unsigned int i=0; i < _light_channel.size(); i++) {

        if (_light_steps_left == 0) {
            _light_channel[i].current = _light_channel[i].target;
        } else {
            double difference = (double) (_light_channel[i].target - _light_channel[i].current) / (_light_steps_left + 1);
            _light_channel[i].current = _light_channel[i].current + difference;
        }

    }

}

void _lightProviderUpdate() {

    _transition();

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX

        for (unsigned char i=0; i<_light_channel.size(); i++) {
            _my92xx->setChannel(_light_channel_map[i], _toPWM(i));
        }
        _my92xx->setState(true);
        _my92xx->update();

    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

        for (unsigned int i=0; i < _light_channel.size(); i++) {
            pwm_set_duty(_toPWM(i), i);
        }
        pwm_start();

    #endif

}





union light_rtcmem_t {
    struct {
        uint8_t channels[5];
        uint8_t brightness;
        uint16_t mired;
    } packed;
    uint64_t value;
};

#define LIGHT_RTCMEM_CHANNELS_MAX sizeof(light_rtcmem_t().packed.channels)

void _lightSaveRtcmem() {
    if (lightChannels() > LIGHT_RTCMEM_CHANNELS_MAX) return;

    light_rtcmem_t light;

    for (unsigned int i=0; i < lightChannels(); i++) {
        light.packed.channels[i] = _light_channel[i].inputValue;
    }

    light.packed.brightness = _light_brightness;
    light.packed.mired = _light_mireds;

    Rtcmem->light = light.value;
}

void _lightRestoreRtcmem() {
    if (lightChannels() > LIGHT_RTCMEM_CHANNELS_MAX) return;

    light_rtcmem_t light;
    light.value = Rtcmem->light;

    for (unsigned int i=0; i < lightChannels(); i++) {
        _light_channel[i].inputValue = light.packed.channels[i];
    }

    _light_brightness = light.packed.brightness;
    _light_mireds = light.packed.mired;
}

void _lightSaveSettings() {
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        setSetting("ch", i, _light_channel[i].inputValue);
    }
    setSetting("brightness", _light_brightness);
    setSetting("mireds", _light_mireds);
    saveSettings();
}

void _lightRestoreSettings() {
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        _light_channel[i].inputValue = getSetting("ch", i, i==0 ? 255 : 0).toInt();
    }
    _light_brightness = getSetting("brightness", LIGHT_MAX_BRIGHTNESS).toInt();
    _light_mireds = getSetting("mireds", _light_mireds).toInt();
    lightUpdate(false, false);
}





#if MQTT_SUPPORT
void _lightMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    String mqtt_group_color = getSetting("mqttGroupColor");

    if (type == MQTT_CONNECT_EVENT) {

        mqttSubscribe(MQTT_TOPIC_BRIGHTNESS);

        if (_light_has_color) {
            mqttSubscribe(MQTT_TOPIC_COLOR_RGB);
            mqttSubscribe(MQTT_TOPIC_COLOR_HSV);
            mqttSubscribe(MQTT_TOPIC_TRANSITION);
        }

        if (_light_has_color || _light_use_cct) {
            mqttSubscribe(MQTT_TOPIC_MIRED);
            mqttSubscribe(MQTT_TOPIC_KELVIN);
        }


        if (mqtt_group_color.length() > 0) mqttSubscribeRaw(mqtt_group_color.c_str());


        char buffer[strlen(MQTT_TOPIC_CHANNEL) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_CHANNEL);
        mqttSubscribe(buffer);

    }

    if (type == MQTT_MESSAGE_EVENT) {


        if ((mqtt_group_color.length() > 0) & (mqtt_group_color.equals(topic))) {
            lightColor(payload, true);
            lightUpdate(true, mqttForward(), false);
            return;
        }


        String t = mqttMagnitude((char *) topic);


        if (t.equals(MQTT_TOPIC_MIRED)) {
            _fromMireds(atol(payload));
            lightUpdate(true, mqttForward());
            return;
        }


        if (t.equals(MQTT_TOPIC_KELVIN)) {
            _fromKelvin(atol(payload));
            lightUpdate(true, mqttForward());
            return;
        }


        if (t.equals(MQTT_TOPIC_COLOR_RGB)) {
            lightColor(payload, true);
            lightUpdate(true, mqttForward());
            return;
        }
        if (t.equals(MQTT_TOPIC_COLOR_HSV)) {
            lightColor(payload, false);
            lightUpdate(true, mqttForward());
            return;
        }


        if (t.equals(MQTT_TOPIC_BRIGHTNESS)) {
            _light_brightness = constrain(atoi(payload), 0, LIGHT_MAX_BRIGHTNESS);
            lightUpdate(true, mqttForward());
            return;
        }


        if (t.equals(MQTT_TOPIC_TRANSITION)) {
            lightTransitionTime(atol(payload));
            return;
        }


        if (t.startsWith(MQTT_TOPIC_CHANNEL)) {
            unsigned int channelID = t.substring(strlen(MQTT_TOPIC_CHANNEL)+1).toInt();
            if (channelID >= _light_channel.size()) {
                DEBUG_MSG_P(PSTR("[LIGHT] Wrong channelID (%d)\n"), channelID);
                return;
            }
            lightChannel(channelID, atoi(payload));
            lightUpdate(true, mqttForward());
            return;
        }

    }

}

void lightMQTT() {

    char buffer[20];

    if (_light_has_color) {


        if (getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1) {
            _toRGB(buffer, sizeof(buffer), true);
        } else {
            _toLong(buffer, sizeof(buffer), true);
        }
        mqttSend(MQTT_TOPIC_COLOR_RGB, buffer);

        _toHSV(buffer, sizeof(buffer), true);
        mqttSend(MQTT_TOPIC_COLOR_HSV, buffer);

    }

    if (_light_has_color || _light_use_cct) {


      snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _light_mireds);
      mqttSend(MQTT_TOPIC_MIRED, buffer);

    }


    for (unsigned int i=0; i < _light_channel.size(); i++) {
        itoa(_light_channel[i].target, buffer, 10);
        mqttSend(MQTT_TOPIC_CHANNEL, i, buffer);
    }


    snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _light_brightness);
    mqttSend(MQTT_TOPIC_BRIGHTNESS, buffer);

}

void lightMQTTGroup() {
    String mqtt_group_color = getSetting("mqttGroupColor");
    if (mqtt_group_color.length()>0) {
        char buffer[20];
        _toCSV(buffer, sizeof(buffer), true);
        mqttSendRaw(mqtt_group_color.c_str(), buffer);
    }
}

#endif





#if BROKER_SUPPORT

void lightBroker() {
    char buffer[10];
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        itoa(_light_channel[i].inputValue, buffer, 10);
        brokerPublish(BROKER_MSG_TYPE_STATUS, MQTT_TOPIC_CHANNEL, i, buffer);
    }
}

#endif





unsigned char lightChannels() {
    return _light_channel.size();
}

bool lightHasColor() {
    return _light_has_color;
}

bool lightUseCCT() {
    return _light_use_cct;
}

void _lightComms(unsigned char mask) {


    #if MQTT_SUPPORT
        if (mask & 0x01) lightMQTT();
        if (mask & 0x02) lightMQTTGroup();
    #endif


    #if WEB_SUPPORT
        wsSend(_lightWebSocketStatus);
    #endif


    #if BROKER_SUPPORT
        lightBroker();
    #endif

}

void lightUpdate(bool save, bool forward, bool group_forward) {

    _generateBrightness();


    for (unsigned int i=0; i < _light_channel.size(); i++) {
        _light_channel[i].target = _light_state && _light_channel[i].state ? _light_channel[i].value : 0;

    }


    _light_steps_left = _light_use_transitions ? _light_transition_time / LIGHT_TRANSITION_STEP : 1;
    _light_transition_ticker.attach_ms(LIGHT_TRANSITION_STEP, _lightProviderUpdate);


    unsigned char mask = 0;
    if (forward) mask += 1;
    if (group_forward) mask += 2;
    _light_comms_ticker.once_ms(LIGHT_COMMS_DELAY, _lightComms, mask);

    _lightSaveRtcmem();

    #if LIGHT_SAVE_ENABLED

        if (save) _light_save_ticker.once(LIGHT_SAVE_DELAY, _lightSaveSettings);
    #endif

};

void lightUpdate(bool save, bool forward) {
    lightUpdate(save, forward, true);
}

#if LIGHT_SAVE_ENABLED == 0
void lightSave() {
    _lightSaveSettings();
}
#endif

void lightState(unsigned char i, bool state) {
    _light_channel[i].state = state;
}

bool lightState(unsigned char i) {
    return _light_channel[i].state;
}

void lightState(bool state) {
    _light_state = state;
}

bool lightState() {
    return _light_state;
}

void lightColor(const char * color, bool rgb) {
    DEBUG_MSG_P(PSTR("[LIGHT] %s: %s\n"), rgb ? "RGB" : "HSV", color);
    if (rgb) {
        _fromRGB(color);
    } else {
        _fromHSV(color);
    }
}

void lightColor(const char * color) {
    lightColor(color, true);
}

void lightColor(unsigned long color) {
    _fromLong(color, false);
}

String lightColor(bool rgb) {
    char str[12];
    if (rgb) {
        _toRGB(str, sizeof(str));
    } else {
        _toHSV(str, sizeof(str));
    }
    return String(str);
}

String lightColor() {
    return lightColor(true);
}

unsigned int lightChannel(unsigned char id) {
    if (id <= _light_channel.size()) {
        return _light_channel[id].inputValue;
    }
    return 0;
}

void lightChannel(unsigned char id, int value) {
    if (id <= _light_channel.size()) {
        _light_channel[id].inputValue = constrain(value, 0, LIGHT_MAX_VALUE);
    }
}

void lightChannelStep(unsigned char id, int steps) {
    lightChannel(id, lightChannel(id) + steps * LIGHT_STEP);
}

unsigned int lightBrightness() {
    return _light_brightness;
}

void lightBrightness(int b) {
    _light_brightness = constrain(b, 0, LIGHT_MAX_BRIGHTNESS);
}

void lightBrightnessStep(int steps) {
    lightBrightness(_light_brightness + steps * LIGHT_STEP);
}

unsigned long lightTransitionTime() {
    if (_light_use_transitions) {
        return _light_transition_time;
    } else {
        return 0;
    }
}

void lightTransitionTime(unsigned long m) {
    if (0 == m) {
        _light_use_transitions = false;
    } else {
        _light_use_transitions = true;
        _light_transition_time = m;
    }
    setSetting("useTransitions", _light_use_transitions);
    setSetting("lightTime", _light_transition_time);
    saveSettings();
}





#if WEB_SUPPORT

bool _lightWebSocketOnReceive(const char * key, JsonVariant& value) {
    if (strncmp(key, "light", 5) == 0) return true;
    if (strncmp(key, "use", 3) == 0) return true;
    return false;
}

void _lightWebSocketStatus(JsonObject& root) {
    if (_light_has_color) {
        if (getSetting("useRGB", LIGHT_USE_RGB).toInt() == 1) {
            root["rgb"] = lightColor(true);
        } else {
            root["hsv"] = lightColor(false);
        }
    }
    if (_light_use_cct) {
        root["useCCT"] = _light_use_cct;
        root["mireds"] = _light_mireds;
    }
    JsonArray& channels = root.createNestedArray("channels");
    for (unsigned char id=0; id < _light_channel.size(); id++) {
        channels.add(lightChannel(id));
    }
    root["brightness"] = lightBrightness();
}

void _lightWebSocketOnSend(JsonObject& root) {
    root["colorVisible"] = 1;
    root["mqttGroupColor"] = getSetting("mqttGroupColor");
    root["useColor"] = _light_has_color;
    root["useWhite"] = _light_use_white;
    root["useGamma"] = _light_use_gamma;
    root["useTransitions"] = _light_use_transitions;
    root["useCSS"] = getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1;
    root["useRGB"] = getSetting("useRGB", LIGHT_USE_RGB).toInt() == 1;
    root["lightTime"] = _light_transition_time;

    _lightWebSocketStatus(root);
}

void _lightWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {

    if (_light_has_color) {
        if (strcmp(action, "color") == 0) {
            if (data.containsKey("rgb")) {
                lightColor(data["rgb"], true);
                lightUpdate(true, true);
            }
            if (data.containsKey("hsv")) {
                lightColor(data["hsv"], false);
                lightUpdate(true, true);
            }
        }
    }

    if (_light_use_cct) {
      if (strcmp(action, "mireds") == 0) {
          _fromMireds(data["mireds"]);
          lightUpdate(true, true);
      }
    }


    if (strcmp(action, "channel") == 0) {
        if (data.containsKey("id") && data.containsKey("value")) {
            lightChannel(data["id"], data["value"]);
            lightUpdate(true, true);
        }
    }

    if (strcmp(action, "brightness") == 0) {
        if (data.containsKey("value")) {
            lightBrightness(data["value"]);
            lightUpdate(true, true);
        }
    }

}

#endif

#if API_SUPPORT

void _lightAPISetup() {

    if (_light_has_color) {

        apiRegister(MQTT_TOPIC_COLOR_RGB,
            [](char * buffer, size_t len) {
                if (getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1) {
                    _toRGB(buffer, len, true);
                } else {
                    _toLong(buffer, len, true);
                }
            },
            [](const char * payload) {
                lightColor(payload, true);
                lightUpdate(true, true);
            }
        );

        apiRegister(MQTT_TOPIC_COLOR_HSV,
            [](char * buffer, size_t len) {
                _toHSV(buffer, len, true);
            },
            [](const char * payload) {
                lightColor(payload, false);
                lightUpdate(true, true);
            }
        );

        apiRegister(MQTT_TOPIC_KELVIN,
            [](char * buffer, size_t len) {},
            [](const char * payload) {
                _fromKelvin(atol(payload));
                lightUpdate(true, true);
            }
        );

        apiRegister(MQTT_TOPIC_MIRED,
            [](char * buffer, size_t len) {},
            [](const char * payload) {
                _fromMireds(atol(payload));
                lightUpdate(true, true);
            }
        );

    }

    for (unsigned int id=0; id<_light_channel.size(); id++) {

        char key[15];
        snprintf_P(key, sizeof(key), PSTR("%s/%d"), MQTT_TOPIC_CHANNEL, id);
        apiRegister(key,
            [id](char * buffer, size_t len) {
                snprintf_P(buffer, len, PSTR("%d"), _light_channel[id].target);
            },
            [id](const char * payload) {
                lightChannel(id, atoi(payload));
                lightUpdate(true, true);
            }
        );

    }

    apiRegister(MQTT_TOPIC_TRANSITION,
        [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), lightTransitionTime());
        },
        [](const char * payload) {
            lightTransitionTime(atol(payload));
        }
    );

    apiRegister(MQTT_TOPIC_BRIGHTNESS,
        [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), _light_brightness);
        },
        [](const char * payload) {
            lightBrightness(atoi(payload));
            lightUpdate(true, true);
        }
    );

}

#endif

#if TERMINAL_SUPPORT

void _lightInitCommands() {

    terminalRegisterCommand(F("BRIGHTNESS"), [](Embedis* e) {
        if (e->argc > 1) {
            const String value(e->argv[1]);
            if( value.length() > 0 ) {
                if( value[0] == '+' || value[0] == '-' ) {
                    lightBrightness(lightBrightness()+String(e->argv[1]).toInt());
                } else {
                    lightBrightness(String(e->argv[1]).toInt());
                }
                lightUpdate(true, true);
            }
        }
        DEBUG_MSG_P(PSTR("Brightness: %d\n"), lightBrightness());
        terminalOK();
    });

    terminalRegisterCommand(F("CHANNEL"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("Wrong arguments"));
        }
        int id = String(e->argv[1]).toInt();
        if (e->argc > 2) {
            int value = String(e->argv[2]).toInt();
            lightChannel(id, value);
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Channel #%d: %d\n"), id, lightChannel(id));
        terminalOK();
    });

    terminalRegisterCommand(F("COLOR"), [](Embedis* e) {
        if (e->argc > 1) {
            String color = String(e->argv[1]);
            lightColor(color.c_str());
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        terminalOK();
    });

    terminalRegisterCommand(F("KELVIN"), [](Embedis* e) {
        if (e->argc > 1) {
            String color = String("K") + String(e->argv[1]);
            lightColor(color.c_str());
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        terminalOK();
    });

    terminalRegisterCommand(F("MIRED"), [](Embedis* e) {
        if (e->argc > 1) {
            const String value(e->argv[1]);
            String color = String("M");
            if( value.length() > 0 ) {
                if( value[0] == '+' || value[0] == '-' ) {
                    color += String(_light_mireds + String(e->argv[1]).toInt());
                } else {
                    color += String(e->argv[1]);
                }
                lightColor(color.c_str());
                lightUpdate(true, true);
            }
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        terminalOK();
    });

}

#endif

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

unsigned long getIOMux(unsigned long gpio) {
    unsigned long muxes[16] = {
        PERIPHS_IO_MUX_GPIO0_U, PERIPHS_IO_MUX_U0TXD_U, PERIPHS_IO_MUX_GPIO2_U, PERIPHS_IO_MUX_U0RXD_U,
        PERIPHS_IO_MUX_GPIO4_U, PERIPHS_IO_MUX_GPIO5_U, PERIPHS_IO_MUX_SD_CLK_U, PERIPHS_IO_MUX_SD_DATA0_U,
        PERIPHS_IO_MUX_SD_DATA1_U, PERIPHS_IO_MUX_SD_DATA2_U, PERIPHS_IO_MUX_SD_DATA3_U, PERIPHS_IO_MUX_SD_CMD_U,
        PERIPHS_IO_MUX_MTDI_U, PERIPHS_IO_MUX_MTCK_U, PERIPHS_IO_MUX_MTMS_U, PERIPHS_IO_MUX_MTDO_U
    };
    return muxes[gpio];
}

unsigned long getIOFunc(unsigned long gpio) {
    unsigned long funcs[16] = {
        FUNC_GPIO0, FUNC_GPIO1, FUNC_GPIO2, FUNC_GPIO3,
        FUNC_GPIO4, FUNC_GPIO5, FUNC_GPIO6, FUNC_GPIO7,
        FUNC_GPIO8, FUNC_GPIO9, FUNC_GPIO10, FUNC_GPIO11,
        FUNC_GPIO12, FUNC_GPIO13, FUNC_GPIO14, FUNC_GPIO15
    };
    return funcs[gpio];
}

#endif

void _lightConfigure() {

    _light_has_color = getSetting("useColor", LIGHT_USE_COLOR).toInt() == 1;
    if (_light_has_color && (_light_channel.size() < 3)) {
        _light_has_color = false;
        setSetting("useColor", _light_has_color);
    }

    _light_use_white = getSetting("useWhite", LIGHT_USE_WHITE).toInt() == 1;
    if (_light_use_white && (_light_channel.size() < 4) && (_light_channel.size() != 2)) {
        _light_use_white = false;
        setSetting("useWhite", _light_use_white);
    }

    _light_use_cct = getSetting("useCCT", LIGHT_USE_CCT).toInt() == 1;
    if (_light_use_cct && (((_light_channel.size() < 5) && (_light_channel.size() != 2)) || !_light_use_white)) {
        _light_use_cct = false;
        setSetting("useCCT", _light_use_cct);
    }

    _light_use_gamma = getSetting("useGamma", LIGHT_USE_GAMMA).toInt() == 1;
    _light_use_transitions = getSetting("useTransitions", LIGHT_USE_TRANSITIONS).toInt() == 1;
    _light_transition_time = getSetting("lightTime", LIGHT_TRANSITION_TIME).toInt();

}

void lightSetup() {

    #ifdef LIGHT_ENABLE_PIN
        pinMode(LIGHT_ENABLE_PIN, OUTPUT);
        digitalWrite(LIGHT_ENABLE_PIN, HIGH);
    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX

        _my92xx = new my92xx(MY92XX_MODEL, MY92XX_CHIPS, MY92XX_DI_PIN, MY92XX_DCKI_PIN, MY92XX_COMMAND);
        for (unsigned char i=0; i<LIGHT_CHANNELS; i++) {
            _light_channel.push_back((channel_t) {0, false, true, 0, 0, 0});
        }

    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

        #ifdef LIGHT_CH1_PIN
            _light_channel.push_back((channel_t) {LIGHT_CH1_PIN, LIGHT_CH1_INVERSE, true, 0, 0, 0});
        #endif

        #ifdef LIGHT_CH2_PIN
            _light_channel.push_back((channel_t) {LIGHT_CH2_PIN, LIGHT_CH2_INVERSE, true, 0, 0, 0});
        #endif

        #ifdef LIGHT_CH3_PIN
            _light_channel.push_back((channel_t) {LIGHT_CH3_PIN, LIGHT_CH3_INVERSE, true, 0, 0, 0});
        #endif

        #ifdef LIGHT_CH4_PIN
            _light_channel.push_back((channel_t) {LIGHT_CH4_PIN, LIGHT_CH4_INVERSE, true, 0, 0, 0});
        #endif

        #ifdef LIGHT_CH5_PIN
            _light_channel.push_back((channel_t) {LIGHT_CH5_PIN, LIGHT_CH5_INVERSE, true, 0, 0, 0});
        #endif

        uint32 pwm_duty_init[PWM_CHANNEL_NUM_MAX];
        uint32 io_info[PWM_CHANNEL_NUM_MAX][3];
        for (unsigned int i=0; i < _light_channel.size(); i++) {
            pwm_duty_init[i] = 0;
            io_info[i][0] = getIOMux(_light_channel[i].pin);
            io_info[i][1] = getIOFunc(_light_channel[i].pin);
            io_info[i][2] = _light_channel[i].pin;
            pinMode(_light_channel[i].pin, OUTPUT);
        }
        pwm_init(LIGHT_MAX_PWM, pwm_duty_init, PWM_CHANNEL_NUM_MAX, io_info);
        pwm_start();


    #endif

    DEBUG_MSG_P(PSTR("[LIGHT] LIGHT_PROVIDER = %d\n"), LIGHT_PROVIDER);
    DEBUG_MSG_P(PSTR("[LIGHT] Number of channels: %d\n"), _light_channel.size());

    _lightConfigure();
    if (rtcmemStatus()) {
        _lightRestoreRtcmem();
    } else {
        _lightRestoreSettings();
    }

    #if WEB_SUPPORT
        wsOnSendRegister(_lightWebSocketOnSend);
        wsOnActionRegister(_lightWebSocketOnAction);
        wsOnReceiveRegister(_lightWebSocketOnReceive);
    #endif

    #if API_SUPPORT
        _lightAPISetup();
    #endif

    #if MQTT_SUPPORT
        mqttRegister(_lightMQTTCallback);
    #endif

    #if TERMINAL_SUPPORT
        _lightInitCommands();
    #endif


    espurnaRegisterReload([]() {
        #if LIGHT_SAVE_ENABLED == 0
            lightSave();
        #endif
        _lightConfigure();
    });

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/lightfox.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/lightfox.ino"
#ifdef FOXEL_LIGHTFOX_DUAL





#define LIGHTFOX_CODE_START 0xA0
#define LIGHTFOX_CODE_LEARN 0xF1
#define LIGHTFOX_CODE_CLEAR 0xF2
#define LIGHTFOX_CODE_STOP 0xA1





void lightfoxLearn() {
    Serial.write(LIGHTFOX_CODE_START);
    Serial.write(LIGHTFOX_CODE_LEARN);
    Serial.write(0x00);
    Serial.write(LIGHTFOX_CODE_STOP);
    Serial.println();
    Serial.flush();
    DEBUG_MSG_P(PSTR("[LIGHTFOX] Learn comman sent\n"));
}

void lightfoxClear() {
    Serial.write(LIGHTFOX_CODE_START);
    Serial.write(LIGHTFOX_CODE_CLEAR);
    Serial.write(0x00);
    Serial.write(LIGHTFOX_CODE_STOP);
    Serial.println();
    Serial.flush();
    DEBUG_MSG_P(PSTR("[LIGHTFOX] Clear comman sent\n"));
}





#if WEB_SUPPORT

void _lightfoxWebSocketOnSend(JsonObject& root) {
    root["lightfoxVisible"] = 1;
    uint8_t buttonsCount = _buttons.size();
    root["lightfoxRelayCount"] = relayCount();
    JsonArray& rfb = root.createNestedArray("lightfoxButtons");
    for (byte id=0; id<buttonsCount; id++) {
        JsonObject& node = rfb.createNestedObject();
        node["id"] = id;
        node["relay"] = getSetting("btnRelay", id, "0");
    }
}

void _lightfoxWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "lightfoxLearn") == 0) lightfoxLearn();
    if (strcmp(action, "lightfoxClear") == 0) lightfoxClear();
}

#endif





#if TERMINAL_SUPPORT

void _lightfoxInitCommands() {

    terminalRegisterCommand(F("LIGHTFOX.LEARN"), [](Embedis* e) {
        lightfoxLearn();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    terminalRegisterCommand(F("LIGHTFOX.CLEAR"), [](Embedis* e) {
        lightfoxClear();
        DEBUG_MSG_P(PSTR("+OK\n"));
    });
}

#endif





void lightfoxSetup() {

    #if WEB_SUPPORT
        wsOnSendRegister(_lightfoxWebSocketOnSend);
        wsOnActionRegister(_lightfoxWebSocketOnAction);
    #endif

    #if TERMINAL_SUPPORT
        _lightfoxInitCommands();
    #endif

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/llmnr.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/llmnr.ino"
#if LLMNR_SUPPORT

#include <ESP8266LLMNR.h>

void llmnrSetup() {
    LLMNR.begin(getSetting("hostname").c_str());
    DEBUG_MSG_P(PSTR("[LLMNR] Configured\n"));
}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/mdns.ino"
# 13 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/mdns.ino"
#if MDNS_SERVER_SUPPORT

#include <ESP8266mDNS.h>

#if MQTT_SUPPORT

void _mdnsFindMQTT() {
    int count = MDNS.queryService("mqtt", "tcp");
    DEBUG_MSG_P(PSTR("[MQTT] MQTT brokers found: %d\n"), count);
    for (int i=0; i<count; i++) {
        DEBUG_MSG_P(PSTR("[MQTT] Broker at %s:%d\n"), MDNS.IP(i).toString().c_str(), MDNS.port(i));
        mqttSetBrokerIfNone(MDNS.IP(i), MDNS.port(i));
    }
}

#endif

void _mdnsServerStart() {
    if (MDNS.begin((char *) getSetting("hostname").c_str())) {
        DEBUG_MSG_P(PSTR("[MDNS] OK\n"));
    } else {
        DEBUG_MSG_P(PSTR("[MDNS] FAIL\n"));
    }
}



void mdnsServerSetup() {

    #if WEB_SUPPORT
        MDNS.addService("http", "tcp", getSetting("webPort", WEB_PORT).toInt());
    #endif

    #if TELNET_SUPPORT
        MDNS.addService("telnet", "tcp", TELNET_PORT);
    #endif


    MDNS.addServiceTxt("arduino", "tcp", "app_name", APP_NAME);
    MDNS.addServiceTxt("arduino", "tcp", "app_version", APP_VERSION);
    MDNS.addServiceTxt("arduino", "tcp", "mac", WiFi.macAddress());
    MDNS.addServiceTxt("arduino", "tcp", "target_board", getBoardName());
    {
        char buffer[6] = {0};
        itoa(ESP.getFlashChipRealSize() / 1024, buffer, 10);
        MDNS.addServiceTxt("arduino", "tcp", "mem_size", (const char *) buffer);
    }
    {
        char buffer[6] = {0};
        itoa(ESP.getFlashChipSize() / 1024, buffer, 10);
        MDNS.addServiceTxt("arduino", "tcp", "sdk_size", (const char *) buffer);
    }
    {
        char buffer[6] = {0};
        itoa(ESP.getFreeSketchSpace(), buffer, 10);
        MDNS.addServiceTxt("arduino", "tcp", "free_space", (const char *) buffer);
    }

    wifiRegister([](justwifi_messages_t code, char * parameter) {

        if (code == MESSAGE_CONNECTED) {
            _mdnsServerStart();
            #if MQTT_SUPPORT
                _mdnsFindMQTT();
            #endif
        }

        if (code == MESSAGE_ACCESSPOINT_CREATED) {
            _mdnsServerStart();
        }

    });

}

#endif





#if MDNS_CLIENT_SUPPORT

#include <WiFiUdp.h>
#include <mDNSResolver.h>

using namespace mDNSResolver;
WiFiUDP _mdns_udp;
Resolver _mdns_resolver(_mdns_udp);

String mdnsResolve(char * name) {

    if (strlen(name) == 0) return String();
    if (WiFi.status() != WL_CONNECTED) return String();

    _mdns_resolver.setLocalIP(WiFi.localIP());
    IPAddress ip = _mdns_resolver.search(name);

    if (ip == INADDR_NONE) return String(name);
    DEBUG_MSG_P(PSTR("[MDNS] '%s' resolved to '%s'\n"), name, ip.toString().c_str());
    return ip.toString();

}

String mdnsResolve(String name) {
    return mdnsResolve((char *) name.c_str());
}

void mdnsClientSetup() {


    espurnaRegisterLoop(mdnsClientLoop);

}

void mdnsClientLoop() {
    _mdns_resolver.loop();
}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/migrate.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/migrate.ino"
void _cmpMoveIndexDown(const char * key, int offset = 0) {
    if (hasSetting(key, 0)) return;
    for (unsigned char index = 1; index < SETTINGS_MAX_LIST_COUNT; index++) {
        if (hasSetting(key, index)) {
            setSetting(key, index - 1, getSetting(key, index).toInt() + offset);
        } else {
            delSetting(key, index - 1);
        }
    }
}







void migrate() {


    unsigned int board = getSetting("board", 0).toInt();
    unsigned int config_version = getSetting("cfg", board > 0 ? 2 : 1).toInt();


    if (config_version == CFG_VERSION) return;
    setSetting("cfg", CFG_VERSION);

    if (config_version == 2) {
        _cmpMoveIndexDown("ledGPIO");
        _cmpMoveIndexDown("ledLogic");
        _cmpMoveIndexDown("btnGPIO");
        _cmpMoveIndexDown("btnRelay", -1);
        _cmpMoveIndexDown("relayGPIO");
        _cmpMoveIndexDown("relayType");
    }

    if (config_version == 1) {

        #if defined(NODEMCU_LOLIN)

            setSetting("board", 2);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(WEMOS_D1_MINI_RELAYSHIELD)

            setSetting("board", 3);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_BASIC)

            setSetting("board", 4);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_TH)

            setSetting("board", 5);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_SV)

            setSetting("board", 6);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_TOUCH)

            setSetting("board", 7);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_POW)

            setSetting("board", 8);
            setSetting("ledGPIO", 0, 15);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("selGPIO", 5);
            setSetting("cf1GPIO", 13);
            setSetting("cfGPIO", 14);

        #elif defined(ITEAD_SONOFF_DUAL)

            setSetting("board", 9);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnRelay", 0, 0xFF);
            setSetting("btnRelay", 1, 0xFF);
            setSetting("btnRelay", 2, 0);
            setSetting("relayProvider", RELAY_PROVIDER_DUAL);
            setSetting("relays", 2);

        #elif defined(ITEAD_1CH_INCHING)

            setSetting("board", 10);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_4CH)

            setSetting("board", 11);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnGPIO", 2, 10);
            setSetting("btnGPIO", 3, 14);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("btnRelay", 2, 2);
            setSetting("btnRelay", 3, 3);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayGPIO", 3, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);
            setSetting("relayType", 3, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SLAMPHER)

            setSetting("board", 12);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_S20)

            setSetting("board", 13);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ELECTRODRAGON_WIFI_IOT)

            setSetting("board", 14);
            setSetting("ledGPIO", 0, 16);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 2);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 13);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(WORKCHOICE_ECOPLUG)

            setSetting("board", 15);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(JANGOE_WIFI_RELAY_NC)

            setSetting("board", 16);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnGPIO", 1, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 2);
            setSetting("relayGPIO", 1, 14);
            setSetting("relayType", 0, RELAY_TYPE_INVERSE);
            setSetting("relayType", 1, RELAY_TYPE_INVERSE);

        #elif defined(JANGOE_WIFI_RELAY_NO)

            setSetting("board", 17);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnGPIO", 1, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 2);
            setSetting("relayGPIO", 1, 14);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(OPENENERGYMONITOR_MQTT_RELAY)

            setSetting("board", 18);
            setSetting("ledGPIO", 0, 16);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(JORGEGARCIA_WIFI_RELAYS)

            setSetting("board", 19);
            setSetting("relayGPIO", 0, 0);
            setSetting("relayGPIO", 1, 2);
            setSetting("relayType", 0, RELAY_TYPE_INVERSE);
            setSetting("relayType", 1, RELAY_TYPE_INVERSE);

        #elif defined(AITHINKER_AI_LIGHT)

            setSetting("board", 20);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9291);
            setSetting("myChips", 1);
            setSetting("myDIGPIO", 13);
            setSetting("myDCKIGPIO", 15);
            setSetting("relays", 1);

        #elif defined(LYASI_LIGHT)

            setSetting("board", 20);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9291);
            setSetting("myChips", 1);
            setSetting("myDIGPIO", 4);
            setSetting("myDCKIGPIO", 5);
            setSetting("relays", 1);

        #elif defined(MAGICHOME_LED_CONTROLLER)

            setSetting("board", 21);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 14);
            setSetting("chGPIO", 1, 5);
            setSetting("chGPIO", 2, 12);
            setSetting("chGPIO", 3, 13);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(MAGICHOME_LED_CONTROLLER_IR)

            setSetting("board", 21);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 5);
            setSetting("chGPIO", 1, 12);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 14);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(ITEAD_MOTOR)

            setSetting("board", 22);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(TINKERMAN_ESPURNA_H06)

            setSetting("board", 23);
            setSetting("ledGPIO", 0, 5);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 4);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_INVERSE);
            setSetting("selGPIO", 2);
            setSetting("cf1GPIO", 13);
            setSetting("cfGPIO", 14);

        #elif defined(HUACANXING_H801)

            setSetting("board", 24);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 5);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 15);
            setSetting("chGPIO", 1, 13);
            setSetting("chGPIO", 2, 12);
            setSetting("chGPIO", 3, 14);
            setSetting("chGPIO", 4, 4);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("chLogic", 4, 0);
            setSetting("relays", 1);

        #elif defined(ITEAD_BNSZ01)

            setSetting("board", 25);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 12);
            setSetting("chLogic", 0, 0);
            setSetting("relays", 1);

        #elif defined(ITEAD_SONOFF_RFBRIDGE)

            setSetting("board", 26);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("relayProvider", RELAY_PROVIDER_RFBRIDGE);
            setSetting("relays", 6);

        #elif defined(ITEAD_SONOFF_4CH_PRO)

            setSetting("board", 27);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnGPIO", 2, 10);
            setSetting("btnGPIO", 3, 14);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("btnRelay", 2, 2);
            setSetting("btnRelay", 3, 3);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayGPIO", 3, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);
            setSetting("relayType", 3, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_B1)

            setSetting("board", 28);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9231);
            setSetting("myChips", 2);
            setSetting("myDIGPIO", 12);
            setSetting("myDCKIGPIO", 14);
            setSetting("relays", 1);

        #elif defined(ITEAD_SONOFF_LED)

            setSetting("board", 29);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 12);
            setSetting("chLogic", 0, 0);
            setSetting("chGPIO", 1, 14);
            setSetting("chLogic", 1, 0);
            setSetting("relays", 1);

        #elif defined(ITEAD_SONOFF_T1_1CH)

            setSetting("board", 30);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 9);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_T1_2CH)

            setSetting("board", 31);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 10);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 4);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_T1_3CH)

            setSetting("board", 32);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnGPIO", 2, 10);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("btnRelay", 2, 2);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_RF)

            setSetting("board", 33);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(WION_50055)

            setSetting("board", 34);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(EXS_WIFI_RELAY_V31)

            setSetting("board", 35);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 13);
            setSetting("relayResetGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(HUACANXING_H802)

            setSetting("board", 36);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 12);
            setSetting("chGPIO", 1, 14);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 15);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(GENERIC_V9261F)

            setSetting("board", 37);

        #elif defined(GENERIC_ECH1560)

            setSetting("board", 38);

        #elif defined(TINKERMAN_ESPURNA_H08)

            setSetting("board", 39);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 4);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("selGPIO", 5);
            setSetting("cf1GPIO", 13);
            setSetting("cfGPIO", 14);

        #elif defined(MANCAVEMADE_ESPLIVE)

            setSetting("board", 40);
            setSetting("btnGPIO", 0, 4);
            setSetting("btnGPIO", 1, 5);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 13);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(INTERMITTECH_QUINLED)

            setSetting("board", 41);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("ledGPIO", 0, 1);
            setSetting("ledLogic", 0, 1);
            setSetting("chGPIO", 0, 0);
            setSetting("chGPIO", 1, 2);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("relays", 1);

        #elif defined(MAGICHOME_LED_CONTROLLER_20)

            setSetting("board", 42);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 5);
            setSetting("chGPIO", 1, 12);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 15);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(ARILUX_AL_LC06)

            setSetting("board", 43);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 12);
            setSetting("chGPIO", 1, 14);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 15);
            setSetting("chGPIO", 4, 5);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("chLogic", 4, 0);
            setSetting("relays", 1);

        #elif defined(XENON_SM_PW702U)

            setSetting("board", 44);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(AUTHOMETION_LYT8266)

            setSetting("board", 45);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 13);
            setSetting("chGPIO", 1, 12);
            setSetting("chGPIO", 2, 14);
            setSetting("chGPIO", 3, 2);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);
            setSetting("enGPIO", 15);

        #elif defined(ARILUX_E27)

            setSetting("board", 46);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9291);
            setSetting("myChips", 1);
            setSetting("myDIGPIO", 13);
            setSetting("myDCKIGPIO", 15);
            setSetting("relays", 1);

        #elif defined(YJZK_SWITCH_2CH)

            setSetting("board", 47);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 0);
            setSetting("ledWifi", 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_DUAL_R2)

            setSetting("board", 48);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnGPIO", 2, 10);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("btnRelay", 2, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(GENERIC_8CH)

            setSetting("board", 49);
            setSetting("relayGPIO", 0, 0);
            setSetting("relayGPIO", 1, 2);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayGPIO", 3, 5);
            setSetting("relayGPIO", 4, 12);
            setSetting("relayGPIO", 5, 13);
            setSetting("relayGPIO", 6, 14);
            setSetting("relayGPIO", 7, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);
            setSetting("relayType", 3, RELAY_TYPE_NORMAL);
            setSetting("relayType", 4, RELAY_TYPE_NORMAL);
            setSetting("relayType", 5, RELAY_TYPE_NORMAL);
            setSetting("relayType", 6, RELAY_TYPE_NORMAL);
            setSetting("relayType", 7, RELAY_TYPE_NORMAL);

        #elif defined(ARILUX_AL_LC01)

            setSetting("board", 50);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 5);
            setSetting("chGPIO", 1, 12);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 14);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(ARILUX_AL_LC11)

            setSetting("board", 51);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 5);
            setSetting("chGPIO", 1, 4);
            setSetting("chGPIO", 2, 14);
            setSetting("chGPIO", 3, 13);
            setSetting("chGPIO", 4, 12);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("chLogic", 4, 0);
            setSetting("relays", 1);

        #elif defined(ARILUX_AL_LC02)

            setSetting("board", 52);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 12);
            setSetting("chGPIO", 1, 5);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 15);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(KMC_70011)

            setSetting("board", 53);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 14);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("selGPIO", 12);
            setSetting("cf1GPIO", 5);
            setSetting("cfGPIO", 4);

        #elif defined(GIZWITS_WITTY_CLOUD)

            setSetting("board", 54);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 4);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 15);
            setSetting("chGPIO", 1, 12);
            setSetting("chGPIO", 2, 13);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("relays", 1);

        #elif defined(EUROMATE_WIFI_STECKER_SCHUKO)

            setSetting("board", 55);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 0);
            setSetting("ledGPIO", 1, 12);
            setSetting("ledLogic", 1, 0);
            setSetting("btnGPIO", 0, 14);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(TONBUX_POWERSTRIP02)

            setSetting("board", 56);
            setSetting("relayGPIO", 0, 4);
            setSetting("relayGPIO", 1, 13);
            setSetting("relayGPIO", 2, 12);
            setSetting("relayGPIO", 3, 14);
            setSetting("relayGPIO", 4, 16);
            setSetting("relayType", 0, RELAY_TYPE_INVERSE);
            setSetting("relayType", 1, RELAY_TYPE_INVERSE);
            setSetting("relayType", 2, RELAY_TYPE_INVERSE);
            setSetting("relayType", 3, RELAY_TYPE_INVERSE);
            setSetting("relayType", 4, RELAY_TYPE_NORMAL);
            setSetting("ledGPIO", 0, 0);
            setSetting("ledLogic", 0, 1);
            setSetting("ledGPIO", 1, 3);
            setSetting("ledLogic", 1, 1);
            setSetting("btnGPIO", 0, 5);
            setSetting("btnRelay", 0, 1);

        #elif defined(LINGAN_SWA1)

            setSetting("board", 57);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(HEYGO_HY02)

            setSetting("board", 58);
            setSetting("ledGPIO", 0, 0);
            setSetting("ledLogic", 0, 1);
            setSetting("ledGPIO", 1, 15);
            setSetting("ledLogic", 1, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("selGPIO", 3);
            setSetting("cf1GPIO", 14);
            setSetting("cfGPIO", 5);

        #elif defined(MAXCIO_WUS002S)

            setSetting("board", 59);
            setSetting("ledGPIO", 0, 3);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 2);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 13);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("selGPIO", 12);
            setSetting("cf1GPIO", 5);
            setSetting("cfGPIO", 4);

        #elif defined(YIDIAN_XSSSA05)

            setSetting("board", 60);
            setSetting("ledGPIO", 0, 0);
            setSetting("ledLogic", 0, 0);
            setSetting("ledGPIO", 1, 5);
            setSetting("ledLogic", 1, 0);
            setSetting("ledGPIO", 2, 2);
            setSetting("ledLogic", 2, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(TONBUX_XSSSA06)

            setSetting("board", 61);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(GREEN_ESP8266RELAY)

            setSetting("board", 62);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 5);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 4);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(IKE_ESPIKE)

            setSetting("board", 63);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("btnGPIO", 1, 12);
            setSetting("btnRelay", 1, 1);
            setSetting("btnGPIO", 2, 13);
            setSetting("btnRelay", 2, 2);
            setSetting("relayGPIO", 0, 4);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayGPIO", 2, 16);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);

        #elif defined(ARNIEX_SWIFITCH)

            setSetting("board", 64);
            setSetting("ledGPIO", 0, 12);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 4);
            setSetting("btnRelay", 0, 1);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayType", 0, RELAY_TYPE_INVERSE);

        #elif defined(GENERIC_ESP01S_RELAY_V40)

            setSetting("board", 65);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 0);
            setSetting("relayGPIO", 0, 0);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(GENERIC_ESP01S_RGBLED_V10)

            setSetting("board", 66);
            setSetting("ledGPIO", 0, 2);

        #elif defined(HELTEC_TOUCHRELAY)

            setSetting("board", 67);
            setSetting("btnGPIO", 0, 14);
            setSetting("btnRelay", 0, 1);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(GENERIC_ESP01S_DHT11_V10)

            setSetting("board", 68);

        #elif defined(GENERIC_ESP01S_DS18B20_V10)

            setSetting("board", 69);

        #elif defined(ZHILDE_EU44_W)

            setSetting("board", 70);
            setSetting("btnGPIO", 0, 3);
            setSetting("ledGPIO", 0, 1);
            setSetting("ledLogic", 0, 1);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayGPIO", 1, 4);
            setSetting("relayGPIO", 2, 12);
            setSetting("relayGPIO", 3, 13);
            setSetting("relayGPIO", 4, 14);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);
            setSetting("relayType", 3, RELAY_TYPE_NORMAL);
            setSetting("relayType", 4, RELAY_TYPE_NORMAL);

        #elif defined(ITEAD_SONOFF_POW_R2)

            setSetting("board", 71);
            setSetting("ledGPIO", 0, 15);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("selGPIO", 5);
            setSetting("cf1GPIO", 13);
            setSetting("cfGPIO", 14);

        #elif defined(LUANI_HVIO)

            setSetting("board", 72);
            setSetting("ledGPIO", 0, 15);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 4);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(ALLNET_4DUINO_IOT_WLAN_RELAIS)

            setSetting("board", 73);
            setSetting("relayGPIO", 0, 14);
            setSetting("relayResetGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_LATCHED);

        #elif defined(TONBUX_MOSQUITO_KILLER)

            setSetting("board", 74);
            setSetting("ledGPIO", 0, 15);
            setSetting("ledLogic", 0, 1);
            setSetting("ledGPIO", 1, 14);
            setSetting("ledLogic", 1, 1);
            setSetting("ledGPIO", 2, 12);
            setSetting("ledLogic", 2, 0);
            setSetting("ledGPIO", 3, 16);
            setSetting("ledLogic", 3, 0);
            setSetting("btnGPIO", 0, 2);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(NEO_COOLCAM_NAS_WR01W)

            setSetting("board", 75);
            setSetting("ledGPIO", 0, 4);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

          #elif defined(PILOTAK_ESP_DIN_V1)

            setSetting("board", 76);
            setSetting("ledGPIO", 0, 16);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 4);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(FORNORM_ZLD_34EU)

            setSetting("board", 77);
            setSetting("btnGPIO", 0, 16);
            setSetting("btnRelay", 0, 3);
            setSetting("ledGPIO", 0, 0);
            setSetting("ledGPIO", 1, 12);
            setSetting("ledGPIO", 2, 3);
            setSetting("ledGPIO", 3, 5);
            setSetting("ledLogic", 0, 1);
            setSetting("ledLogic", 1, 1);
            setSetting("ledLogic", 2, 1);
            setSetting("ledLogic", 3, 1);
            setSetting("ledMode", 0, LED_MODE_FINDME);
            setSetting("ledMode", 1, LED_MODE_FOLLOW);
            setSetting("ledMode", 2, LED_MODE_FOLLOW);
            setSetting("ledMode", 3, LED_MODE_FOLLOW);
            setSetting("ledRelay", 1, 1);
            setSetting("ledRelay", 2, 2);
            setSetting("ledRelay", 3, 3);
            setSetting("relayGPIO", 0, 14);
            setSetting("relayGPIO", 1, 13);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayGPIO", 3, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);
            setSetting("relayType", 3, RELAY_TYPE_NORMAL);

        #elif defined(BH_ONOFRE)

            setSetting("board", 78);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 0, 1);
            setSetting("relayGPIO", 0, 4);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(BLITZWOLF_BWSHPX)

            setSetting("board", 79);
            setSetting("ledGPIO", 0, 2);
            setSetting("ledLogic", 0, 1);
            setSetting("ledGPIO", 1, 0);
            setSetting("ledLogic", 1, 1);
            setSetting("ledMode", 1, LED_MODE_FINDME);
            setSetting("ledRelay", 1, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("selGPIO", 12);
            setSetting("cf1GPIO", 14);
            setSetting("cfGPIO", 5);
            setSetting("pwrRatioC", 25740);
            setSetting("pwrRatioV", 313400);
            setSetting("pwrRatioP", 3414290);
            setSetting("hlwSelC", LOW);
            setSetting("hlwIntM", FALLING);

        #elif defined(TINKERMAN_RFM69GW)

            setSetting("board", 80);
            setSetting("btnGPIO", 0, 0);

        #elif defined(ITEAD_SONOFF_IFAN02)

            setSetting("board", 81);

            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnGPIO", 2, 10);
            setSetting("btnGPIO", 3, 14);

            setSetting("ledGPIO", 1, 13);
            setSetting("ledLogic", 1, 1);

            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayGPIO", 3, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);
            setSetting("relayType", 3, RELAY_TYPE_NORMAL);

        #elif defined(GENERIC_AG_L4)

            setSetting("board", 82);

            setSetting("btnGPIO", 0, 4);
            setSetting("btnGPIO", 1, 2);
            setSetting("btnRelay", 0, 0);

            setSetting("ledGPIO", 0, 5);
            setSetting("ledGPIO", 1, 16);
            setSetting("ledLogic", 0, 0);
            setSetting("ledLogic", 1, 1);

            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 14);
            setSetting("chGPIO", 1, 13);
            setSetting("chGPIO", 2, 12);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("relays", 1);

        #elif defined(ALLTERCO_SHELLY1)

            setSetting("board", 83);
            setSetting("btnGPIO", 0, 5);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 4);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(LOHAS_9W)

            setSetting("board", 84);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_MY92XX);
            setSetting("myModel", MY92XX_MODEL_MY9231);
            setSetting("myChips", 2);
            setSetting("myDIGPIO", 13);
            setSetting("myDCKIGPIO", 15);
            setSetting("relays", 1);

        #elif defined(YJZK_SWITCH_1CH)

            setSetting("board", 85);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 0);
            setSetting("ledWifi", 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(YJZK_SWITCH_3CH)

            setSetting("board", 86);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 0);
            setSetting("ledWifi", 0);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnGPIO", 1, 9);
            setSetting("btnGPIO", 2, 10);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("btnRelay", 2, 2);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayGPIO", 2, 4);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("relayType", 2, RELAY_TYPE_NORMAL);

        #elif defined(XIAOMI_SMART_DESK_LAMP)

            setSetting("board", 87);

            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("relays", 1);
            setSetting("chGPIO", 0, 5);
            setSetting("chGPIO", 1, 4);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);

            setSetting("btnGPIO", 0, 2);
            setSetting("btnGPIO", 1, 14);
            setSetting("btnRelay", 0, 0);
            setSetting("btnLngDelay", 500);
            setSetting("btnDblClick", 0, BUTTON_MODE_NONE);
            setSetting("btnLngClick", 0, BUTTON_MODE_NONE);
            setSetting("btnLngLngClick", 0, BUTTON_MODE_NONE);
            setSetting("btnDblClick", 1, BUTTON_MODE_AP);
            setSetting("btnLngLngClick", 1, BUTTON_MODE_RESET);

            setSetting("enc1stGPIO", 0, 12);
            setSetting("enc2ndGPIO", 0, 13);
            setSetting("encBtnGPIO", 0, 2);
            setSetting("encMode", ENCODER_MODE_RATIO);

        #elif defined(ALLTERCO_SHELLY2)

            setSetting("board", 88);
            setSetting("btnGPIO", 0, 12);
            setSetting("btnGPIO", 1, 14);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 4);
            setSetting("relayGPIO", 1, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(PHYX_ESP12_RGB)

            setSetting("board", 89);

            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("relays", 1);
            setSetting("chGPIO", 0, 4);
            setSetting("chGPIO", 1, 14);
            setSetting("chGPIO", 2, 12);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 3, 0);

        #elif defined(IWOOLE_LED_TABLE_LAMP)

            setSetting("board", 90);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 12);
            setSetting("chGPIO", 1, 5);
            setSetting("chGPIO", 2, 14);
            setSetting("chGPIO", 3, 4);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #elif defined(EXS_WIFI_RELAY_V50)

            setSetting("board", 91);

            setSetting("btnGPIO", 0, 5);
            setSetting("btnGPIO", 1, 4);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);

            setSetting("relayGPIO", 0, 14);
            setSetting("relayGPIO", 1, 13);
            setSetting("relayResetGPIO", 0, 16);
            setSetting("relayResetGPIO", 1, 12);
            setSetting("relayType", 0, RELAY_TYPE_LATCHED);
            setSetting("relayType", 0, RELAY_TYPE_LATCHED);

            setSetting("ledGPIO", 1, 15);
            setSetting("ledLogic", 1, 0);

        #elif defined(TONBUX_XSSSA01)

            setSetting("board", 92);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 5);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(EUROMATE_WIFI_STECKER_SCHUKO_V2)

            setSetting("board", 93);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("ledGPIO", 1, 12);
            setSetting("ledLogic", 1, 1);
            setSetting("btnGPIO", 0, 5);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 4);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);

        #elif defined(OUKITEL_P1)

            setSetting("board", 94);
            setSetting("ledGPIO", 0, 0);
            setSetting("ledLogic", 0, 0);
            setSetting("btnGPIO", 0, 13);
            setSetting("btnRelay", 0, 0);
            setSetting("relayGPIO", 0, 12);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayGPIO", 1, 15);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);

        #elif defined(DIGOO_NX_SP202)

            setSetting("board", 95);
            setSetting("ledGPIO", 0, 13);
            setSetting("ledLogic", 0, 1);
            setSetting("btnGPIO", 0, 0);
            setSetting("btnRelay", 0, 0);
            setSetting("btnGPIO", 1, 16);
            setSetting("btnRelay", 1, 1);
            setSetting("relayGPIO", 0, 15);
            setSetting("relayType", 0, RELAY_TYPE_NORMAL);
            setSetting("relayGPIO", 1, 14);
            setSetting("relayType", 1, RELAY_TYPE_NORMAL);
            setSetting("selGPIO", 12);
            setSetting("cf1GPIO", 5);
            setSetting("cfGPIO", 4);
            setSetting("pwrRatioC", 23296);
            setSetting("pwrRatioV", 310085);
            setSetting("pwrRatioP", 3368471);
            setSetting("hlwSelC", LOW);
            setSetting("hlwIntM", FALLING);

        #elif defined(FOXEL_LIGHTFOX_DUAL)

            setSetting("board", 96);
            setSetting("btnRelay", 0, 0);
            setSetting("btnRelay", 1, 1);
            setSetting("btnRelay", 2, 1);
            setSetting("btnRelay", 3, 0);
            setSetting("relayProvider", RELAY_PROVIDER_DUAL);
            setSetting("relays", 2);

        #elif defined(GENERIC_GU10)

            setSetting("board", 97);
            setSetting("relayProvider", RELAY_PROVIDER_LIGHT);
            setSetting("lightProvider", LIGHT_PROVIDER_DIMMER);
            setSetting("chGPIO", 0, 14);
            setSetting("chGPIO", 1, 12);
            setSetting("chGPIO", 2, 13);
            setSetting("chGPIO", 3, 4);
            setSetting("chLogic", 0, 0);
            setSetting("chLogic", 1, 0);
            setSetting("chLogic", 2, 0);
            setSetting("chLogic", 3, 0);
            setSetting("relays", 1);

        #else




        #endif

    }

    saveSettings();

}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/mqtt.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/mqtt.ino"
#if MQTT_SUPPORT

#include <EEPROM_Rotate.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <vector>
#include <utility>
#include <Ticker.h>

#if MQTT_USE_ASYNC

#include <AsyncMqttClient.h>
AsyncMqttClient _mqtt;

#else

#include <PubSubClient.h>
PubSubClient _mqtt;
bool _mqtt_connected = false;

WiFiClient _mqtt_client;
#if ASYNC_TCP_SSL_ENABLED
WiFiClientSecure _mqtt_client_secure;
#endif

#endif

bool _mqtt_enabled = MQTT_ENABLED;
bool _mqtt_use_json = false;
unsigned long _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;
unsigned long _mqtt_last_connection = 0;
bool _mqtt_connecting = false;
unsigned char _mqtt_qos = MQTT_QOS;
bool _mqtt_retain = MQTT_RETAIN;
unsigned long _mqtt_keepalive = MQTT_KEEPALIVE;
String _mqtt_topic;
String _mqtt_topic_json;
String _mqtt_setter;
String _mqtt_getter;
bool _mqtt_forward;
String _mqtt_user;
String _mqtt_pass;
String _mqtt_will;
String _mqtt_server;
uint16_t _mqtt_port;
String _mqtt_clientid;

std::vector<mqtt_callback_f> _mqtt_callbacks;

typedef struct {
    unsigned char parent = 255;
    char * topic;
    char * message = NULL;
} mqtt_message_t;
std::vector<mqtt_message_t> _mqtt_queue;
Ticker _mqtt_flush_ticker;





void _mqttConnect() {


    if (!_mqtt_enabled) return;


    if (_mqtt.connected() || _mqtt_connecting) return;


    if (millis() - _mqtt_last_connection < _mqtt_reconnect_delay) return;


    _mqtt_reconnect_delay += MQTT_RECONNECT_DELAY_STEP;
    if (_mqtt_reconnect_delay > MQTT_RECONNECT_DELAY_MAX) {
        _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MAX;
    }

    #if MDNS_CLIENT_SUPPORT
        _mqtt_server = mdnsResolve(_mqtt_server);
    #endif

    DEBUG_MSG_P(PSTR("[MQTT] Connecting to broker at %s:%u\n"), _mqtt_server.c_str(), _mqtt_port);

    #if MQTT_USE_ASYNC
        _mqtt_connecting = true;

        _mqtt.setServer(_mqtt_server.c_str(), _mqtt_port);
        _mqtt.setClientId(_mqtt_clientid.c_str());
        _mqtt.setKeepAlive(_mqtt_keepalive);
        _mqtt.setCleanSession(false);
        _mqtt.setWill(_mqtt_will.c_str(), _mqtt_qos, _mqtt_retain, "0");
        if (_mqtt_user.length() && _mqtt_pass.length()) {
            DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqtt_user.c_str());
            _mqtt.setCredentials(_mqtt_user.c_str(), _mqtt_pass.c_str());
        }

        #if ASYNC_TCP_SSL_ENABLED

            bool secure = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
            _mqtt.setSecure(secure);
            if (secure) {
                DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));
                unsigned char fp[20] = {0};
                if (sslFingerPrintArray(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {
                    _mqtt.addServerFingerprint(fp);
                } else {
                    DEBUG_MSG_P(PSTR("[MQTT] Wrong fingerprint\n"));
                }
            }

        #endif

        DEBUG_MSG_P(PSTR("[MQTT] Client ID: %s\n"), _mqtt_clientid.c_str());
        DEBUG_MSG_P(PSTR("[MQTT] QoS: %d\n"), _mqtt_qos);
        DEBUG_MSG_P(PSTR("[MQTT] Retain flag: %d\n"), _mqtt_retain ? 1 : 0);
        DEBUG_MSG_P(PSTR("[MQTT] Keepalive time: %ds\n"), _mqtt_keepalive);
        DEBUG_MSG_P(PSTR("[MQTT] Will topic: %s\n"), _mqtt_will.c_str());

        _mqtt.connect();

    #else

        bool response = true;

        #if ASYNC_TCP_SSL_ENABLED

            bool secure = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
            if (secure) {
                DEBUG_MSG_P(PSTR("[MQTT] Using SSL\n"));
                if (_mqtt_client_secure.connect(_mqtt_server.c_str(), _mqtt_port)) {
                    char fp[60] = {0};
                    if (sslFingerPrintChar(getSetting("mqttFP", MQTT_SSL_FINGERPRINT).c_str(), fp)) {
                        if (_mqtt_client_secure.verify(fp, _mqtt_server.c_str())) {
                            _mqtt.setClient(_mqtt_client_secure);
                        } else {
                            DEBUG_MSG_P(PSTR("[MQTT] Invalid fingerprint\n"));
                            response = false;
                        }
                        _mqtt_client_secure.stop();
                        yield();
                    } else {
                        DEBUG_MSG_P(PSTR("[MQTT] Wrong fingerprint\n"));
                        response = false;
                    }
                } else {
                    DEBUG_MSG_P(PSTR("[MQTT] Client connection failed\n"));
                    response = false;
                }

            } else {
                _mqtt.setClient(_mqtt_client);
            }

        #else

            _mqtt.setClient(_mqtt_client);

        #endif

        if (response) {

            _mqtt.setServer(_mqtt_server.c_str(), _mqtt_port);

            if (_mqtt_user.length() && _mqtt_pass.length()) {
                DEBUG_MSG_P(PSTR("[MQTT] Connecting as user %s\n"), _mqtt_user.c_str());
                response = _mqtt.connect(_mqtt_clientid.c_str(), _mqtt_user.c_str(), _mqtt_pass.c_str(), _mqtt_will.c_str(), _mqtt_qos, _mqtt_retain, "0");
            } else {
    response = _mqtt.connect(_mqtt_clientid.c_str(), _mqtt_will.c_str(), _mqtt_qos, _mqtt_retain, "0");
            }

            DEBUG_MSG_P(PSTR("[MQTT] Client ID: %s\n"), _mqtt_clientid.c_str());
            DEBUG_MSG_P(PSTR("[MQTT] QoS: %d\n"), _mqtt_qos);
            DEBUG_MSG_P(PSTR("[MQTT] Retain flag: %d\n"), _mqtt_retain ? 1 : 0);
            DEBUG_MSG_P(PSTR("[MQTT] Keepalive time: %ds\n"), _mqtt_keepalive);
            DEBUG_MSG_P(PSTR("[MQTT] Will topic: %s\n"), _mqtt_will.c_str());

        }

        if (response) {
            _mqttOnConnect();
        } else {
            DEBUG_MSG_P(PSTR("[MQTT] Connection failed\n"));
     _mqtt_last_connection = millis();
        }

    #endif

}

void _mqttPlaceholders(String& text) {

    text.replace("{hostname}", getSetting("hostname"));
    text.replace("{magnitude}", "#");

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    text.replace("{mac}", mac);

}

template<typename T>
void _mqttApplySetting(T& current, T& updated) {
    if (current != updated) {
        current = std::move(updated);
        mqttDisconnect();
    }
}

template<typename T>
void _mqttApplySetting(T& current, const T& updated) {
    if (current != updated) {
        current = updated;
        mqttDisconnect();
    }
}

template<typename T>
void _mqttApplyTopic(T& current, const char* magnitude) {
    String updated = mqttTopic(magnitude, false);
    if (current != updated) {
        mqttFlush();
        current = std::move(updated);
    }
}

void _mqttConfigure() {


    {
        String server = getSetting("mqttServer", MQTT_SERVER);
        uint16_t port = getSetting("mqttPort", MQTT_PORT).toInt();
        bool enabled = false;
        if (server.length()) {
            enabled = getSetting("mqttEnabled", MQTT_ENABLED).toInt() == 1;
        }

        _mqttApplySetting(_mqtt_server, server);
        _mqttApplySetting(_mqtt_enabled, enabled);
        _mqttApplySetting(_mqtt_port, port);

        if (!enabled) return;
    }


    {
        String topic = getSetting("mqttTopic", MQTT_TOPIC);
        if (topic.endsWith("/")) topic.remove(topic.length()-1);


        _mqttPlaceholders(topic);

        if (topic.indexOf("#") == -1) topic.concat("/#");
        _mqttApplySetting(_mqtt_topic, topic);

        _mqttApplyTopic(_mqtt_will, MQTT_TOPIC_STATUS);
    }


    {
        String setter = getSetting("mqttSetter", MQTT_SETTER);
        String getter = getSetting("mqttGetter", MQTT_GETTER);
        bool forward = !setter.equals(getter) && RELAY_REPORT_STATUS;

        _mqttApplySetting(_mqtt_setter, setter);
        _mqttApplySetting(_mqtt_getter, getter);
        _mqttApplySetting(_mqtt_forward, forward);
    }


    {
        String user = getSetting("mqttUser", MQTT_USER);
        _mqttPlaceholders(user);

        String pass = getSetting("mqttPassword", MQTT_PASS);

        unsigned char qos = getSetting("mqttQoS", MQTT_QOS).toInt();
        bool retain = getSetting("mqttRetain", MQTT_RETAIN).toInt() == 1;
        unsigned long keepalive = getSetting("mqttKeep", MQTT_KEEPALIVE).toInt();

        String id = getSetting("mqttClientID", getIdentifier());
        _mqttPlaceholders(id);

        _mqttApplySetting(_mqtt_user, user);
        _mqttApplySetting(_mqtt_pass, pass);
        _mqttApplySetting(_mqtt_qos, qos);
        _mqttApplySetting(_mqtt_retain, retain);
        _mqttApplySetting(_mqtt_keepalive, keepalive);
        _mqttApplySetting(_mqtt_clientid, id);
    }


    {
        _mqttApplySetting(_mqtt_use_json, getSetting("mqttUseJson", MQTT_USE_JSON).toInt() == 1);
        _mqttApplyTopic(_mqtt_topic_json, MQTT_TOPIC_JSON);
    }

    _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;

}

void _mqttBackwards() {
    String mqttTopic = getSetting("mqttTopic", MQTT_TOPIC);
    if (mqttTopic.indexOf("{identifier}") > 0) {
        mqttTopic.replace("{identifier}", "{hostname}");
        setSetting("mqttTopic", mqttTopic);
    }
}

void _mqttInfo() {
    DEBUG_MSG_P(PSTR("[MQTT] Async %s, SSL %s, Autoconnect %s\n"),
        MQTT_USE_ASYNC ? "ENABLED" : "DISABLED",
        ASYNC_TCP_SSL_ENABLED ? "ENABLED" : "DISABLED",
        MQTT_AUTOCONNECT ? "ENABLED" : "DISABLED"
    );
    DEBUG_MSG_P(PSTR("[MQTT] Client %s, %s\n"),
        _mqtt_enabled ? "ENABLED" : "DISABLED",
        _mqtt.connected() ? "CONNECTED" : "DISCONNECTED"
    );
    DEBUG_MSG_P(PSTR("[MQTT] Retry %s (Now %u, Last %u, Delay %u, Step %u)\n"),
        _mqtt_connecting ? "CONNECTING" : "WAITING",
        millis(),
        _mqtt_last_connection,
        _mqtt_reconnect_delay,
        MQTT_RECONNECT_DELAY_STEP
    );
}





#if WEB_SUPPORT

bool _mqttWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "mqtt", 3) == 0);
}

void _mqttWebSocketOnSend(JsonObject& root) {
    root["mqttVisible"] = 1;
    root["mqttStatus"] = mqttConnected();
    root["mqttEnabled"] = mqttEnabled();
    root["mqttServer"] = getSetting("mqttServer", MQTT_SERVER);
    root["mqttPort"] = getSetting("mqttPort", MQTT_PORT);
    root["mqttUser"] = getSetting("mqttUser", MQTT_USER);
    root["mqttClientID"] = getSetting("mqttClientID");
    root["mqttPassword"] = getSetting("mqttPassword", MQTT_PASS);
    root["mqttKeep"] = _mqtt_keepalive;
    root["mqttRetain"] = _mqtt_retain;
    root["mqttQoS"] = _mqtt_qos;
    #if ASYNC_TCP_SSL_ENABLED
        root["mqttsslVisible"] = 1;
        root["mqttUseSSL"] = getSetting("mqttUseSSL", MQTT_SSL_ENABLED).toInt() == 1;
        root["mqttFP"] = getSetting("mqttFP", MQTT_SSL_FINGERPRINT);
    #endif
    root["mqttTopic"] = getSetting("mqttTopic", MQTT_TOPIC);
    root["mqttUseJson"] = getSetting("mqttUseJson", MQTT_USE_JSON).toInt() == 1;
}

#endif





#if TERMINAL_SUPPORT

void _mqttInitCommands() {

    terminalRegisterCommand(F("MQTT.RESET"), [](Embedis* e) {
        _mqttConfigure();
        mqttDisconnect();
        terminalOK();
    });

    terminalRegisterCommand(F("MQTT.INFO"), [](Embedis* e) {
        _mqttInfo();
        terminalOK();
    });

}

#endif





void _mqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {


        mqttSubscribe(MQTT_TOPIC_ACTION);


        systemSendHeartbeat();

    }

    if (type == MQTT_MESSAGE_EVENT) {


        String t = mqttMagnitude((char *) topic);


        if (t.equals(MQTT_TOPIC_ACTION)) {
            if (strcmp(payload, MQTT_ACTION_RESET) == 0) {
                deferredReset(100, CUSTOM_RESET_MQTT);
            }
        }

    }

}

void _mqttOnConnect() {

    DEBUG_MSG_P(PSTR("[MQTT] Connected!\n"));
    _mqtt_reconnect_delay = MQTT_RECONNECT_DELAY_MIN;

    _mqtt_last_connection = millis();
    _mqtt_connecting = false;


    mqttUnsubscribeRaw("#");


    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (_mqtt_callbacks[i])(MQTT_CONNECT_EVENT, NULL, NULL);
    }

}

void _mqttOnDisconnect() {


    _mqtt_last_connection = millis();
    _mqtt_connecting = false;

    DEBUG_MSG_P(PSTR("[MQTT] Disconnected!\n"));


    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (_mqtt_callbacks[i])(MQTT_DISCONNECT_EVENT, NULL, NULL);
    }

}

void _mqttOnMessage(char* topic, char* payload, unsigned int len) {

    if (len == 0) return;

    char message[len + 1];
    strlcpy(message, (char *) payload, len + 1);

    #if MQTT_SKIP_RETAINED
        if (millis() - _mqtt_last_connection < MQTT_SKIP_TIME) {
            DEBUG_MSG_P(PSTR("[MQTT] Received %s => %s - SKIPPED\n"), topic, message);
   return;
  }
    #endif
    DEBUG_MSG_P(PSTR("[MQTT] Received %s => %s\n"), topic, message);


    for (unsigned char i = 0; i < _mqtt_callbacks.size(); i++) {
        (_mqtt_callbacks[i])(MQTT_MESSAGE_EVENT, topic, message);
    }

}
# 491 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/mqtt.ino"
String mqttMagnitude(char * topic) {

    String pattern = _mqtt_topic + _mqtt_setter;
    int position = pattern.indexOf("#");
    if (position == -1) return String();
    String start = pattern.substring(0, position);
    String end = pattern.substring(position + 1);

    String magnitude = String(topic);
    if (magnitude.startsWith(start) && magnitude.endsWith(end)) {
        magnitude.replace(start, "");
        magnitude.replace(end, "");
    } else {
        magnitude = String();
    }

    return magnitude;

}
# 519 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/mqtt.ino"
String mqttTopic(const char * magnitude, bool is_set) {
    String output = _mqtt_topic;
    output.replace("#", magnitude);
    output += is_set ? _mqtt_setter : _mqtt_getter;
    return output;
}
# 535 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/mqtt.ino"
String mqttTopic(const char * magnitude, unsigned int index, bool is_set) {
    char buffer[strlen(magnitude)+5];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s/%d"), magnitude, index);
    return mqttTopic(buffer, is_set);
}



void mqttSendRaw(const char * topic, const char * message, bool retain) {

    if (_mqtt.connected()) {
        #if MQTT_USE_ASYNC
            unsigned int packetId = _mqtt.publish(topic, _mqtt_qos, retain, message);
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s (PID %d)\n"), topic, message, packetId);
        #else
            _mqtt.publish(topic, message, retain);
            DEBUG_MSG_P(PSTR("[MQTT] Sending %s => %s\n"), topic, message);
        #endif
    }
}


void mqttSendRaw(const char * topic, const char * message) {
    mqttSendRaw (topic, message, _mqtt_retain);
}

void mqttSend(const char * topic, const char * message, bool force, bool retain) {

    bool useJson = force ? false : _mqtt_use_json;


    if (useJson) {


        mqttEnqueue(topic, message);


        _mqtt_flush_ticker.once_ms(MQTT_USE_JSON_DELAY, mqttFlush);


    } else {
        mqttSendRaw(mqttTopic(topic, false).c_str(), message, retain);

    }

}

void mqttSend(const char * topic, const char * message, bool force) {
    mqttSend(topic, message, force, _mqtt_retain);
}

void mqttSend(const char * topic, const char * message) {
    mqttSend(topic, message, false);
}

void mqttSend(const char * topic, unsigned int index, const char * message, bool force, bool retain) {
    char buffer[strlen(topic)+5];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s/%d"), topic, index);
    mqttSend(buffer, message, force, retain);
}

void mqttSend(const char * topic, unsigned int index, const char * message, bool force) {
    mqttSend(topic, index, message, force, _mqtt_retain);
}

void mqttSend(const char * topic, unsigned int index, const char * message) {
    mqttSend(topic, index, message, false);
}



unsigned char _mqttBuildTree(JsonObject& root, char parent) {

    unsigned char count = 0;


    for (unsigned char i=0; i<_mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        if (element.parent == parent) {
            ++count;
            JsonObject& elements = root.createNestedObject(element.topic);
            unsigned char num = _mqttBuildTree(elements, i);
            if (0 == num) {
                if (isNumber(element.message)) {
                    double value = atof(element.message);
                    if (value == int(value)) {
                        root.set(element.topic, int(value));
                    } else {
                        root.set(element.topic, value);
                    }
                } else {
                    root.set(element.topic, element.message);
                }
            }
        }
    }

    return count;

}

void mqttFlush() {

    if (!_mqtt.connected()) return;
    if (_mqtt_queue.size() == 0) return;


    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    _mqttBuildTree(root, 255);


    #if NTP_SUPPORT && MQTT_ENQUEUE_DATETIME
        if (ntpSynced()) root[MQTT_TOPIC_TIME] = ntpDateTime();
    #endif
    #if MQTT_ENQUEUE_MAC
        root[MQTT_TOPIC_MAC] = WiFi.macAddress();
    #endif
    #if MQTT_ENQUEUE_HOSTNAME
        root[MQTT_TOPIC_HOSTNAME] = getSetting("hostname");
    #endif
    #if MQTT_ENQUEUE_IP
        root[MQTT_TOPIC_IP] = getIP();
    #endif
    #if MQTT_ENQUEUE_MESSAGE_ID
        root[MQTT_TOPIC_MESSAGE_ID] = (Rtcmem->mqtt)++;
    #endif


    String output;
    root.printTo(output);
    jsonBuffer.clear();

    mqttSendRaw(_mqtt_topic_json.c_str(), output.c_str(), false);


    for (unsigned char i = 0; i < _mqtt_queue.size(); i++) {
        mqtt_message_t element = _mqtt_queue[i];
        free(element.topic);
        if (element.message) {
            free(element.message);
        }
    }
    _mqtt_queue.clear();

}

int8_t mqttEnqueue(const char * topic, const char * message, unsigned char parent) {



    if (!_mqtt.connected()) return -1;


    if (_mqtt_queue.size() >= MQTT_QUEUE_MAX_SIZE) mqttFlush();

    int8_t index = _mqtt_queue.size();


    mqtt_message_t element;
    element.parent = parent;
    element.topic = strdup(topic);
    if (NULL != message) {
        element.message = strdup(message);
    }
    _mqtt_queue.push_back(element);

    return index;

}

int8_t mqttEnqueue(const char * topic, const char * message) {
    return mqttEnqueue(topic, message, 255);
}



void mqttSubscribeRaw(const char * topic) {
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        #if MQTT_USE_ASYNC
            unsigned int packetId = _mqtt.subscribe(topic, _mqtt_qos);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s (PID %d)\n"), topic, packetId);
        #else
            _mqtt.subscribe(topic, _mqtt_qos);
            DEBUG_MSG_P(PSTR("[MQTT] Subscribing to %s\n"), topic);
        #endif
    }
}

void mqttSubscribe(const char * topic) {
    mqttSubscribeRaw(mqttTopic(topic, true).c_str());
}

void mqttUnsubscribeRaw(const char * topic) {
    if (_mqtt.connected() && (strlen(topic) > 0)) {
        #if MQTT_USE_ASYNC
            unsigned int packetId = _mqtt.unsubscribe(topic);
            DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing to %s (PID %d)\n"), topic, packetId);
        #else
            _mqtt.unsubscribe(topic);
            DEBUG_MSG_P(PSTR("[MQTT] Unsubscribing to %s\n"), topic);
        #endif
    }
}

void mqttUnsubscribe(const char * topic) {
    mqttUnsubscribeRaw(mqttTopic(topic, true).c_str());
}



void mqttEnabled(bool status) {
    _mqtt_enabled = status;
}

bool mqttEnabled() {
    return _mqtt_enabled;
}

bool mqttConnected() {
    return _mqtt.connected();
}

void mqttDisconnect() {
    if (_mqtt.connected()) {
        DEBUG_MSG_P(PSTR("[MQTT] Disconnecting\n"));
        _mqtt.disconnect();
    }
}

bool mqttForward() {
    return _mqtt_forward;
}

void mqttRegister(mqtt_callback_f callback) {
    _mqtt_callbacks.push_back(callback);
}

void mqttSetBroker(IPAddress ip, unsigned int port) {
    setSetting("mqttServer", ip.toString());
    _mqtt_server = ip.toString();

    setSetting("mqttPort", port);
    _mqtt_port = port;

    mqttEnabled(MQTT_AUTOCONNECT);
}

void mqttSetBrokerIfNone(IPAddress ip, unsigned int port) {
    if (getSetting("mqttServer", MQTT_SERVER).length() == 0) mqttSetBroker(ip, port);
}





void mqttSetup() {

    _mqttBackwards();
    _mqttInfo();

    #if MQTT_USE_ASYNC

        _mqtt.onConnect([](bool sessionPresent) {
            _mqttOnConnect();
        });
        _mqtt.onDisconnect([](AsyncMqttClientDisconnectReason reason) {
            if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
                DEBUG_MSG_P(PSTR("[MQTT] TCP Disconnected\n"));
            }
            if (reason == AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED) {
                DEBUG_MSG_P(PSTR("[MQTT] Identifier Rejected\n"));
            }
            if (reason == AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE) {
                DEBUG_MSG_P(PSTR("[MQTT] Server unavailable\n"));
            }
            if (reason == AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS) {
                DEBUG_MSG_P(PSTR("[MQTT] Malformed credentials\n"));
            }
            if (reason == AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED) {
                DEBUG_MSG_P(PSTR("[MQTT] Not authorized\n"));
            }
            #if ASYNC_TCP_SSL_ENABLED
            if (reason == AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT) {
                DEBUG_MSG_P(PSTR("[MQTT] Bad fingerprint\n"));
            }
            #endif
            _mqttOnDisconnect();
        });
        _mqtt.onMessage([](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
            _mqttOnMessage(topic, payload, len);
        });
        _mqtt.onSubscribe([](uint16_t packetId, uint8_t qos) {
            DEBUG_MSG_P(PSTR("[MQTT] Subscribe ACK for PID %d\n"), packetId);
        });
        _mqtt.onPublish([](uint16_t packetId) {
            DEBUG_MSG_P(PSTR("[MQTT] Publish ACK for PID %d\n"), packetId);
        });

    #else

        _mqtt.setCallback([](char* topic, byte* payload, unsigned int length) {
            _mqttOnMessage(topic, (char *) payload, length);
        });

    #endif

    _mqttConfigure();
    mqttRegister(_mqttCallback);

    #if WEB_SUPPORT
        wsOnSendRegister(_mqttWebSocketOnSend);
        wsOnReceiveRegister(_mqttWebSocketOnReceive);
    #endif

    #if TERMINAL_SUPPORT
        _mqttInitCommands();
    #endif


    espurnaRegisterLoop(mqttLoop);
    espurnaRegisterReload(_mqttConfigure);

}

void mqttLoop() {

    if (WiFi.status() != WL_CONNECTED) return;

    #if MQTT_USE_ASYNC

        _mqttConnect();

    #else

        if (_mqtt.connected()) {

            _mqtt.loop();

        } else {

            if (_mqtt_connected) {
                _mqttOnDisconnect();
                _mqtt_connected = false;
            }

            _mqttConnect();

        }

    #endif

}

#else

bool mqttForward() {
    return false;
}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/netbios.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/netbios.ino"
#if NETBIOS_SUPPORT

#include <ESP8266NetBIOS.h>

void netbiosSetup() {
    static WiFiEventHandler _netbios_wifi_onSTA = WiFi.onStationModeGotIP([](WiFiEventStationModeGotIP ipInfo) {
        NBNS.begin(getSetting("hostname").c_str());
        DEBUG_MSG_P(PSTR("[NETBIOS] Configured\n"));
    });
}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/nofuss.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/nofuss.ino"
#if NOFUSS_SUPPORT

#include "NoFUSSClient.h"

unsigned long _nofussLastCheck = 0;
unsigned long _nofussInterval = 0;
bool _nofussEnabled = false;





#if WEB_SUPPORT

bool _nofussWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "nofuss", 6) == 0);
}

void _nofussWebSocketOnSend(JsonObject& root) {
    root["nofussVisible"] = 1;
    root["nofussEnabled"] = getSetting("nofussEnabled", NOFUSS_ENABLED).toInt() == 1;
    root["nofussServer"] = getSetting("nofussServer", NOFUSS_SERVER);
}

#endif

void _nofussConfigure() {

    String nofussServer = getSetting("nofussServer", NOFUSS_SERVER);
    #if MDNS_CLIENT_SUPPORT
        nofussServer = mdnsResolve(nofussServer);
    #endif

    if (nofussServer.length() == 0) {
        setSetting("nofussEnabled", 0);
        _nofussEnabled = false;
    } else {
        _nofussEnabled = getSetting("nofussEnabled", NOFUSS_ENABLED).toInt() == 1;
    }
    _nofussInterval = getSetting("nofussInterval", NOFUSS_INTERVAL).toInt();
    _nofussLastCheck = 0;

    if (!_nofussEnabled) {

        DEBUG_MSG_P(PSTR("[NOFUSS] Disabled\n"));

    } else {

        NoFUSSClient.setServer(nofussServer);
        NoFUSSClient.setDevice(APP_NAME "_" DEVICE);
        NoFUSSClient.setVersion(APP_VERSION);
        NoFUSSClient.setBuild(String(__UNIX_TIMESTAMP__));

        DEBUG_MSG_P(PSTR("[NOFUSS] Server : %s\n"), nofussServer.c_str());
        DEBUG_MSG_P(PSTR("[NOFUSS] Dervice: %s\n"), APP_NAME "_" DEVICE);
        DEBUG_MSG_P(PSTR("[NOFUSS] Version: %s\n"), APP_VERSION);
        DEBUG_MSG_P(PSTR("[NOFUSS] Build: %s\n"), String(__UNIX_TIMESTAMP__).c_str());
        DEBUG_MSG_P(PSTR("[NOFUSS] Enabled\n"));

    }

}

#if TERMINAL_SUPPORT

void _nofussInitCommands() {

    terminalRegisterCommand(F("NOFUSS"), [](Embedis* e) {
        terminalOK();
        nofussRun();
    });

}

#endif



void nofussRun() {
    NoFUSSClient.handle();
    _nofussLastCheck = millis();
}

void nofussSetup() {

    _nofussConfigure();

    NoFUSSClient.onMessage([](nofuss_t code) {

        if (code == NOFUSS_START) {
         DEBUG_MSG_P(PSTR("[NoFUSS] Start\n"));
        }

        if (code == NOFUSS_UPTODATE) {
         DEBUG_MSG_P(PSTR("[NoFUSS] Already in the last version\n"));
        }

        if (code == NOFUSS_NO_RESPONSE_ERROR) {
         DEBUG_MSG_P(PSTR("[NoFUSS] Wrong server response: %d %s\n"), NoFUSSClient.getErrorNumber(), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_PARSE_ERROR) {
         DEBUG_MSG_P(PSTR("[NoFUSS] Error parsing server response\n"));
        }

        if (code == NOFUSS_UPDATING) {
         DEBUG_MSG_P(PSTR("[NoFUSS] Updating\n"));
         DEBUG_MSG_P(PSTR("         New version: %s\n"), (char *) NoFUSSClient.getNewVersion().c_str());
         DEBUG_MSG_P(PSTR("         Firmware: %s\n"), (char *) NoFUSSClient.getNewFirmware().c_str());
         DEBUG_MSG_P(PSTR("         File System: %s\n"), (char *) NoFUSSClient.getNewFileSystem().c_str());
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"message\": 1}"));
            #endif


            eepromRotate(false);
        }

        if (code == NOFUSS_FILESYSTEM_UPDATE_ERROR) {
         DEBUG_MSG_P(PSTR("[NoFUSS] File System Update Error: %s\n"), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FILESYSTEM_UPDATED) {
         DEBUG_MSG_P(PSTR("[NoFUSS] File System Updated\n"));
        }

        if (code == NOFUSS_FIRMWARE_UPDATE_ERROR) {
            DEBUG_MSG_P(PSTR("[NoFUSS] Firmware Update Error: %s\n"), (char *) NoFUSSClient.getErrorString().c_str());
        }

        if (code == NOFUSS_FIRMWARE_UPDATED) {
         DEBUG_MSG_P(PSTR("[NoFUSS] Firmware Updated\n"));
        }

        if (code == NOFUSS_RESET) {
         DEBUG_MSG_P(PSTR("[NoFUSS] Resetting board\n"));
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"action\": \"reload\"}"));
            #endif
            nice_delay(100);
        }

        if (code == NOFUSS_END) {
            DEBUG_MSG_P(PSTR("[NoFUSS] End\n"));
            eepromRotate(true);
        }

    });

    #if WEB_SUPPORT
        wsOnSendRegister(_nofussWebSocketOnSend);
        wsOnReceiveRegister(_nofussWebSocketOnReceive);
    #endif

    #if TERMINAL_SUPPORT
        _nofussInitCommands();
    #endif


    espurnaRegisterLoop(nofussLoop);
    espurnaRegisterReload(_nofussConfigure);

}

void nofussLoop() {

    if (!_nofussEnabled) return;
    if (!wifiConnected()) return;
    if ((_nofussLastCheck > 0) && ((millis() - _nofussLastCheck) < _nofussInterval)) return;

    nofussRun();

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ntp.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ntp.ino"
#if NTP_SUPPORT

#include <TimeLib.h>
#include <WiFiClient.h>
#include <Ticker.h>

#include "libs/NtpClientWrap.h"

Ticker _ntp_defer;

bool _ntp_report = false;
bool _ntp_configure = false;
bool _ntp_want_sync = false;





#if WEB_SUPPORT

bool _ntpWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "ntp", 3) == 0);
}

void _ntpWebSocketOnSend(JsonObject& root) {
    root["ntpVisible"] = 1;
    root["ntpStatus"] = (timeStatus() == timeSet);
    root["ntpServer"] = getSetting("ntpServer", NTP_SERVER);
    root["ntpOffset"] = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    root["ntpDST"] = getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1;
    root["ntpRegion"] = getSetting("ntpRegion", NTP_DST_REGION).toInt();
}

#endif

time_t _ntpSyncProvider() {
    _ntp_want_sync = true;
    return 0;
}

void _ntpWantSync() {
    _ntp_want_sync = true;
}



int inline _ntpSyncInterval() {
    return secureRandom(NTP_SYNC_INTERVAL, NTP_SYNC_INTERVAL * 2);
}

int inline _ntpUpdateInterval() {
    return secureRandom(NTP_UPDATE_INTERVAL, NTP_UPDATE_INTERVAL * 2);
}

void _ntpStart() {

    _ntpConfigure();


    NTPw.setInterval(_ntpSyncInterval(), _ntpUpdateInterval());
    DEBUG_MSG_P(PSTR("[NTP] Update intervals: %us / %us\n"),
        NTPw.getShortInterval(), NTPw.getLongInterval());



    setSyncProvider(_ntpSyncProvider);
    _ntp_want_sync = false;

    setSyncInterval(NTPw.getShortInterval());

}

void _ntpConfigure() {

    _ntp_configure = false;

    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    int sign = offset > 0 ? 1 : -1;
    offset = abs(offset);
    int tz_hours = sign * (offset / 60);
    int tz_minutes = sign * (offset % 60);
    if (NTPw.getTimeZone() != tz_hours || NTPw.getTimeZoneMinutes() != tz_minutes) {
        NTPw.setTimeZone(tz_hours, tz_minutes);
        _ntp_report = true;
    }

    bool daylight = getSetting("ntpDST", NTP_DAY_LIGHT).toInt() == 1;
    if (NTPw.getDayLight() != daylight) {
        NTPw.setDayLight(daylight);
        _ntp_report = true;
    }

    String server = getSetting("ntpServer", NTP_SERVER);
    if (!NTPw.getNtpServerName().equals(server)) {
        NTPw.setNtpServerName(server);
    }

    uint8_t dst_region = getSetting("ntpRegion", NTP_DST_REGION).toInt();
    NTPw.setDSTZone(dst_region);



    NTPw.setNTPTimeout(getSetting("ntpTimeout", NTP_TIMEOUT).toInt());

}

void _ntpReport() {

    _ntp_report = false;

    #if WEB_SUPPORT
        wsSend(_ntpWebSocketOnSend);
    #endif

    if (ntpSynced()) {
        time_t t = now();
        DEBUG_MSG_P(PSTR("[NTP] UTC Time  : %s\n"), ntpDateTime(ntpLocal2UTC(t)).c_str());
        DEBUG_MSG_P(PSTR("[NTP] Local Time: %s\n"), ntpDateTime(t).c_str());
    }

}

#if BROKER_SUPPORT

void inline _ntpBroker() {
    static unsigned char last_minute = 60;
    if (ntpSynced() && (minute() != last_minute)) {
        last_minute = minute();
        brokerPublish(BROKER_MSG_TYPE_DATETIME, MQTT_TOPIC_DATETIME, ntpDateTime().c_str());
    }
}

#endif

void _ntpLoop() {


    if (!wifiConnected()) return;

    if (_ntp_configure) _ntpConfigure();



    if (_ntp_want_sync) {
        _ntp_want_sync = false;
        NTPw.getTime();
    }


    if (_ntp_report) _ntpReport();

    #if BROKER_SUPPORT
        _ntpBroker();
    #endif

}


void _ntpBackwards() {
    moveSetting("ntpServer1", "ntpServer");
    delSetting("ntpServer2");
    delSetting("ntpServer3");
    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    if (-30 < offset && offset < 30) {
        offset *= 60;
        setSetting("ntpOffset", offset);
    }
}



bool ntpSynced() {
    #if NTP_WAIT_FOR_SYNC

        return (NTPw.getFirstSync() > 0);
    #else

        return true;
    #endif
}

String ntpDateTime(time_t t) {
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer),
        PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
        year(t), month(t), day(t), hour(t), minute(t), second(t)
    );
    return String(buffer);
}

String ntpDateTime() {
    if (ntpSynced()) return ntpDateTime(now());
    return String();
}


time_t ntpLocal2UTC(time_t local) {
    int offset = getSetting("ntpOffset", NTP_TIME_OFFSET).toInt();
    if (NTPw.isSummerTime()) offset += 60;
    return local - offset * 60;
}



void ntpSetup() {

    _ntpBackwards();

    #if TERMINAL_SUPPORT
        terminalRegisterCommand(F("NTP"), [](Embedis* e) {
            if (ntpSynced()) {
                _ntpReport();
                terminalOK();
            } else {
                DEBUG_MSG_P(PSTR("[NTP] Not synced\n"));
            }
        });

        terminalRegisterCommand(F("NTP.SYNC"), [](Embedis* e) {
            _ntpWantSync();
            terminalOK();
        });
    #endif

    NTPw.onNTPSyncEvent([](NTPSyncEvent_t error) {
        if (error) {
            #if WEB_SUPPORT
                wsSend_P(PSTR("{\"ntpStatus\": false}"));
            #endif
            if (error == noResponse) {
                DEBUG_MSG_P(PSTR("[NTP] Error: NTP server not reachable\n"));
            } else if (error == invalidAddress) {
                DEBUG_MSG_P(PSTR("[NTP] Error: Invalid NTP server address\n"));
            }
        } else {
            _ntp_report = true;
            setTime(NTPw.getLastNTPSync());
        }
    });

    wifiRegister([](justwifi_messages_t code, char * parameter) {
        if (code == MESSAGE_CONNECTED) {
            if (!ntpSynced()) {
                _ntp_defer.once_ms(secureRandom(NTP_START_DELAY, NTP_START_DELAY * 15), _ntpWantSync);
            }
        }
    });

    #if WEB_SUPPORT
        wsOnSendRegister(_ntpWebSocketOnSend);
        wsOnReceiveRegister(_ntpWebSocketOnReceive);
    #endif


    espurnaRegisterLoop(_ntpLoop);
    espurnaRegisterReload([]() { _ntp_configure = true; });


    _ntpStart();

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ota.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ota.ino"
#include "ArduinoOTA.h"





void _otaConfigure() {
    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(getSetting("hostname").c_str());
    #if USE_PASSWORD
        ArduinoOTA.setPassword(getAdminPass().c_str());
    #endif
}

void _otaLoop() {
    ArduinoOTA.handle();
}





#if TERMINAL_SUPPORT || OTA_MQTT_SUPPORT

#include <ESPAsyncTCP.h>
AsyncClient * _ota_client;
char * _ota_host;
char * _ota_url;
unsigned int _ota_port = 80;
unsigned long _ota_size = 0;

const char OTA_REQUEST_TEMPLATE[] PROGMEM =
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: ESPurna\r\n"
    "Connection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 0\r\n\r\n\r\n";


void _otaFrom(const char * host, unsigned int port, const char * url) {

    if (_ota_host) free(_ota_host);
    if (_ota_url) free(_ota_url);
    _ota_host = strdup(host);
    _ota_url = strdup(url);
    _ota_port = port;
    _ota_size = 0;

    if (_ota_client == NULL) {
        _ota_client = new AsyncClient();
    }

    _ota_client->onDisconnect([](void *s, AsyncClient *c) {

        DEBUG_MSG_P(PSTR("\n"));

        if (Update.end(true)){
            DEBUG_MSG_P(PSTR("[OTA] Success: %u bytes\n"), _ota_size);
            deferredReset(100, CUSTOM_RESET_OTA);
        } else {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
            eepromRotate(true);
        }

        DEBUG_MSG_P(PSTR("[OTA] Disconnected\n"));

        _ota_client->free();
        delete _ota_client;
        _ota_client = NULL;
        free(_ota_host);
        _ota_host = NULL;
        free(_ota_url);
        _ota_url = NULL;

    }, 0);

    _ota_client->onTimeout([](void *s, AsyncClient *c, uint32_t time) {
        _ota_client->close(true);
    }, 0);

    _ota_client->onData([](void * arg, AsyncClient * c, void * data, size_t len) {

        char * p = (char *) data;

        if (_ota_size == 0) {

            Update.runAsync(true);
            if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
                #ifdef DEBUG_PORT
                    Update.printError(DEBUG_PORT);
                #endif
            }

            p = strstr((char *)data, "\r\n\r\n") + 4;
            len = len - (p - (char *) data);

        }

        if (!Update.hasError()) {
            if (Update.write((uint8_t *) p, len) != len) {
                #ifdef DEBUG_PORT
                    Update.printError(DEBUG_PORT);
                #endif
            }
        }

        _ota_size += len;
        DEBUG_MSG_P(PSTR("[OTA] Progress: %u bytes\r"), _ota_size);

        delay(0);

    }, NULL);

    _ota_client->onConnect([](void * arg, AsyncClient * client) {

        #if ASYNC_TCP_SSL_ENABLED
            if (443 == _ota_port) {
                uint8_t fp[20] = {0};
                sslFingerPrintArray(getSetting("otafp", OTA_GITHUB_FP).c_str(), fp);
                SSL * ssl = _ota_client->getSSL();
                if (ssl_match_fingerprint(ssl, fp) != SSL_OK) {
                    DEBUG_MSG_P(PSTR("[OTA] Warning: certificate doesn't match\n"));
                }
            }
        #endif


        eepromRotate(false);

        DEBUG_MSG_P(PSTR("[OTA] Downloading %s\n"), _ota_url);
        char buffer[strlen_P(OTA_REQUEST_TEMPLATE) + strlen(_ota_url) + strlen(_ota_host)];
        snprintf_P(buffer, sizeof(buffer), OTA_REQUEST_TEMPLATE, _ota_url, _ota_host);
        client->write(buffer);

    }, NULL);

    #if ASYNC_TCP_SSL_ENABLED
        bool connected = _ota_client->connect(host, port, 443 == port);
    #else
        bool connected = _ota_client->connect(host, port);
    #endif

    if (!connected) {
        DEBUG_MSG_P(PSTR("[OTA] Connection failed\n"));
        _ota_client->close(true);
    }

}

void _otaFrom(String url) {
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        DEBUG_MSG_P(PSTR("[OTA] Incorrect URL specified\n"));
        return;
    }


    unsigned int port = 80;
    if (url.startsWith("https://")) port = 443;
    url = url.substring(url.indexOf("/") + 2);


    String host = url.substring(0, url.indexOf("/"));


    int p = host.indexOf(":");
    if (p > 0) {
        port = host.substring(p + 1).toInt();
        host = host.substring(0, p);
    }


    String uri = url.substring(url.indexOf("/"));

    _otaFrom(host.c_str(), port, uri.c_str());

}

#endif


#if TERMINAL_SUPPORT

void _otaInitCommands() {

    terminalRegisterCommand(F("OTA"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("Wrong arguments"));
        } else {
            terminalOK();
            String url = String(e->argv[1]);
            _otaFrom(url);
        }
    });

}

#endif

#if OTA_MQTT_SUPPORT

void _otaMQTTCallback(unsigned int type, const char * topic, const char * payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_OTA);
    }

    if (type == MQTT_MESSAGE_EVENT) {

        String t = mqttMagnitude((char *) topic);
        if (t.equals(MQTT_TOPIC_OTA)) {
            DEBUG_MSG_P(PSTR("[OTA] Initiating from URL: %s\n"), payload);
            _otaFrom(payload);
        }
    }
}

#endif



void otaSetup() {

    _otaConfigure();

    #if TERMINAL_SUPPORT
        _otaInitCommands();
    #endif

    #if OTA_MQTT_SUPPORT
        mqttRegister(_otaMQTTCallback);
    #endif


    espurnaRegisterLoop(_otaLoop);
    espurnaRegisterReload(_otaConfigure);



    ArduinoOTA.onStart([]() {


        eepromRotate(false);

        DEBUG_MSG_P(PSTR("[OTA] Start\n"));

        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"message\": 2}"));
        #endif

    });

    ArduinoOTA.onEnd([]() {
        DEBUG_MSG_P(PSTR("\n"));
        DEBUG_MSG_P(PSTR("[OTA] Done, restarting...\n"));
        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"action\": \"reload\"}"));
        #endif
        deferredReset(100, CUSTOM_RESET_OTA);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static unsigned int _progOld;

        unsigned int _prog = (progress / (total / 100));
        if (_prog != _progOld) {
            DEBUG_MSG_P(PSTR("[OTA] Progress: %u%%\r"), _prog);
            _progOld = _prog;
        }
    });

    ArduinoOTA.onError([](ota_error_t error) {
        #if DEBUG_SUPPORT
            DEBUG_MSG_P(PSTR("\n[OTA] Error #%u: "), error);
            if (error == OTA_AUTH_ERROR) DEBUG_MSG_P(PSTR("Auth Failed\n"));
            else if (error == OTA_BEGIN_ERROR) DEBUG_MSG_P(PSTR("Begin Failed\n"));
            else if (error == OTA_CONNECT_ERROR) DEBUG_MSG_P(PSTR("Connect Failed\n"));
            else if (error == OTA_RECEIVE_ERROR) DEBUG_MSG_P(PSTR("Receive Failed\n"));
            else if (error == OTA_END_ERROR) DEBUG_MSG_P(PSTR("End Failed\n"));
        #endif
        eepromRotate(true);
    });

    ArduinoOTA.begin();

}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/relay.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/relay.ino"
#include <EEPROM_Rotate.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

typedef struct {



    unsigned char pin;
    unsigned char type;
    unsigned char reset_pin;
    unsigned long delay_on;
    unsigned long delay_off;
    unsigned char pulse;
    unsigned long pulse_ms;



    bool current_status;
    bool target_status;
    unsigned long fw_start;
    unsigned char fw_count;
    unsigned long change_time;
    bool report;
    bool group_report;



    Ticker pulseTicker;

} relay_t;
std::vector<relay_t> _relays;
bool _relayRecursive = false;
Ticker _relaySaveTicker;





void _relayProviderStatus(unsigned char id, bool status) {


    if (id >= _relays.size()) return;


    _relays[id].current_status = status;

    #if RELAY_PROVIDER == RELAY_PROVIDER_RFBRIDGE
        rfbStatus(id, status);
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_DUAL


        unsigned char mask=0;
        for (unsigned char i=0; i<_relays.size(); i++) {
            if (_relays[i].current_status) mask = mask + (1 << i);
        }

        DEBUG_MSG_P(PSTR("[RELAY] [DUAL] Sending relay mask: %d\n"), mask);


        Serial.flush();
        Serial.write(0xA0);
        Serial.write(0x04);
        Serial.write(mask);
        Serial.write(0xA1);
        Serial.flush();

    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_STM
        Serial.flush();
        Serial.write(0xA0);
        Serial.write(id + 1);
        Serial.write(status);
        Serial.write(0xA1 + status + id);



        delay(100);

        Serial.flush();
    #endif

    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT


        uint8_t physical = _relays.size() - DUMMY_RELAY_COUNT;



        if (id >= physical) {







            if (DUMMY_RELAY_COUNT == lightChannels()) {
                lightState(id-physical, status);
                lightState(true);
            } else if (DUMMY_RELAY_COUNT == (lightChannels() + 1u)) {
                if (id == physical) {
                    lightState(status);
                } else {
                    lightState(id-1-physical, status);
                }
            } else {
                lightState(status);
            }

            lightUpdate(true, true);
            return;

        }

    #endif

    #if (RELAY_PROVIDER == RELAY_PROVIDER_RELAY) || (RELAY_PROVIDER == RELAY_PROVIDER_LIGHT)




        if (_relays[id].type == RELAY_TYPE_NORMAL) {
            digitalWrite(_relays[id].pin, status);
        } else if (_relays[id].type == RELAY_TYPE_INVERSE) {
            digitalWrite(_relays[id].pin, !status);
        } else if (_relays[id].type == RELAY_TYPE_LATCHED || _relays[id].type == RELAY_TYPE_LATCHED_INVERSE) {
            bool pulse = RELAY_TYPE_LATCHED ? HIGH : LOW;
            digitalWrite(_relays[id].pin, !pulse);
            if (GPIO_NONE != _relays[id].reset_pin) digitalWrite(_relays[id].reset_pin, !pulse);
            if (status || (GPIO_NONE == _relays[id].reset_pin)) {
                digitalWrite(_relays[id].pin, pulse);
            } else {
                digitalWrite(_relays[id].reset_pin, pulse);
            }
            nice_delay(RELAY_LATCHING_PULSE);
            digitalWrite(_relays[id].pin, !pulse);
            if (GPIO_NONE != _relays[id].reset_pin) digitalWrite(_relays[id].reset_pin, !pulse);
        }

    #endif

}






void _relayProcess(bool mode) {

    unsigned long current_time = millis();

    for (unsigned char id = 0; id < _relays.size(); id++) {

        bool target = _relays[id].target_status;


        if (target == _relays[id].current_status) continue;


        if (target != mode) continue;


        if (current_time < _relays[id].change_time) continue;

        DEBUG_MSG_P(PSTR("[RELAY] #%d set to %s\n"), id, target ? "ON" : "OFF");


        _relayProviderStatus(id, target);


        #if BROKER_SUPPORT
            brokerPublish(BROKER_MSG_TYPE_STATUS, MQTT_TOPIC_RELAY, id, target ? "1" : "0");
        #endif


        #if MQTT_SUPPORT
            relayMQTT(id);
        #endif

        if (!_relayRecursive) {

            relayPulse(id);



            unsigned char boot_mode = getSetting("relayBoot", id, RELAY_BOOT_MODE).toInt();
            bool save_eeprom = ((RELAY_BOOT_SAME == boot_mode) || (RELAY_BOOT_TOGGLE == boot_mode));
            _relaySaveTicker.once_ms(RELAY_SAVE_DELAY, relaySave, save_eeprom);

            #if WEB_SUPPORT
                wsSend(_relayWebSocketUpdate);
            #endif

        }

        _relays[id].report = false;
        _relays[id].group_report = false;

    }

}

#if defined(ITEAD_SONOFF_IFAN02)

unsigned char _relay_ifan02_speeds[] = {0, 1, 3, 5};

unsigned char getSpeed() {
    unsigned char speed =
        (_relays[1].target_status ? 1 : 0) +
        (_relays[2].target_status ? 2 : 0) +
        (_relays[3].target_status ? 4 : 0);
    for (unsigned char i=0; i<4; i++) {
        if (_relay_ifan02_speeds[i] == speed) return i;
    }
    return 0;
}

void setSpeed(unsigned char speed) {
    if ((0 <= speed) & (speed <= 3)) {
        if (getSpeed() == speed) return;
        unsigned char states = _relay_ifan02_speeds[speed];
        for (unsigned char i=0; i<3; i++) {
            relayStatus(i+1, states & 1 == 1);
            states >>= 1;
        }
    }
}

#endif





void _relayMaskRtcmem(uint32_t mask) {
    Rtcmem->relay = mask;
}

uint32_t _relayMaskRtcmem() {
    return Rtcmem->relay;
}

void relayPulse(unsigned char id) {

    _relays[id].pulseTicker.detach();

    byte mode = _relays[id].pulse;
    if (mode == RELAY_PULSE_NONE) return;
    unsigned long ms = _relays[id].pulse_ms;
    if (ms == 0) return;

    bool status = relayStatus(id);
    bool pulseStatus = (mode == RELAY_PULSE_ON);

    if (pulseStatus != status) {
        DEBUG_MSG_P(PSTR("[RELAY] Scheduling relay #%d back in %lums (pulse)\n"), id, ms);
        _relays[id].pulseTicker.once_ms(ms, relayToggle, id);

        _relays[id].pulse = getSetting("relayPulse", id, RELAY_PULSE_MODE).toInt();
        _relays[id].pulse_ms = 1000 * getSetting("relayTime", id, RELAY_PULSE_MODE).toFloat();
    }

}

bool relayStatus(unsigned char id, bool status, bool report, bool group_report) {

    if (id >= _relays.size()) return false;

    bool changed = false;

    if (_relays[id].current_status == status) {

        if (_relays[id].target_status != status) {
            DEBUG_MSG_P(PSTR("[RELAY] #%d scheduled change cancelled\n"), id);
            _relays[id].target_status = status;
            _relays[id].report = false;
            _relays[id].group_report = false;
            changed = true;
        }


        #if RELAY_PROVIDER == RELAY_PROVIDER_RFBRIDGE
            rfbStatus(id, status);
        #endif


        relayPulse(id);

    } else {

        unsigned long current_time = millis();
        unsigned long fw_end = _relays[id].fw_start + 1000 * RELAY_FLOOD_WINDOW;
        unsigned long delay = status ? _relays[id].delay_on : _relays[id].delay_off;

        _relays[id].fw_count++;
        _relays[id].change_time = current_time + delay;


        if (current_time < _relays[id].fw_start || fw_end <= current_time) {


            _relays[id].fw_start = current_time;
            _relays[id].fw_count = 1;


        } else if (_relays[id].fw_count >= RELAY_FLOOD_CHANGES) {



            if (fw_end - delay > current_time) {
                _relays[id].change_time = fw_end;
            }

        }

        _relays[id].target_status = status;
        if (report) _relays[id].report = true;
        if (group_report) _relays[id].group_report = true;

        relaySync(id);

        DEBUG_MSG_P(PSTR("[RELAY] #%d scheduled %s in %u ms\n"),
                id, status ? "ON" : "OFF",
                (_relays[id].change_time - current_time));

        changed = true;

    }

    return changed;

}

bool relayStatus(unsigned char id, bool status) {
    return relayStatus(id, status, mqttForward(), true);
}

bool relayStatus(unsigned char id) {


    if (id >= _relays.size()) return false;


    return _relays[id].current_status;

}

void relaySync(unsigned char id) {


    if (_relays.size() < 2) return;


    if (_relayRecursive) return;


    _relayRecursive = true;

    byte relaySync = getSetting("relaySync", RELAY_SYNC).toInt();
    bool status = _relays[id].target_status;


    if (relaySync == RELAY_SYNC_SAME) {
        for (unsigned short i=0; i<_relays.size(); i++) {
            if (i != id) relayStatus(i, status);
        }


    } else if (relaySync == RELAY_SYNC_FIRST) {
        if (id == 0) {
            for (unsigned short i=1; i<_relays.size(); i++) {
                relayStatus(i, status);
            }
        }


    } else if (status) {
        if (relaySync != RELAY_SYNC_ANY) {
            for (unsigned short i=0; i<_relays.size(); i++) {
                if (i != id) relayStatus(i, false);
            }
        }


    } else {
        if (relaySync == RELAY_SYNC_ONE) {
            unsigned char i = (id + 1) % _relays.size();
            relayStatus(i, true);
        }
    }


    _relayRecursive = false;

}

void relaySave(bool eeprom) {

    auto mask = std::bitset<RELAY_SAVE_MASK_MAX>(0);

    unsigned char count = relayCount();
    if (count > RELAY_SAVE_MASK_MAX) count = RELAY_SAVE_MASK_MAX;

    for (unsigned int i=0; i < count; ++i) {
        mask.set(i, relayStatus(i));
    }

    const uint32_t mask_value = mask.to_ulong();

    DEBUG_MSG_P(PSTR("[RELAY] Setting relay mask: %u\n"), mask_value);


    _relayMaskRtcmem(mask_value);







    if (eeprom) {
        EEPROMr.write(EEPROM_RELAY_STATUS, mask_value);


        eepromCommit();
    }

}

void relaySave() {
    relaySave(false);
}

void relayToggle(unsigned char id, bool report, bool group_report) {
    if (id >= _relays.size()) return;
    relayStatus(id, !relayStatus(id), report, group_report);
}

void relayToggle(unsigned char id) {
    relayToggle(id, mqttForward(), true);
}

unsigned char relayCount() {
    return _relays.size();
}

unsigned char relayParsePayload(const char * payload) {




    if (payload[0] == '0') return 0;
    if (payload[0] == '1') return 1;
    if (payload[0] == '2') return 2;


    char * p = ltrim((char *)payload);


    unsigned int l = strlen(p);
    if (l>6) l=6;
    for (unsigned char i=0; i<l; i++) {
        p[i] = tolower(p[i]);
    }

    unsigned int value = 0xFF;
    if (strcmp(p, "off") == 0) {
        value = 0;
    } else if (strcmp(p, "on") == 0) {
        value = 1;
    } else if (strcmp(p, "toggle") == 0) {
        value = 2;
    } else if (strcmp(p, "query") == 0) {
        value = 3;
    }

    return value;

}


void _relayBackwards() {

    for (unsigned int i=0; i<_relays.size(); i++) {
        if (!hasSetting("mqttGroupInv", i)) continue;
        setSetting("mqttGroupSync", i, getSetting("mqttGroupInv", i));
        delSetting("mqttGroupInv", i);
    }

}

void _relayBoot() {

    _relayRecursive = true;
    bool trigger_save = false;
    uint32_t stored_mask = 0;

    if (rtcmemStatus()) {
        stored_mask = _relayMaskRtcmem();
    } else {
        stored_mask = EEPROMr.read(EEPROM_RELAY_STATUS);
    }

    DEBUG_MSG_P(PSTR("[RELAY] Retrieving mask: %u\n"), stored_mask);

    auto mask = std::bitset<RELAY_SAVE_MASK_MAX>(stored_mask);


    bool status;
    for (unsigned char i=0; i<relayCount(); ++i) {

        unsigned char boot_mode = getSetting("relayBoot", i, RELAY_BOOT_MODE).toInt();
        DEBUG_MSG_P(PSTR("[RELAY] Relay #%u boot mode %u\n"), i, boot_mode);

        status = false;
        switch (boot_mode) {
            case RELAY_BOOT_SAME:
                if (i < 8) {
                    status = mask.test(i);
                }
                break;
            case RELAY_BOOT_TOGGLE:
                if (i < 8) {
                    status = !mask[i];
                    mask.flip(i);
                    trigger_save = true;
                }
                break;
            case RELAY_BOOT_ON:
                status = true;
                break;
            case RELAY_BOOT_OFF:
            default:
                break;
        }

        _relays[i].current_status = !status;
        _relays[i].target_status = status;
        #if RELAY_PROVIDER == RELAY_PROVIDER_STM
            _relays[i].change_time = millis() + 3000 + 1000 * i;
        #else
            _relays[i].change_time = millis();
        #endif
    }


    if (trigger_save) {
        _relayMaskRtcmem(mask.to_ulong());

        EEPROMr.write(EEPROM_RELAY_STATUS, mask.to_ulong());
        eepromCommit();
    }

    _relayRecursive = false;

}

void _relayConfigure() {
    for (unsigned int i=0; i<_relays.size(); i++) {
        _relays[i].pulse = getSetting("relayPulse", i, RELAY_PULSE_MODE).toInt();
        _relays[i].pulse_ms = 1000 * getSetting("relayTime", i, RELAY_PULSE_MODE).toFloat();

        if (GPIO_NONE == _relays[i].pin) continue;

        pinMode(_relays[i].pin, OUTPUT);
        if (GPIO_NONE != _relays[i].reset_pin) {
            pinMode(_relays[i].reset_pin, OUTPUT);
        }
        if (_relays[i].type == RELAY_TYPE_INVERSE) {

            digitalWrite(_relays[i].pin, HIGH);
        }
    }
}





#if WEB_SUPPORT

bool _relayWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "relay", 5) == 0);
}

void _relayWebSocketUpdate(JsonObject& root) {
    JsonArray& relay = root.createNestedArray("relayStatus");
    for (unsigned char i=0; i<relayCount(); i++) {
        relay.add<uint8_t>(_relays[i].target_status);
    }
}

String _relayFriendlyName(unsigned char i) {
    String res = String("GPIO") + String(_relays[i].pin);

    if (GPIO_NONE == _relays[i].pin) {
        #if (RELAY_PROVIDER == RELAY_PROVIDER_LIGHT)
            uint8_t physical = _relays.size() - DUMMY_RELAY_COUNT;
            if (i >= physical) {
                if (DUMMY_RELAY_COUNT == lightChannels()) {
                    res = String("CH") + String(i-physical);
                } else if (DUMMY_RELAY_COUNT == (lightChannels() + 1u)) {
                    if (physical == i) {
                        res = String("Light");
                    } else {
                        res = String("CH") + String(i-1-physical);
                    }
                } else {
                    res = String("Light");
                }
            } else {
                res = String("?");
            }
        #else
            res = String("SW") + String(i);
        #endif
    }

    return res;
}

void _relayWebSocketSendRelays() {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& relays = root.createNestedObject("relayConfig");

    relays["size"] = relayCount();
    relays["start"] = 0;

    JsonArray& gpio = relays.createNestedArray("gpio");
    JsonArray& type = relays.createNestedArray("type");
    JsonArray& reset = relays.createNestedArray("reset");
    JsonArray& boot = relays.createNestedArray("boot");
    JsonArray& pulse = relays.createNestedArray("pulse");
    JsonArray& pulse_time = relays.createNestedArray("pulse_time");

    #if MQTT_SUPPORT
        JsonArray& group = relays.createNestedArray("group");
        JsonArray& group_sync = relays.createNestedArray("group_sync");
        JsonArray& on_disconnect = relays.createNestedArray("on_disc");
    #endif

    for (unsigned char i=0; i<relayCount(); i++) {
        gpio.add(_relayFriendlyName(i));

        type.add(_relays[i].type);
        reset.add(_relays[i].reset_pin);
        boot.add(getSetting("relayBoot", i, RELAY_BOOT_MODE).toInt());

        pulse.add(_relays[i].pulse);
        pulse_time.add(_relays[i].pulse_ms / 1000.0);

        #if MQTT_SUPPORT
            group.add(getSetting("mqttGroup", i, ""));
            group_sync.add(getSetting("mqttGroupSync", i, 0).toInt());
            on_disconnect.add(getSetting("relayOnDisc", i, 0).toInt());
        #endif
    }

    wsSend(root);
}

void _relayWebSocketOnStart(JsonObject& root) {

    if (relayCount() == 0) return;


    _relayWebSocketSendRelays();


    _relayWebSocketUpdate(root);


    if (relayCount() > 1) {
        root["multirelayVisible"] = 1;
        root["relaySync"] = getSetting("relaySync", RELAY_SYNC);
    }

    root["relayVisible"] = 1;

}

void _relayWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {

    if (strcmp(action, "relay") != 0) return;

    if (data.containsKey("status")) {

        unsigned char value = relayParsePayload(data["status"]);

        if (value == 3) {

            wsSend(_relayWebSocketUpdate);

        } else if (value < 3) {

            unsigned int relayID = 0;
            if (data.containsKey("id")) {
                String value = data["id"];
                relayID = value.toInt();
            }


            if (value == 0) {
                relayStatus(relayID, false);
            } else if (value == 1) {
                relayStatus(relayID, true);
            } else if (value == 2) {
                relayToggle(relayID);
            }

        }

    }

}

void relaySetupWS() {
    wsOnSendRegister(_relayWebSocketOnStart);
    wsOnActionRegister(_relayWebSocketOnAction);
    wsOnReceiveRegister(_relayWebSocketOnReceive);
}

#endif





#if API_SUPPORT

void relaySetupAPI() {

    char key[20];


    for (unsigned int relayID=0; relayID<relayCount(); relayID++) {

        snprintf_P(key, sizeof(key), PSTR("%s/%d"), MQTT_TOPIC_RELAY, relayID);
        apiRegister(key,
            [relayID](char * buffer, size_t len) {
    snprintf_P(buffer, len, PSTR("%d"), _relays[relayID].target_status ? 1 : 0);
            },
            [relayID](const char * payload) {

                unsigned char value = relayParsePayload(payload);

                if (value == 0xFF) {
                    DEBUG_MSG_P(PSTR("[RELAY] Wrong payload (%s)\n"), payload);
                    return;
                }

                if (value == 0) {
                    relayStatus(relayID, false);
                } else if (value == 1) {
                    relayStatus(relayID, true);
                } else if (value == 2) {
                    relayToggle(relayID);
                }

            }
        );

        snprintf_P(key, sizeof(key), PSTR("%s/%d"), MQTT_TOPIC_PULSE, relayID);
        apiRegister(key,
            [relayID](char * buffer, size_t len) {
                dtostrf((double) _relays[relayID].pulse_ms / 1000, 1-len, 3, buffer);
            },
            [relayID](const char * payload) {

                unsigned long pulse = 1000 * String(payload).toFloat();
                if (0 == pulse) return;

                if (RELAY_PULSE_NONE != _relays[relayID].pulse) {
                    DEBUG_MSG_P(PSTR("[RELAY] Overriding relay #%d pulse settings\n"), relayID);
                }

                _relays[relayID].pulse_ms = pulse;
                _relays[relayID].pulse = relayStatus(relayID) ? RELAY_PULSE_ON : RELAY_PULSE_OFF;
                relayToggle(relayID, true, false);

            }
        );

        #if defined(ITEAD_SONOFF_IFAN02)

            apiRegister(MQTT_TOPIC_SPEED,
                [relayID](char * buffer, size_t len) {
                    snprintf(buffer, len, "%u", getSpeed());
                },
                [relayID](const char * payload) {
                    setSpeed(atoi(payload));
                }
            );

        #endif

    }

}

#endif





#if MQTT_SUPPORT

void _relayMQTTGroup(unsigned char id) {
    String topic = getSetting("mqttGroup", id, "");
    if (!topic.length()) return;

    unsigned char mode = getSetting("mqttGroupSync", id, RELAY_GROUP_SYNC_NORMAL).toInt();
    if (mode == RELAY_GROUP_SYNC_RECEIVEONLY) return;

    bool status = relayStatus(id);
    if (mode == RELAY_GROUP_SYNC_INVERSE) status = !status;
    mqttSendRaw(topic.c_str(), status ? RELAY_MQTT_ON : RELAY_MQTT_OFF);
}

void relayMQTT(unsigned char id) {

    if (id >= _relays.size()) return;


    if (_relays[id].report) {
        _relays[id].report = false;
        mqttSend(MQTT_TOPIC_RELAY, id, _relays[id].current_status ? RELAY_MQTT_ON : RELAY_MQTT_OFF);
    }


    if (_relays[id].group_report) {
        _relays[id].group_report = false;
        _relayMQTTGroup(id);
    }


    #if defined (ITEAD_SONOFF_IFAN02)
        char buffer[5];
        snprintf(buffer, sizeof(buffer), "%u", getSpeed());
        mqttSend(MQTT_TOPIC_SPEED, buffer);
    #endif

}

void relayMQTT() {
    for (unsigned int id=0; id < _relays.size(); id++) {
        mqttSend(MQTT_TOPIC_RELAY, id, _relays[id].current_status ? RELAY_MQTT_ON : RELAY_MQTT_OFF);
    }
}

void relayStatusWrap(unsigned char id, unsigned char value, bool is_group_topic) {
    switch (value) {
        case 0:
            relayStatus(id, false, mqttForward(), !is_group_topic);
            break;
        case 1:
            relayStatus(id, true, mqttForward(), !is_group_topic);
            break;
        case 2:
            relayToggle(id, true, true);
            break;
        default:
            _relays[id].report = true;
            relayMQTT(id);
            break;
    }
}

void relayMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {


        #if (HEARTBEAT_MODE == HEARTBEAT_NONE) or (not HEARTBEAT_REPORT_RELAY)
            relayMQTT();
        #endif


        char relay_topic[strlen(MQTT_TOPIC_RELAY) + 3];
        snprintf_P(relay_topic, sizeof(relay_topic), PSTR("%s/+"), MQTT_TOPIC_RELAY);
        mqttSubscribe(relay_topic);


        char pulse_topic[strlen(MQTT_TOPIC_PULSE) + 3];
        snprintf_P(pulse_topic, sizeof(pulse_topic), PSTR("%s/+"), MQTT_TOPIC_PULSE);
        mqttSubscribe(pulse_topic);

        #if defined(ITEAD_SONOFF_IFAN02)
            mqttSubscribe(MQTT_TOPIC_SPEED);
        #endif


        for (unsigned int i=0; i < _relays.size(); i++) {
            String t = getSetting("mqttGroup", i, "");
            if (t.length() > 0) mqttSubscribeRaw(t.c_str());
        }

    }

    if (type == MQTT_MESSAGE_EVENT) {

        String t = mqttMagnitude((char *) topic);


        if (t.startsWith(MQTT_TOPIC_PULSE)) {

            unsigned int id = t.substring(strlen(MQTT_TOPIC_PULSE)+1).toInt();

            if (id >= relayCount()) {
                DEBUG_MSG_P(PSTR("[RELAY] Wrong relayID (%d)\n"), id);
                return;
            }

            unsigned long pulse = 1000 * String(payload).toFloat();
            if (0 == pulse) return;

            if (RELAY_PULSE_NONE != _relays[id].pulse) {
                DEBUG_MSG_P(PSTR("[RELAY] Overriding relay #%d pulse settings\n"), id);
            }

            _relays[id].pulse_ms = pulse;
            _relays[id].pulse = relayStatus(id) ? RELAY_PULSE_ON : RELAY_PULSE_OFF;
            relayToggle(id, true, false);

            return;

        }


        if (t.startsWith(MQTT_TOPIC_RELAY)) {


            unsigned int id = t.substring(strlen(MQTT_TOPIC_RELAY)+1).toInt();
            if (id >= relayCount()) {
                DEBUG_MSG_P(PSTR("[RELAY] Wrong relayID (%d)\n"), id);
                return;
            }


            unsigned char value = relayParsePayload(payload);
            if (value == 0xFF) return;

            relayStatusWrap(id, value, false);

            return;
        }



        for (unsigned int i=0; i < _relays.size(); i++) {

            String t = getSetting("mqttGroup", i, "");

            if ((t.length() > 0) && t.equals(topic)) {

                unsigned char value = relayParsePayload(payload);
                if (value == 0xFF) return;

                if (value < 2) {
                    if (getSetting("mqttGroupSync", i, RELAY_GROUP_SYNC_NORMAL).toInt() == RELAY_GROUP_SYNC_INVERSE) {
                        value = 1 - value;
                    }
                }

                DEBUG_MSG_P(PSTR("[RELAY] Matched group topic for relayID %d\n"), i);
                relayStatusWrap(i, value, true);

            }
        }


        #if defined (ITEAD_SONOFF_IFAN02)
            if (t.startsWith(MQTT_TOPIC_SPEED)) {
                setSpeed(atoi(payload));
            }
        #endif

    }

    if (type == MQTT_DISCONNECT_EVENT) {
        for (unsigned int i=0; i < _relays.size(); i++){
            int reaction = getSetting("relayOnDisc", i, 0).toInt();
            if (1 == reaction) {
                DEBUG_MSG_P(PSTR("[RELAY] Reset relay (%d) due to MQTT disconnection\n"), i);
                relayStatusWrap(i, false, false);
            } else if(2 == reaction) {
                DEBUG_MSG_P(PSTR("[RELAY] Set relay (%d) due to MQTT disconnection\n"), i);
                relayStatusWrap(i, true, false);
            }
        }

    }

}

void relaySetupMQTT() {
    mqttRegister(relayMQTTCallback);
}

#endif





#if TERMINAL_SUPPORT

void _relayInitCommands() {

    terminalRegisterCommand(F("RELAY"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("Wrong arguments"));
            return;
        }
        int id = String(e->argv[1]).toInt();
        if (id >= relayCount()) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong relayID (%d)\n"), id);
            return;
        }

        if (e->argc > 2) {
            int value = String(e->argv[2]).toInt();
            if (value == 2) {
                relayToggle(id);
            } else {
                relayStatus(id, value == 1);
            }
        }
        DEBUG_MSG_P(PSTR("Status: %s\n"), _relays[id].target_status ? "true" : "false");
        if (_relays[id].pulse != RELAY_PULSE_NONE) {
            DEBUG_MSG_P(PSTR("Pulse: %s\n"), (_relays[id].pulse == RELAY_PULSE_ON) ? "ON" : "OFF");
            DEBUG_MSG_P(PSTR("Pulse time: %d\n"), _relays[id].pulse_ms);

        }
        terminalOK();
    });

}

#endif





void _relayLoop() {
    _relayProcess(false);
    _relayProcess(true);
}

void relaySetup() {


    #if RELAY1_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY1_PIN, RELAY1_TYPE, RELAY1_RESET_PIN, RELAY1_DELAY_ON, RELAY1_DELAY_OFF });
    #endif
    #if RELAY2_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY2_PIN, RELAY2_TYPE, RELAY2_RESET_PIN, RELAY2_DELAY_ON, RELAY2_DELAY_OFF });
    #endif
    #if RELAY3_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY3_PIN, RELAY3_TYPE, RELAY3_RESET_PIN, RELAY3_DELAY_ON, RELAY3_DELAY_OFF });
    #endif
    #if RELAY4_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY4_PIN, RELAY4_TYPE, RELAY4_RESET_PIN, RELAY4_DELAY_ON, RELAY4_DELAY_OFF });
    #endif
    #if RELAY5_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY5_PIN, RELAY5_TYPE, RELAY5_RESET_PIN, RELAY5_DELAY_ON, RELAY5_DELAY_OFF });
    #endif
    #if RELAY6_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY6_PIN, RELAY6_TYPE, RELAY6_RESET_PIN, RELAY6_DELAY_ON, RELAY6_DELAY_OFF });
    #endif
    #if RELAY7_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY7_PIN, RELAY7_TYPE, RELAY7_RESET_PIN, RELAY7_DELAY_ON, RELAY7_DELAY_OFF });
    #endif
    #if RELAY8_PIN != GPIO_NONE
        _relays.push_back((relay_t) { RELAY8_PIN, RELAY8_TYPE, RELAY8_RESET_PIN, RELAY8_DELAY_ON, RELAY8_DELAY_OFF });
    #endif




    for (unsigned char i=0; i < DUMMY_RELAY_COUNT; i++) {
        _relays.push_back((relay_t) {GPIO_NONE, RELAY_TYPE_NORMAL, 0, 0, 0});
    }

    _relayBackwards();
    _relayConfigure();
    _relayBoot();
    _relayLoop();

    #if WEB_SUPPORT
        relaySetupWS();
    #endif
    #if API_SUPPORT
        relaySetupAPI();
    #endif
    #if MQTT_SUPPORT
        relaySetupMQTT();
    #endif
    #if TERMINAL_SUPPORT
        _relayInitCommands();
    #endif


    espurnaRegisterLoop(_relayLoop);
    espurnaRegisterReload(_relayConfigure);

    DEBUG_MSG_P(PSTR("[RELAY] Number of relays: %d\n"), _relays.size());

}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/rfbridge.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/rfbridge.ino"
#if RF_SUPPORT

#include <queue>
#include <Ticker.h>

#if RFB_DIRECT
#include <RCSwitch.h>
#endif







#define RF_MESSAGE_SIZE 9
#define RF_MAX_MESSAGE_SIZE (112+4)
#define RF_CODE_START 0xAA
#define RF_CODE_ACK 0xA0
#define RF_CODE_LEARN 0xA1
#define RF_CODE_LEARN_KO 0xA2
#define RF_CODE_LEARN_OK 0xA3
#define RF_CODE_RFIN 0xA4
#define RF_CODE_RFOUT 0xA5
#define RF_CODE_SNIFFING_ON 0xA6
#define RF_CODE_SNIFFING_OFF 0xA7
#define RF_CODE_RFOUT_NEW 0xA8
#define RF_CODE_LEARN_NEW 0xA9
#define RF_CODE_LEARN_KO_NEW 0xAA
#define RF_CODE_LEARN_OK_NEW 0xAB
#define RF_CODE_RFOUT_BUCKET 0xB0
#define RF_CODE_STOP 0x55



#define RF_MAX_KEY_LENGTH (9)





unsigned char _uartbuf[RF_MESSAGE_SIZE+3] = {0};
unsigned char _uartpos = 0;
unsigned char _learnId = 0;

bool _learnStatus = true;
bool _rfbin = false;

typedef struct {
    byte code[RF_MESSAGE_SIZE];
    byte times;
} rfb_message_t;
static std::queue<rfb_message_t> _rfb_message_queue;

#if RFB_DIRECT
    RCSwitch * _rfModem;
    bool _learning = false;
#endif

bool _rfb_receive = false;
bool _rfb_transmit = false;
unsigned char _rfb_repeat = RF_SEND_TIMES;

#if WEB_SUPPORT
    Ticker _rfb_sendcodes;
#endif
# 83 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/rfbridge.ino"
static bool _rfbToChar(byte * in, char * out, int n = RF_MESSAGE_SIZE) {
    for (unsigned char p = 0; p<n; p++) {
        sprintf_P(&out[p*2], PSTR("%02X"), in[p]);
    }
    return true;
}

#if WEB_SUPPORT

void _rfbWebSocketSendCodeArray(unsigned char start, unsigned char size) {

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    JsonObject& rfb = root.createNestedObject("rfb");
    rfb["size"] = size;
    rfb["start"] = start;

    JsonArray& on = rfb.createNestedArray("on");
    JsonArray& off = rfb.createNestedArray("off");

    for (byte id=start; id<start+size; id++) {
        on.add(rfbRetrieve(id, true));
        off.add(rfbRetrieve(id, false));
    }

    wsSend(root);

}

void _rfbWebSocketSendCode(unsigned char id) {
    _rfbWebSocketSendCodeArray(id, 1);
}

void _rfbWebSocketSendCodes() {
    _rfbWebSocketSendCodeArray(0, relayCount());
}

void _rfbWebSocketOnSend(JsonObject& root) {
    root["rfbVisible"] = 1;
    root["rfbRepeat"] = getSetting("rfbRepeat", RF_SEND_TIMES).toInt();
    root["rfbCount"] = relayCount();
    #if RFB_DIRECT
        root["rfbdirectVisible"] = 1;
        root["rfbRX"] = getSetting("rfbRX", RFB_RX_PIN).toInt();
        root["rfbTX"] = getSetting("rfbTX", RFB_TX_PIN).toInt();
    #endif
    _rfb_sendcodes.once_ms(1000, _rfbWebSocketSendCodes);
}

void _rfbWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "rfblearn") == 0) rfbLearn(data["id"], data["status"]);
    if (strcmp(action, "rfbforget") == 0) rfbForget(data["id"], data["status"]);
    if (strcmp(action, "rfbsend") == 0) rfbStore(data["id"], data["status"], data["data"].as<const char*>());
}

bool _rfbWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "rfb", 3) == 0);
}

#endif




static int _rfbToArray(const char * in, byte * out, int length = RF_MESSAGE_SIZE * 2) {
    int n = strlen(in);
    if (n > RF_MAX_MESSAGE_SIZE*2 || (length > 0 && n != length)) return 0;
    char tmp[3] = {0,0,0};
    n /= 2;
    for (unsigned char p = 0; p<n; p++) {
        memcpy(tmp, &in[p*2], 2);
        out[p] = strtol(tmp, NULL, 16);
    }
    return n;
}

void _rfbSendRaw(const byte *message, const unsigned char n = RF_MESSAGE_SIZE) {
    for (unsigned char j=0; j<n; j++) {
        Serial.write(message[j]);
    }
}

void _rfbSend() {

    if (!_rfb_transmit) return;


    if (_rfb_message_queue.empty()) return;

    static unsigned long last = 0;
    if (millis() - last < RF_SEND_DELAY) return;
    last = millis();


    rfb_message_t message = _rfb_message_queue.front();
    _rfb_message_queue.pop();
    _rfbSend(message.code);


    if (message.times > 1) {
        message.times = message.times - 1;
        _rfb_message_queue.push(message);
    }

    yield();

}

void _rfbSend(byte * code, unsigned char times) {

    if (!_rfb_transmit) return;


    #if RFB_DIRECT
        times = 1;
    #endif

    char buffer[RF_MESSAGE_SIZE];
    _rfbToChar(code, buffer);
    DEBUG_MSG_P(PSTR("[RF] Enqueuing MESSAGE '%s' %d time(s)\n"), buffer, times);

    rfb_message_t message;
    memcpy(message.code, code, RF_MESSAGE_SIZE);
    message.times = times;
    _rfb_message_queue.push(message);

}

void _rfbSendRawOnce(byte *code, unsigned char length) {
    char buffer[length*2];
    _rfbToChar(code, buffer, length);
    DEBUG_MSG_P(PSTR("[RF] Sending RAW MESSAGE '%s'\n"), buffer);
    _rfbSendRaw(code, length);
}

bool _rfbMatch(char* code, unsigned char& relayID, unsigned char& value, char* buffer = NULL) {

    if (strlen(code) != 18) return false;

    bool found = false;
    String compareto = String(&code[12]);
    compareto.toUpperCase();
    DEBUG_MSG_P(PSTR("[RF] Trying to match code %s\n"), compareto.c_str());

    for (unsigned char i=0; i<relayCount(); i++) {

        String code_on = rfbRetrieve(i, true);
        if (code_on.length() && code_on.endsWith(compareto)) {
            DEBUG_MSG_P(PSTR("[RF] Match ON code for relay %d\n"), i);
            value = 1;
            found = true;
            if (buffer) strcpy(buffer, code_on.c_str());
        }

        String code_off = rfbRetrieve(i, false);
        if (code_off.length() && code_off.endsWith(compareto)) {
            DEBUG_MSG_P(PSTR("[RF] Match OFF code for relay %d\n"), i);
            if (found) value = 2;
            found = true;
            if (buffer) strcpy(buffer, code_off.c_str());
        }

        if (found) {
            relayID = i;
            return true;
        }

    }

    return false;

}

void _rfbDecode() {

    static unsigned long last = 0;
    if (millis() - last < RF_RECEIVE_DELAY) return;
    last = millis();

    byte action = _uartbuf[0];
    char buffer[RF_MESSAGE_SIZE * 2 + 1] = {0};
    DEBUG_MSG_P(PSTR("[RF] Action 0x%02X\n"), action);

    if (action == RF_CODE_LEARN_KO) {
        _rfbAck();
        DEBUG_MSG_P(PSTR("[RF] Learn timeout\n"));
        #if WEB_SUPPORT
            wsSend_P(PSTR("{\"action\": \"rfbTimeout\"}"));
        #endif
    }

    if (action == RF_CODE_LEARN_OK || action == RF_CODE_RFIN) {

        _rfbAck();
        _rfbToChar(&_uartbuf[1], buffer);

        DEBUG_MSG_P(PSTR("[RF] Received message '%s'\n"), buffer);

    }

    if (action == RF_CODE_LEARN_OK) {

        DEBUG_MSG_P(PSTR("[RF] Learn success\n"));
        rfbStore(_learnId, _learnStatus, buffer);


        #if WEB_SUPPORT
            _rfbWebSocketSendCode(_learnId);
        #endif

    }

    if (action == RF_CODE_RFIN) {





        unsigned char id;
        unsigned char status;
        bool matched = _rfbMatch(buffer, id, status, buffer);

        if (matched) {
            DEBUG_MSG_P(PSTR("[RF] Matched message '%s'\n"), buffer);
            _rfbin = true;
            if (status == 2) {
                relayToggle(id);
            } else {
                relayStatus(id, status == 1);
            }
        }

        #if MQTT_SUPPORT
            mqttSend(MQTT_TOPIC_RFIN, buffer, false, false);
        #endif

    }

}

bool _rfbCompare(const char * code1, const char * code2) {
    return strcmp(&code1[12], &code2[12]) == 0;
}

bool _rfbSameOnOff(unsigned char id) {
    return _rfbCompare(rfbRetrieve(id, true).c_str(), rfbRetrieve(id, false).c_str());
}

void _rfbParseRaw(char * raw) {
    byte message[RF_MAX_MESSAGE_SIZE];
    int len = _rfbToArray(raw, message, 0);
    if (len > 0) {
        _rfbSendRawOnce(message, len);
    }
}

void _rfbParseCode(char * code) {



    char * tok = strtok(code, ",");


    unsigned char id;
    unsigned char status = 0;
    if (_rfbMatch(tok, id, status)) {
        if (status == 2) {
            relayToggle(id);
        } else {
            relayStatus(id, status == 1);
        }
        return;
    }

    byte message[RF_MESSAGE_SIZE];
    int len = _rfbToArray(tok, message, 0);
    if (len) {
        tok = strtok(NULL, ",");
        byte times = (tok != NULL) ? atoi(tok) : 1;
        _rfbSend(message, times);
    }

}






#if !RFB_DIRECT

void _rfbAck() {
    DEBUG_MSG_P(PSTR("[RF] Sending ACK\n"));
    Serial.println();
    Serial.write(RF_CODE_START);
    Serial.write(RF_CODE_ACK);
    Serial.write(RF_CODE_STOP);
    Serial.flush();
    Serial.println();
}

void _rfbLearnImpl() {
    DEBUG_MSG_P(PSTR("[RF] Sending LEARN\n"));
    Serial.println();
    Serial.write(RF_CODE_START);
    Serial.write(RF_CODE_LEARN);
    Serial.write(RF_CODE_STOP);
    Serial.flush();
    Serial.println();
}

void _rfbSend(byte * message) {
    Serial.println();
    Serial.write(RF_CODE_START);
    Serial.write(RF_CODE_RFOUT);
    _rfbSendRaw(message);
    Serial.write(RF_CODE_STOP);
    Serial.flush();
    Serial.println();
}

void _rfbReceive() {

    static bool receiving = false;

    while (Serial.available()) {

        yield();
        byte c = Serial.read();


        if (receiving) {
            if (c == RF_CODE_STOP && (_uartpos == 1 || _uartpos == RF_MESSAGE_SIZE + 1)) {
                _rfbDecode();
                receiving = false;
            } else if (_uartpos <= RF_MESSAGE_SIZE) {
                _uartbuf[_uartpos++] = c;
            } else {

                receiving = false;
            }
        } else if (c == RF_CODE_START) {
            _uartpos = 0;
            receiving = true;
        }

    }

}

#else

void _rfbAck() {}

void _rfbLearnImpl() {
    DEBUG_MSG_P(PSTR("[RF] Entering LEARN mode\n"));
    _learning = true;
}

void _rfbSend(byte * message) {

    if (!_rfb_transmit) return;

    unsigned int protocol = message[1];
    unsigned int timing =
        (message[2] << 8) |
        (message[3] << 0) ;
    unsigned int bitlength = message[4];
    unsigned long rf_code =
        (message[5] << 24) |
        (message[6] << 16) |
        (message[7] << 8) |
        (message[8] << 0) ;
    _rfModem->setProtocol(protocol);
    if (timing > 0) {
        _rfModem->setPulseLength(timing);
    }
    _rfModem->send(rf_code, bitlength);
    _rfModem->resetAvailable();

}

void _rfbReceive() {

    if (!_rfb_receive) return;

    static long learn_start = 0;
    if (!_learning && learn_start) {
        learn_start = 0;
    }
    if (_learning) {
        if (!learn_start) {
            DEBUG_MSG_P(PSTR("[RF] Arming learn timeout\n"));
            learn_start = millis();
        }
        if (learn_start > 0 && millis() - learn_start > RF_LEARN_TIMEOUT) {
            DEBUG_MSG_P(PSTR("[RF] Learn timeout triggered\n"));
            memset(_uartbuf, 0, sizeof(_uartbuf));
            _uartbuf[0] = RF_CODE_LEARN_KO;
            _rfbDecode();
            _learning = false;
        }
    }

    if (_rfModem->available()) {
        static unsigned long last = 0;
        if (millis() - last > RF_DEBOUNCE) {
            last = millis();
            unsigned long rf_code = _rfModem->getReceivedValue();
            if ( rf_code > 0) {
                DEBUG_MSG_P(PSTR("[RF] Received code: %08X\n"), rf_code);
                unsigned int timing = _rfModem->getReceivedDelay();
                memset(_uartbuf, 0, sizeof(_uartbuf));
                unsigned char *msgbuf = _uartbuf + 1;
                _uartbuf[0] = _learning ? RF_CODE_LEARN_OK: RF_CODE_RFIN;
                msgbuf[0] = 0xC0;
                msgbuf[1] = _rfModem->getReceivedProtocol();
                msgbuf[2] = timing >> 8;
                msgbuf[3] = timing >> 0;
                msgbuf[4] = _rfModem->getReceivedBitlength();
                msgbuf[5] = rf_code >> 24;
                msgbuf[6] = rf_code >> 16;
                msgbuf[7] = rf_code >> 8;
                msgbuf[8] = rf_code >> 0;
                _rfbDecode();
                _learning = false;
            }
        }
        _rfModem->resetAvailable();
    }

    yield();

}

#endif

void _rfbLearn() {

    _rfbLearnImpl();

    #if WEB_SUPPORT
        char buffer[100];
        snprintf_P(buffer, sizeof(buffer), PSTR("{\"action\": \"rfbLearn\", \"data\":{\"id\": %d, \"status\": %d}}"), _learnId, _learnStatus ? 1 : 0);
        wsSend(buffer);
    #endif

}

#if MQTT_SUPPORT

void _rfbMqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {

        char buffer[strlen(MQTT_TOPIC_RFLEARN) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_RFLEARN);
        mqttSubscribe(buffer);

        if (_rfb_transmit) {
            mqttSubscribe(MQTT_TOPIC_RFOUT);
        }

        #if !RFB_DIRECT
            mqttSubscribe(MQTT_TOPIC_RFRAW);
        #endif

    }

    if (type == MQTT_MESSAGE_EVENT) {


        String t = mqttMagnitude((char *) topic);


        if (t.startsWith(MQTT_TOPIC_RFLEARN)) {

            _learnId = t.substring(strlen(MQTT_TOPIC_RFLEARN)+1).toInt();
            if (_learnId >= relayCount()) {
                DEBUG_MSG_P(PSTR("[RF] Wrong learnID (%d)\n"), _learnId);
                return;
            }
            _learnStatus = (char)payload[0] != '0';
            _rfbLearn();
            return;

        }

        if (t.equals(MQTT_TOPIC_RFOUT)) {
            _rfbParseCode((char *) payload);
        }

        #if !RFB_DIRECT
            if (t.equals(MQTT_TOPIC_RFRAW)) {
                _rfbParseRaw((char *) payload);
            }
        #endif

    }

}

#endif

#if API_SUPPORT

void _rfbAPISetup() {

    apiRegister(MQTT_TOPIC_RFOUT,
        [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("OK"));
        },
        [](const char * payload) {
            _rfbParseCode((char *) payload);
        }
    );

    apiRegister(MQTT_TOPIC_RFLEARN,
        [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("OK"));
        },
        [](const char * payload) {

            char * tok = strtok((char *) payload, ",");
            if (NULL == tok) return;
            if (!isNumber(tok)) return;
            _learnId = atoi(tok);
            if (_learnId >= relayCount()) {
                DEBUG_MSG_P(PSTR("[RF] Wrong learnID (%d)\n"), _learnId);
                return;
            }
            tok = strtok(NULL, ",");
            if (NULL == tok) return;
            _learnStatus = (char) tok[0] != '0';
            _rfbLearn();
        }
    );

    #if !RFB_DIRECT
        apiRegister(MQTT_TOPIC_RFRAW,
            [](char * buffer, size_t len) {
                snprintf_P(buffer, len, PSTR("OK"));
            },
            [](const char * payload) {
                _rfbParseRaw((char *)payload);
            }
        );
    #endif

}

#endif

#if TERMINAL_SUPPORT

void _rfbInitCommands() {

    terminalRegisterCommand(F("LEARN"), [](Embedis* e) {

        if (e->argc < 3) {
            terminalError(F("Wrong arguments"));
            return;
        }

        int id = String(e->argv[1]).toInt();
        if (id >= relayCount()) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong relayID (%d)\n"), id);
            return;
        }

        int status = String(e->argv[2]).toInt();

        rfbLearn(id, status == 1);

        terminalOK();

    });

    terminalRegisterCommand(F("FORGET"), [](Embedis* e) {

        if (e->argc < 3) {
            terminalError(F("Wrong arguments"));
            return;
        }

        int id = String(e->argv[1]).toInt();
        if (id >= relayCount()) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong relayID (%d)\n"), id);
            return;
        }

        int status = String(e->argv[2]).toInt();

        rfbForget(id, status == 1);

        terminalOK();

    });

}

#endif





void rfbStore(unsigned char id, bool status, const char * code) {
    DEBUG_MSG_P(PSTR("[RF] Storing %d-%s => '%s'\n"), id, status ? "ON" : "OFF", code);
    char key[RF_MAX_KEY_LENGTH] = {0};
    snprintf_P(key, sizeof(key), PSTR("rfb%s%d"), status ? "ON" : "OFF", id);
    setSetting(key, code);
}

String rfbRetrieve(unsigned char id, bool status) {
    char key[RF_MAX_KEY_LENGTH] = {0};
    snprintf_P(key, sizeof(key), PSTR("rfb%s%d"), status ? "ON" : "OFF", id);
    return getSetting(key);
}

void rfbStatus(unsigned char id, bool status) {

    String value = rfbRetrieve(id, status);
    if (value.length() > 0) {

        bool same = _rfbSameOnOff(id);

        byte message[RF_MAX_MESSAGE_SIZE];
        int len = _rfbToArray(value.c_str(), message, 0);

        if (len == RF_MESSAGE_SIZE &&
                (message[0] != RF_CODE_START ||
                 message[1] != RF_CODE_RFOUT_BUCKET ||
                 message[2] + 4 != len ||
                 message[len-1] != RF_CODE_STOP)) {

            if (!_rfbin) {
                unsigned char times = same ? 1 : _rfb_repeat;
                _rfbSend(message, times);
            }

        } else {
            _rfbSendRawOnce(message, len);
        }

    }

    _rfbin = false;

}

void rfbLearn(unsigned char id, bool status) {
    _learnId = id;
    _learnStatus = status;
    _rfbLearn();
}

void rfbForget(unsigned char id, bool status) {

    char key[RF_MAX_KEY_LENGTH] = {0};
    snprintf_P(key, sizeof(key), PSTR("rfb%s%d"), status ? "ON" : "OFF", id);
    delSetting(key);


    #if WEB_SUPPORT
        char wsb[100];
        snprintf_P(wsb, sizeof(wsb), PSTR("{\"rfb\":[{\"id\": %d, \"status\": %d, \"data\": \"\"}]}"), id, status ? 1 : 0);
        wsSend(wsb);
    #endif

}





void rfbSetup() {

    #if MQTT_SUPPORT
        mqttRegister(_rfbMqttCallback);
    #endif

    #if API_SUPPORT
        _rfbAPISetup();
    #endif

    #if WEB_SUPPORT
        wsOnSendRegister(_rfbWebSocketOnSend);
        wsOnActionRegister(_rfbWebSocketOnAction);
        wsOnReceiveRegister(_rfbWebSocketOnReceive);
    #endif

    #if TERMINAL_SUPPORT
        _rfbInitCommands();
    #endif

    _rfb_repeat = getSetting("rfbRepeat", RF_SEND_TIMES).toInt();

    #if RFB_DIRECT
        unsigned char rx = getSetting("rfbRX", RFB_RX_PIN).toInt();
        unsigned char tx = getSetting("rfbTX", RFB_TX_PIN).toInt();

        _rfb_receive = gpioValid(rx);
        _rfb_transmit = gpioValid(tx);
        if (!_rfb_transmit && !_rfb_receive) {
            DEBUG_MSG_P(PSTR("[RF] Neither RX or TX are set\n"));
            return;
        }

        _rfModem = new RCSwitch();
        if (_rfb_receive) {
            _rfModem->enableReceive(rx);
            DEBUG_MSG_P(PSTR("[RF] RF receiver on GPIO %u\n"), rx);
        }
        if (_rfb_transmit) {
            _rfModem->enableTransmit(tx);
            _rfModem->setRepeatTransmit(_rfb_repeat);
            DEBUG_MSG_P(PSTR("[RF] RF transmitter on GPIO %u\n"), tx);
        }
    #else
        _rfb_receive = true;
        _rfb_transmit = true;
    #endif


    espurnaRegisterLoop(rfbLoop);

}

void rfbLoop() {
    _rfbReceive();
    _rfbSend();
}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/rfm69.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/rfm69.ino"
#if RFM69_SUPPORT

#include "libs/RFM69Wrap.h"

#define RFM69_PACKET_SEPARATOR ':'





RFM69Wrap * _rfm69_radio;

struct _node_t {
    unsigned long count = 0;
    unsigned long missing = 0;
    unsigned long duplicates = 0;
    unsigned char lastPacketID = 0;
};

_node_t _rfm69_node_info[RFM69_MAX_NODES];
unsigned char _rfm69_node_count;
unsigned long _rfm69_packet_count;





#if WEB_SUPPORT

void _rfm69WebSocketOnSend(JsonObject& root) {

    root["rfm69Visible"] = 1;
    root["rfm69Topic"] = getSetting("rfm69Topic", RFM69_DEFAULT_TOPIC);
    root["packetCount"] = _rfm69_packet_count;
    root["nodeCount"] = _rfm69_node_count;
    JsonArray& mappings = root.createNestedArray("mapping");
    for (unsigned char i=0; i<RFM69_MAX_TOPICS; i++) {
        unsigned char node = getSetting("node", i, 0).toInt();
        if (0 == node) break;
        JsonObject& mapping = mappings.createNestedObject();
        mapping["node"] = node;
        mapping["key"] = getSetting("key", i, "");
        mapping["topic"] = getSetting("topic", i, "");
    }

}

bool _rfm69WebSocketOnReceive(const char * key, JsonVariant& value) {
    if (strncmp(key, "rfm69", 5) == 0) return true;
    if (strncmp(key, "node", 4) == 0) return true;
    if (strncmp(key, "key", 3) == 0) return true;
    if (strncmp(key, "topic", 5) == 0) return true;
    return false;
}

void _rfm69WebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "clear-counts") == 0) _rfm69Clear();
}


#endif

void _rfm69CleanNodes(unsigned char num) {


    int i = 0;
    while (i < num) {
        if (getSetting("node", i, 0).toInt() == 0) break;
        if (getSetting("key", i, "").length() == 0) break;
        if (getSetting("topic", i, "").length() == 0) break;
        ++i;
    }


    while (i < WIFI_MAX_NETWORKS) {
        delSetting("node", i);
        delSetting("key", i);
        delSetting("topic", i);
        ++i;
    }

}

void _rfm69Configure() {
    _rfm69CleanNodes(RFM69_MAX_TOPICS);
}





void _rfm69Debug(const char * level, packet_t * data) {

    DEBUG_MSG_P(
        PSTR("[RFM69] %s: messageID:%05d senderID:%03d targetID:%03d packetID:%03d rssi:%-04d key:%s value:%s\n"),
        level,
        data->messageID,
        data->senderID,
        data->targetID,
        data->packetID,
        data->rssi,
        data->key,
        data->value
    );

}

void _rfm69Process(packet_t * data) {


    if (data->senderID >= RFM69_MAX_NODES) return;


    if (_rfm69_node_info[data->senderID].count == 0) ++_rfm69_node_count;



    if (data->packetID > 0) {
        if (_rfm69_node_info[data->senderID].count > 0) {

            unsigned char gap = data->packetID - _rfm69_node_info[data->senderID].lastPacketID;

            if (gap == 0) {
                _rfm69_node_info[data->senderID].duplicates = _rfm69_node_info[data->senderID].duplicates + 1;

                return;
            }

            if ((gap > 1) && (data->packetID > 1)) {
                _rfm69_node_info[data->senderID].missing = _rfm69_node_info[data->senderID].missing + gap - 1;
                DEBUG_MSG_P(PSTR("[RFM69] %u missing packets detected\n"), gap - 1);
            }
        }

    }

    _rfm69Debug("OK ", data);

    _rfm69_node_info[data->senderID].lastPacketID = data->packetID;
    _rfm69_node_info[data->senderID].count = _rfm69_node_info[data->senderID].count + 1;


    {
        char buffer[200];
        snprintf_P(
            buffer,
            sizeof(buffer) - 1,
            PSTR("{\"nodeCount\": %d, \"packetCount\": %lu, \"packet\": {\"senderID\": %u, \"targetID\": %u, \"packetID\": %u, \"key\": \"%s\", \"value\": \"%s\", \"rssi\": %d, \"duplicates\": %d, \"missing\": %d}}"),
            _rfm69_node_count, _rfm69_packet_count,
            data->senderID, data->targetID, data->packetID, data->key, data->value, data->rssi,
            _rfm69_node_info[data->senderID].duplicates , _rfm69_node_info[data->senderID].missing);
        wsSend(buffer);
    }


    if (!RFM69_PROMISCUOUS_SENDS && (RFM69_GATEWAY_ID != data->targetID)) return;


    for (unsigned int i=0; i<RFM69_MAX_TOPICS; i++) {
        unsigned char node = getSetting("node", i, 0).toInt();
        if (0 == node) break;
        if ((node == data->senderID) && (getSetting("key", i, "").equals(data->key))) {
            mqttSendRaw((char *) getSetting("topic", i, "").c_str(), (char *) String(data->value).c_str());
            return;
        }
    }


    String topic = getSetting("rfm69Topic", RFM69_DEFAULT_TOPIC);
    if (topic.length() > 0) {
        topic.replace("{node}", String(data->senderID));
        topic.replace("{key}", String(data->key));
        mqttSendRaw((char *) topic.c_str(), (char *) String(data->value).c_str());
    }

}

void _rfm69Loop() {

    if (_rfm69_radio->receiveDone()) {

        uint8_t senderID = _rfm69_radio->SENDERID;
        uint8_t targetID = _rfm69_radio->TARGETID;
        int16_t rssi = _rfm69_radio->RSSI;
        uint8_t length = _rfm69_radio->DATALEN;
        char buffer[length + 1];
        strncpy(buffer, (const char *) _rfm69_radio->DATA, length);
        buffer[length] = 0;



        if (!RFM69_PROMISCUOUS) {
            if (_rfm69_radio->ACKRequested()) _rfm69_radio->sendACK();
        }

        uint8_t parts = 1;
        for (uint8_t i=0; i<length; i++) {
            if (buffer[i] == RFM69_PACKET_SEPARATOR) ++parts;
        }

        if (parts > 1) {

            char sep[2] = {RFM69_PACKET_SEPARATOR, 0};

            uint8_t packetID = 0;
            char * key = strtok(buffer, sep);
            char * value = strtok(NULL, sep);
            if (parts > 2) {
                char * packet = strtok(NULL, sep);
                packetID = atoi(packet);
            }

            packet_t message;

            message.messageID = ++_rfm69_packet_count;
            message.packetID = packetID;
            message.senderID = senderID;
            message.targetID = targetID;
            message.key = key;
            message.value = value;
            message.rssi = rssi;

            _rfm69Process(&message);

        }

    }

}

void _rfm69Clear() {
    for(unsigned int i=0; i<RFM69_MAX_NODES; i++) {
        _rfm69_node_info[i].duplicates = 0;
        _rfm69_node_info[i].missing = 0;
        _rfm69_node_info[i].count = 0;
    }
    _rfm69_node_count = 0;
    _rfm69_packet_count = 0;
}





void rfm69Setup() {

    delay(10);

    _rfm69Configure();

    _rfm69_radio = new RFM69Wrap(RFM69_CS_PIN, RFM69_IRQ_PIN, RFM69_IS_RFM69HW, digitalPinToInterrupt(RFM69_IRQ_PIN));
    _rfm69_radio->initialize(RFM69_FREQUENCY, RFM69_NODE_ID, RFM69_NETWORK_ID);
    _rfm69_radio->encrypt(RFM69_ENCRYPTKEY);
    _rfm69_radio->promiscuous(RFM69_PROMISCUOUS);
    _rfm69_radio->enableAutoPower(0);
    if (RFM69_IS_RFM69HW) _rfm69_radio->setHighPower();

    DEBUG_MSG_P(PSTR("[RFM69] Worning at %u MHz\n"), RFM69_FREQUENCY == RF69_433MHZ ? 433 : RFM69_FREQUENCY == RF69_868MHZ ? 868 : 915);
    DEBUG_MSG_P(PSTR("[RFM69] Node %u\n"), RFM69_NODE_ID);
    DEBUG_MSG_P(PSTR("[RFM69] Network %u\n"), RFM69_NETWORK_ID);
    DEBUG_MSG_P(PSTR("[RFM69] Promiscuous mode %s\n"), RFM69_PROMISCUOUS ? "ON" : "OFF");

    #if WEB_SUPPORT
        wsOnSendRegister(_rfm69WebSocketOnSend);
        wsOnReceiveRegister(_rfm69WebSocketOnReceive);
        wsOnActionRegister(_rfm69WebSocketOnAction);
    #endif


    espurnaRegisterLoop(_rfm69Loop);
    espurnaRegisterReload(_rfm69Configure);

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/rtcmem.ino"






bool _rtcmem_status = false;

void _rtcmemErase() {
    auto ptr = reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR);
    const auto end = ptr + RTCMEM_BLOCKS;
    DEBUG_MSG_P(PSTR("[RTCMEM] Erasing start=%p end=%p\n"), ptr, end);
    do {
        *ptr = 0;
    } while (++ptr != end);
}

void _rtcmemInit() {
    _rtcmemErase();
    Rtcmem->magic = RTCMEM_MAGIC;
}


bool _rtcmemStatus() {
    bool readable;

    switch (systemResetReason()) {
        case REASON_EXT_SYS_RST:
        case REASON_WDT_RST:
        case REASON_DEFAULT_RST:
            readable = false;
            break;
        default:
            readable = true;
    }

    readable = readable and (RTCMEM_MAGIC == Rtcmem->magic);

    return readable;
}

#if TERMINAL_SUPPORT

void _rtcmemInitCommands() {
    terminalRegisterCommand(F("RTCMEM.REINIT"), [](Embedis* e) {
        _rtcmemInit();
    });

    terminalRegisterCommand(F("RTCMEM.DUMP"), [](Embedis* e) {

        DEBUG_MSG_P(PSTR("[RTCMEM] boot_status=%u status=%u blocks_used=%u\n"),
            _rtcmem_status, _rtcmemStatus(), RtcmemSize);

        String line;
        line.reserve(96);
        char buffer[16] = {0};

        auto addr = reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR);

        uint8_t block = 1;
        uint8_t offset = 0;
        uint8_t start = 0;

        do {

            offset = block - 1;

            snprintf(buffer, sizeof(buffer), "%08x ", *(addr + offset));
            line += buffer;

            if ((block % 8) == 0) {
                DEBUG_MSG_P(PSTR("%02u %p: %s\n"), start, addr+start, line.c_str());
                start = block;
                line = "";
            }

            ++block;

        } while (block<(RTCMEM_BLOCKS+1));

    });
}

#endif

bool rtcmemStatus() {
    return _rtcmem_status;
}

void rtcmemSetup() {
    _rtcmem_status = _rtcmemStatus();
    if (!_rtcmem_status) {
        _rtcmemInit();
    }

    #if TERMINAL_SUPPORT
        _rtcmemInitCommands();
    #endif
}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/scheduler.ino"
# 10 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/scheduler.ino"
#if SCHEDULER_SUPPORT

#include <TimeLib.h>



#if WEB_SUPPORT

bool _schWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "sch", 3) == 0);
}

void _schWebSocketOnSend(JsonObject &root){

    if (!relayCount()) return;

    root["schVisible"] = 1;
    root["maxSchedules"] = SCHEDULER_MAX_SCHEDULES;

    JsonObject &schedules = root.createNestedObject("schedules");
    uint8_t size = 0;

    JsonArray& enabled = schedules.createNestedArray("schEnabled");
    JsonArray& switch_ = schedules.createNestedArray("schSwitch");
    JsonArray& action = schedules.createNestedArray("schAction");
    JsonArray& type = schedules.createNestedArray("schType");
    JsonArray& hour = schedules.createNestedArray("schHour");
    JsonArray& minute = schedules.createNestedArray("schMinute");
    JsonArray& utc = schedules.createNestedArray("schUTC");
    JsonArray& weekdays = schedules.createNestedArray("schWDs");

    for (byte i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {
        if (!hasSetting("schSwitch", i)) break;
        ++size;

        enabled.add<uint8_t>(getSetting("schEnabled", i, 1).toInt() == 1);
        utc.add<uint8_t>(getSetting("schUTC", i, 0).toInt() == 1);

        switch_.add(getSetting("schSwitch", i, 0).toInt());
        action.add(getSetting("schAction", i, 0).toInt());
        type.add(getSetting("schType", i, 0).toInt());
        hour.add(getSetting("schHour", i, 0).toInt());
        minute.add(getSetting("schMinute", i, 0).toInt());
        weekdays.add(getSetting("schWDs", i, ""));
    }

    schedules["size"] = size;
    schedules["start"] = 0;

}

#endif



void _schConfigure() {

    bool delete_flag = false;

    for (unsigned char i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {

        int sch_switch = getSetting("schSwitch", i, 0xFF).toInt();
        if (sch_switch == 0xFF) delete_flag = true;

        if (delete_flag) {

            delSetting("schEnabled", i);
            delSetting("schSwitch", i);
            delSetting("schAction", i);
            delSetting("schHour", i);
            delSetting("schMinute", i);
            delSetting("schWDs", i);
            delSetting("schType", i);
            delSetting("schUTC", i);

        } else {

            #if DEBUG_SUPPORT

                bool sch_enabled = getSetting("schEnabled", i, 1).toInt() == 1;
                int sch_action = getSetting("schAction", i, 0).toInt();
                int sch_hour = getSetting("schHour", i, 0).toInt();
                int sch_minute = getSetting("schMinute", i, 0).toInt();
                bool sch_utc = getSetting("schUTC", i, 0).toInt() == 1;
                String sch_weekdays = getSetting("schWDs", i, "");
                unsigned char sch_type = getSetting("schType", i, SCHEDULER_TYPE_SWITCH).toInt();

                DEBUG_MSG_P(
                    PSTR("[SCH] Schedule #%d: %s #%d to %d at %02d:%02d %s on %s%s\n"),
                    i, SCHEDULER_TYPE_SWITCH == sch_type ? "switch" : "channel", sch_switch,
                    sch_action, sch_hour, sch_minute, sch_utc ? "UTC" : "local time",
                    (char *) sch_weekdays.c_str(),
                    sch_enabled ? "" : " (disabled)"
                );

            #endif

        }

    }

}

bool _schIsThisWeekday(time_t t, String weekdays){


    int w = weekday(t) - 1;
    if (0 == w) w = 7;

    char pch;
    char * p = (char *) weekdays.c_str();
    unsigned char position = 0;
    while ((pch = p[position++])) {
        if ((pch - '0') == w) return true;
    }
    return false;

}

int _schMinutesLeft(time_t t, unsigned char schedule_hour, unsigned char schedule_minute){
    unsigned char now_hour = hour(t);
    unsigned char now_minute = minute(t);
    return (schedule_hour - now_hour) * 60 + schedule_minute - now_minute;
}

void _schCheck() {

    time_t local_time = now();
    time_t utc_time = ntpLocal2UTC(local_time);


    for (unsigned char i = 0; i < SCHEDULER_MAX_SCHEDULES; i++) {

        int sch_switch = getSetting("schSwitch", i, 0xFF).toInt();
        if (sch_switch == 0xFF) break;


        if (getSetting("schEnabled", i, 1).toInt() == 0) continue;


        bool sch_utc = getSetting("schUTC", i, 0).toInt() == 1;
        time_t t = sch_utc ? utc_time : local_time;

        String sch_weekdays = getSetting("schWDs", i, "");
        if (_schIsThisWeekday(t, sch_weekdays)) {

            int sch_hour = getSetting("schHour", i, 0).toInt();
            int sch_minute = getSetting("schMinute", i, 0).toInt();
            int minutes_to_trigger = _schMinutesLeft(t, sch_hour, sch_minute);

            if (minutes_to_trigger == 0) {

                unsigned char sch_type = getSetting("schType", i, SCHEDULER_TYPE_SWITCH).toInt();

                if (SCHEDULER_TYPE_SWITCH == sch_type) {
                    int sch_action = getSetting("schAction", i, 0).toInt();
                    DEBUG_MSG_P(PSTR("[SCH] Switching switch %d to %d\n"), sch_switch, sch_action);
                    if (sch_action == 2) {
                        relayToggle(sch_switch);
                    } else {
                        relayStatus(sch_switch, sch_action);
                    }
                }

                #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
                    if (SCHEDULER_TYPE_DIM == sch_type) {
                        int sch_brightness = getSetting("schAction", i, -1).toInt();
                        DEBUG_MSG_P(PSTR("[SCH] Set channel %d value to %d\n"), sch_switch, sch_brightness);
                        lightChannel(sch_switch, sch_brightness);
                        lightUpdate(true, true);
                    }
                #endif

                DEBUG_MSG_P(PSTR("[SCH] Schedule #%d TRIGGERED!!\n"), i);






            } else if (minutes_to_trigger > 0) {

                #if DEBUG_SUPPORT
                    if ((minutes_to_trigger % 15 == 0) || (minutes_to_trigger < 15)) {
                        DEBUG_MSG_P(
                            PSTR("[SCH] %d minutes to trigger schedule #%d\n"),
                            minutes_to_trigger, i
                        );
                    }
                #endif

            }

        }

    }

}

void _schLoop() {


    if (!ntpSynced()) return;


    static unsigned long last_minute = 60;
    unsigned char current_minute = minute();
    if (current_minute != last_minute) {
        last_minute = current_minute;
        _schCheck();
    }

}



void schSetup() {

    _schConfigure();


    #if WEB_SUPPORT
        wsOnSendRegister(_schWebSocketOnSend);
        wsOnReceiveRegister(_schWebSocketOnReceive);
    #endif


    espurnaRegisterLoop(_schLoop);
    espurnaRegisterReload(_schConfigure);

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/sensor.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/sensor.ino"
#if SENSOR_SUPPORT

#include <vector>
#include "filters/LastFilter.h"
#include "filters/MaxFilter.h"
#include "filters/MedianFilter.h"
#include "filters/MovingAverageFilter.h"
#include "sensors/BaseSensor.h"

#include <float.h>

typedef struct {
    BaseSensor * sensor;
    BaseFilter * filter;
    unsigned char local;
    unsigned char type;
    unsigned char decimals;
    unsigned char global;
    double last;
    double reported;
    double min_change;
    double max_change;
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





unsigned char _magnitudeDecimals(unsigned char type) {



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
        if (_sensor_energy_units == ENERGY_KWH) value = value / 3600000;
    }
    if (type == MAGNITUDE_POWER_ACTIVE ||
        type == MAGNITUDE_POWER_APPARENT ||
        type == MAGNITUDE_POWER_REACTIVE) {
        if (_sensor_power_units == POWER_KILOWATTS) value = value / 1000;
    }

    return roundTo(value, decimals);

}



#if WEB_SUPPORT

template<typename T> void _sensorWebSocketMagnitudes(JsonObject& root, T prefix) {


    String ws_name = String(prefix);
    ws_name.concat("Magnitudes");


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

        root["pwrUnits"] = _sensor_power_units;
        root["eneUnits"] = _sensor_energy_units;
        root["tmpUnits"] = _sensor_temperature_units;
        root["tmpCorrection"] = _sensor_temperature_correction;
        root["humCorrection"] = _sensor_humidity_correction;
        root["snsRead"] = _sensor_read_interval / 1000;
        root["snsReport"] = _sensor_report_every;
        root["snsSave"] = _sensor_save_every;
    }
# 289 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/sensor.ino"
}

#endif

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

#endif

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
                DEBUG_MSG_P(PSTR("Device %d/%s - Current: %s Voltage: %s Power: %s Energy: %s\n"),
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


    if (_sensor_save_every > 0) {
        save_count = (save_count + 1) % _sensor_save_every;
        if (0 == save_count) {
            setSetting("eneTotal", value);
            saveSettings();
        }
    }


    Rtcmem->energy = value;
}





void _sensorLoad() {
# 480 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/sensor.ino"
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

        const unsigned char number = constrain(getSetting("bmx280Number", BMX280_NUMBER).toInt(), 1, 2);



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
# 575 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/sensor.ino"
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
        GeigerSensor * sensor = new GeigerSensor();
        sensor->setGPIO(GEIGER_PIN);
        sensor->setMode(GEIGER_PIN_MODE);
        sensor->setDebounceTime(GEIGER_DEBOUNCE);
        sensor->setInterruptMode(GEIGER_INTERRUPT_MODE);
        sensor->setCPM2SievertFactor(GEIGER_CPM2SIEVERT);
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


        if (_sensors[i]->ready()) continue;
        DEBUG_MSG_P(PSTR("[SENSOR] Initializing %s\n"), _sensors[i]->description().c_str());


        _sensors[i]->begin();
        if (!_sensors[i]->ready()) {
            if (_sensors[i]->error() != 0) DEBUG_MSG_P(PSTR("[SENSOR]  -> ERROR %d\n"), _sensors[i]->error());
            _sensors_ready = false;
            continue;
        }


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
            } else if (MAGNITUDE_COUNT == type || MAGNITUDE_GEIGER_CPM == type || MAGNITUDE_GEIGER_SIEVERT == type) {
                new_magnitude.filter = new MovingAverageFilter();
            } else {
                new_magnitude.filter = new MedianFilter();
            }
            new_magnitude.filter->resize(_sensor_report_every);

            _magnitudes.push_back(new_magnitude);

            DEBUG_MSG_P(PSTR("[SENSOR]  -> %s:%d\n"), magnitudeTopic(type).c_str(), _counts[type]);

            _counts[type] = _counts[type] + 1;

        }


        _sensors[i]->onEvent([i](unsigned char type, double value) {
            _sensorCallback(i, type, value);
        });



        #if MICS2710_SUPPORT
            if (_sensors[i]->getID() == SENSOR_MICS2710_ID) {
                MICS2710Sensor * sensor = (MICS2710Sensor *) _sensors[i];
                sensor->setR0(getSetting("snsR0", MICS2710_R0).toInt());
            }
        #endif

        #if MICS5525_SUPPORT
            if (_sensors[i]->getID() == SENSOR_MICS5525_ID) {
                MICS5525Sensor * sensor = (MICS5525Sensor *) _sensors[i];
                sensor->setR0(getSetting("snsR0", MICS5525_R0).toInt());
            }
        #endif

        #if EMON_ANALOG_SUPPORT

            if (_sensors[i]->getID() == SENSOR_EMON_ANALOG_ID) {
                EmonAnalogSensor * sensor = (EmonAnalogSensor *) _sensors[i];
                sensor->setCurrentRatio(0, getSetting("pwrRatioC", EMON_CURRENT_RATIO).toFloat());
                sensor->setVoltage(getSetting("pwrVoltage", EMON_MAINS_VOLTAGE).toInt());

                double value = _sensorEnergyTotal();

                if (value > 0) sensor->resetEnergy(0, value);
            }

        #endif

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

        #endif

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

        #endif

        #if PULSEMETER_SUPPORT
            if (_sensors[i]->getID() == SENSOR_PULSEMETER_ID) {
                PulseMeterSensor * sensor = (PulseMeterSensor *) _sensors[i];
                sensor->setEnergyRatio(getSetting("pwrRatioE", PULSEMETER_ENERGY_RATIO).toInt());
            }
        #endif

    }

}

void _sensorConfigure() {


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


    for (unsigned char i=0; i<_sensors.size(); i++) {

        #if MICS2710_SUPPORT

            if (_sensors[i]->getID() == SENSOR_MICS2710_ID) {
                if (getSetting("snsResetCalibration", 0).toInt() == 1) {
                    MICS2710Sensor * sensor = (MICS2710Sensor *) _sensors[i];
                    sensor->calibrate();
                    setSetting("snsR0", sensor->getR0());
                }
            }

        #endif

        #if MICS5525_SUPPORT

            if (_sensors[i]->getID() == SENSOR_MICS5525_ID) {
                if (getSetting("snsResetCalibration", 0).toInt() == 1) {
                    MICS5525Sensor * sensor = (MICS5525Sensor *) _sensors[i];
                    sensor->calibrate();
                    setSetting("snsR0", sensor->getR0());
                }
            }

        #endif

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

        #endif

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

        #endif

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

        #endif

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
        #endif

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

        #endif

    }


    for (unsigned char i=0; i<_magnitudes.size(); i++) {
        _magnitudes[i].filter->resize(_sensor_report_every);
    }


    if (0 == _sensor_save_every) {
        delSetting("eneTotal");
    }


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
        #endif

    #endif

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
    #endif

}





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



void sensorSetup() {


    moveSetting("powerUnits", "pwrUnits");
    moveSetting("energyUnits", "eneUnits");


    moveSettings("pzEneTotal", "pzemEneTotal");


    _sensorLoad();
    _sensorInit();


    _sensorConfigure();


    #if WEB_SUPPORT
        wsOnSendRegister(_sensorWebSocketStart);
        wsOnReceiveRegister(_sensorWebSocketOnReceive);
        wsOnSendRegister(_sensorWebSocketSendData);
    #endif


    #if API_SUPPORT
        _sensorAPISetup();
    #endif


    #if TERMINAL_SUPPORT
        _sensorInitCommands();
    #endif


    espurnaRegisterLoop(sensorLoop);
    espurnaRegisterReload(_sensorConfigure);

}

void sensorLoop() {


    static unsigned long last_init = 0;
    if (!_sensors_ready) {
        if (millis() - last_init > SENSOR_INIT_INTERVAL) {
            last_init = millis();
            _sensorInit();
        }
    }

    if (_magnitudes.size() == 0) return;


    _sensorTick();


    static unsigned long last_update = 0;
    static unsigned long report_count = 0;
    if (millis() - last_update > _sensor_read_interval) {

        last_update = millis();
        report_count = (report_count + 1) % _sensor_report_every;

        double value_raw;
        double value_show;
        double value_filtered;


        _sensorPre();


        #if SENSOR_POWER_CHECK_STATUS
            bool relay_off = (relayCount() == 1) && (relayStatus(0) == 0);
        #endif


        for (unsigned char i=0; i<_magnitudes.size(); i++) {

            sensor_magnitude_t magnitude = _magnitudes[i];

            if (magnitude.sensor->status()) {





                value_raw = magnitude.sensor->value(magnitude.local);


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





                magnitude.filter->add(value_raw);


                if (MAGNITUDE_COUNT == magnitude.type ||
                    MAGNITUDE_GEIGER_CPM ==magnitude. type ||
                    MAGNITUDE_GEIGER_SIEVERT == magnitude.type) {
                    value_raw = magnitude.filter->result();
                }





                value_show = _magnitudeProcess(magnitude.type, magnitude.decimals, value_raw);





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
                #endif






                bool report = (0 == report_count);
                if ((MAGNITUDE_ENERGY == magnitude.type) && (magnitude.max_change > 0)) {

                    report = (fabs(value_show - magnitude.reported) >= magnitude.max_change);
                }

                if (report) {

                    value_filtered = magnitude.filter->result();
                    value_filtered = _magnitudeProcess(magnitude.type, magnitude.decimals, value_filtered);
                    magnitude.filter->reset();


                    if (fabs(value_filtered - magnitude.reported) >= magnitude.min_change) {
                        _magnitudes[i].reported = value_filtered;
                        _sensorReport(i, value_filtered);
                    }



                    if (MAGNITUDE_ENERGY == magnitude.type) {
                        _sensorEnergyTotal(value_raw);
                    }

                }

            }
        }


        _sensorPost();

        #if WEB_SUPPORT
            wsSend(_sensorWebSocketSendData);
        #endif

        #if THINGSPEAK_SUPPORT
            if (report_count == 0) tspkFlush();
        #endif

    }

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/settings.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/settings.ino"
#include <vector>
#include "libs/EmbedisWrap.h"





unsigned long settingsSize() {
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
    }
    return SPI_FLASH_SEC_SIZE - pos + EEPROM_DATA_END;
}



unsigned int settingsKeyCount() {
    unsigned count = 0;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
        len = EEPROMr.read(pos);
        pos = pos - len - 2;
        count ++;
    }
    return count;
}

String settingsKeyName(unsigned int index) {

    String s;

    unsigned count = 0;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROMr.read(pos)) {
        if (0xFF == len) break;
        pos = pos - len - 2;
        if (count == index) {
            s.reserve(len);
            for (unsigned char i = 0 ; i < len; i++) {
                s += (char) EEPROMr.read(pos + i + 1);
            }
            break;
        }
        count++;
        len = EEPROMr.read(pos);
        pos = pos - len - 2;
    }

    return s;

}

std::vector<String> _settingsKeys() {


    std::vector<String> keys;


    unsigned int size = settingsKeyCount();
    for (unsigned int i=0; i<size; i++) {


        String key = settingsKeyName(i);
        bool inserted = false;
        for (unsigned char j=0; j<keys.size(); j++) {


            if (keys[j].compareTo(key) > 0) {
                keys.insert(keys.begin() + j, key);
                inserted = true;
                break;
            }

        }


        if (!inserted) keys.push_back(key);

    }

    return keys;
}





void moveSetting(const char * from, const char * to) {
    String value = getSetting(from);
    if (value.length() > 0) setSetting(to, value);
    delSetting(from);
}

void moveSetting(const char * from, const char * to, unsigned int index) {
    String value = getSetting(from, index, "");
    if (value.length() > 0) setSetting(to, index, value);
    delSetting(from, index);
}

void moveSettings(const char * from, const char * to) {
    unsigned int index = 0;
    while (index < 100) {
        String value = getSetting(from, index, "");
        if (value.length() == 0) break;
        setSetting(to, index, value);
        delSetting(from, index);
        index++;
    }
}

template<typename T> String getSetting(const String& key, T defaultValue) {
    String value;
    if (!Embedis::get(key, value)) value = String(defaultValue);
    return value;
}

template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue) {
    return getSetting(key + String(index), defaultValue);
}

String getSetting(const String& key) {
    return getSetting(key, "");
}

template<typename T> bool setSetting(const String& key, T value) {
    return Embedis::set(key, String(value));
}

template<typename T> bool setSetting(const String& key, unsigned int index, T value) {
    return setSetting(key + String(index), value);
}

bool delSetting(const String& key) {
    return Embedis::del(key);
}

bool delSetting(const String& key, unsigned int index) {
    return delSetting(key + String(index));
}

bool hasSetting(const String& key) {
    return getSetting(key).length() != 0;
}

bool hasSetting(const String& key, unsigned int index) {
    return getSetting(key, index, "").length() != 0;
}

void saveSettings() {
    #if not SETTINGS_AUTOSAVE
        eepromCommit();
    #endif
}

void resetSettings() {
    for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
        EEPROMr.write(i, 0xFF);
    }
    EEPROMr.commit();
}





size_t settingsMaxSize() {
    size_t size = EEPROM_SIZE;
    if (size > SPI_FLASH_SEC_SIZE) size = SPI_FLASH_SEC_SIZE;
    size = (size + 3) & (~3);
    return size;
}

bool settingsRestoreJson(JsonObject& data) {


    const char* app = data["app"];
    if (!app || strcmp(app, APP_NAME) != 0) {
        DEBUG_MSG_P(PSTR("[SETTING] Wrong or missing 'app' key\n"));
        return false;
    }


    bool is_backup = data["backup"];
    if (is_backup) {
        for (unsigned int i = EEPROM_DATA_END; i < SPI_FLASH_SEC_SIZE; i++) {
            EEPROMr.write(i, 0xFF);
        }
    }


    for (auto element : data) {
        if (strcmp(element.key, "app") == 0) continue;
        if (strcmp(element.key, "version") == 0) continue;
        if (strcmp(element.key, "backup") == 0) continue;
        setSetting(element.key, element.value.as<char*>());
    }


    saveSettings();

    DEBUG_MSG_P(PSTR("[SETTINGS] Settings restored successfully\n"));
    return true;

}

void settingsGetJson(JsonObject& root) {


    std::vector<String> keys = _settingsKeys();


    for (unsigned int i=0; i<keys.size(); i++) {
        String value = getSetting(keys[i]);
        root[keys[i]] = value;
    }

}





void settingsSetup() {

    Embedis::dictionary( F("EEPROM"),
        SPI_FLASH_SEC_SIZE,
        [](size_t pos) -> char { return EEPROMr.read(pos); },
        [](size_t pos, char value) { EEPROMr.write(pos, value); },
        #if SETTINGS_AUTOSAVE
            []() { eepromCommit(); }
        #else
            []() {}
        #endif
    );

}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ssdp.ino"
# 11 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ssdp.ino"
#if SSDP_SUPPORT

#include <ESP8266SSDP.h>

const char _ssdp_template[] PROGMEM =
    "<?xml version=\"1.0\"?>"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
        "<specVersion>"
            "<major>1</major>"
            "<minor>0</minor>"
        "</specVersion>"
        "<URLBase>http://%s:%u/</URLBase>"
        "<device>"
            "<deviceType>%s</deviceType>"
            "<friendlyName>%s</friendlyName>"
            "<presentationURL>/</presentationURL>"
            "<serialNumber>%u</serialNumber>"
            "<modelName>%s</modelName>"
            "<modelNumber>%s</modelNumber>"
            "<modelURL>%s</modelURL>"
            "<manufacturer>%s</manufacturer>"
            "<manufacturerURL>%s</manufacturerURL>"
            "<UDN>uuid:38323636-4558-4dda-9188-cda0e6%06x</UDN>"
        "</device>"
    "</root>\r\n"
    "\r\n";

void ssdpSetup() {

    webServer()->on("/description.xml", HTTP_GET, [](AsyncWebServerRequest *request) {

        DEBUG_MSG_P(PSTR("[SSDP] Schema request\n"));

        IPAddress ip = WiFi.localIP();
        uint32_t chipId = ESP.getChipId();

        char response[strlen_P(_ssdp_template) + 100];
        snprintf_P(response, sizeof(response), _ssdp_template,
            ip.toString().c_str(),
            webPort(),
            SSDP_DEVICE_TYPE,
            getSetting("hostname").c_str(),
            chipId,
            APP_NAME,
            APP_VERSION,
            APP_WEBSITE,
            DEVICE_NAME,
            "",
            chipId
        );

        request->send(200, "text/xml", response);

    });

    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(webPort());
    SSDP.setDeviceType(SSDP_DEVICE_TYPE);
    SSDP.setName(getSetting("hostname"));
    SSDP.setSerialNumber(String(ESP.getChipId()));
    SSDP.setModelName(APP_NAME);
    SSDP.setModelNumber(APP_VERSION);
    SSDP.setModelURL(APP_WEBSITE);
    SSDP.setManufacturer(DEVICE_NAME);
    SSDP.setManufacturerURL("");
    SSDP.setURL("/");
    SSDP.begin();

    DEBUG_MSG_P(PSTR("[SSDP] Started\n"));

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/system.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/system.ino"
#include <Ticker.h>
#include <EEPROM_Rotate.h>



bool _system_send_heartbeat = false;
unsigned char _heartbeat_mode = HEARTBEAT_MODE;
unsigned long _heartbeat_interval = HEARTBEAT_INTERVAL;


unsigned short int _load_average = 100;



union system_rtcmem_t {
    struct {
        uint8_t stability_counter;
        uint8_t reset_reason;
        uint16_t _reserved_;
    } parts;
    uint32_t value;
};

uint8_t systemStabilityCounter() {
    system_rtcmem_t data;
    data.value = Rtcmem->sys;
    return data.parts.stability_counter;
}

void systemStabilityCounter(uint8_t counter) {
    system_rtcmem_t data;
    data.value = Rtcmem->sys;
    data.parts.stability_counter = counter;
    Rtcmem->sys = data.value;
}

uint8_t _systemResetReason() {
    system_rtcmem_t data;
    data.value = Rtcmem->sys;
    return data.parts.reset_reason;
}

void _systemResetReason(uint8_t reason) {
    system_rtcmem_t data;
    data.value = Rtcmem->sys;
    data.parts.reset_reason = reason;
    Rtcmem->sys = data.value;
}

#if SYSTEM_CHECK_ENABLED
# 67 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/system.ino"
bool _systemStable = true;

void systemCheck(bool stable) {
    uint8_t value = 0;

    if (stable) {
        value = 0;
        DEBUG_MSG_P(PSTR("[MAIN] System OK\n"));
    } else {
        if (!rtcmemStatus()) {
            systemStabilityCounter(1);
            return;
        }

        value = systemStabilityCounter();

        if (++value > SYSTEM_CHECK_MAX) {
            _systemStable = false;
            value = 0;
            DEBUG_MSG_P(PSTR("[MAIN] System UNSTABLE\n"));
        }
    }

    systemStabilityCounter(value);
}

bool systemCheck() {
    return _systemStable;
}

void systemCheckLoop() {
    static bool checked = false;
    if (!checked && (millis() > SYSTEM_CHECK_TIME)) {

        systemCheck(true);
        checked = true;
    }
}

#endif




Ticker _defer_reset;
uint8_t _reset_reason = 0;


uint32_t systemResetReason() {
    return resetInfo.reason;
}

void customResetReason(unsigned char reason) {
    _reset_reason = reason;
    _systemResetReason(reason);
}

unsigned char customResetReason() {
    static unsigned char status = 255;
    if (status == 255) {
        if (rtcmemStatus()) status = _systemResetReason();
        if (status > 0) customResetReason(0);
        if (status > CUSTOM_RESET_MAX) status = 0;
    }
    return status;
}

void reset() {
    ESP.restart();
}

void deferredReset(unsigned long delay, unsigned char reason) {
    _defer_reset.once_ms(delay, customResetReason, reason);
}

bool checkNeedsReset() {
    return _reset_reason > 0;
}



void systemSendHeartbeat() {
    _system_send_heartbeat = true;
}

bool systemGetHeartbeat() {
    return _system_send_heartbeat;
}

unsigned long systemLoadAverage() {
    return _load_average;
}

void _systemSetupHeartbeat() {
    _heartbeat_mode = getSetting("hbMode", HEARTBEAT_MODE).toInt();
    _heartbeat_interval = getSetting("hbInterval", HEARTBEAT_INTERVAL).toInt();
}

#if WEB_SUPPORT
    bool _systemWebSocketOnReceive(const char * key, JsonVariant& value) {
        if (strncmp(key, "sys", 3) == 0) return true;
        if (strncmp(key, "hb", 2) == 0) return true;
        return false;
    }
#endif

void systemLoop() {





    if (checkNeedsReset()) {
        reset();
    }





    #if SYSTEM_CHECK_ENABLED
        systemCheckLoop();
    #endif





    if (_system_send_heartbeat && _heartbeat_mode == HEARTBEAT_ONCE) {
        heartbeat();
        _system_send_heartbeat = false;
    } else if (_heartbeat_mode == HEARTBEAT_REPEAT || _heartbeat_mode == HEARTBEAT_REPEAT_STATUS) {
        static unsigned long last_hbeat = 0;
        #if NTP_SUPPORT
            if ((_system_send_heartbeat && ntpSynced()) || (millis() - last_hbeat > _heartbeat_interval * 1000)) {
        #else
            if (_system_send_heartbeat || (millis() - last_hbeat > _heartbeat_interval * 1000)) {
        #endif
            last_hbeat = millis();
            heartbeat();
           _system_send_heartbeat = false;
        }
    }





    static unsigned long last_loadcheck = 0;
    static unsigned long load_counter_temp = 0;
    load_counter_temp++;

    if (millis() - last_loadcheck > LOADAVG_INTERVAL) {

        static unsigned long load_counter = 0;
        static unsigned long load_counter_max = 1;

        load_counter = load_counter_temp;
        load_counter_temp = 0;
        if (load_counter > load_counter_max) {
            load_counter_max = load_counter;
        }
        _load_average = 100 - (100 * load_counter / load_counter_max);
        last_loadcheck = millis();

    }

}

void _systemSetupSpecificHardware() {


    #if defined(MANCAVEMADE_ESPLIVE)
        pinMode(16, OUTPUT);
        digitalWrite(16, HIGH);
    #endif



    #if (RF_SUPPORT && !RFB_DIRECT) || (RELAY_PROVIDER == RELAY_PROVIDER_DUAL) || (RELAY_PROVIDER == RELAY_PROVIDER_STM)
        Serial.begin(SERIAL_BAUDRATE);
    #endif

}

void systemSetup() {

    #if SPIFFS_SUPPORT
        SPIFFS.begin();
    #endif


    #if SYSTEM_CHECK_ENABLED
        systemCheck(false);
    #endif

    #if WEB_SUPPORT
        wsOnReceiveRegister(_systemWebSocketOnReceive);
    #endif


    _systemSetupSpecificHardware();


    espurnaRegisterLoop(systemLoop);


    _systemSetupHeartbeat();

}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/telnet.ino"
# 11 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/telnet.ino"
#if TELNET_SUPPORT

#if TELNET_SERVER == TELNET_SERVER_WIFISERVER
    #include <ESP8266WiFi.h>
    WiFiServer _telnetServer = WiFiServer(TELNET_PORT);
    std::unique_ptr<WiFiClient> _telnetClients[TELNET_MAX_CLIENTS];
#else
    #include <ESPAsyncTCP.h>
    AsyncServer _telnetServer = AsyncServer(TELNET_PORT);
    std::unique_ptr<AsyncClient> _telnetClients[TELNET_MAX_CLIENTS];
#endif

bool _telnetFirst = true;

bool _telnetAuth = TELNET_AUTHENTICATION;
bool _telnetClientsAuth[TELNET_MAX_CLIENTS];





#if WEB_SUPPORT

bool _telnetWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "telnet", 6) == 0);
}

void _telnetWebSocketOnSend(JsonObject& root) {
    root["telnetVisible"] = 1;
    root["telnetSTA"] = getSetting("telnetSTA", TELNET_STA).toInt() == 1;
    root["telnetAuth"] = getSetting("telnetAuth", TELNET_AUTHENTICATION).toInt() == 1;
}

#endif

void _telnetDisconnect(unsigned char clientId) {

    #if TELNET_SERVER == TELNET_SERVER_WIFISERVER
        _telnetClients[clientId]->stop();
    #endif
    _telnetClients[clientId] = nullptr;
    wifiReconnectCheck();
    DEBUG_MSG_P(PSTR("[TELNET] Client #%d disconnected\n"), clientId);
}

bool _telnetWrite(unsigned char clientId, const char *data, size_t len) {
    if (_telnetClients[clientId] && _telnetClients[clientId]->connected()) {
        return (_telnetClients[clientId]->write(data, len) > 0);
    }
    return false;
}

unsigned char _telnetWrite(const char *data, size_t len) {
    unsigned char count = 0;
    for (unsigned char i = 0; i < TELNET_MAX_CLIENTS; i++) {

        if (_telnetAuth && !_telnetClientsAuth[i]) {
            continue;
        }

        if (_telnetWrite(i, data, len)) ++count;
    }
    return count;
}

unsigned char _telnetWrite(const char *data) {
    return _telnetWrite(data, strlen(data));
}

bool _telnetWrite(unsigned char clientId, const char * message) {
    return _telnetWrite(clientId, message, strlen(message));
}

void _telnetData(unsigned char clientId, void *data, size_t len) {

    if (_telnetFirst) {
        _telnetFirst = false;
        return;
    }


    char * p = (char *) data;


    if (len >= 2) {
        if ((p[0] == 0xFF) && (p[1] == 0xEC)) {
            _telnetDisconnect(clientId);
            return;
        }
    }

    if ((strncmp(p, "close", 5) == 0) || (strncmp(p, "quit", 4) == 0)) {
        _telnetDisconnect(clientId);
        return;
    }


    #ifdef ESPURNA_CORE
        const bool authenticated = true;
    #else
        const bool authenticated = _telnetClientsAuth[clientId];
    #endif

    if (_telnetAuth && !authenticated) {
        String password = getAdminPass();
        if (strncmp(p, password.c_str(), password.length()) == 0) {
            DEBUG_MSG_P(PSTR("[TELNET] Client #%d authenticated\n"), clientId);
            _telnetWrite(clientId, "Password correct, welcome!\n");
            _telnetClientsAuth[clientId] = true;
        } else {
            _telnetWrite(clientId, "Password (try again): ");
        }
        return;
    }


    #if TERMINAL_SUPPORT
        terminalInject(data, len);
    #endif
}

void _telnetNotifyConnected(unsigned char i) {

    DEBUG_MSG_P(PSTR("[TELNET] Client #%u connected\n"), i);


    #if TERMINAL_SUPPORT == 0
        info();
        wifiDebug();
        crashDump();
        crashClear();
    #endif

    #ifdef ESPURNA_CORE
        _telnetClientsAuth[i] = true;
    #else
        _telnetClientsAuth[i] = !_telnetAuth;
        if (_telnetAuth) {
            if (getAdminPass().length()) {
                _telnetWrite(i, "Password: ");
            } else {
                _telnetClientsAuth[i] = true;
            }
        }
    #endif

    _telnetFirst = true;
    wifiReconnectCheck();

}

#if TELNET_SERVER == TELNET_SERVER_WIFISERVER

void _telnetLoop() {
    if (_telnetServer.hasClient()) {
        int i;

        for (i = 0; i < TELNET_MAX_CLIENTS; i++) {
            if (!_telnetClients[i] || !_telnetClients[i]->connected()) {

                _telnetClients[i] = std::unique_ptr<WiFiClient>(new WiFiClient(_telnetServer.available()));

                if (_telnetClients[i]->localIP() != WiFi.softAPIP()) {

                    #ifdef ESPURNA_CORE
                        bool telnetSTA = true;
                    #else
                        bool telnetSTA = getSetting("telnetSTA", TELNET_STA).toInt() == 1;
                    #endif

                    if (!telnetSTA) {
                        DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Only local connections\n"));
                        _telnetDisconnect(i);
                        return;
                    }
                }

                _telnetNotifyConnected(i);

                break;
            }
        }


        if (i == TELNET_MAX_CLIENTS) {
            DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Too many connections\n"));
            _telnetServer.available().stop();
            return;
        }
    }

    for (int i = 0; i < TELNET_MAX_CLIENTS; i++) {
        if (_telnetClients[i]) {

            if (!_telnetClients[i]->connected()) {
                _telnetDisconnect(i);
            } else {

                while (_telnetClients[i] && _telnetClients[i]->available()) {
                    char data[TERMINAL_BUFFER_SIZE];
                    size_t len = _telnetClients[i]->available();
                    unsigned int r = _telnetClients[i]->readBytes(data, min(sizeof(data), len));

                    _telnetData(i, data, r);
                }
            }
        }
    }
}

#else

void _telnetNewClient(AsyncClient* client) {
    if (client->localIP() != WiFi.softAPIP()) {

        #ifdef ESPURNA_CORE
            bool telnetSTA = true;
        #else
            bool telnetSTA = getSetting("telnetSTA", TELNET_STA).toInt() == 1;
        #endif

        if (!telnetSTA) {
            DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Only local connections\n"));
            client->onDisconnect([](void *s, AsyncClient *c) {
                delete c;
            });
            client->close(true);
            return;
        }
    }

    for (unsigned char i = 0; i < TELNET_MAX_CLIENTS; i++) {

        if (!_telnetClients[i] || !_telnetClients[i]->connected()) {

            _telnetClients[i] = std::unique_ptr<AsyncClient>(client);

            _telnetClients[i]->onAck([i](void *s, AsyncClient *c, size_t len, uint32_t time) {
            }, 0);

            _telnetClients[i]->onData([i](void *s, AsyncClient *c, void *data, size_t len) {
                _telnetData(i, data, len);
            }, 0);

            _telnetClients[i]->onDisconnect([i](void *s, AsyncClient *c) {
                _telnetDisconnect(i);
            }, 0);

            _telnetClients[i]->onError([i](void *s, AsyncClient *c, int8_t error) {
                DEBUG_MSG_P(PSTR("[TELNET] Error %s (%d) on client #%u\n"), c->errorToString(error), error, i);
            }, 0);

            _telnetClients[i]->onTimeout([i](void *s, AsyncClient *c, uint32_t time) {
                DEBUG_MSG_P(PSTR("[TELNET] Timeout on client #%u at %lu\n"), i, time);
                c->close();
            }, 0);

            _telnetNotifyConnected(i);
            return;
        }

    }

    DEBUG_MSG_P(PSTR("[TELNET] Rejecting - Too many connections\n"));
    client->onDisconnect([](void *s, AsyncClient *c) {
        delete c;
    });
    client->close(true);
}

#endif





bool telnetConnected() {
    for (unsigned char i = 0; i < TELNET_MAX_CLIENTS; i++) {
        if (_telnetClients[i] && _telnetClients[i]->connected()) return true;
    }
    return false;
}

unsigned char telnetWrite(unsigned char ch) {
    char data[1] = {ch};
    return _telnetWrite(data, 1);
}

void _telnetConfigure() {
    _telnetAuth = getSetting("telnetAuth", TELNET_AUTHENTICATION).toInt() == 1;
}

void telnetSetup() {
    #if TELNET_SERVER == TELNET_SERVER_WIFISERVER
        espurnaRegisterLoop(_telnetLoop);
        _telnetServer.setNoDelay(true);
        _telnetServer.begin();
    #else
        _telnetServer.onClient([](void *s, AsyncClient* c) {
            _telnetNewClient(c);
        }, 0);
        _telnetServer.begin();
    #endif

    #if WEB_SUPPORT
        wsOnSendRegister(_telnetWebSocketOnSend);
        wsOnReceiveRegister(_telnetWebSocketOnReceive);
    #endif

    espurnaRegisterReload(_telnetConfigure);
    _telnetConfigure();

    DEBUG_MSG_P(PSTR("[TELNET] %s server, Listening on port %d\n"),
        (TELNET_SERVER == TELNET_SERVER_WIFISERVER) ? "Sync" : "Async",
        TELNET_PORT);

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/terminal.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/terminal.ino"
#if TERMINAL_SUPPORT

#include <vector>
#include "libs/EmbedisWrap.h"
#include <Stream.h>
#include "libs/StreamInjector.h"
#include "libs/HeapStats.h"

StreamInjector _serial = StreamInjector(TERMINAL_BUFFER_SIZE);
EmbedisWrap embedis(_serial, TERMINAL_BUFFER_SIZE);

#if SERIAL_RX_ENABLED
    char _serial_rx_buffer[TERMINAL_BUFFER_SIZE];
    static unsigned char _serial_rx_pointer = 0;
#endif





void _terminalHelpCommand() {


    std::vector<String> commands;
    unsigned char size = embedis.getCommandCount();
    for (unsigned int i=0; i<size; i++) {

        String command = embedis.getCommandName(i);
        bool inserted = false;
        for (unsigned char j=0; j<commands.size(); j++) {


            if (commands[j].compareTo(command) > 0) {
                commands.insert(commands.begin() + j, command);
                inserted = true;
                break;
            }

        }


        if (!inserted) commands.push_back(command);

    }


    DEBUG_MSG_P(PSTR("Available commands:\n"));
    for (unsigned char i=0; i<commands.size(); i++) {
        DEBUG_MSG_P(PSTR("> %s\n"), (commands[i]).c_str());
    }

}

void _terminalKeysCommand() {


    std::vector<String> keys = _settingsKeys();


    DEBUG_MSG_P(PSTR("Current settings:\n"));
    for (unsigned int i=0; i<keys.size(); i++) {
        String value = getSetting(keys[i]);
        DEBUG_MSG_P(PSTR("> %s => \"%s\"\n"), (keys[i]).c_str(), value.c_str());
    }

    unsigned long freeEEPROM = SPI_FLASH_SEC_SIZE - settingsSize();
    UNUSED(freeEEPROM);
    DEBUG_MSG_P(PSTR("Number of keys: %d\n"), keys.size());
    DEBUG_MSG_P(PSTR("Current EEPROM sector: %u\n"), EEPROMr.current());
    DEBUG_MSG_P(PSTR("Free EEPROM: %d bytes (%d%%)\n"), freeEEPROM, 100 * freeEEPROM / SPI_FLASH_SEC_SIZE);

}

void _terminalInitCommand() {

    terminalRegisterCommand(F("COMMANDS"), [](Embedis* e) {
        _terminalHelpCommand();
        terminalOK();
    });

    terminalRegisterCommand(F("ERASE.CONFIG"), [](Embedis* e) {
        terminalOK();
        customResetReason(CUSTOM_RESET_TERMINAL);
        eraseSDKConfig();
        *((int*) 0) = 0;
    });

    terminalRegisterCommand(F("FACTORY.RESET"), [](Embedis* e) {
        resetSettings();
        terminalOK();
    });

    terminalRegisterCommand(F("GPIO"), [](Embedis* e) {
        int pin = -1;

        if (e->argc < 2) {
            DEBUG_MSG("Printing all GPIO pins:\n");
        } else {
            pin = String(e->argv[1]).toInt();
            if (!gpioValid(pin)) {
                terminalError(F("Invalid GPIO pin"));
                return;
            }

            if (e->argc > 2) {
                bool state = String(e->argv[2]).toInt() == 1;
                digitalWrite(pin, state);
            }
        }

        for (int i = 0; i <= 15; i++) {
            if (gpioValid(i) && (pin == -1 || pin == i)) {
                DEBUG_MSG_P(PSTR("GPIO %s pin %d is %s\n"), GPEP(i) ? "output" : "input", i, digitalRead(i) == HIGH ? "HIGH" : "LOW");
            }
        }

        terminalOK();
    });

    terminalRegisterCommand(F("HEAP"), [](Embedis* e) {
        infoHeapStats();
        terminalOK();
    });

    terminalRegisterCommand(F("STACK"), [](Embedis* e) {
        infoMemory("Stack", CONT_STACKSIZE, getFreeStack());
        terminalOK();
    });

    terminalRegisterCommand(F("HELP"), [](Embedis* e) {
        _terminalHelpCommand();
        terminalOK();
    });

    terminalRegisterCommand(F("INFO"), [](Embedis* e) {
        info();
        terminalOK();
    });

    terminalRegisterCommand(F("KEYS"), [](Embedis* e) {
        _terminalKeysCommand();
        terminalOK();
    });

    terminalRegisterCommand(F("GET"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("Wrong arguments"));
            return;
        }

        for (unsigned char i = 1; i < e->argc; i++) {
            String key = String(e->argv[i]);
            String value;
            if (!Embedis::get(key, value)) {
                DEBUG_MSG_P(PSTR("> %s =>\n"), key.c_str());
                continue;
            }

            DEBUG_MSG_P(PSTR("> %s => \"%s\"\n"), key.c_str(), value.c_str());
        }

        terminalOK();
    });

    terminalRegisterCommand(F("RELOAD"), [](Embedis* e) {
        espurnaReload();
        terminalOK();
    });

    terminalRegisterCommand(F("RESET"), [](Embedis* e) {
        terminalOK();
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    terminalRegisterCommand(F("RESET.SAFE"), [](Embedis* e) {
        systemStabilityCounter(SYSTEM_CHECK_MAX);
        terminalOK();
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    terminalRegisterCommand(F("UPTIME"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("Uptime: %d seconds\n"), getUptime());
        terminalOK();
    });

    terminalRegisterCommand(F("CONFIG"), [](Embedis* e) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        settingsGetJson(root);
        String output;
        root.printTo(output);
        DEBUG_MSG(output.c_str());

    });

    #if not SETTINGS_AUTOSAVE
        terminalRegisterCommand(F("SAVE"), [](Embedis* e) {
            eepromCommit();
            DEBUG_MSG_P(PSTR("\n+OK\n"));
        });
    #endif

}

void _terminalLoop() {

    #if DEBUG_SERIAL_SUPPORT
        while (DEBUG_PORT.available()) {
            _serial.inject(DEBUG_PORT.read());
        }
    #endif

    embedis.process();

    #if SERIAL_RX_ENABLED

        while (SERIAL_RX_PORT.available() > 0) {
            char rc = SERIAL_RX_PORT.read();
            _serial_rx_buffer[_serial_rx_pointer++] = rc;
            if ((_serial_rx_pointer == TERMINAL_BUFFER_SIZE) || (rc == 10)) {
                terminalInject(_serial_rx_buffer, (size_t) _serial_rx_pointer);
                _serial_rx_pointer = 0;
            }
        }

    #endif

}





void terminalInject(void *data, size_t len) {
    _serial.inject((char *) data, len);
}

Stream & terminalSerial() {
    return (Stream &) _serial;
}

void terminalRegisterCommand(const String& name, void (*call)(Embedis*)) {
    Embedis::command(name, call);
};

void terminalOK() {
    DEBUG_MSG_P(PSTR("+OK\n"));
}

void terminalError(const String& error) {
    DEBUG_MSG_P(PSTR("-ERROR: %s\n"), error.c_str());
}

void terminalSetup() {

    _serial.callback([](uint8_t ch) {
        #if TELNET_SUPPORT
            telnetWrite(ch);
        #endif
        #if DEBUG_SERIAL_SUPPORT
            DEBUG_PORT.write(ch);
        #endif
    });

    _terminalInitCommand();

    #if SERIAL_RX_ENABLED
        SERIAL_RX_PORT.begin(SERIAL_RX_BAUDRATE);
    #endif


    espurnaRegisterLoop(_terminalLoop);

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/thermostat.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/thermostat.ino"
#if THERMOSTAT_SUPPORT

#include <ArduinoJson.h>
#include <float.h>

bool _thermostat_enabled = true;
bool _thermostat_mode_cooler = false;

const char* NAME_THERMOSTAT_ENABLED = "thermostatEnabled";
const char* NAME_THERMOSTAT_MODE = "thermostatMode";
const char* NAME_TEMP_RANGE_MIN = "tempRangeMin";
const char* NAME_TEMP_RANGE_MAX = "tempRangeMax";
const char* NAME_REMOTE_SENSOR_NAME = "remoteSensorName";
const char* NAME_REMOTE_TEMP_MAX_WAIT = "remoteTempMaxWait";
const char* NAME_ALONE_ON_TIME = "aloneOnTime";
const char* NAME_ALONE_OFF_TIME = "aloneOffTime";
const char* NAME_MAX_ON_TIME = "maxOnTime";
const char* NAME_MIN_OFF_TIME = "minOffTime";
const char* NAME_BURN_TOTAL = "burnTotal";
const char* NAME_BURN_TODAY = "burnToday";
const char* NAME_BURN_YESTERDAY = "burnYesterday";
const char* NAME_BURN_THIS_MONTH = "burnThisMonth";
const char* NAME_BURN_PREV_MONTH = "burnPrevMonth";
const char* NAME_BURN_DAY = "burnDay";
const char* NAME_BURN_MONTH = "burnMonth";
const char* NAME_OPERATION_MODE = "thermostatOperationMode";

#define ASK_TEMP_RANGE_INTERVAL_INITIAL 15000
#define ASK_TEMP_RANGE_INTERVAL_REGULAR 60000
#define MILLIS_IN_SEC 1000
#define MILLIS_IN_MIN 60000
#define THERMOSTAT_STATE_UPDATE_INTERVAL 60000
#define THERMOSTAT_RELAY 0
#define THERMOSTAT_TEMP_RANGE_MIN 10
#define THERMOSTAT_TEMP_RANGE_MIN_MIN 3
#define THERMOSTAT_TEMP_RANGE_MIN_MAX 30
#define THERMOSTAT_TEMP_RANGE_MAX 20
#define THERMOSTAT_TEMP_RANGE_MAX_MIN 8
#define THERMOSTAT_TEMP_RANGE_MAX_MAX 35
#define THERMOSTAT_ALONE_ON_TIME 5
#define THERMOSTAT_ALONE_OFF_TIME 55
#define THERMOSTAT_MAX_ON_TIME 30
#define THERMOSTAT_MIN_OFF_TIME 10

unsigned long _thermostat_remote_temp_max_wait = THERMOSTAT_REMOTE_TEMP_MAX_WAIT * MILLIS_IN_SEC;
unsigned long _thermostat_alone_on_time = THERMOSTAT_ALONE_ON_TIME * MILLIS_IN_MIN;
unsigned long _thermostat_alone_off_time = THERMOSTAT_ALONE_OFF_TIME * MILLIS_IN_MIN;
unsigned long _thermostat_max_on_time = THERMOSTAT_MAX_ON_TIME * MILLIS_IN_MIN;
unsigned long _thermostat_min_off_time = THERMOSTAT_MIN_OFF_TIME * MILLIS_IN_MIN;
unsigned int _thermostat_on_time_for_day = 0;
unsigned int _thermostat_burn_total = 0;
unsigned int _thermostat_burn_today = 0;
unsigned int _thermostat_burn_yesterday = 0;
unsigned int _thermostat_burn_this_month = 0;
unsigned int _thermostat_burn_prev_month = 0;
unsigned int _thermostat_burn_day = 0;
unsigned int _thermostat_burn_month = 0;

struct temp_t {
  float temp;
  unsigned long last_update = 0;
  bool need_display_update = false;
};
temp_t _remote_temp;

struct temp_range_t {
  int min = THERMOSTAT_TEMP_RANGE_MIN;
  int max = THERMOSTAT_TEMP_RANGE_MAX;
  unsigned long last_update = 0;
  unsigned long ask_time = 0;
  unsigned int ask_interval = 0;
  bool need_display_update = true;
};
temp_range_t _temp_range;

enum temperature_source_t {temp_none, temp_local, temp_remote};
struct thermostat_t {
  unsigned long last_update = 0;
  unsigned long last_switch = 0;
  String remote_sensor_name;
  unsigned int temperature_source = temp_none;
};
thermostat_t _thermostat;

enum thermostat_cycle_type {cooling, heating};
unsigned int _thermostat_cycle = heating;
String thermostat_remote_sensor_topic;


void thermostatEnabled(bool enabled) {
    _thermostat_enabled = enabled;
}


bool thermostatEnabled() {
    return _thermostat_enabled;
}


void thermostatModeCooler(bool cooler) {
    _thermostat_mode_cooler = cooler;
}


bool thermostatModeCooler() {
    return _thermostat_mode_cooler;
}


std::vector<thermostat_callback_f> _thermostat_callbacks;

void thermostatRegister(thermostat_callback_f callback) {
    _thermostat_callbacks.push_back(callback);
}


void updateOperationMode() {
  #if WEB_SUPPORT
    String message;
    if (_thermostat.temperature_source == temp_remote) {
      message = "{\"thermostatVisible\": 1, \"thermostatOperationMode\": \"remote temperature\"}";
      updateRemoteTemp(true);
    } else if (_thermostat.temperature_source == temp_local) {
      message = "{\"thermostatVisible\": 1, \"thermostatOperationMode\": \"local temperature\"}";
      updateRemoteTemp(false);
    } else {
      message = "{\"thermostatVisible\": 1, \"thermostatOperationMode\": \"autonomous\"}";
      updateRemoteTemp(false);
    }
    wsSend(message.c_str());
  #endif
}


void updateRemoteTemp(bool remote_temp_actual) {
  #if WEB_SUPPORT
      char tmp_str[6];
      if (remote_temp_actual) {
        dtostrf(_remote_temp.temp, 1-sizeof(tmp_str), 1, tmp_str);
      } else {
        strcpy(tmp_str, "\"?\"");
      }
      char buffer[100];
      snprintf_P(buffer, sizeof(buffer), PSTR("{\"thermostatVisible\": 1, \"remoteTmp\": %s}"), tmp_str);
      wsSend(buffer);
  #endif
}




void thermostatMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
      mqttSubscribeRaw(thermostat_remote_sensor_topic.c_str());
      mqttSubscribe(MQTT_TOPIC_HOLD_TEMP);
      _temp_range.ask_interval = ASK_TEMP_RANGE_INTERVAL_INITIAL;
      _temp_range.ask_time = millis();
    }

    if (type == MQTT_MESSAGE_EVENT) {


        String t = mqttMagnitude((char *) topic);

        if (strcmp(topic, thermostat_remote_sensor_topic.c_str()) != 0
         && !t.equals(MQTT_TOPIC_HOLD_TEMP))
           return;


        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(payload);
        if (!root.success()) {
            DEBUG_MSG_P(PSTR("[THERMOSTAT] Error parsing data\n"));
            return;
        }


        if (strcmp(topic, thermostat_remote_sensor_topic.c_str()) == 0) {
            if (root.containsKey(magnitudeTopic(MAGNITUDE_TEMPERATURE))) {
                String remote_temp = root[magnitudeTopic(MAGNITUDE_TEMPERATURE)];
                _remote_temp.temp = remote_temp.toFloat();
                _remote_temp.last_update = millis();
                _remote_temp.need_display_update = true;
                DEBUG_MSG_P(PSTR("[THERMOSTAT] Remote sensor temperature: %s\n"), remote_temp.c_str());
                updateRemoteTemp(true);
            }
        }


        if (t.equals(MQTT_TOPIC_HOLD_TEMP)) {
            if (root.containsKey(MQTT_TOPIC_HOLD_TEMP_MIN)) {
                int t_min = root[MQTT_TOPIC_HOLD_TEMP_MIN];
                int t_max = root[MQTT_TOPIC_HOLD_TEMP_MAX];
                if (t_min < THERMOSTAT_TEMP_RANGE_MIN_MIN || t_min > THERMOSTAT_TEMP_RANGE_MIN_MAX ||
                    t_max < THERMOSTAT_TEMP_RANGE_MAX_MIN || t_max > THERMOSTAT_TEMP_RANGE_MAX_MAX) {
                    DEBUG_MSG_P(PSTR("[THERMOSTAT] Hold temperature range error\n"));
                    return;
                }
                _temp_range.min = root[MQTT_TOPIC_HOLD_TEMP_MIN];
                _temp_range.max = root[MQTT_TOPIC_HOLD_TEMP_MAX];
                setSetting(NAME_TEMP_RANGE_MIN, _temp_range.min);
                setSetting(NAME_TEMP_RANGE_MAX, _temp_range.max);
                saveSettings();
                _temp_range.ask_interval = ASK_TEMP_RANGE_INTERVAL_REGULAR;
                _temp_range.last_update = millis();
                _temp_range.need_display_update = true;

                DEBUG_MSG_P(PSTR("[THERMOSTAT] Hold temperature range: (%d - %d)\n"), _temp_range.min, _temp_range.max);

                #if WEB_SUPPORT
                    char buffer[100];
                    snprintf_P(buffer, sizeof(buffer), PSTR("{\"thermostatVisible\": 1, \"tempRangeMin\": %d, \"tempRangeMax\": %d}"), _temp_range.min, _temp_range.max);
                    wsSend(buffer);
                #endif
            } else {
                DEBUG_MSG_P(PSTR("[THERMOSTAT] Error temperature range data\n"));
            }
        }
    }
}

#if MQTT_SUPPORT

void thermostatSetupMQTT() {
    mqttRegister(thermostatMQTTCallback);
}
#endif


void notifyRangeChanged(bool min) {
  DEBUG_MSG_P(PSTR("[THERMOSTAT] notifyRangeChanged %s = %d\n"), min ? "MIN" : "MAX", min ? _temp_range.min : _temp_range.max);
  char tmp_str[6];
  sprintf(tmp_str, "%d", min ? _temp_range.min : _temp_range.max);

  mqttSend(min ? MQTT_TOPIC_NOTIFY_TEMP_RANGE_MIN : MQTT_TOPIC_NOTIFY_TEMP_RANGE_MAX, tmp_str, true);
}




void commonSetup() {
  _thermostat_enabled = getSetting(NAME_THERMOSTAT_ENABLED).toInt() == 1;
  DEBUG_MSG_P(PSTR("[THERMOSTAT] _thermostat_enabled = %d\n"), _thermostat_enabled);

  _thermostat_mode_cooler = getSetting(NAME_THERMOSTAT_MODE).toInt() == 1;
  DEBUG_MSG_P(PSTR("[THERMOSTAT] _thermostat_mode_cooler = %d\n"), _thermostat_mode_cooler);

  _temp_range.min = getSetting(NAME_TEMP_RANGE_MIN, THERMOSTAT_TEMP_RANGE_MIN).toInt();
  _temp_range.max = getSetting(NAME_TEMP_RANGE_MAX, THERMOSTAT_TEMP_RANGE_MAX).toInt();
  DEBUG_MSG_P(PSTR("[THERMOSTAT] _temp_range.min = %d\n"), _temp_range.min);
  DEBUG_MSG_P(PSTR("[THERMOSTAT] _temp_range.max = %d\n"), _temp_range.max);

  _thermostat.remote_sensor_name = getSetting(NAME_REMOTE_SENSOR_NAME);
  thermostat_remote_sensor_topic = _thermostat.remote_sensor_name + String("/") + String(MQTT_TOPIC_JSON);

  _thermostat_remote_temp_max_wait = getSetting(NAME_REMOTE_TEMP_MAX_WAIT, THERMOSTAT_REMOTE_TEMP_MAX_WAIT).toInt() * MILLIS_IN_SEC;
  _thermostat_alone_on_time = getSetting(NAME_ALONE_ON_TIME, THERMOSTAT_ALONE_ON_TIME).toInt() * MILLIS_IN_MIN;
  _thermostat_alone_off_time = getSetting(NAME_ALONE_OFF_TIME, THERMOSTAT_ALONE_OFF_TIME).toInt() * MILLIS_IN_MIN;
  _thermostat_max_on_time = getSetting(NAME_MAX_ON_TIME, THERMOSTAT_MAX_ON_TIME).toInt() * MILLIS_IN_MIN;
  _thermostat_min_off_time = getSetting(NAME_MIN_OFF_TIME, THERMOSTAT_MIN_OFF_TIME).toInt() * MILLIS_IN_MIN;
}


void thermostatConfigure() {
  commonSetup();

  _thermostat.temperature_source = temp_none;
  _thermostat_burn_total = getSetting(NAME_BURN_TOTAL).toInt();
  _thermostat_burn_today = getSetting(NAME_BURN_TODAY).toInt();
  _thermostat_burn_yesterday = getSetting(NAME_BURN_YESTERDAY).toInt();
  _thermostat_burn_this_month = getSetting(NAME_BURN_THIS_MONTH).toInt();
  _thermostat_burn_prev_month = getSetting(NAME_BURN_PREV_MONTH).toInt();
  _thermostat_burn_day = getSetting(NAME_BURN_DAY).toInt();
  _thermostat_burn_month = getSetting(NAME_BURN_MONTH).toInt();
}


void _thermostatReload() {
  int prev_temp_range_min = _temp_range.min;
  int prev_temp_range_max = _temp_range.max;

  commonSetup();

  if (_temp_range.min != prev_temp_range_min)
    notifyRangeChanged(true);
  if (_temp_range.max != prev_temp_range_max)
    notifyRangeChanged(false);
}

#if WEB_SUPPORT

void _thermostatWebSocketOnSend(JsonObject& root) {
  root["thermostatEnabled"] = thermostatEnabled();
  root["thermostatMode"] = thermostatModeCooler();
  root["thermostatVisible"] = 1;
  root[NAME_TEMP_RANGE_MIN] = _temp_range.min;
  root[NAME_TEMP_RANGE_MAX] = _temp_range.max;
  root[NAME_REMOTE_SENSOR_NAME] = _thermostat.remote_sensor_name;
  root[NAME_REMOTE_TEMP_MAX_WAIT] = _thermostat_remote_temp_max_wait / MILLIS_IN_SEC;
  root[NAME_MAX_ON_TIME] = _thermostat_max_on_time / MILLIS_IN_MIN;
  root[NAME_MIN_OFF_TIME] = _thermostat_min_off_time / MILLIS_IN_MIN;
  root[NAME_ALONE_ON_TIME] = _thermostat_alone_on_time / MILLIS_IN_MIN;
  root[NAME_ALONE_OFF_TIME] = _thermostat_alone_off_time / MILLIS_IN_MIN;
  root[NAME_BURN_TODAY] = _thermostat_burn_today;
  root[NAME_BURN_YESTERDAY] = _thermostat_burn_yesterday;
  root[NAME_BURN_THIS_MONTH] = _thermostat_burn_this_month;
  root[NAME_BURN_PREV_MONTH] = _thermostat_burn_prev_month;
  root[NAME_BURN_TOTAL] = _thermostat_burn_total;
  if (_thermostat.temperature_source == temp_remote) {
    root[NAME_OPERATION_MODE] = "remote temperature";
    root["remoteTmp"] = _remote_temp.temp;
  } else if (_thermostat.temperature_source == temp_local) {
    root[NAME_OPERATION_MODE] = "local temperature";
    root["remoteTmp"] = "?";
  } else {
    root[NAME_OPERATION_MODE] = "autonomous";
    root["remoteTmp"] = "?";
  }
}


bool _thermostatWebSocketOnReceive(const char * key, JsonVariant& value) {
    if (strncmp(key, NAME_THERMOSTAT_ENABLED, strlen(NAME_THERMOSTAT_ENABLED)) == 0) return true;
    if (strncmp(key, NAME_THERMOSTAT_MODE, strlen(NAME_THERMOSTAT_MODE)) == 0) return true;
    if (strncmp(key, NAME_TEMP_RANGE_MIN, strlen(NAME_TEMP_RANGE_MIN)) == 0) return true;
    if (strncmp(key, NAME_TEMP_RANGE_MAX, strlen(NAME_TEMP_RANGE_MAX)) == 0) return true;
    if (strncmp(key, NAME_REMOTE_SENSOR_NAME, strlen(NAME_REMOTE_SENSOR_NAME)) == 0) return true;
    if (strncmp(key, NAME_REMOTE_TEMP_MAX_WAIT, strlen(NAME_REMOTE_TEMP_MAX_WAIT)) == 0) return true;
    if (strncmp(key, NAME_MAX_ON_TIME, strlen(NAME_MAX_ON_TIME)) == 0) return true;
    if (strncmp(key, NAME_MIN_OFF_TIME, strlen(NAME_MIN_OFF_TIME)) == 0) return true;
    if (strncmp(key, NAME_ALONE_ON_TIME, strlen(NAME_ALONE_ON_TIME)) == 0) return true;
    if (strncmp(key, NAME_ALONE_OFF_TIME, strlen(NAME_ALONE_OFF_TIME)) == 0) return true;
    return false;
}


void _thermostatWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "thermostat_reset_counters") == 0) resetBurnCounters();
}
#endif


void thermostatSetup() {
  thermostatConfigure();

  #if MQTT_SUPPORT
    thermostatSetupMQTT();
  #endif


  #if WEB_SUPPORT
      wsOnSendRegister(_thermostatWebSocketOnSend);
      wsOnReceiveRegister(_thermostatWebSocketOnReceive);
      wsOnActionRegister(_thermostatWebSocketOnAction);
  #endif

  espurnaRegisterLoop(thermostatLoop);
  espurnaRegisterReload(_thermostatReload);
}


void sendTempRangeRequest() {
  DEBUG_MSG_P(PSTR("[THERMOSTAT] sendTempRangeRequest\n"));
  mqttSend(MQTT_TOPIC_ASK_TEMP_RANGE, "", true);
}


void setThermostatState(bool state) {
  DEBUG_MSG_P(PSTR("[THERMOSTAT] setThermostatState: %s\n"), state ? "ON" : "OFF");
  relayStatus(THERMOSTAT_RELAY, state, mqttForward(), false);
  _thermostat.last_switch = millis();

  for (unsigned char i = 0; i < _thermostat_callbacks.size(); i++) {
      (_thermostat_callbacks[i])(state);
  }
}


void debugPrintSwitch(bool state, double temp) {
  char tmp_str[6];
  dtostrf(temp, 1-sizeof(tmp_str), 1, tmp_str);
  DEBUG_MSG_P(PSTR("[THERMOSTAT] switch %s, temp: %s, min: %d, max: %d, mode: %s, relay: %s, last switch %d\n"),
   state ? "ON" : "OFF", tmp_str, _temp_range.min, _temp_range.max, _thermostat_mode_cooler ? "COOLER" : "HEATER", relayStatus(THERMOSTAT_RELAY) ? "ON" : "OFF", millis() - _thermostat.last_switch);
}


inline bool lastSwitchEarlierThan(unsigned int comparing_time) {
  return millis() - _thermostat.last_switch > comparing_time;
}


inline void switchThermostat(bool state, double temp) {
    debugPrintSwitch(state, temp);
    setThermostatState(state);
}




void checkTempAndAdjustRelay(double temp) {
  if (_thermostat_mode_cooler == false) {

    if (relayStatus(THERMOSTAT_RELAY) && temp > _temp_range.max) {
      _thermostat_cycle = cooling;
      switchThermostat(false, temp);

    } else if (relayStatus(THERMOSTAT_RELAY) && lastSwitchEarlierThan(_thermostat_max_on_time)) {
      switchThermostat(false, temp);

    } else if (!relayStatus(THERMOSTAT_RELAY) && temp < _temp_range.min
        && (_thermostat.last_switch == 0 || lastSwitchEarlierThan(_thermostat_min_off_time))) {
      _thermostat_cycle = heating;
      switchThermostat(true, temp);


    } else if (!relayStatus(THERMOSTAT_RELAY) && _thermostat_cycle == heating
        && lastSwitchEarlierThan(_thermostat_min_off_time)) {
      switchThermostat(true, temp);
    }
  } else {

    if (relayStatus(THERMOSTAT_RELAY) && temp < _temp_range.min) {
      _thermostat_cycle = heating;
      switchThermostat(false, temp);

    } else if (relayStatus(THERMOSTAT_RELAY) && lastSwitchEarlierThan(_thermostat_max_on_time)) {
      switchThermostat(false, temp);

    } else if (!relayStatus(THERMOSTAT_RELAY) && temp > _temp_range.max
        && (_thermostat.last_switch == 0 || lastSwitchEarlierThan(_thermostat_min_off_time))) {
      _thermostat_cycle = cooling;
      switchThermostat(true, temp);


    } else if (!relayStatus(THERMOSTAT_RELAY) && _thermostat_cycle == cooling
        && lastSwitchEarlierThan(_thermostat_min_off_time)) {
      switchThermostat(true, temp);
    }
  }
}


void updateCounters() {
  if (relayStatus(THERMOSTAT_RELAY)) {
    setSetting(NAME_BURN_TOTAL, ++_thermostat_burn_total);
    setSetting(NAME_BURN_TODAY, ++_thermostat_burn_today);
    setSetting(NAME_BURN_THIS_MONTH, ++_thermostat_burn_this_month);
  }

  if (ntpSynced()) {
    String value = NTP.getDateStr();
    unsigned int day = value.substring(0, 2).toInt();
    unsigned int month = value.substring(3, 5).toInt();
    if (day != _thermostat_burn_day) {
      _thermostat_burn_yesterday = _thermostat_burn_today;
      _thermostat_burn_today = 0;
      _thermostat_burn_day = day;
      setSetting(NAME_BURN_YESTERDAY, _thermostat_burn_yesterday);
      setSetting(NAME_BURN_TODAY, _thermostat_burn_today);
      setSetting(NAME_BURN_DAY, _thermostat_burn_day);
    }
    if (month != _thermostat_burn_month) {
      _thermostat_burn_prev_month = _thermostat_burn_this_month;
      _thermostat_burn_this_month = 0;
      _thermostat_burn_month = month;
      setSetting(NAME_BURN_PREV_MONTH, _thermostat_burn_prev_month);
      setSetting(NAME_BURN_THIS_MONTH, _thermostat_burn_this_month);
      setSetting(NAME_BURN_MONTH, _thermostat_burn_month);
    }
  }
}


double getLocalTemperature() {
  #if SENSOR_SUPPORT
      for (byte i=0; i<magnitudeCount(); i++) {
          if (magnitudeType(i) == MAGNITUDE_TEMPERATURE) {
              double temp = magnitudeValue(i);
              char tmp_str[6];
              dtostrf(temp, 1-sizeof(tmp_str), 1, tmp_str);
              DEBUG_MSG_P(PSTR("[THERMOSTAT] getLocalTemperature temp: %s\n"), tmp_str);
              return temp > -0.1 && temp < 0.1 ? DBL_MIN : temp;
          }
      }
  #endif
  return DBL_MIN;
}


double getLocalHumidity() {
  #if SENSOR_SUPPORT
      for (byte i=0; i<magnitudeCount(); i++) {
          if (magnitudeType(i) == MAGNITUDE_HUMIDITY) {
              double hum = magnitudeValue(i);
              char tmp_str[4];
              dtostrf(hum, 1-sizeof(tmp_str), 0, tmp_str);
              DEBUG_MSG_P(PSTR("[THERMOSTAT] getLocalHumidity hum: %s\%\n"), tmp_str);
              return hum > -0.1 && hum < 0.1 ? DBL_MIN : hum;
          }
      }
  #endif
  return DBL_MIN;
}




void thermostatLoop(void) {

  if (!thermostatEnabled())
    return;


  if (mqttConnected()) {
    if (millis() - _temp_range.ask_time > _temp_range.ask_interval) {
      _temp_range.ask_time = millis();
      sendTempRangeRequest();
    }
  }


  if (millis() - _thermostat.last_update > THERMOSTAT_STATE_UPDATE_INTERVAL) {
    _thermostat.last_update = millis();
    updateCounters();
    unsigned int last_temp_src = _thermostat.temperature_source;
    if (_remote_temp.last_update != 0 && millis() - _remote_temp.last_update < _thermostat_remote_temp_max_wait) {

      _thermostat.temperature_source = temp_remote;
      DEBUG_MSG_P(PSTR("[THERMOSTAT] setup thermostat by remote temperature\n"));
      checkTempAndAdjustRelay(_remote_temp.temp);
    } else if (getLocalTemperature() != DBL_MIN) {

      _thermostat.temperature_source = temp_local;
      DEBUG_MSG_P(PSTR("[THERMOSTAT] setup thermostat by local temperature\n"));
      checkTempAndAdjustRelay(getLocalTemperature());

    } else {

      _thermostat.temperature_source = temp_none;
      DEBUG_MSG_P(PSTR("[THERMOSTAT] setup thermostat by timeout\n"));
      if (relayStatus(THERMOSTAT_RELAY) && millis() - _thermostat.last_switch > _thermostat_alone_on_time) {
        setThermostatState(false);
      } else if (!relayStatus(THERMOSTAT_RELAY) && millis() - _thermostat.last_switch > _thermostat_alone_off_time) {
        setThermostatState(false);
      }
    }
    if (last_temp_src != _thermostat.temperature_source) {
      updateOperationMode();
    }
  }
}


String getBurnTimeStr(unsigned int burn_time) {
  char burnTimeStr[18] = { 0 };
  if (burn_time < 60) {
    sprintf(burnTimeStr, "%d мин.", burn_time);
  } else {
    sprintf(burnTimeStr, "%d ч. %d мин.", (int)floor(burn_time / 60), burn_time % 60);
  }
  return String(burnTimeStr);
}


void resetBurnCounters() {
  DEBUG_MSG_P(PSTR("[THERMOSTAT] resetBurnCounters\n"));
  setSetting(NAME_BURN_TOTAL, 0);
  setSetting(NAME_BURN_TODAY, 0);
  setSetting(NAME_BURN_YESTERDAY, 0);
  setSetting(NAME_BURN_THIS_MONTH, 0);
  setSetting(NAME_BURN_PREV_MONTH, 0);
  _thermostat_burn_total = 0;
  _thermostat_burn_today = 0;
  _thermostat_burn_yesterday = 0;
  _thermostat_burn_this_month = 0;
  _thermostat_burn_prev_month = 0;
}

#endif
# 598 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/thermostat.ino"
#if THERMOSTAT_DISPLAY_SUPPORT

#include "SSD1306.h"

#define wifi_on_width 16
#define wifi_on_height 16
const char wifi_on_bits[] PROGMEM = {
  0x00, 0x00, 0x0E, 0x00, 0x7E, 0x00, 0xFE, 0x01, 0xE0, 0x03, 0x80, 0x07,
  0x02, 0x0F, 0x1E, 0x1E, 0x3E, 0x1C, 0x78, 0x38, 0xE0, 0x38, 0xC0, 0x31,
  0xC6, 0x71, 0x8E, 0x71, 0x8E, 0x73, 0x00, 0x00, };

#define mqtt_width 16
#define mqtt_height 16
const char mqtt_bits[] PROGMEM = {
  0x00, 0x00, 0x00, 0x08, 0x00, 0x18, 0x00, 0x38, 0xEA, 0x7F, 0xEA, 0x7F,
  0x00, 0x38, 0x10, 0x18, 0x18, 0x08, 0x1C, 0x00, 0xFE, 0x57, 0xFE, 0x57,
  0x1C, 0x00, 0x18, 0x00, 0x10, 0x00, 0x00, 0x00, };

#define remote_temp_width 16
#define remote_temp_height 16
const char remote_temp_bits[] PROGMEM = {
  0x00, 0x00, 0xE0, 0x18, 0x10, 0x25, 0x10, 0x25, 0x90, 0x19, 0x50, 0x01,
  0x50, 0x01, 0xD0, 0x01, 0x50, 0x01, 0x50, 0x01, 0xD0, 0x01, 0x50, 0x01,
  0xE0, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0x00, 0x00, };

#define server_width 16
#define server_height 16
const char server_bits[] PROGMEM = {
  0x00, 0x00, 0xF8, 0x1F, 0xFC, 0x3F, 0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30,
  0x0C, 0x30, 0x0C, 0x30, 0x0C, 0x30, 0xF8, 0x1F, 0xFC, 0x3F, 0xFE, 0x7F,
  0x1E, 0x78, 0xFE, 0x7F, 0xFC, 0x3F, 0x00, 0x00, };

#define LOCAL_TEMP_UPDATE_INTERVAL 60000
#define LOCAL_HUM_UPDATE_INTERVAL 61000

SSD1306 display(0x3c, 1, 3);

unsigned long _local_temp_last_update = 0xFFFF;
unsigned long _local_hum_last_update = 0xFFFF;
bool _display_wifi_status = true;
bool _display_mqtt_status = true;
bool _display_server_status = true;
bool _display_remote_temp_status = true;
bool _display_need_refresh = false;
bool _temp_range_need_update = true;

void drawIco(int16_t x, int16_t y, const char *ico, bool on = true) {
  display.drawIco16x16(x, y, ico, !on);
  _display_need_refresh = true;
}


void display_wifi_status(bool on) {
  _display_wifi_status = on;
  drawIco(0, 0, wifi_on_bits, on);
}


void display_mqtt_status(bool on) {
  _display_mqtt_status = on;
  drawIco(17, 0, mqtt_bits, on);
}


void display_server_status(bool on) {
  _display_server_status = on;
  drawIco(34, 0, server_bits, on);
}


void display_remote_temp_status(bool on) {
  _display_remote_temp_status = on;
  drawIco(51, 0, remote_temp_bits, on);
}


void display_temp_range() {
  _temp_range.need_display_update = false;
  display.setColor(BLACK);
  display.fillRect(68, 0, 60, 16);
  display.setColor(WHITE);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.setFont(ArialMT_Plain_16);
  String temp_range = String(_temp_range.min) + "°- " + String(_temp_range.max) + "°";
  display.drawString(128, 0, temp_range);
  _display_need_refresh = true;
}


void display_remote_temp() {
  _remote_temp.need_display_update = false;
  display.setColor(BLACK);
  display.fillRect(0, 16, 128, 16);
  display.setColor(WHITE);
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  String temp_range_title = String("Remote   t");
  display.drawString(0, 16, temp_range_title);

  String temp_range_vol = String("= ") + (_display_remote_temp_status ? String(_remote_temp.temp, 1) : String("?")) + "°";
  display.drawString(75, 16, temp_range_vol);

  _display_need_refresh = true;
}


void display_local_temp() {
  display.setColor(BLACK);
  display.fillRect(0, 32, 128, 16);
  display.setColor(WHITE);
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  String local_temp_title = String("Local    t");
  display.drawString(0, 32, local_temp_title);

  String local_temp_vol = String("= ") + (getLocalTemperature() != DBL_MIN ? String(getLocalTemperature(), 1) : String("?")) + "°";
  display.drawString(75, 32, local_temp_vol);

  _display_need_refresh = true;
}


void display_local_humidity() {
  display.setColor(BLACK);
  display.fillRect(0, 48, 128, 16);
  display.setColor(WHITE);
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  String local_hum_title = String("Local    h ");
  display.drawString(0, 48, local_hum_title);

  String local_hum_vol = String("= ") + (getLocalHumidity() != DBL_MIN ? String(getLocalHumidity(), 0) : String("?")) + "%";
  display.drawString(75, 48, local_hum_vol);

  _display_need_refresh = true;
}



void displaySetup() {
  display.init();
  display.flipScreenVertically();





    espurnaRegisterLoop(displayLoop);
}


void displayLoop() {
  _display_need_refresh = false;




  if (!_display_wifi_status) {
    if (wifiConnected() && WiFi.getMode() != WIFI_AP)
      display_wifi_status(true);
  } else if (!wifiConnected() || WiFi.getMode() == WIFI_AP) {
    display_wifi_status(false);
  }

  if (!_display_mqtt_status) {
    if (mqttConnected())
      display_mqtt_status(true);
  } else if (!mqttConnected()) {
    display_mqtt_status(false);
  }

  if (millis() - _temp_range.last_update < THERMOSTAT_SERVER_LOST_INTERVAL) {
    if (!_display_server_status)
      display_server_status(true);
  } else if (_display_server_status) {
    display_server_status(false);
  }

  if (millis() - _remote_temp.last_update < _thermostat_remote_temp_max_wait) {
    if (!_display_remote_temp_status)
      display_remote_temp_status(true);
  } else if (_display_remote_temp_status) {
    display_remote_temp_status(false);
    display_remote_temp();
  }




  if (_temp_range.need_display_update) {
      display_temp_range();
  }




  if (_remote_temp.need_display_update) {
      display_remote_temp();
  }




  if (millis() - _local_temp_last_update > LOCAL_TEMP_UPDATE_INTERVAL) {
      _local_temp_last_update = millis();
      display_local_temp();
  }




  if (millis() - _local_hum_last_update > LOCAL_HUM_UPDATE_INTERVAL) {
      _local_hum_last_update = millis();
      display_local_humidity();
  }




  if (_display_need_refresh) {
    yield();
    display.display();
  }
}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/thinkspeak.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/thinkspeak.ino"
#if THINGSPEAK_SUPPORT

#if THINGSPEAK_USE_ASYNC
#include <ESPAsyncTCP.h>
#else
#include <ESP8266WiFi.h>
#endif

#define THINGSPEAK_DATA_BUFFER_SIZE 256

const char THINGSPEAK_REQUEST_TEMPLATE[] PROGMEM =
    "POST %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "User-Agent: ESPurna\r\n"
    "Connection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: %d\r\n\r\n";

bool _tspk_enabled = false;
bool _tspk_clear = false;

char * _tspk_queue[THINGSPEAK_FIELDS] = {NULL};
String _tspk_data;

bool _tspk_flush = false;
unsigned long _tspk_last_flush = 0;
unsigned char _tspk_tries = THINGSPEAK_TRIES;

#if THINGSPEAK_USE_ASYNC
AsyncClient * _tspk_client;
bool _tspk_connecting = false;
bool _tspk_connected = false;
#endif



#if BROKER_SUPPORT
void _tspkBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload) {


    if (BROKER_MSG_TYPE_STATUS == type) {
        tspkEnqueueRelay(id, (char *) payload);
        tspkFlush();
    }


    if (BROKER_MSG_TYPE_SENSOR == type) {


    }

}
#endif


#if WEB_SUPPORT

bool _tspkWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "tspk", 4) == 0);
}

void _tspkWebSocketOnSend(JsonObject& root) {

    unsigned char visible = 0;

    root["tspkEnabled"] = getSetting("tspkEnabled", THINGSPEAK_ENABLED).toInt() == 1;
    root["tspkKey"] = getSetting("tspkKey");
    root["tspkClear"] = getSetting("tspkClear", THINGSPEAK_CLEAR_CACHE).toInt() == 1;

    JsonArray& relays = root.createNestedArray("tspkRelays");
    for (byte i=0; i<relayCount(); i++) {
        relays.add(getSetting("tspkRelay", i, 0).toInt());
    }
    if (relayCount() > 0) visible = 1;

    #if SENSOR_SUPPORT
        _sensorWebSocketMagnitudes(root, "tspk");
        visible = visible || (magnitudeCount() > 0);
    #endif

    root["tspkVisible"] = visible;

}

#endif

void _tspkConfigure() {
    _tspk_clear = getSetting("tspkClear", THINGSPEAK_CLEAR_CACHE).toInt() == 1;
    _tspk_enabled = getSetting("tspkEnabled", THINGSPEAK_ENABLED).toInt() == 1;
    if (_tspk_enabled && (getSetting("tspkKey").length() == 0)) {
        _tspk_enabled = false;
        setSetting("tspkEnabled", 0);
    }
    if (_tspk_enabled && !_tspk_client) _tspkInitClient();
}

#if THINGSPEAK_USE_ASYNC

enum class tspk_state_t : uint8_t {
    NONE,
    HEADERS,
    BODY
};

tspk_state_t _tspk_client_state = tspk_state_t::NONE;
unsigned long _tspk_client_ts = 0;
constexpr const unsigned long THINGSPEAK_CLIENT_TIMEOUT = 5000;

void _tspkInitClient() {

    _tspk_client = new AsyncClient();

    _tspk_client->onDisconnect([](void * s, AsyncClient * client) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Disconnected\n"));
        _tspk_data = "";
        _tspk_client_ts = 0;
        _tspk_last_flush = millis();
        _tspk_connected = false;
        _tspk_connecting = false;
        _tspk_client_state = tspk_state_t::NONE;
    }, nullptr);

    _tspk_client->onTimeout([](void * s, AsyncClient * client, uint32_t time) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Network timeout after %ums\n"), time);
        client->close(true);
    }, nullptr);

    _tspk_client->onPoll([](void * s, AsyncClient * client) {
        uint32_t ts = millis() - _tspk_client_ts;
        if (ts > THINGSPEAK_CLIENT_TIMEOUT) {
            DEBUG_MSG_P(PSTR("[THINGSPEAK] No response after %ums\n"), ts);
            client->close(true);
        }
    }, nullptr);

    _tspk_client->onData([](void * arg, AsyncClient * client, void * response, size_t len) {

        char * p = nullptr;

        do {

            p = nullptr;

            switch (_tspk_client_state) {
                case tspk_state_t::NONE:
                {
                    p = strnstr(reinterpret_cast<const char *>(response), "HTTP/1.1 200 OK", len);
                    if (!p) {
                        client->close(true);
                        return;
                    }
                    _tspk_client_state = tspk_state_t::HEADERS;
                    continue;
                }
                case tspk_state_t::HEADERS:
                {
                    p = strnstr(reinterpret_cast<const char *>(response), "\r\n\r\n", len);
                    if (!p) return;
                    _tspk_client_state = tspk_state_t::BODY;
                }
                case tspk_state_t::BODY:
                {
                    if (!p) {
                        p = strnstr(reinterpret_cast<const char *>(response), "\r\n\r\n", len);
                        if (!p) return;
                    }

                    unsigned int code = (p) ? atoi(&p[4]) : 0;
                    DEBUG_MSG_P(PSTR("[THINGSPEAK] Response value: %u\n"), code);

                    if ((0 == code) && _tspk_tries) {
                        _tspk_flush = true;
                        DEBUG_MSG_P(PSTR("[THINGSPEAK] Re-enqueuing %u more time(s)\n"), _tspk_tries);
                    } else {
                        _tspkClearQueue();
                    }

                    client->close(true);

                    _tspk_client_state = tspk_state_t::NONE;
                }
            }

        } while (_tspk_client_state != tspk_state_t::NONE);

    }, nullptr);

    _tspk_client->onConnect([](void * arg, AsyncClient * client) {

        _tspk_connected = true;
        _tspk_connecting = false;

        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connected to %s:%u\n"), THINGSPEAK_HOST, THINGSPEAK_PORT);

        #if THINGSPEAK_USE_SSL
            uint8_t fp[20] = {0};
            sslFingerPrintArray(THINGSPEAK_FINGERPRINT, fp);
            SSL * ssl = _tspk_client->getSSL();
            if (ssl_match_fingerprint(ssl, fp) != SSL_OK) {
                DEBUG_MSG_P(PSTR("[THINGSPEAK] Warning: certificate doesn't match\n"));
            }
        #endif

        DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"), THINGSPEAK_URL, _tspk_data.c_str());
        char headers[strlen_P(THINGSPEAK_REQUEST_TEMPLATE) + strlen(THINGSPEAK_URL) + strlen(THINGSPEAK_HOST) + 1];
        snprintf_P(headers, sizeof(headers),
            THINGSPEAK_REQUEST_TEMPLATE,
            THINGSPEAK_URL,
            THINGSPEAK_HOST,
            _tspk_data.length()
        );

        client->write(headers);
        client->write(_tspk_data.c_str());

    }, nullptr);

}

void _tspkPost() {

    if (_tspk_connected || _tspk_connecting) return;

    _tspk_client_ts = millis();

    #if ASYNC_TCP_SSL_ENABLED
        bool connected = _tspk_client->connect(THINGSPEAK_HOST, THINGSPEAK_PORT, THINGSPEAK_USE_SSL);
    #else
        bool connected = _tspk_client->connect(THINGSPEAK_HOST, THINGSPEAK_PORT);
    #endif

    _tspk_connecting = connected;

    if (!connected) {
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));
        _tspk_client->close(true);
    }

}

#else

void _tspkPost() {

    #if THINGSPEAK_USE_SSL
        WiFiClientSecure _tspk_client;
    #else
        WiFiClient _tspk_client;
    #endif

    if (_tspk_client.connect(THINGSPEAK_HOST, THINGSPEAK_PORT)) {

        DEBUG_MSG_P(PSTR("[THINGSPEAK] Connected to %s:%u\n"), THINGSPEAK_HOST, THINGSPEAK_PORT);

        if (!_tspk_client.verify(THINGSPEAK_FINGERPRINT, THINGSPEAK_HOST)) {
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Warning: certificate doesn't match\n"));
        }

        DEBUG_MSG_P(PSTR("[THINGSPEAK] POST %s?%s\n"), THINGSPEAK_URL, _tspk_data.c_str());
        char headers[strlen_P(THINGSPEAK_REQUEST_TEMPLATE) + strlen(THINGSPEAK_URL) + strlen(THINGSPEAK_HOST) + 1];
        snprintf_P(headers, sizeof(headers),
            THINGSPEAK_REQUEST_TEMPLATE,
            THINGSPEAK_URL,
            THINGSPEAK_HOST,
            _tspk_data.length()
        );

        _tspk_client.print(headers);
        _tspk_client.print(_tspk_data);

        nice_delay(100);

        String response = _tspk_client.readString();
        int pos = response.indexOf("\r\n\r\n");
        unsigned int code = (pos > 0) ? response.substring(pos + 4).toInt() : 0;
        DEBUG_MSG_P(PSTR("[THINGSPEAK] Response value: %u\n"), code);
        _tspk_client.stop();

        _tspk_last_flush = millis();
        if ((0 == code) && _tspk_tries) {
            _tspk_flush = true;
            DEBUG_MSG_P(PSTR("[THINGSPEAK] Re-enqueuing %u more time(s)\n"), _tspk_tries);
        } else {
            _tspkClearQueue();
        }

        return;

    }

    DEBUG_MSG_P(PSTR("[THINGSPEAK] Connection failed\n"));

}

#endif

void _tspkEnqueue(unsigned char index, char * payload) {
    DEBUG_MSG_P(PSTR("[THINGSPEAK] Enqueuing field #%u with value %s\n"), index, payload);
    --index;
    if (_tspk_queue[index] != NULL) free(_tspk_queue[index]);
    _tspk_queue[index] = strdup(payload);
}

void _tspkClearQueue() {
    _tspk_tries = THINGSPEAK_TRIES;
    if (_tspk_clear) {
        for (unsigned char id=0; id<THINGSPEAK_FIELDS; id++) {
            if (_tspk_queue[id] != NULL) {
                free(_tspk_queue[id]);
                _tspk_queue[id] = NULL;
            }
        }
    }
}

void _tspkFlush() {

    if (!_tspk_flush) return;
    if (millis() - _tspk_last_flush < THINGSPEAK_MIN_INTERVAL) return;
    if (_tspk_connected || _tspk_connecting) return;

    _tspk_last_flush = millis();
    _tspk_flush = false;
    _tspk_data.reserve(THINGSPEAK_DATA_BUFFER_SIZE);


    for (unsigned char id=0; id<THINGSPEAK_FIELDS; id++) {
        if (_tspk_queue[id] != NULL) {
            if (_tspk_data.length() > 0) _tspk_data.concat("&");
            char buf[32] = {0};
            snprintf_P(buf, sizeof(buf), PSTR("field%u=%s"), (id + 1), _tspk_queue[id]);
            _tspk_data.concat(buf);
        }
    }


    if (_tspk_data.length()) {
        _tspk_data.concat("&api_key=");
        _tspk_data.concat(getSetting("tspkKey"));
        --_tspk_tries;
        _tspkPost();
    }

}



bool tspkEnqueueRelay(unsigned char index, char * payload) {
    if (!_tspk_enabled) return true;
    unsigned char id = getSetting("tspkRelay", index, 0).toInt();
    if (id > 0) {
        _tspkEnqueue(id, payload);
        return true;
    }
    return false;
}

bool tspkEnqueueMeasurement(unsigned char index, char * payload) {
    if (!_tspk_enabled) return true;
    unsigned char id = getSetting("tspkMagnitude", index, 0).toInt();
    if (id > 0) {
        _tspkEnqueue(id, payload);
        return true;
    }
    return false;
}

void tspkFlush() {
    _tspk_flush = true;
}

bool tspkEnabled() {
    return _tspk_enabled;
}

void tspkSetup() {

    _tspkConfigure();

    #if WEB_SUPPORT
        wsOnSendRegister(_tspkWebSocketOnSend);
        wsOnReceiveRegister(_tspkWebSocketOnReceive);
    #endif

    #if BROKER_SUPPORT
        brokerRegister(_tspkBrokerCallback);
    #endif

    DEBUG_MSG_P(PSTR("[THINGSPEAK] Async %s, SSL %s\n"),
        THINGSPEAK_USE_ASYNC ? "ENABLED" : "DISABLED",
        THINGSPEAK_USE_SSL ? "ENABLED" : "DISABLED"
    );


    espurnaRegisterLoop(tspkLoop);
    espurnaRegisterReload(_tspkConfigure);

}

void tspkLoop() {
    if (!_tspk_enabled) return;
    if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return;
    _tspkFlush();
}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/uartmqtt.ino"
# 10 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/uartmqtt.ino"
#if UART_MQTT_SUPPORT

char _uartmqttBuffer[UART_MQTT_BUFFER_SIZE];
bool _uartmqttNewData = false;

#if UART_MQTT_USE_SOFT
    #include <SoftwareSerial.h>
    SoftwareSerial _uart_mqtt_serial(UART_MQTT_RX_PIN, UART_MQTT_TX_PIN, false, UART_MQTT_BUFFER_SIZE);
    #define UART_MQTT_PORT _uart_mqtt_serial
#else
    #define UART_MQTT_PORT UART_MQTT_HW_PORT
#endif





void _uartmqttReceiveUART() {

    static unsigned char ndx = 0;

    while (UART_MQTT_PORT.available() > 0 && _uartmqttNewData == false) {

        char rc = UART_MQTT_PORT.read();

        if (rc != UART_MQTT_TERMINATION) {

            _uartmqttBuffer[ndx] = rc;
            if (ndx < UART_MQTT_BUFFER_SIZE - 1) ndx++;

        } else {

            _uartmqttBuffer[ndx] = '\0';
            _uartmqttNewData = true;
            ndx = 0;

        }

    }

}

void _uartmqttSendMQTT() {
    if (_uartmqttNewData == true && MQTT_SUPPORT) {
        DEBUG_MSG_P(PSTR("[UART_MQTT] Send data over MQTT: %s\n"), _uartmqttBuffer);
        mqttSend(MQTT_TOPIC_UARTIN, _uartmqttBuffer);
        _uartmqttNewData = false;
    }
}

void _uartmqttSendUART(const char * message) {
    DEBUG_MSG_P(PSTR("[UART_MQTT] Send data over UART: %s\n"), message);
    UART_MQTT_PORT.println(message);
}

void _uartmqttMQTTCallback(unsigned int type, const char * topic, const char * payload) {

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





void _uartmqttLoop() {
    _uartmqttReceiveUART();
    _uartmqttSendMQTT();
}

void uartmqttSetup() {


    UART_MQTT_PORT.begin(UART_MQTT_BAUDRATE);


    mqttRegister(_uartmqttMQTTCallback);


    espurnaRegisterLoop(_uartmqttLoop);

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/utils.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/utils.ino"
#include <Ticker.h>
#include "libs/HeapStats.h"

String getIdentifier() {
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s-%06X"), APP_NAME, ESP.getChipId());
    return String(buffer);
}

void setDefaultHostname() {
    if (strlen(HOSTNAME) > 0) {
        setSetting("hostname", HOSTNAME);
    } else {
        setSetting("hostname", getIdentifier());
    }
}

void setBoardName() {
    #ifndef ESPURNA_CORE
        setSetting("boardName", DEVICE_NAME);
    #endif
}

String getBoardName() {
    return getSetting("boardName", DEVICE_NAME);
}

String getAdminPass() {
    return getSetting("adminPass", ADMIN_PASS);
}

String getCoreVersion() {
    String version = ESP.getCoreVersion();
    #ifdef ARDUINO_ESP8266_RELEASE
        if (version.equals("00000000")) {
            version = String(ARDUINO_ESP8266_RELEASE);
        }
    #endif
    version.replace("_", ".");
    return version;
}

String getCoreRevision() {
    #ifdef ARDUINO_ESP8266_GIT_VER
        return String(ARDUINO_ESP8266_GIT_VER, 16);
    #else
        return String("");
    #endif
}

unsigned char getHeartbeatMode() {
    return getSetting("hbMode", HEARTBEAT_MODE).toInt();
}

unsigned char getHeartbeatInterval() {
    return getSetting("hbInterval", HEARTBEAT_INTERVAL).toInt();
}

String getEspurnaModules() {
    return FPSTR(espurna_modules);
}

#if SENSOR_SUPPORT
String getEspurnaSensors() {
    return FPSTR(espurna_sensors);
}
#endif

String getEspurnaWebUI() {
    return FPSTR(espurna_webui);
}

String buildTime() {
    #if NTP_SUPPORT
        return ntpDateTime(__UNIX_TIMESTAMP__);
    #else
        char buffer[20];
        snprintf_P(
            buffer, sizeof(buffer), PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
            __TIME_YEAR__, __TIME_MONTH__, __TIME_DAY__,
            __TIME_HOUR__, __TIME_MINUTE__, __TIME_SECOND__
        );
        return String(buffer);
    #endif
}

unsigned long getUptime() {

    static unsigned long last_uptime = 0;
    static unsigned char uptime_overflows = 0;

    if (millis() < last_uptime) ++uptime_overflows;
    last_uptime = millis();
    unsigned long uptime_seconds = uptime_overflows * (UPTIME_OVERFLOW / 1000) + (last_uptime / 1000);

    return uptime_seconds;

}




namespace Heartbeat {
    enum Report : uint32_t {
        Status = 1 << 1,
        Ssid = 1 << 2,
        Ip = 1 << 3,
        Mac = 1 << 4,
        Rssi = 1 << 5,
        Uptime = 1 << 6,
        Datetime = 1 << 7,
        Freeheap = 1 << 8,
        Vcc = 1 << 9,
        Relay = 1 << 10,
        Light = 1 << 11,
        Hostname = 1 << 12,
        App = 1 << 13,
        Version = 1 << 14,
        Board = 1 << 15,
        Loadavg = 1 << 16,
        Interval = 1 << 17,
        Description = 1 << 18,
        Range = 1 << 19,
        Remote_temp = 1 << 20
    };

    constexpr uint32_t defaultValue() {
        return (Status * (HEARTBEAT_REPORT_STATUS)) | \
            (Ssid * (HEARTBEAT_REPORT_SSID)) | \
            (Ip * (HEARTBEAT_REPORT_IP)) | \
            (Mac * (HEARTBEAT_REPORT_MAC)) | \
            (Rssi * (HEARTBEAT_REPORT_RSSI)) | \
            (Uptime * (HEARTBEAT_REPORT_UPTIME)) | \
            (Datetime * (HEARTBEAT_REPORT_DATETIME)) | \
            (Freeheap * (HEARTBEAT_REPORT_FREEHEAP)) | \
            (Vcc * (HEARTBEAT_REPORT_VCC)) | \
            (Relay * (HEARTBEAT_REPORT_RELAY)) | \
            (Light * (HEARTBEAT_REPORT_LIGHT)) | \
            (Hostname * (HEARTBEAT_REPORT_HOSTNAME)) | \
            (Description * (HEARTBEAT_REPORT_DESCRIPTION)) | \
            (App * (HEARTBEAT_REPORT_APP)) | \
            (Version * (HEARTBEAT_REPORT_VERSION)) | \
            (Board * (HEARTBEAT_REPORT_BOARD)) | \
            (Loadavg * (HEARTBEAT_REPORT_LOADAVG)) | \
            (Interval * (HEARTBEAT_REPORT_INTERVAL)) | \
            (Range * (HEARTBEAT_REPORT_RANGE)) | \
            (Remote_temp * (HEARTBEAT_REPORT_REMOTE_TEMP));
    }

    uint32_t currentValue() {
        const String cfg = getSetting("hbReport");
        if (!cfg.length()) return defaultValue();

        return strtoul(cfg.c_str(), NULL, 10);
    }

}

void heartbeat() {

    unsigned long uptime_seconds = getUptime();
    heap_stats_t heap_stats = getHeapStats();

    UNUSED(uptime_seconds);
    UNUSED(heap_stats);

    #if MQTT_SUPPORT
        unsigned char _heartbeat_mode = getHeartbeatMode();
        bool serial = !mqttConnected();
    #else
        bool serial = true;
    #endif





    if (serial) {
        DEBUG_MSG_P(PSTR("[MAIN] Uptime: %lu seconds\n"), uptime_seconds);
        infoHeapStats();
        #if ADC_MODE_VALUE == ADC_VCC
            DEBUG_MSG_P(PSTR("[MAIN] Power: %lu mV\n"), ESP.getVcc());
        #endif
        #if NTP_SUPPORT
            if (ntpSynced()) DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), (char *) ntpDateTime().c_str());
        #endif
    }

    const uint32_t hb_cfg = Heartbeat::currentValue();
    if (!hb_cfg) return;





    #if MQTT_SUPPORT
        if (!serial && (_heartbeat_mode == HEARTBEAT_REPEAT || systemGetHeartbeat())) {
            if (hb_cfg & Heartbeat::Interval)
                mqttSend(MQTT_TOPIC_INTERVAL, String(getHeartbeatInterval() / 1000).c_str());

            if (hb_cfg & Heartbeat::App)
                mqttSend(MQTT_TOPIC_APP, APP_NAME);

            if (hb_cfg & Heartbeat::Version)
                mqttSend(MQTT_TOPIC_VERSION, APP_VERSION);

            if (hb_cfg & Heartbeat::Board)
                mqttSend(MQTT_TOPIC_BOARD, getBoardName().c_str());

            if (hb_cfg & Heartbeat::Hostname)
                mqttSend(MQTT_TOPIC_HOSTNAME, getSetting("hostname", getIdentifier()).c_str());

            if (hb_cfg & Heartbeat::Description) {
                if (hasSetting("desc")) {
                    mqttSend(MQTT_TOPIC_DESCRIPTION, getSetting("desc").c_str());
                }
            }

            if (hb_cfg & Heartbeat::Ssid)
                mqttSend(MQTT_TOPIC_SSID, WiFi.SSID().c_str());

            if (hb_cfg & Heartbeat::Ip)
                mqttSend(MQTT_TOPIC_IP, getIP().c_str());

            if (hb_cfg & Heartbeat::Mac)
                mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str());

            if (hb_cfg & Heartbeat::Rssi)
                mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());

            if (hb_cfg & Heartbeat::Uptime)
                mqttSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());

            #if NTP_SUPPORT
                if ((hb_cfg & Heartbeat::Datetime) && (ntpSynced()))
                    mqttSend(MQTT_TOPIC_DATETIME, ntpDateTime().c_str());
            #endif

            if (hb_cfg & Heartbeat::Freeheap)
                mqttSend(MQTT_TOPIC_FREEHEAP, String(heap_stats.available).c_str());

            if (hb_cfg & Heartbeat::Relay)
                relayMQTT();

            #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
                if (hb_cfg & Heartbeat::Light)
                    lightMQTT();
            #endif

            if ((hb_cfg & Heartbeat::Vcc) && (ADC_MODE_VALUE == ADC_VCC))
                mqttSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());

            if (hb_cfg & Heartbeat::Status)
                mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

            if (hb_cfg & Heartbeat::Loadavg)
                mqttSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str());

            #if THERMOSTAT_SUPPORT
                if (hb_cfg & Heartbeat::Range) {
                    mqttSend(MQTT_TOPIC_HOLD_TEMP "_" MQTT_TOPIC_HOLD_TEMP_MIN, String(_temp_range.min).c_str());
                    mqttSend(MQTT_TOPIC_HOLD_TEMP "_" MQTT_TOPIC_HOLD_TEMP_MAX, String(_temp_range.max).c_str());
                }

                if (hb_cfg & Heartbeat::Remote_temp) {
                    char remote_temp[6];
                    dtostrf(_remote_temp.temp, 1-sizeof(remote_temp), 1, remote_temp);
                    mqttSend(MQTT_TOPIC_REMOTE_TEMP, String(remote_temp).c_str());
                }
            #endif

        } else if (!serial && _heartbeat_mode == HEARTBEAT_REPEAT_STATUS) {
            mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);
        }

    #endif





    #if INFLUXDB_SUPPORT
        if (hb_cfg & Heartbeat::Uptime)
            idbSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());

        if (hb_cfg & Heartbeat::Freeheap)
            idbSend(MQTT_TOPIC_FREEHEAP, String(heap_stats.available).c_str());

        if (hb_cfg & Heartbeat::Rssi)
            idbSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());

        if ((hb_cfg & Heartbeat::Vcc) && (ADC_MODE_VALUE == ADC_VCC))
            idbSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());

        if (hb_cfg & Heartbeat::Loadavg)
            idbSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str());

        if (hb_cfg & Heartbeat::Ssid)
            idbSend(MQTT_TOPIC_SSID, WiFi.SSID().c_str());
    #endif

}





extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

unsigned int info_bytes2sectors(size_t size) {
    return (int) (size + SPI_FLASH_SEC_SIZE - 1) / SPI_FLASH_SEC_SIZE;
}

unsigned long info_ota_space() {
    return (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
}

unsigned long info_filesystem_space() {
    return ((uint32_t)&_SPIFFS_end - (uint32_t)&_SPIFFS_start);
}

unsigned long info_eeprom_space() {
    return EEPROMr.reserved() * SPI_FLASH_SEC_SIZE;
}

void _info_print_memory_layout_line(const char * name, unsigned long bytes, bool reset) {
    static unsigned long index = 0;
    if (reset) index = 0;
    if (0 == bytes) return;
    unsigned int _sectors = info_bytes2sectors(bytes);
    DEBUG_MSG_P(PSTR("[MAIN] %-20s: %8lu bytes / %4d sectors (%4d to %4d)\n"), name, bytes, _sectors, index, index + _sectors - 1);
    index += _sectors;
}

void _info_print_memory_layout_line(const char * name, unsigned long bytes) {
    _info_print_memory_layout_line(name, bytes, false);
}

void infoMemory(const char * name, unsigned int total_memory, unsigned int free_memory) {

    DEBUG_MSG_P(
        PSTR("[MAIN] %-6s: %5u bytes initially | %5u bytes used (%2u%%) | %5u bytes free (%2u%%)\n"),
        name,
        total_memory,
        total_memory - free_memory,
        100 * (total_memory - free_memory) / total_memory,
        free_memory,
        100 * free_memory / total_memory
    );

}

const char* _info_wifi_sleep_mode(WiFiSleepType_t type) {
    switch (type) {
        case WIFI_NONE_SLEEP: return "NONE";
        case WIFI_LIGHT_SLEEP: return "LIGHT";
        case WIFI_MODEM_SLEEP: return "MODEM";
        default: return "UNKNOWN";
    }
}


void info() {

    DEBUG_MSG_P(PSTR("\n\n---8<-------\n\n"));



    #if defined(APP_REVISION)
        DEBUG_MSG_P(PSTR("[MAIN] " APP_NAME " " APP_VERSION " (" APP_REVISION ")\n"));
    #else
        DEBUG_MSG_P(PSTR("[MAIN] " APP_NAME " " APP_VERSION "\n"));
    #endif
    DEBUG_MSG_P(PSTR("[MAIN] " APP_AUTHOR "\n"));
    DEBUG_MSG_P(PSTR("[MAIN] " APP_WEBSITE "\n\n"));
    DEBUG_MSG_P(PSTR("[MAIN] CPU chip ID: 0x%06X\n"), ESP.getChipId());
    DEBUG_MSG_P(PSTR("[MAIN] CPU frequency: %u MHz\n"), ESP.getCpuFreqMHz());
    DEBUG_MSG_P(PSTR("[MAIN] SDK version: %s\n"), ESP.getSdkVersion());
    DEBUG_MSG_P(PSTR("[MAIN] Core version: %s\n"), getCoreVersion().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] Core revision: %s\n"), getCoreRevision().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] Build time: %lu\n"), __UNIX_TIMESTAMP__);
    DEBUG_MSG_P(PSTR("\n"));



    FlashMode_t mode = ESP.getFlashChipMode();
    UNUSED(mode);
    DEBUG_MSG_P(PSTR("[MAIN] Flash chip ID: 0x%06X\n"), ESP.getFlashChipId());
    DEBUG_MSG_P(PSTR("[MAIN] Flash speed: %u Hz\n"), ESP.getFlashChipSpeed());
    DEBUG_MSG_P(PSTR("[MAIN] Flash mode: %s\n"), mode == FM_QIO ? "QIO" : mode == FM_QOUT ? "QOUT" : mode == FM_DIO ? "DIO" : mode == FM_DOUT ? "DOUT" : "UNKNOWN");
    DEBUG_MSG_P(PSTR("\n"));



    _info_print_memory_layout_line("Flash size (CHIP)", ESP.getFlashChipRealSize(), true);
    _info_print_memory_layout_line("Flash size (SDK)", ESP.getFlashChipSize(), true);
    _info_print_memory_layout_line("Reserved", 1 * SPI_FLASH_SEC_SIZE, true);
    _info_print_memory_layout_line("Firmware size", ESP.getSketchSize());
    _info_print_memory_layout_line("Max OTA size", info_ota_space());
    _info_print_memory_layout_line("SPIFFS size", info_filesystem_space());
    _info_print_memory_layout_line("EEPROM size", info_eeprom_space());
    _info_print_memory_layout_line("Reserved", 4 * SPI_FLASH_SEC_SIZE);
    DEBUG_MSG_P(PSTR("\n"));



    #if SPIFFS_SUPPORT
        FSInfo fs_info;
        bool fs = SPIFFS.info(fs_info);
        if (fs) {
            DEBUG_MSG_P(PSTR("[MAIN] SPIFFS total size   : %8u bytes / %4d sectors\n"), fs_info.totalBytes, info_bytes2sectors(fs_info.totalBytes));
            DEBUG_MSG_P(PSTR("[MAIN]        used size    : %8u bytes\n"), fs_info.usedBytes);
            DEBUG_MSG_P(PSTR("[MAIN]        block size   : %8u bytes\n"), fs_info.blockSize);
            DEBUG_MSG_P(PSTR("[MAIN]        page size    : %8u bytes\n"), fs_info.pageSize);
            DEBUG_MSG_P(PSTR("[MAIN]        max files    : %8u\n"), fs_info.maxOpenFiles);
            DEBUG_MSG_P(PSTR("[MAIN]        max length   : %8u\n"), fs_info.maxPathLength);
        } else {
            DEBUG_MSG_P(PSTR("[MAIN] No SPIFFS partition\n"));
        }
        DEBUG_MSG_P(PSTR("\n"));
    #endif



    eepromSectorsDebug();
    DEBUG_MSG_P(PSTR("\n"));



    static bool show_frag_stats = false;

    infoMemory("EEPROM", SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE - settingsSize());
    infoHeapStats(show_frag_stats);
    infoMemory("Stack", CONT_STACKSIZE, getFreeStack());
    DEBUG_MSG_P(PSTR("\n"));

    show_frag_stats = true;



    DEBUG_MSG_P(PSTR("[MAIN] Boot version: %d\n"), ESP.getBootVersion());
    DEBUG_MSG_P(PSTR("[MAIN] Boot mode: %d\n"), ESP.getBootMode());
    unsigned char reason = customResetReason();
    if (reason > 0) {
        char buffer[32];
        strcpy_P(buffer, custom_reset_string[reason-1]);
        DEBUG_MSG_P(PSTR("[MAIN] Last reset reason: %s\n"), buffer);
    } else {
        DEBUG_MSG_P(PSTR("[MAIN] Last reset reason: %s\n"), (char *) ESP.getResetReason().c_str());
        DEBUG_MSG_P(PSTR("[MAIN] Last reset info: %s\n"), (char *) ESP.getResetInfo().c_str());
    }
    DEBUG_MSG_P(PSTR("\n"));



    DEBUG_MSG_P(PSTR("[MAIN] Board: %s\n"), getBoardName().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] Support: %s\n"), getEspurnaModules().c_str());
    #if SENSOR_SUPPORT
        DEBUG_MSG_P(PSTR("[MAIN] Sensors: %s\n"), getEspurnaSensors().c_str());
    #endif
    DEBUG_MSG_P(PSTR("[MAIN] WebUI image: %s\n"), getEspurnaWebUI().c_str());
    DEBUG_MSG_P(PSTR("\n"));



    DEBUG_MSG_P(PSTR("[MAIN] Firmware MD5: %s\n"), (char *) ESP.getSketchMD5().c_str());
    #if ADC_MODE_VALUE == ADC_VCC
        DEBUG_MSG_P(PSTR("[MAIN] Power: %u mV\n"), ESP.getVcc());
    #endif
    if (espurnaLoopDelay()) {
        DEBUG_MSG_P(PSTR("[MAIN] Power saving delay value: %lu ms\n"), espurnaLoopDelay());
    }

    const WiFiSleepType_t sleep_mode = WiFi.getSleepMode();
    if (sleep_mode != WIFI_NONE_SLEEP) {
        DEBUG_MSG_P(PSTR("[MAIN] WiFi Sleep Mode: %s\n"), _info_wifi_sleep_mode(sleep_mode));
    }



    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) {
            DEBUG_MSG_P(PSTR("\n"));
            DEBUG_MSG_P(PSTR("[MAIN] Device is in SAFE MODE\n"));
        }
    #endif



    DEBUG_MSG_P(PSTR("\n\n---8<-------\n\n"));

}





#if ASYNC_TCP_SSL_ENABLED

bool sslCheckFingerPrint(const char * fingerprint) {
    return (strlen(fingerprint) == 59);
}

bool sslFingerPrintArray(const char * fingerprint, unsigned char * bytearray) {


    if (!sslCheckFingerPrint(fingerprint)) return false;


    for (unsigned int i=0; i<20; i++) {
        bytearray[i] = strtol(fingerprint + 3*i, NULL, 16);
    }

    return true;

}

bool sslFingerPrintChar(const char * fingerprint, char * destination) {


    if (!sslCheckFingerPrint(fingerprint)) return false;


    strncpy(destination, fingerprint, 59);


    for (unsigned char i = 0; i<59; i++) {
        if (destination[i] == ':') destination[i] = ' ';
    }

    return true;

}

#endif







bool eraseSDKConfig() {
    #if defined(ARDUINO_ESP8266_RELEASE_2_3_0)
        const size_t cfgsize = 0x4000;
        size_t cfgaddr = ESP.getFlashChipSize() - cfgsize;

        for (size_t offset = 0; offset < cfgsize; offset += SPI_FLASH_SEC_SIZE) {
            if (!ESP.flashEraseSector((cfgaddr + offset) / SPI_FLASH_SEC_SIZE)) {
                return false;
            }
        }

        return true;
    #else
        return ESP.eraseConfig();
    #endif
}





char * ltrim(char * s) {
    char *p = s;
    while ((unsigned char) *p == ' ') ++p;
    return p;
}

double roundTo(double num, unsigned char positions) {
    double multiplier = 1;
    while (positions-- > 0) multiplier *= 10;
    return round(num * multiplier) / multiplier;
}

void nice_delay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) delay(1);
}


int __get_adc_mode() {
    return (int) (ADC_MODE_VALUE);
}

bool isNumber(const char * s) {
    unsigned char len = strlen(s);
    if (0 == len) return false;
    bool decimal = false;
    bool digit = false;
    for (unsigned char i=0; i<len; i++) {
        if (('-' == s[i]) || ('+' == s[i])) {
            if (i>0) return false;
        } else if (s[i] == '.') {
            if (!digit) return false;
            if (decimal) return false;
            decimal = true;
        } else if (!isdigit(s[i])) {
            return false;
        } else {
            digit = true;
        }
    }
    return digit;
}


char* strnstr(const char* buffer, const char* token, size_t n) {
  size_t token_len = strnlen(token, n);
  if (token_len == 0) {
    return const_cast<char*>(buffer);
  }

  for (const char* p = buffer; *p && (p + token_len <= buffer + n); p++) {
    if ((*p == *token) && (strncmp(p, token, token_len) == 0)) {
      return const_cast<char*>(p);
    }
  }

  return nullptr;
}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/web.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/web.ino"
#if WEB_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

#if WEB_EMBEDDED

#if WEBUI_IMAGE == WEBUI_IMAGE_SMALL
    #include "static/index.small.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_LIGHT
    #include "static/index.light.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_SENSOR
    #include "static/index.sensor.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_RFBRIDGE
    #include "static/index.rfbridge.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_RFM69
    #include "static/index.rfm69.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_LIGHTFOX
    #include "static/index.lightfox.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_THERMOSTAT
    #include "static/index.thermostat.html.gz.h"
#elif WEBUI_IMAGE == WEBUI_IMAGE_FULL
    #include "static/index.all.html.gz.h"
#endif

#endif

#if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
#include "static/server.cer.h"
#include "static/server.key.h"
#endif



AsyncWebServer * _server;
char _last_modified[50];
std::vector<uint8_t> * _webConfigBuffer;
bool _webConfigSuccess = false;

std::vector<web_request_callback_f> _web_request_callbacks;
std::vector<web_body_callback_f> _web_body_callbacks;





void _onReset(AsyncWebServerRequest *request) {
    deferredReset(100, CUSTOM_RESET_HTTP);
    request->send(200);
}

void _onDiscover(AsyncWebServerRequest *request) {

    webLog(request);

    AsyncResponseStream *response = request->beginResponseStream("text/json");

    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["app"] = APP_NAME;
    root["version"] = APP_VERSION;
    root["hostname"] = getSetting("hostname");
    root["device"] = getBoardName();
    root.printTo(*response);

    request->send(response);

}

void _onGetConfig(AsyncWebServerRequest *request) {

    webLog(request);
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }

    AsyncResponseStream *response = request->beginResponseStream("text/json");

    char buffer[100];
    snprintf_P(buffer, sizeof(buffer), PSTR("attachment; filename=\"%s-backup.json\""), (char *) getSetting("hostname").c_str());
    response->addHeader("Content-Disposition", buffer);
    response->addHeader("X-XSS-Protection", "1; mode=block");
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("X-Frame-Options", "deny");

    response->printf("{\n\"app\": \"%s\"", APP_NAME);
    response->printf(",\n\"version\": \"%s\"", APP_VERSION);
    response->printf(",\n\"backup\": \"1\"");
    #if NTP_SUPPORT
        response->printf(",\n\"timestamp\": \"%s\"", ntpDateTime().c_str());
    #endif


    unsigned long count = settingsKeyCount();
    for (unsigned int i=0; i<count; i++) {
        String key = settingsKeyName(i);
        String value = getSetting(key);
        response->printf(",\n\"%s\": \"%s\"", key.c_str(), value.c_str());
    }
    response->printf("\n}");

    request->send(response);

}

void _onPostConfig(AsyncWebServerRequest *request) {
    webLog(request);
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }
    request->send(_webConfigSuccess ? 200 : 400);
}

void _onPostConfigData(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }


    if (final && (index == 0)) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject((char *) data);
        if (root.success()) _webConfigSuccess = settingsRestoreJson(root);
        return;
    }


    if (index == 0) if (_webConfigBuffer) delete _webConfigBuffer;


    if (!_webConfigBuffer) {
        _webConfigBuffer = new std::vector<uint8_t>();
        _webConfigSuccess = false;
    }


    if (len > 0) {
        _webConfigBuffer->reserve(_webConfigBuffer->size() + len);
        _webConfigBuffer->insert(_webConfigBuffer->end(), data, data + len);
    }


    if (final) {

        _webConfigBuffer->push_back(0);


        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject((char *) _webConfigBuffer->data());
        if (root.success()) _webConfigSuccess = settingsRestoreJson(root);
        delete _webConfigBuffer;

    }

}

#if WEB_EMBEDDED
void _onHome(AsyncWebServerRequest *request) {

    webLog(request);
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }

    if (request->header("If-Modified-Since").equals(_last_modified)) {

        request->send(304);

    } else {

        #if ASYNC_TCP_SSL_ENABLED



            DEBUG_MSG_P(PSTR("[MAIN] Free heap: %d bytes\n"), getFreeHeap());
            size_t max = (getFreeHeap() / 3) & 0xFFE0;

            AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [max](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {


                size_t len = webui_image_len - index;
                if (len > maxLen) len = maxLen;
                if (len > max) len = max;
                if (len > 0) memcpy_P(buffer, webui_image + index, len);

                DEBUG_MSG_P(PSTR("[WEB] Sending %d%%%% (max chunk size: %4d)\r"), int(100 * index / webui_image_len), max);
                if (len == 0) DEBUG_MSG_P(PSTR("\n"));


                return len;

            });

        #else

            AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", webui_image, webui_image_len);

        #endif

        response->addHeader("Content-Encoding", "gzip");
        response->addHeader("Last-Modified", _last_modified);
        response->addHeader("X-XSS-Protection", "1; mode=block");
        response->addHeader("X-Content-Type-Options", "nosniff");
        response->addHeader("X-Frame-Options", "deny");
        request->send(response);

    }

}
#endif

#if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED

int _onCertificate(void * arg, const char *filename, uint8_t **buf) {

#if WEB_EMBEDDED

    if (strcmp(filename, "server.cer") == 0) {
        uint8_t * nbuf = (uint8_t*) malloc(server_cer_len);
        memcpy_P(nbuf, server_cer, server_cer_len);
        *buf = nbuf;
        DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
        return server_cer_len;
    }

    if (strcmp(filename, "server.key") == 0) {
        uint8_t * nbuf = (uint8_t*) malloc(server_key_len);
        memcpy_P(nbuf, server_key, server_key_len);
        *buf = nbuf;
        DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
        return server_key_len;
    }

    DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - ERROR\n"), filename);
    *buf = 0;
    return 0;

#else

    File file = SPIFFS.open(filename, "r");
    if (file) {
        size_t size = file.size();
        uint8_t * nbuf = (uint8_t*) malloc(size);
        if (nbuf) {
            size = file.read(nbuf, size);
            file.close();
            *buf = nbuf;
            DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - OK\n"), filename);
            return size;
        }
        file.close();
    }
    DEBUG_MSG_P(PSTR("[WEB] SSL File: %s - ERROR\n"), filename);
    *buf = 0;
    return 0;

#endif

}

#endif

void _onUpgrade(AsyncWebServerRequest *request) {

    webLog(request);
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }

    char buffer[10];
    if (!Update.hasError()) {
        sprintf_P(buffer, PSTR("OK"));
    } else {
        sprintf_P(buffer, PSTR("ERROR %d"), Update.getError());
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", buffer);
    response->addHeader("Connection", "close");
    response->addHeader("X-XSS-Protection", "1; mode=block");
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("X-Frame-Options", "deny");
    if (Update.hasError()) {
        eepromRotate(true);
    } else {
        deferredReset(100, CUSTOM_RESET_UPGRADE);
    }
    request->send(response);

}

void _onUpgradeFile(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getSetting("hostname").c_str());
    }

    if (!index) {


        eepromRotate(false);

        DEBUG_MSG_P(PSTR("[UPGRADE] Start: %s\n"), filename.c_str());
        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
        }

    }

    if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
        }
    }

    if (final) {
        if (Update.end(true)){
            DEBUG_MSG_P(PSTR("[UPGRADE] Success:  %u bytes\n"), index + len);
        } else {
            #ifdef DEBUG_PORT
                Update.printError(DEBUG_PORT);
            #endif
        }
    } else {


    }
}

bool _onAPModeRequest(AsyncWebServerRequest *request) {

    if ((WiFi.getMode() & WIFI_AP) > 0) {
        const String domain = getSetting("hostname") + ".";
        const String host = request->header("Host");
        const String ip = WiFi.softAPIP().toString();


        if (host.equals(ip)) return true;
        if (host.startsWith(domain)) return true;



        request->send(404);
        request->client()->close();

        return false;
    }

    return true;

}

void _onRequest(AsyncWebServerRequest *request){

    if (!_onAPModeRequest(request)) return;


    for (unsigned char i = 0; i < _web_request_callbacks.size(); i++) {
        bool response = (_web_request_callbacks[i])(request);
        if (response) return;
    }


    request->send(404);



    request->client()->close();

}

void _onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

    if (!_onAPModeRequest(request)) return;


    for (unsigned char i = 0; i < _web_body_callbacks.size(); i++) {
        bool response = (_web_body_callbacks[i])(request, data, len, index, total);
        if (response) return;
    }


    request->send(404);
    request->client()->close();

}




bool webAuthenticate(AsyncWebServerRequest *request) {
    #if USE_PASSWORD
        String password = getAdminPass();
        char httpPassword[password.length() + 1];
        password.toCharArray(httpPassword, password.length() + 1);
        return request->authenticate(WEB_USERNAME, httpPassword);
    #else
        return true;
    #endif
}



AsyncWebServer * webServer() {
    return _server;
}

void webBodyRegister(web_body_callback_f callback) {
    _web_body_callbacks.push_back(callback);
}

void webRequestRegister(web_request_callback_f callback) {
    _web_request_callbacks.push_back(callback);
}

unsigned int webPort() {
    #if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
        return 443;
    #else
        return getSetting("webPort", WEB_PORT).toInt();
    #endif
}

void webLog(AsyncWebServerRequest *request) {
    DEBUG_MSG_P(PSTR("[WEBSERVER] Request: %s %s\n"), request->methodToString(), request->url().c_str());
}

void webSetup() {


    snprintf_P(_last_modified, sizeof(_last_modified), PSTR("%s %s GMT"), __DATE__, __TIME__);


    unsigned int port = webPort();
    _server = new AsyncWebServer(port);


    _server->rewrite("/", "/index.html");


    #if WEB_EMBEDDED
        _server->on("/index.html", HTTP_GET, _onHome);
    #endif


    _server->on("/reset", HTTP_GET, _onReset);
    _server->on("/config", HTTP_GET, _onGetConfig);
    _server->on("/config", HTTP_POST | HTTP_PUT, _onPostConfig, _onPostConfigData);
    _server->on("/upgrade", HTTP_POST, _onUpgrade, _onUpgradeFile);
    _server->on("/discover", HTTP_GET, _onDiscover);


    #if SPIFFS_SUPPORT
        _server->serveStatic("/", SPIFFS, "/")
            .setLastModified(_last_modified)
            .setFilter([](AsyncWebServerRequest *request) -> bool {
                webLog(request);
                return true;
            });
    #endif



    _server->onRequestBody(_onBody);
    _server->onNotFound(_onRequest);


    #if ASYNC_TCP_SSL_ENABLED & WEB_SSL_ENABLED
        _server->onSslFileRequest(_onCertificate, NULL);
        _server->beginSecure("server.cer", "server.key", NULL);
    #else
        _server->begin();
    #endif

    DEBUG_MSG_P(PSTR("[WEBSERVER] Webserver running on port %u\n"), port);

}

#endif
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/wifi.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/wifi.ino"
#include "JustWifi.h"
#include <Ticker.h>

uint32_t _wifi_scan_client_id = 0;
bool _wifi_wps_running = false;
bool _wifi_smartconfig_running = false;
bool _wifi_smartconfig_initial = false;
uint8_t _wifi_ap_mode = WIFI_AP_FALLBACK;





void _wifiCheckAP() {

    if ((WIFI_AP_FALLBACK == _wifi_ap_mode) &&
        (jw.connected()) &&
        ((WiFi.getMode() & WIFI_AP) > 0) &&
        (WiFi.softAPgetStationNum() == 0)
    ) {
        jw.enableAP(false);
    }

}

void _wifiConfigure() {

    jw.setHostname(getSetting("hostname").c_str());
    #if USE_PASSWORD
        jw.setSoftAP(getSetting("hostname").c_str(), getAdminPass().c_str());
    #else
        jw.setSoftAP(getSetting("hostname").c_str());
    #endif
    jw.setConnectTimeout(WIFI_CONNECT_TIMEOUT);
    wifiReconnectCheck();
    jw.enableAPFallback(WIFI_FALLBACK_APMODE);
    jw.cleanNetworks();

    _wifi_ap_mode = getSetting("apmode", WIFI_AP_FALLBACK).toInt();


    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) return;
    #endif


    _wifiClean(WIFI_MAX_NETWORKS);

    int i;
    for (i = 0; i< WIFI_MAX_NETWORKS; i++) {
        if (getSetting("ssid" + String(i)).length() == 0) break;
        if (getSetting("ip" + String(i)).length() == 0) {
            jw.addNetwork(
                getSetting("ssid" + String(i)).c_str(),
                getSetting("pass" + String(i)).c_str()
            );
        } else {
            jw.addNetwork(
                getSetting("ssid" + String(i)).c_str(),
                getSetting("pass" + String(i)).c_str(),
                getSetting("ip" + String(i)).c_str(),
                getSetting("gw" + String(i)).c_str(),
                getSetting("mask" + String(i)).c_str(),
                getSetting("dns" + String(i)).c_str()
            );
        }
    }

    #if JUSTWIFI_ENABLE_SMARTCONFIG
        if (i == 0) _wifi_smartconfig_initial = true;
    #endif

    jw.enableScan(getSetting("wifiScan", WIFI_SCAN_NETWORKS).toInt() == 1);

    unsigned char sleep_mode = getSetting("wifiSleep", WIFI_SLEEP_MODE).toInt();
    sleep_mode = constrain(sleep_mode, 0, 2);

    WiFi.setSleepMode(static_cast<WiFiSleepType_t>(sleep_mode));
}

void _wifiScan(uint32_t client_id = 0) {

    DEBUG_MSG_P(PSTR("[WIFI] Start scanning\n"));

    #if WEB_SUPPORT
        String output;
    #endif

    unsigned char result = WiFi.scanNetworks();

    if (result == WIFI_SCAN_FAILED) {
        DEBUG_MSG_P(PSTR("[WIFI] Scan failed\n"));
        #if WEB_SUPPORT
            output = String("Failed scan");
        #endif
    } else if (result == 0) {
        DEBUG_MSG_P(PSTR("[WIFI] No networks found\n"));
        #if WEB_SUPPORT
            output = String("No networks found");
        #endif
    } else {

        DEBUG_MSG_P(PSTR("[WIFI] %d networks found:\n"), result);


        for (int8_t i = 0; i < result; ++i) {

            String ssid_scan;
            int32_t rssi_scan;
            uint8_t sec_scan;
            uint8_t* BSSID_scan;
            int32_t chan_scan;
            bool hidden_scan;
            char buffer[128];

            WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan, hidden_scan);

            snprintf_P(buffer, sizeof(buffer),
                PSTR("BSSID: %02X:%02X:%02X:%02X:%02X:%02X SEC: %s RSSI: %3d CH: %2d SSID: %s"),
                BSSID_scan[0], BSSID_scan[1], BSSID_scan[2], BSSID_scan[3], BSSID_scan[4], BSSID_scan[5],
                (sec_scan != ENC_TYPE_NONE ? "YES" : "NO "),
                rssi_scan,
                chan_scan,
                (char *) ssid_scan.c_str()
            );

            DEBUG_MSG_P(PSTR("[WIFI] > %s\n"), buffer);

            #if WEB_SUPPORT
                if (client_id > 0) output = output + String(buffer) + String("<br />");
            #endif

        }

    }

    #if WEB_SUPPORT
        if (client_id > 0) {
            output = String("{\"scanResult\": \"") + output + String("\"}");
            wsSend(client_id, output.c_str());
        }
    #endif

    WiFi.scanDelete();

}

bool _wifiClean(unsigned char num) {

    bool changed = false;
    int i = 0;


    while (i < num) {


        if (!hasSetting("ssid", i)) {
            delSetting("ssid", i);
            break;
        }


        if (!hasSetting("pass", i)) delSetting("pass", i);
        if (!hasSetting("ip", i)) delSetting("ip", i);
        if (!hasSetting("gw", i)) delSetting("gw", i);
        if (!hasSetting("mask", i)) delSetting("mask", i);
        if (!hasSetting("dns", i)) delSetting("dns", i);

        ++i;

    }


    while (i < WIFI_MAX_NETWORKS) {
        changed = hasSetting("ssid", i);
        delSetting("ssid", i);
        delSetting("pass", i);
        delSetting("ip", i);
        delSetting("gw", i);
        delSetting("mask", i);
        delSetting("dns", i);
        ++i;
    }

    return changed;

}


void _wifiInject() {

    if (strlen(WIFI1_SSID)) {

        if (!hasSetting("ssid", 0)) {
            setSetting("ssid", 0, WIFI1_SSID);
            setSetting("pass", 0, WIFI1_PASS);
            setSetting("ip", 0, WIFI1_IP);
            setSetting("gw", 0, WIFI1_GW);
            setSetting("mask", 0, WIFI1_MASK);
            setSetting("dns", 0, WIFI1_DNS);
        }

        if (strlen(WIFI2_SSID)) {
            if (!hasSetting("ssid", 1)) {
                setSetting("ssid", 1, WIFI2_SSID);
                setSetting("pass", 1, WIFI2_PASS);
                setSetting("ip", 1, WIFI2_IP);
                setSetting("gw", 1, WIFI2_GW);
                setSetting("mask", 1, WIFI2_MASK);
                setSetting("dns", 1, WIFI2_DNS);
            }

            if (strlen(WIFI3_SSID)) {
                if (!hasSetting("ssid", 2)) {
                    setSetting("ssid", 2, WIFI3_SSID);
                    setSetting("pass", 2, WIFI3_PASS);
                    setSetting("ip", 2, WIFI3_IP);
                    setSetting("gw", 2, WIFI3_GW);
                    setSetting("mask", 2, WIFI3_MASK);
                    setSetting("dns", 2, WIFI3_DNS);
                }

                if (strlen(WIFI4_SSID)) {
                    if (!hasSetting("ssid", 3)) {
                        setSetting("ssid", 3, WIFI4_SSID);
                        setSetting("pass", 3, WIFI4_PASS);
                        setSetting("ip", 3, WIFI4_IP);
                        setSetting("gw", 3, WIFI4_GW);
                        setSetting("mask", 3, WIFI4_MASK);
                        setSetting("dns", 3, WIFI4_DNS);
                    }
                }
            }
        }
    }
}

void _wifiCallback(justwifi_messages_t code, char * parameter) {

    if (MESSAGE_WPS_START == code) {
        _wifi_wps_running = true;
    }

    if (MESSAGE_SMARTCONFIG_START == code) {
        _wifi_smartconfig_running = true;
    }

    if (MESSAGE_WPS_ERROR == code || MESSAGE_SMARTCONFIG_ERROR == code) {
        _wifi_wps_running = false;
        _wifi_smartconfig_running = false;
    }

    if (MESSAGE_WPS_SUCCESS == code || MESSAGE_SMARTCONFIG_SUCCESS == code) {

        String ssid = WiFi.SSID();
        String pass = WiFi.psk();


        uint8_t count = 0;
        while (count < WIFI_MAX_NETWORKS) {
            if (!hasSetting("ssid", count)) break;
            if (ssid.equals(getSetting("ssid", count, ""))) break;
            count++;
        }


        if (WIFI_MAX_NETWORKS == count) count = 0;

        setSetting("ssid", count, ssid);
        setSetting("pass", count, pass);

        _wifi_wps_running = false;
        _wifi_smartconfig_running = false;

    }

}

#if WIFI_AP_CAPTIVE

#include "DNSServer.h"

DNSServer _wifi_dnsServer;

void _wifiCaptivePortal(justwifi_messages_t code, char * parameter) {

    if (MESSAGE_ACCESSPOINT_CREATED == code) {
        _wifi_dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        _wifi_dnsServer.start(53, "*", WiFi.softAPIP());
        DEBUG_MSG_P(PSTR("[WIFI] Captive portal enabled\n"));
    }

    if (MESSAGE_CONNECTED == code) {
        _wifi_dnsServer.stop();
        DEBUG_MSG_P(PSTR("[WIFI] Captive portal disabled\n"));
    }

}

#endif

#if DEBUG_SUPPORT

void _wifiDebugCallback(justwifi_messages_t code, char * parameter) {



    if (code == MESSAGE_SCANNING) {
        DEBUG_MSG_P(PSTR("[WIFI] Scanning\n"));
    }

    if (code == MESSAGE_SCAN_FAILED) {
        DEBUG_MSG_P(PSTR("[WIFI] Scan failed\n"));
    }

    if (code == MESSAGE_NO_NETWORKS) {
        DEBUG_MSG_P(PSTR("[WIFI] No networks found\n"));
    }

    if (code == MESSAGE_NO_KNOWN_NETWORKS) {
        DEBUG_MSG_P(PSTR("[WIFI] No known networks found\n"));
    }

    if (code == MESSAGE_FOUND_NETWORK) {
        DEBUG_MSG_P(PSTR("[WIFI] %s\n"), parameter);
    }



    if (code == MESSAGE_CONNECTING) {
        DEBUG_MSG_P(PSTR("[WIFI] Connecting to %s\n"), parameter);
    }

    if (code == MESSAGE_CONNECT_WAITING) {

    }

    if (code == MESSAGE_CONNECT_FAILED) {
        DEBUG_MSG_P(PSTR("[WIFI] Could not connect to %s\n"), parameter);
    }

    if (code == MESSAGE_CONNECTED) {
        wifiDebug(WIFI_STA);
    }

    if (code == MESSAGE_DISCONNECTED) {
        DEBUG_MSG_P(PSTR("[WIFI] Disconnected\n"));
    }



    if (code == MESSAGE_ACCESSPOINT_CREATING) {
        DEBUG_MSG_P(PSTR("[WIFI] Creating access point\n"));
    }

    if (code == MESSAGE_ACCESSPOINT_CREATED) {
        wifiDebug(WIFI_AP);
    }

    if (code == MESSAGE_ACCESSPOINT_FAILED) {
        DEBUG_MSG_P(PSTR("[WIFI] Could not create access point\n"));
    }

    if (code == MESSAGE_ACCESSPOINT_DESTROYED) {
        DEBUG_MSG_P(PSTR("[WIFI] Access point destroyed\n"));
    }



    if (code == MESSAGE_WPS_START) {
        DEBUG_MSG_P(PSTR("[WIFI] WPS started\n"));
    }

    if (code == MESSAGE_WPS_SUCCESS) {
        DEBUG_MSG_P(PSTR("[WIFI] WPS succeded!\n"));
    }

    if (code == MESSAGE_WPS_ERROR) {
        DEBUG_MSG_P(PSTR("[WIFI] WPS failed\n"));
    }



    if (code == MESSAGE_SMARTCONFIG_START) {
        DEBUG_MSG_P(PSTR("[WIFI] Smart Config started\n"));
    }

    if (code == MESSAGE_SMARTCONFIG_SUCCESS) {
        DEBUG_MSG_P(PSTR("[WIFI] Smart Config succeded!\n"));
    }

    if (code == MESSAGE_SMARTCONFIG_ERROR) {
        DEBUG_MSG_P(PSTR("[WIFI] Smart Config failed\n"));
    }

}

#endif





#if TERMINAL_SUPPORT

void _wifiInitCommands() {

    terminalRegisterCommand(F("WIFI"), [](Embedis* e) {
        wifiDebug();
        terminalOK();
    });

    terminalRegisterCommand(F("WIFI.RESET"), [](Embedis* e) {
        _wifiConfigure();
        wifiDisconnect();
        terminalOK();
    });

    terminalRegisterCommand(F("WIFI.AP"), [](Embedis* e) {
        wifiStartAP();
        terminalOK();
    });

    #if defined(JUSTWIFI_ENABLE_WPS)
        terminalRegisterCommand(F("WIFI.WPS"), [](Embedis* e) {
            wifiStartWPS();
            terminalOK();
        });
    #endif

    #if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
        terminalRegisterCommand(F("WIFI.SMARTCONFIG"), [](Embedis* e) {
            wifiStartSmartConfig();
            terminalOK();
        });
    #endif

    terminalRegisterCommand(F("WIFI.SCAN"), [](Embedis* e) {
        _wifiScan();
        terminalOK();
    });

}

#endif





#if WEB_SUPPORT

bool _wifiWebSocketOnReceive(const char * key, JsonVariant& value) {
    if (strncmp(key, "wifi", 4) == 0) return true;
    if (strncmp(key, "ssid", 4) == 0) return true;
    if (strncmp(key, "pass", 4) == 0) return true;
    if (strncmp(key, "ip", 2) == 0) return true;
    if (strncmp(key, "gw", 2) == 0) return true;
    if (strncmp(key, "mask", 4) == 0) return true;
    if (strncmp(key, "dns", 3) == 0) return true;
    return false;
}

void _wifiWebSocketOnSend(JsonObject& root) {
    root["maxNetworks"] = WIFI_MAX_NETWORKS;
    root["wifiScan"] = getSetting("wifiScan", WIFI_SCAN_NETWORKS).toInt() == 1;
    JsonArray& wifi = root.createNestedArray("wifi");
    for (byte i=0; i<WIFI_MAX_NETWORKS; i++) {
        if (!hasSetting("ssid", i)) break;
        JsonObject& network = wifi.createNestedObject();
        network["ssid"] = getSetting("ssid", i, "");
        network["pass"] = getSetting("pass", i, "");
        network["ip"] = getSetting("ip", i, "");
        network["gw"] = getSetting("gw", i, "");
        network["mask"] = getSetting("mask", i, "");
        network["dns"] = getSetting("dns", i, "");
    }
}

void _wifiWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "scan") == 0) _wifi_scan_client_id = client_id;
}

#endif





void wifiDebug(WiFiMode_t modes) {

    #if DEBUG_SUPPORT
    bool footer = false;

    if (((modes & WIFI_STA) > 0) && ((WiFi.getMode() & WIFI_STA) > 0)) {

        uint8_t * bssid = WiFi.BSSID();
        DEBUG_MSG_P(PSTR("[WIFI] ------------------------------------- MODE STA\n"));
        DEBUG_MSG_P(PSTR("[WIFI] SSID  %s\n"), WiFi.SSID().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP    %s\n"), WiFi.localIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC   %s\n"), WiFi.macAddress().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] GW    %s\n"), WiFi.gatewayIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] DNS   %s\n"), WiFi.dnsIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MASK  %s\n"), WiFi.subnetMask().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] HOST  http://%s.local\n"), WiFi.hostname().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] BSSID %02X:%02X:%02X:%02X:%02X:%02X\n"),
            bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], bssid[6]
        );
        DEBUG_MSG_P(PSTR("[WIFI] CH    %d\n"), WiFi.channel());
        DEBUG_MSG_P(PSTR("[WIFI] RSSI  %d\n"), WiFi.RSSI());
        footer = true;

    }

    if (((modes & WIFI_AP) > 0) && ((WiFi.getMode() & WIFI_AP) > 0)) {
        DEBUG_MSG_P(PSTR("[WIFI] -------------------------------------- MODE AP\n"));
        DEBUG_MSG_P(PSTR("[WIFI] SSID  %s\n"), getSetting("hostname").c_str());
        DEBUG_MSG_P(PSTR("[WIFI] PASS  %s\n"), getAdminPass().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] IP    %s\n"), WiFi.softAPIP().toString().c_str());
        DEBUG_MSG_P(PSTR("[WIFI] MAC   %s\n"), WiFi.softAPmacAddress().c_str());
        footer = true;
    }

    if (WiFi.getMode() == 0) {
        DEBUG_MSG_P(PSTR("[WIFI] ------------------------------------- MODE OFF\n"));
        DEBUG_MSG_P(PSTR("[WIFI] No connection\n"));
        footer = true;
    }

    if (footer) {
        DEBUG_MSG_P(PSTR("[WIFI] ----------------------------------------------\n"));
    }
    #endif

}

void wifiDebug() {
    wifiDebug(WIFI_AP_STA);
}





String getIP() {
    if (WiFi.getMode() == WIFI_AP) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

String getNetwork() {
    if (WiFi.getMode() == WIFI_AP) {
        return jw.getAPSSID();
    }
    return WiFi.SSID();
}

bool wifiConnected() {
    return jw.connected();
}

void wifiDisconnect() {
    jw.disconnect();
}

void wifiStartAP(bool only) {
    if (only) {
        jw.enableSTA(false);
        jw.disconnect();
        jw.resetReconnectTimeout();
    }
    jw.enableAP(true);
}

void wifiStartAP() {
    wifiStartAP(true);
}

#if defined(JUSTWIFI_ENABLE_WPS)
void wifiStartWPS() {
    jw.enableAP(false);
    jw.disconnect();
    jw.startWPS();
}
#endif

#if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
void wifiStartSmartConfig() {
    jw.enableAP(false);
    jw.disconnect();
    jw.startSmartConfig();
}
#endif

void wifiReconnectCheck() {
    bool connected = false;
    #if WEB_SUPPORT
        if (wsConnected()) connected = true;
    #endif
    #if TELNET_SUPPORT
        if (telnetConnected()) connected = true;
    #endif
    jw.setReconnectTimeout(connected ? 0 : WIFI_RECONNECT_INTERVAL);
}

uint8_t wifiState() {
    uint8_t state = 0;
    if (jw.connected()) state += WIFI_STATE_STA;
    if (jw.connectable()) state += WIFI_STATE_AP;
    if (_wifi_wps_running) state += WIFI_STATE_WPS;
    if (_wifi_smartconfig_running) state += WIFI_STATE_SMARTCONFIG;
    return state;
}

void wifiRegister(wifi_callback_f callback) {
    jw.subscribe(callback);
}





void wifiSetup() {

    _wifiInject();
    _wifiConfigure();

    #if JUSTWIFI_ENABLE_SMARTCONFIG
        if (_wifi_smartconfig_initial) jw.startSmartConfig();
    #endif


    wifiRegister(_wifiCallback);
    #if WIFI_AP_CAPTIVE
        wifiRegister(_wifiCaptivePortal);
    #endif
    #if DEBUG_SUPPORT
        wifiRegister(_wifiDebugCallback);
    #endif

    #if WEB_SUPPORT
        wsOnSendRegister(_wifiWebSocketOnSend);
        wsOnReceiveRegister(_wifiWebSocketOnReceive);
        wsOnActionRegister(_wifiWebSocketOnAction);
    #endif

    #if TERMINAL_SUPPORT
        _wifiInitCommands();
    #endif


    espurnaRegisterLoop(wifiLoop);
    espurnaRegisterReload(_wifiConfigure);

}

void wifiLoop() {


    jw.loop();


    #if WIFI_AP_CAPTIVE
        if ((WiFi.getMode() & WIFI_AP) == WIFI_AP) {
            _wifi_dnsServer.processNextRequest();
        }
    #endif


    if (_wifi_scan_client_id > 0) {
        _wifiScan(_wifi_scan_client_id);
        _wifi_scan_client_id = 0;
    }


    static unsigned long last = 0;
    if (millis() - last > 60000) {
        last = millis();
        _wifiCheckAP();
    }

}
# 1 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ws.ino"
# 9 "/Users/toni/Documents/sato/satostation/firmware/espurna/code/espurna/ws.ino"
#if WEB_SUPPORT

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <vector>
#include "libs/WebSocketIncommingBuffer.h"

AsyncWebSocket _ws("/ws");
Ticker _web_defer;

std::vector<ws_on_send_callback_f> _ws_on_send_callbacks;
std::vector<ws_on_action_callback_f> _ws_on_action_callbacks;
std::vector<ws_on_receive_callback_f> _ws_on_receive_callbacks;





typedef struct {
    IPAddress ip;
    unsigned long timestamp = 0;
} ws_ticket_t;
ws_ticket_t _ticket[WS_BUFFER_SIZE];

void _onAuth(AsyncWebServerRequest *request) {

    webLog(request);
    if (!webAuthenticate(request)) return request->requestAuthentication();

    IPAddress ip = request->client()->remoteIP();
    unsigned long now = millis();
    unsigned short index;
    for (index = 0; index < WS_BUFFER_SIZE; index++) {
        if (_ticket[index].ip == ip) break;
        if (_ticket[index].timestamp == 0) break;
        if (now - _ticket[index].timestamp > WS_TIMEOUT) break;
    }
    if (index == WS_BUFFER_SIZE) {
        request->send(429);
    } else {
        _ticket[index].ip = ip;
        _ticket[index].timestamp = now;
        request->send(200, "text/plain", "OK");
    }

}

bool _wsAuth(AsyncWebSocketClient * client) {

    IPAddress ip = client->remoteIP();
    unsigned long now = millis();
    unsigned short index = 0;

    for (index = 0; index < WS_BUFFER_SIZE; index++) {
        if ((_ticket[index].ip == ip) && (now - _ticket[index].timestamp < WS_TIMEOUT)) break;
    }

    if (index == WS_BUFFER_SIZE) {
        return false;
    }

    return true;

}

#if DEBUG_WEB_SUPPORT

bool wsDebugSend(const char* prefix, const char* message) {
    if (!wsConnected()) return false;
    if (getFreeHeap() < (strlen(message) * 3)) return false;

    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    JsonObject &weblog = root.createNestedObject("weblog");

    weblog.set("message", message);
    if (prefix && (prefix[0] != '\0')) {
        weblog.set("prefix", prefix);
    }

    wsSend(root);

    return true;
}
#endif



#if MQTT_SUPPORT
void _wsMQTTCallback(unsigned int type, const char * topic, const char * payload) {
    if (type == MQTT_CONNECT_EVENT) wsSend_P(PSTR("{\"mqttStatus\": true}"));
    if (type == MQTT_DISCONNECT_EVENT) wsSend_P(PSTR("{\"mqttStatus\": false}"));
}
#endif

bool _wsStore(String key, String value) {


    if (key == "webPort") {
        if ((value.toInt() == 0) || (value.toInt() == 80)) {
            return delSetting(key);
        }
    }

    if (value != getSetting(key)) {
        return setSetting(key, value);
    }

    return false;

}

bool _wsStore(String key, JsonArray& value) {

    bool changed = false;

    unsigned char index = 0;
    for (auto element : value) {
        if (_wsStore(key + index, element.as<String>())) changed = true;
        index++;
    }


    for (unsigned char i=index; i<SETTINGS_MAX_LIST_COUNT; i++) {
        if (!delSetting(key, index)) break;
        changed = true;
    }

    return changed;

}

void _wsParse(AsyncWebSocketClient *client, uint8_t * payload, size_t length) {




    uint32_t client_id = client->id();


    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((char *) payload);
    if (!root.success()) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] Error parsing data\n"));
        wsSend_P(client_id, PSTR("{\"message\": 3}"));
        return;
    }



    const char* action = root["action"];
    if (action) {

        DEBUG_MSG_P(PSTR("[WEBSOCKET] Requested action: %s\n"), action);

        if (strcmp(action, "reboot") == 0) {
            deferredReset(100, CUSTOM_RESET_WEB);
            return;
        }

        if (strcmp(action, "reconnect") == 0) {
            _web_defer.once_ms(100, wifiDisconnect);
            return;
        }

        if (strcmp(action, "factory_reset") == 0) {
            DEBUG_MSG_P(PSTR("\n\nFACTORY RESET\n\n"));
            resetSettings();
            deferredReset(100, CUSTOM_RESET_FACTORY);
            return;
        }

        JsonObject& data = root["data"];
        if (data.success()) {


            for (unsigned char i = 0; i < _ws_on_action_callbacks.size(); i++) {
                (_ws_on_action_callbacks[i])(client_id, action, data);
            }


            if (strcmp(action, "restore") == 0) {
                if (settingsRestoreJson(data)) {
                    wsSend_P(client_id, PSTR("{\"message\": 5}"));
                } else {
                    wsSend_P(client_id, PSTR("{\"message\": 4}"));
                }
            }

            return;

        }

    };



    JsonObject& config = root["config"];
    if (config.success()) {

        DEBUG_MSG_P(PSTR("[WEBSOCKET] Parsing configuration data\n"));

        String adminPass;
        bool save = false;

        for (auto kv: config) {

            bool changed = false;
            String key = kv.key;
            JsonVariant& value = kv.value;


            if (key == "adminPass") {
                if (!value.is<JsonArray&>()) continue;
                JsonArray& values = value.as<JsonArray&>();
                if (values.size() != 2) continue;
                if (values[0].as<String>().equals(values[1].as<String>())) {
                    String password = values[0].as<String>();
                    if (password.length() > 0) {
                        setSetting(key, password);
                        save = true;
                        wsSend_P(client_id, PSTR("{\"action\": \"reload\"}"));
                    }
                } else {
                    wsSend_P(client_id, PSTR("{\"message\": 7}"));
                }
                continue;
            }


            bool found = false;
            for (unsigned char i = 0; i < _ws_on_receive_callbacks.size(); i++) {
                found |= (_ws_on_receive_callbacks[i])(key.c_str(), value);


                if (found) break;
            }
            if (!found) {
                delSetting(key);
                continue;
            }


            if (value.is<JsonArray&>()) {
                if (_wsStore(key, value.as<JsonArray&>())) changed = true;
            } else {
                if (_wsStore(key, value.as<String>())) changed = true;
            }


            if (changed) {
                save = true;
            }

        }


        if (save) {


            espurnaReload();


            saveSettings();

            wsSend_P(client_id, PSTR("{\"message\": 8}"));

        } else {

            wsSend_P(client_id, PSTR("{\"message\": 9}"));

        }

    }

}

void _wsUpdate(JsonObject& root) {
    root["heap"] = getFreeHeap();
    root["uptime"] = getUptime();
    root["rssi"] = WiFi.RSSI();
    root["loadaverage"] = systemLoadAverage();
    #if ADC_MODE_VALUE == ADC_VCC
        root["vcc"] = ESP.getVcc();
    #endif
    #if NTP_SUPPORT
        if (ntpSynced()) root["now"] = now();
    #endif
}

void _wsDoUpdate(bool reset = false) {
    static unsigned long last = millis();
    if (reset) {
        last = millis() + WS_UPDATE_INTERVAL;
        return;
    }

    if (millis() - last > WS_UPDATE_INTERVAL) {
        last = millis();
        wsSend(_wsUpdate);
    }
}


bool _wsOnReceive(const char * key, JsonVariant& value) {
    if (strncmp(key, "ws", 2) == 0) return true;
    if (strncmp(key, "admin", 5) == 0) return true;
    if (strncmp(key, "hostname", 8) == 0) return true;
    if (strncmp(key, "desc", 4) == 0) return true;
    if (strncmp(key, "webPort", 7) == 0) return true;
    return false;
}

void _wsOnStart(JsonObject& root) {
    char chipid[7];
    snprintf_P(chipid, sizeof(chipid), PSTR("%06X"), ESP.getChipId());
    uint8_t * bssid = WiFi.BSSID();
    char bssid_str[20];
    snprintf_P(bssid_str, sizeof(bssid_str),
        PSTR("%02X:%02X:%02X:%02X:%02X:%02X"),
        bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]
    );

    root["webMode"] = WEB_MODE_NORMAL;

    root["app_name"] = APP_NAME;
    root["app_version"] = APP_VERSION;
    root["app_build"] = buildTime();
    #if defined(APP_REVISION)
        root["app_revision"] = APP_REVISION;
    #endif
    root["manufacturer"] = MANUFACTURER;
    root["chipid"] = String(chipid);
    root["mac"] = WiFi.macAddress();
    root["bssid"] = String(bssid_str);
    root["channel"] = WiFi.channel();
    root["device"] = DEVICE;
    root["hostname"] = getSetting("hostname");
    root["desc"] = getSetting("desc");
    root["network"] = getNetwork();
    root["deviceip"] = getIP();
    root["sketch_size"] = ESP.getSketchSize();
    root["free_size"] = ESP.getFreeSketchSpace();
    root["sdk"] = ESP.getSdkVersion();
    root["core"] = getCoreVersion();

    root["btnDelay"] = getSetting("btnDelay", BUTTON_DBLCLICK_DELAY).toInt();
    root["webPort"] = getSetting("webPort", WEB_PORT).toInt();
    root["wsAuth"] = getSetting("wsAuth", WS_AUTHENTICATION).toInt() == 1;
    #if TERMINAL_SUPPORT
        root["cmdVisible"] = 1;
    #endif
    root["hbMode"] = getSetting("hbMode", HEARTBEAT_MODE).toInt();
    root["hbInterval"] = getSetting("hbInterval", HEARTBEAT_INTERVAL).toInt();

    _wsDoUpdate(true);

}

void wsSend(JsonObject& root) {
    size_t len = root.measureLength();
    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len);

    if (buffer) {
        root.printTo(reinterpret_cast<char*>(buffer->get()), len + 1);
        _ws.textAll(buffer);
    }
}

void wsSend(uint32_t client_id, JsonObject& root) {
    AsyncWebSocketClient* client = _ws.client(client_id);
    if (client == nullptr) return;

    size_t len = root.measureLength();
    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len);

    if (buffer) {
        root.printTo(reinterpret_cast<char*>(buffer->get()), len + 1);
        client->text(buffer);
    }
}

void _wsStart(uint32_t client_id) {
    #if USE_PASSWORD && WEB_FORCE_PASS_CHANGE
        bool changePassword = getAdminPass().equals(ADMIN_PASS);
    #else
        bool changePassword = false;
    #endif

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    if (changePassword) {
        root["webMode"] = WEB_MODE_PASSWORD;
        wsSend(root);
        return;
    }

    for (auto& callback : _ws_on_send_callbacks) {
        callback(root);
    }

    wsSend(client_id, root);
}

void _wsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){

    if (type == WS_EVT_CONNECT) {

        client->_tempObject = nullptr;

        #ifndef NOWSAUTH
            if (!_wsAuth(client)) {
                wsSend_P(client->id(), PSTR("{\"message\": 10}"));
                DEBUG_MSG_P(PSTR("[WEBSOCKET] Validation check failed\n"));
                client->close();
                return;
            }
        #endif

        IPAddress ip = client->remoteIP();
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u connected, ip: %d.%d.%d.%d, url: %s\n"), client->id(), ip[0], ip[1], ip[2], ip[3], server->url());
        _wsStart(client->id());
        client->_tempObject = new WebSocketIncommingBuffer(&_wsParse, true);
        wifiReconnectCheck();

    } else if(type == WS_EVT_DISCONNECT) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u disconnected\n"), client->id());
        if (client->_tempObject) {
            delete (WebSocketIncommingBuffer *) client->_tempObject;
        }
        wifiReconnectCheck();

    } else if(type == WS_EVT_ERROR) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u error(%u): %s\n"), client->id(), *((uint16_t*)arg), (char*)data);

    } else if(type == WS_EVT_PONG) {
        DEBUG_MSG_P(PSTR("[WEBSOCKET] #%u pong(%u): %s\n"), client->id(), len, len ? (char*) data : "");

    } else if(type == WS_EVT_DATA) {

        WebSocketIncommingBuffer *buffer = (WebSocketIncommingBuffer *)client->_tempObject;
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        buffer->data_event(client, info, data, len);

    }

}

void _wsLoop() {
    if (!wsConnected()) return;
    _wsDoUpdate();
}





bool wsConnected() {
    return (_ws.count() > 0);
}

bool wsConnected(uint32_t client_id) {
    return _ws.hasClient(client_id);
}

void wsOnSendRegister(ws_on_send_callback_f callback) {
    _ws_on_send_callbacks.push_back(callback);
}

void wsOnReceiveRegister(ws_on_receive_callback_f callback) {
    _ws_on_receive_callbacks.push_back(callback);
}

void wsOnActionRegister(ws_on_action_callback_f callback) {
    _ws_on_action_callbacks.push_back(callback);
}

void wsSend(ws_on_send_callback_f callback) {
    if (_ws.count() > 0) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        callback(root);

        wsSend(root);
    }
}

void wsSend(const char * payload) {
    if (_ws.count() > 0) {
        _ws.textAll(payload);
    }
}

void wsSend_P(PGM_P payload) {
    if (_ws.count() > 0) {
        char buffer[strlen_P(payload)];
        strcpy_P(buffer, payload);
        _ws.textAll(buffer);
    }
}

void wsSend(uint32_t client_id, ws_on_send_callback_f callback) {
    AsyncWebSocketClient* client = _ws.client(client_id);
    if (client == nullptr) return;

    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    callback(root);

    size_t len = root.measureLength();
    AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len);

    if (buffer) {
        root.printTo(reinterpret_cast<char*>(buffer->get()), len + 1);
        client->text(buffer);
    }
}

void wsSend(uint32_t client_id, const char * payload) {
    _ws.text(client_id, payload);
}

void wsSend_P(uint32_t client_id, PGM_P payload) {
    char buffer[strlen_P(payload)];
    strcpy_P(buffer, payload);
    _ws.text(client_id, buffer);
}

void wsSetup() {

    _ws.onEvent(_wsEvent);
    webServer()->addHandler(&_ws);


    const String webDomain = getSetting("webDomain", WEB_REMOTE_DOMAIN);
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", webDomain);
    if (!webDomain.equals("*")) {
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Credentials", "true");
    }

    webServer()->on("/auth", HTTP_GET, _onAuth);

    #if MQTT_SUPPORT
        mqttRegister(_wsMQTTCallback);
    #endif

    wsOnSendRegister(_wsOnStart);
    wsOnReceiveRegister(_wsOnReceive);
    espurnaRegisterLoop(_wsLoop);
}

#endif