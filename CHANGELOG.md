# ESPurna change log

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [1.9.7] 2017-10-25
### Fixed
- Fix Alexa interface switching on all lights (#256)

## [1.9.6] 2017-10-23
### Fixed
- Fix power report in Domoticz (#236)
- Fix Sonoff POW in AP mode (#241)
- Fix Home Automation auto-discovery (support for single relay switches and RGB lights, #235)
- Check WS authentication only on start event

### Added
- Support for 2.4.0 RC2 Arduino Core that fixes KRACK vulnerablity (pre-built images are compiled against this, #242)
- Support for ManCaveMade ESPLive board (thanks to Michael A. Cox)
- Support for InterMIT Tech QuinLED 2.6 (thanks to Colin Shorts)
- Support for Magic Home LED Controller 2.0 (thanks to users @gimi87 and @soif, #231)
- Support for Arilux AL-LC06 (thanks to Martijn Kruissen)
- Support for Xenon SM-PW702U Wifi boards (thanks to Joshua Harden, #212)
- Support for Authometion LYT8266 (testing, thanks to Joe Blellik, #213)
- Support for an external button for D1 Mini boards (thanks to user @PieBru, #239)
- Option to query relay status via MQTT or WS (thanks to Wesley Tuzza)
- Automatically install dependencies for web interface builder (thanks to Hermann Kraus)
- Support for HSV and IR for Magic Home LED Controller (optional, disabled by default, thanks to Wesley Tuzza)
- Added option to report DS18B20 temperatures based on changes (thanks to Michael A. Cox)
- Safer buffer handling for websocket data (thanks to Hermann Kraus & Björn Bergman)
- Updates HL8012 library with energy counting support (thanks to Hermann Kraus)
- Added option to disable light color persistence to avoid flickering (#191)
- Option to enable TELNET in STA mode from web UI (#203)

### Changed
- Changed default MQTT base topic to "{identifier}" (no leading slashes, #208)
- Prevent reconnecting when in AP mode if a web session or a telnet session is active (#244)
- Web UI checks for pending changes before reset/reconnect options (#226)
- Increase WIFI connect timeout and reconnect interval

## [1.9.5] 2017-09-28
### Fixed
- Revert to JustWifi 1.1.4 (#228)

## [1.9.4] 2017-09-22
### Added
- Added ESPurna specific mDNS text registers (app_name, app_version, device_name)
- Crash dump info is stored in EEPROM and retrieved via terminal ("crash" command)
- Support for Huacanxing H802
- Support for powermeters based on V9261F IC
- Support for powermeters based on ECH1560 IC (beta, untested)

### Changed
- Changed behaviour on MQTT connection failure (#215)
- Removed boot delay
- Refactor power modules
- Updated JustWifi library

### Fixed
- Set all esp8285 devices to use esp01_1m (#210, #225)
- Removed wifi gain option since it prevents some devices to connect (#204)

## [1.9.3] 2017-09-04
### Added
- New "erase.config" option in terminal to delete SDK settings
- Added error code to error message when updating from web UI
- Fixed Web UI to be behind a proxy (http://tinkerman.cat/secure-remote-access-to-your-iot-devices/)
- Support "ON", "OFF" and "TOGGLE" (also lowercase) as payload in relay MQTT, API and WS (http://tinkerman.cat/using-google-assistant-control-your-esp8266-devices/)

### Changed
- Updated fauxmoESP library to 2.2.0

### Fixed
- Fix HLW8012 calibration (#194)
- Fix telnet dropping connection
- Fix WiFiSecureClient connection with PubSubClient (#64)

## [1.9.2] 2017-08-31
### Added
- System stability check (turns off everything except WIFI AP, OTA and telnet if there is a boot crash loop) (#196)
- Telnet support (enabled by default only on AP interface)
- Option to set WiFi gain from web UI
- Option to disable MQTT from web UI
- MQTT autodiscover, with the option to autoconnect if no broker defined
- Home Assistant MQTT autodiscover feature
- List enabled modules in INIT debug info
- Counter module (counts and reports transitions in a digital pin)

### Changed
- Updated NoFUSS support
- Web UI documentation changes
- Changes in terminal commands ("reconnect" is now "reset.wifi", also new commands added)

### Fixed
- Crash in settings saving (#190) and fixed UDP debug conditional build clauses

## [1.9.1] 2017-08-27
### Added
- Support to build without NTP support
- Added current time, uptime, free heap, firmware size and free space to web interface

### Changed
- Changed settings keys for Itead Sonoff RF Bridge
- Disable Domoticz by default

### Fixed
- Fixed build flags for DHT and DS18B20 in platformio.ini file
- Fixed Itead Sonoff B1 by updating the my9291 library
- Fixed light status on boot (#157)
- Fixed CSS bug cause by a bad merge

## [1.9.0] 2017-08-25
### Added
- Support for IteadStudio BN-SZ01 Ceiling Light (#132)
- Support for IteadStudio Sonoff RF Bridge (#173)
- Support for IteadStudio Sonoff 4CH Pro (#174)
- Support for IteadStudio Sonoff B1
- Support for IteadStudio Sonoff LED
- Support for IteadStudio Sonoff T1 wall switches (1, 2 and 4 channels)
- Support for WiOn 50055 WiFi Wall Outlet & Tap
- Support for EXS WiFi Relay v3.1 (and other future latching relay boards) (#152)
- TLS/SSL support for MQTT (caution: eats a lot of memory, do not use with web interface) (#64)
- Add support for delayed ON/OFF switches (#123, #161, #188)
- Added ON and OFF actions for button events (previously only TOGGLE available) (#182)
- Sliders in web interface to control dimmer channels independently (also for brightness)
- Debug info about MQTT disconnect reason

### Changed
- MQTT setters ending with "/set" by default
- Using DOUT flash mode on all devices (#167)
- Longer timeout for WiFi connection (better chances for Sonoff Basic to connect)
- Changed MQTT topics for light devices (COLOR, BRIGHTNESS, MIRED, KELVIN, CHANNEL) (#144)
- Changed the way light devices are defined (see LIGHT_PROVIDER_DIMMER)
- Allow to disable color picker in web interface
- API returns processed values for HLW8012 sensor (not raw values anymore) (#176)
- Major refactoring of settings

### Fixed
- Discard MQTT messages with empty payload (#185)
- Wifi connection issue (https://github.com/esp8266/Arduino/issues/2186)
- Alexa connection issue

## [1.8.3] 2017-07-23
### Added
- Issue #85 and #90. Option to report MQTT messages with JSON payloads
- Issue #170. Updated DebouceEvent library to allow disabling double click and get faster click responses
- Using memory layout with no SPIFFS for 1Mb devices

### Changed
- Rename settings s/POW/HLW8012/
- Return times in ISO8601 format

### Fixed
- Issue #168. Added H801 to arduino.h file
- Issue #171. Fix corrupted will message

## [1.8.2] 2017-07-16
### Added
- InfluxDB support via HTTP API
- Added custom reset reason to debug log
- Enable WIFI debug on hardware reset (button long click)

### Changed
- Issue #159. Allow decimals in relay pulse interval
- Updated HLW8012 library

### Fixed
- Issue #148. Fix bug in conditional compilation check
- Issue #149. Using different pulse counters for each relay (thanks to Lauris Ieviņš)
- Issue #141. Limit relay pulse interval to 60s
- Fixed units for apparent & reactive power (thanks to Lauris Ieviņš)
- Fixed mDNS setup when using custom HTTP port for web interface

## [1.8.1] 2017-05-22
### Fixed
- Issue #140. Fix no relay control bug in Sonoff Dual

## [1.8.0] 2017-05-21
### Added
- Added gamma correction to RGB strips. Thanks to Chris Ward.
- Added support for Huacanxing H801 WiFi LED Controller. Thanks to Minh Phuong Ly.
- Issue #138. Added NTP configuration from web interface
- Issue #128. Report color when booting and in heartbeat stream.
- Issue #126. Show NTP status in web interface.
- Added filter limits on POW readings.
- Added color temperature to RGB calculation. Thanks to Sacha Telgenhof.
- Issue #120. Added relay flood protection. Thanks to Izik Dubnov.
- Support for "#RRGGBB", "RRR,GGG,BBB" and "WWW" color formats.
- Issue #117. Added build date & time to web interface.

### Fixed
- Fix MQTT_RELAY board conifugration. Thanks to Denis French.
- Issue #125. Fix bug in relay status reading from EEPROM
- Issue #127. Fix button action in DUAL.
- Fix bug in Sonoff POW current reading. Thanks to Emmanuel Tatto.
- Minimizing my9291 flickering when booting.
- Fix conditional flags in hardware.ino to support Arduino IDE.

## [1.7.1] 2017-03-28
### Fixed
- Issue #113. Fix restoring color from EEPROM upon reboot
- Issue #113. Fix bug in API handlers

## [1.7.0] 2017-03-27
### Added
- Web interface embedded in firmware image by default
- Upload firmware image from web interface
- Added API entry point to change light color
- Added generic analog sensor. Thanks to Francesco Boscarino
- Report RSSI value in debug console and MQTT status messages
- Added support for Magic Home LED Controller
- Added support for ESPurna-H Board (based on HLW8012)
- Added forward compatible code for v2.0

### Changed
- Added ellipsis (...) in debug messages longer than 80 characters
- Changed topic constants in code
- Prevent the SDK from saving WiFi configuration to flash

### Fixed
- Issue #113. Fix light bulb state to OFF in library prevented the bulb from turning on
- Issue #58. Added code to handle spurious readings
- Fix bug in HLW8012 calibration current parameter casting to int instead of float
- Issue #115. Removed local declaration of _mqttForward variable. Thanks to Paweł Fiedor
- Fix MQTT will topic. Thanks to Asbjorn Tronhus

## [1.6.9] 2017-03-12
### Added
- Two stage read for DS18B20 devices. Thanks to Izik Dubnov.
- Option to report the relay status via MQTT periodically
- Terminal commands to change relay status an light color
- Added debug via UDP (disabled by default)
- Moved debug strings to PROGMEM. ~1.5KByes memory freed
- Avoid broadcasting websocket messages if no clients connected

### Fixed
- Fixing use after free bug that leads to corrupted auth credentials. Thanks to David Guillen

## [1.6.8] 2017-03-01
### Added
- Issue #85. Heartbeat reports now free heap, uptime and VCC every 5 minutes

### Changed
- Wait two minutes instead of one in AP mode before trying to reconnect to the router
- Issue #92. Debug log enabled by default in Arduino IDE
- Issue #91. Using AsyncMqttClient as default MQTT client again

### Fixed
- Report data from all sensors via websocket even if no MQTT connection
- Issue #92. Fix unknown reference in Arduino IDE
- Split data.h contents into 1k lines, otherwise Arduino IDE chokes on them
- Discard empty MQTT topic while subscribing

## [1.6.7] 2017-02-25
### Added
- Support for OpenLight / AI-Light by AI-Thinker based on MY9291 LED driver
- Issue #87. Factory reset when physical button pressed for >10 seconds

## [1.6.6] 2017-02-23
### Fixed
- Issue #82. Fix critical bug on Sonoff Dual

## [1.6.5] 2017-02-22
### Added
- Option to backup and restore settings from the web interface
- Footer in the web interface

### Changed
- Using PubSubClient as MQTT client by default (please read the documentation)
- Double & long clicks do nothing except for the first defined button

### Fixed
- Issue #79. Fix bug in WiFi led notification & MQTT connectivity (using PubSubClient)
- Issue #73. Fix bug when building without Domoticz support
- Fix Gulp tasks dependencies

## [1.6.4] 2017-02-20
### Added
- Option to embed the web interface in the firmware, disabled by default
- Change relay status with a GET request (browser friendly)
- Support for PROGMEM debug messages (only wifi module has been changed)
- Option to disable mDNS, enabled by default
- Show current web server port in debug log
- Issue #75. Link relays to LEDs
- Issue #76. Using http://espurna.local when in AP mode

### Changed
- Images and favicon is now embedded in the HTML
- Authentication challenge only in /auth request. All static contents are un-authenticated
- HTTP response code when out of websocket slots changed from 423 to 429

### Fixed
- Memory leak in MQTT connection method
- Wait 60 seconds before retrying to connect when in AP mode
- Issue #24 & #74. Update ESPAsyncTCP and ESPAsyncWebServer to latest GIT version that supports MSS defragmenting
- Issue #73. Fixes for windows machines

### Removed
- Captive portal removed, mDNS resolution for AP mode too

## [1.6.3] 2017-02-15
### Added
- Issue #69. Temperature unit configuration from the web interface
- Issue #55. WebServer port configurable from the web interface, defaults to 80
- Expand network configuration when adding a new network

### Changed
- Merged web contents except images in a single compressed file for reliability
- Update support for Itead Motor Clockwise/Anticlockwise board
- Scan for strongest network only if more than 1 network configured

### Fixed
- Issue #71. Added default values for netmask and DNS in web configuration
- Fixed Itead 1CH self-locking/inching board definition
- Fixed PlatformIO environments for ESP8285 boards (4CH and Touch)

## [1.6.2] 2017-02-10
### Fixed
- Check if there is an MQTT broker defined before the MQTT_MAX_TRIES check

## [1.6.1] 2017-02-10
### Added
- Added support for [Jorge Garcia's Wifi+Relay Board Kit](https://www.tindie.com/products/jorgegarciadev/wifi--relays-board-kit/)
- Reporting current and energy incrementals to a separate counters in Domoticz (thanks to Toni Arte)
- Force WiFi reconnect after MQTT_MAX_TRIES fails trying to connect to MQTT broker

## [1.6.0] 2017-02-05
### Added
- Added support for toggle switches
- Allow reset the board via an MQTT message
- Allow reset the board via an RPC (HTTP) message
- Added support for ADC121 I2C for current monitoring (Check [http://tinkerman.cat/power-monitoring-sonoff-th-adc121/](http://tinkerman.cat/power-monitoring-sonoff-th-adc121/))
- Reporting voltage to Domoticz (only HLW8012)
- Map button events to actions (toggle relay, AP mode, reset, pulse mode)

### Changed
- Reporting energy incrementals (Domoticz, MQTT)

### Removed
- Removed current monitor bypass when relay is OFF
- Removed energy API entry point

## [1.5.4] 2017-02-03
### Fixed
- Issue #50. Fix type bug in window variable when calculating energy for HLW8012 devices (Sonoff POW)

## [1.5.3] 2017-02-02
### Fixed
- Issue #50 and #54. Fixed domoticz MQTT message format

### Added
- Energy calculation and aggregation. API entry points and MQTT messages.

## [1.5.2] 2017-01-29
### Fixed
- Fix bug in emon topic payload

## [1.5.1] 2017-01-28
### Added
- OpenEnergyMonitor WiFi MQTT Relay / Thermostat support (thanks to Denis French)

### Fixed
- NTP connection refresh upon wifi connection
- Filesystem image build using local gulp installation

## [1.5.0] 2017-01-21
### Added
- Pulse mode. Allows to define a pulse time after which the relay will switch back
- API entry points for sensor data (power, current, voltage, temperature and humidity)
- Export sensor data to Domoticz (power, current, voltage, temperature and humidity)
- Configurable (in code) mapping between buttons and relays
- MQTT messages for button events
- Added support for Itead Studio 1CH inching/self locking smart switch board
- Added support for Jan Goedeke Wifi Relay boards (both NC and NO versions)
- Notify OTA updates to websocket clients, automatically reload page
- Support for pulse mode notification LED and button
- Revert relay state mode on boot (thanks to Minh Phuong Ly)

### Fixed
- MQTT will topic
- Crash with HLW812 interrupts while trying to create a WIFI connection
- Issue #20 Better inline documentation for Alexa and Domoticz default settings
- Issue #39 Fixed autoconnect issue with static IP (fixed in JustWifi library)
- Issue #41 Added password requirements to initial password change page

### Changed
- Changed LED pattern for WIFI notifications (shorter pulses)

## [1.4.4] 2017-01-13
### Added
- Adding current, voltage, apparent and reactive power reports to Sonoff POW (Web & MQTT)

### Fixed
- Issue #35 Fixed frequent MQTT connection drops after WIFI reconnect
- Defer wifi disconnection from web interface to allow request to return

### Changed
- Move all Arduino IDE configuration values to their own file
- Using latest HLW8012 library in interrupt mode

## [1.4.3] 2017-01-11
### Fixed
- Issue #6 Using forked Time library to prevent conflict with Arduino Core for ESP8266 time.h file in windows machines

## [1.4.2] 2017-01-09
### Added
- Support for inverse logic relays

### Fixed
- Issue #31. Fixed error in relay identification from MQTT messages

## [1.4.1] 2017-01-05
### Added
- Alexa support by default on all devices
- Added support for Wemos D1 Mini board with official Relay Shield

### Fixed
- Multi-packet websocket frames

## [1.4.0] 2016-12-31
### Added
- Domoticz support via MQTT (https://www.domoticz.com/wiki/MQTT)
- Support for static IP connections

### Fixed
- Issue #16. Enforce minimum password strength in web interface

### Changed
- Using default client_id provided by AsyncMqttClient
- Allow up to 5 different WIFI networks

### Removed
- File system version file

## [1.3.1] 2016-12-31
### Fixed
- data_dir fix for PlatformIO

## [1.3.0] 2016-12-30
### Changed
- Arduino IDE support (changes in the folder structure and documentation)

## [1.2.0] 2016-12-27
### Added
- Force password changing if it's the default one
- Added Last-Modified header to static contents
- Added DNS captive portal for AP mode
- Added support for Sonoff 4CH
- Added support for WorkChoice ecoPlug (ECOPLUG). Thanks to David Myers
- Added support for Sonoff SV
- Added support for Sonoff Touch
- Comment out hardware selection in hardware.h if using Arduino IDE
- Added support for MQTT get/set suffixes (/status, /set, ...)
- Added support for LED notifications via MQTT
- Added EEPROM check commands to terminal interface

### Changed
- Using unreleased AsyncMqttClient with stability improvements
- Better decoupling between MQTT and relays/websockets
- Skipping retained MQTT messages (configurable)

### Fixed
- Issue #11 Compile error when building sonoff-dual-debug
- Issue #14 MQTT Connection with Username an Password not working
- Issue #17 Moved static variable 'pending' to class variable

## [1.1.0] 2016-12-06
### Added
- Added support for DS18B20 temperature sensor. Thanks to Francesco Boscarino
- Added reset command from console
- Added support for multirelay boards like Sonoff DUAL or Electrodragon ESP Relay Board

### Changed
- Not using espressif8266_stage in default environment
- Relay MQTT topics
- API entry points

### Removed
- Old non protected API

## [1.0.3] 2016-11-29

### Added
- WeMo emulation through the fauxmoESP library (control your switch from Alexa!)
- REST API for relay management
- Better dependency definitions in platformio.ini
- Option to define inverse logic to on-board LED
- Built data folder included in repo

### Changed
- Using non-interrupt driven mode for HLW8012
- Better documentation
- Small changes to web interface
- Same admin password for web, OTA and WIFI AP mode

### Fixed
- Prevent fauxmoESP to be compiled by default

### Removed
- Removed ESPurna board to its own repo

## [1.0.1] 2016-11-13

### Added
- Basic authentication and CSRF to websocket requests

## [1.0.0] 2016-11-13

### Added
- Using ESPAsyncWebServer (for web & websockets) and AsyncMqttClient

## [0.9.9] 2016-11-12

### Added
- Preliminary support for Sonoff POW
- Replace AJAX requests with websockets
- Using sprites for images
- Hostname can be changed
- Added initial relay state mode
- Reconnect and reset buttons on web interface

### Changed
- Changed long click to reset and double click to AP mode
- Using officially supported platformio.ini file by default

### Fixed
- Removed unnecessary memory inefficient code
- Temprary fix for Adafruit DHT library (see https://github.com/adafruit/DHT-sensor-library/issues/62)

## [0.9.8] 2016-10-06

### Added
- Using PureCMS for the web interface
- Using gulp to build the filesystem files
- Using Embedis for configuration

### Changed
- Updated JustWifi library
- Web interface changes
- Using custom platformio.ini file
- Loads of changes in modules
- Added DEBUG_MSG

### Fixed
- Clean gulp builder script

## [0.9.7] 2016-08-28

### Changed
- Moving wifi management to library (JustWifi)
- Split code into modules

## [0.9.6] 2016-08-12

### Added
- Added heartbeat, version and fsversion MQTT messages

### Changed
- GZip 3rd party contents

## [0.9.5] 2016-07-31
- Initial stable version
