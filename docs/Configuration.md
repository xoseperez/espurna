# Configuration

*TODO*

## First boot

On normal boot (i.e. button not pressed) it will execute the firmware. It configures the hardware (button, LED, relay), the SPIFFS memory access, the WIFI, the WebServer and MQTT connection and whatever other enabled modules.

Obviously the default values for WIFI network and MQTT will probably not match your requirements. The device will start in Soft AP creating a WIFI SSID named "DEVICE_XXXXXX", where DEVICE will be an identifier of your device and XXXXXX are the last 3 bytes of the radio MAC. Connect with phone, PC, laptop, whatever to that network, password is "fibonacci". Once connected browse to 192.168.4.1. It will then present an **authentication challenge**. Default user and password are "admin" and "fibonacci" (again). Then you will be presented a configuration page where you will be able to define different configuration parameters, including changing the default password. The same password is used for the WIFI Access Point, for the web interface and for the OTA firmware upload.

## WiFi configuration

You can configure up to three WIFI networks. It will then try to connect to the configure WIFI networks in order of signal strength. If none of the 3 attempts succeed it will default to SoftAP mode again. You can also switch to SoftAP mode by double click the on-board button or reset the board long clicking it.

## MQTT

Once connected to an access point (so in station mode) the board will try to connect the MQTT server defined.

The device will publish the relay state to the given topic and it will subscribe to the same topic for remote switching. Don't worry, it avoids infinite loops.

You can also use "{identifier}" as place holder in the topic. It will be translated to your device ID (same as the soft AP network it creates).
