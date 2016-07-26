#ifndef _AUTO_OTA_h
#define _AUTO_OTA_h

#include <functional>
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>

typedef enum {
    AUTO_OTA_START,
    AUTO_OTA_UPTODATE,
    AUTO_OTA_UPDATING,
    AUTO_OTA_FILESYSTEM_UPDATED,
    AUTO_OTA_FIRMWARE_UPDATED,
    AUTO_OTA_RESET,
    AUTO_OTA_END,
    AUTO_OTA_NO_RESPONSE_ERROR,
    AUTO_OTA_PARSE_ERROR,
    AUTO_OTA_FILESYSTEM_UPDATE_ERROR,
    AUTO_OTA_FIRMWARE_UPDATE_ERROR
} auto_ota_t;

class AutoOTAClass {

  public:

    typedef std::function<void(auto_ota_t)> TMessageFunction;

    void setServer(String server);
    void setModel(String model);
    void setVersion(String version);

    String getNewVersion();
    String getNewFirmware();
    String getNewFileSystem();

    int getErrorNumber();
    String getErrorString();

    void onMessage(TMessageFunction fn);
    void handle();

  private:

    String _server;
    String _model;
    String _version;

    String _newVersion;
    String _newFirmware;
    String _newFileSystem;

    int _errorNumber;
    String _errorString;

    TMessageFunction _callback;

    String _getPayload();
    bool _checkUpdates();
    void _doUpdate();

};

extern AutoOTAClass AutoOTA;

#endif /* _AUTO_OTA_h */
