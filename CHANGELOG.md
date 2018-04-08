# ESPurna change log

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [1.12.5] 2018-04-08
### Fixed
- Fixed expected power calibration ([#676](https://github.com/xoseperez/espurna/issues/676))
- Do not show empty time strings ([#691](https://github.com/xoseperez/espurna/issues/691), thanks to @PieBru)
- Fix load average calculation when system check is disabled ([#707](https://github.com/xoseperez/espurna/issues/707))
- Fixed unstability issues with NtpClientLib using temporary fork ([#743](https://github.com/xoseperez/espurna/issues/743))
- Fixed typos in homeassistant module (thanks to @Cabalist)
- Fixed default HLW8012 calibration for KMC devices (thanks to @gn0st1c)
- Fix MQTT query request
- Fix scheduler debug message
- Fix NTP offset value

### Added
- Option to change NTP timeout via compile-time setting ([#452](https://github.com/xoseperez/espurna/issues/452))
- Added humidity correction to web UI ([#626](https://github.com/xoseperez/espurna/issues/626), tahnks to @ManuelW77)
- Added support for USA DST calculation ([#664](https://github.com/xoseperez/espurna/issues/664))
- Option to reset energy count ([#671](https://github.com/xoseperez/espurna/issues/671))
- Added Sonoff SV prebuild image ([#698](https://github.com/xoseperez/espurna/issues/698), thanks to @akasma74)
- Check and remove unused config keys ([#730](https://github.com/xoseperez/espurna/issues/730))
- Visual Studio metadata files added to .gitignore ([#731](https://github.com/xoseperez/espurna/issues/731), thanks to @gn0st1c)
- Added default MQTT and SSL settings to web UI ([#732](https://github.com/xoseperez/espurna/issues/732), thanks to @mcspr)
- Added option to the web UI to set the light transition length in milliseconds ([#739](https://github.com/xoseperez/espurna/issues/739))
- Improved testing with Travis (thanks to @lobradov)
- Change dimmers using schedule (thanks to @wysiwyng)
- Debug console in web UI (thanks to @lobradov), including command execution
- Option to reset relays in MQTT disconection (thanks to @a-tom-s)
- Option to disable system check from custom header (thanks to @phuonglm)
- Added "board" topic to the heartbeat messages (thanks to @mcspr)
- Added methods to create hierarchical MQTT JSON responses
- Added RESET.SAFE command to reboot into safe mode
- Added SDK and Core versions to the web UI
- Added revision to web UI (only when built from build.sh)
- Support for OBI Powerplug Adapter ([#622](https://github.com/xoseperez/espurna/issues/622), thanks to @Geitde)
- Support for Tunbox Powerstrip02 (thanks to @gn0st1c)
- Support for Lingan SWA1 (thanks to @gn0st1c)
- Support for Heygo HY02 (thanks to @gn0st1c)
- Support for Maxcio WUS0025 (thanks to @gn0st1c)
- Support for Yidian XSSSA05 SWA1 (thanks to @gn0st1c)
- Support for ArnieX Swifitch (thanks to @LubergAlexander)
- Support for IKE ESPIKE board
- Support for AM2320 sensors via I2C (thanks to @gn0st1c)
- Support for GUVAS12SD sensor (thanks to @gn0st1c)

### Changed
- Removed hostname size limit ([#576](https://github.com/xoseperez/espurna/issues/576), [#659](https://github.com/xoseperez/espurna/issues/659))
- Reworked RGBW implementation (thanks to @Skaronator)
- Several web UI layout changes (thanks to @lobradov & @mcspr)
- Button MQTT messages will not have the retain flag (thanks to @lobradov)
- Remove unnecessary code from boot log (thanks to @gn0st1c)
- Updated logo and favicon, added gitter channel
- Force reporting power values as 0 if relay is off
- Using gulp-crass for CSS minification
- Using WIFI_NONE_SLEEP by default

## [1.12.4] 2018-03-05
### Fixed
- Adding a 1ms delay after UDP send to avoid loosing packets ([#438](https://github.com/xoseperez/espurna/issues/438))
- Fixed void return in BMX280 sensor ([#489](https://github.com/xoseperez/espurna/issues/489))
- Fix MQTT keep alive cannot be more than 255 seconds ([#515](https://github.com/xoseperez/espurna/issues/515))
- Do not show scheduler tab in Web UI if build without scheduler support ([#527](https://github.com/xoseperez/espurna/issues/527))
- Fix inline documentation for Sonoff 4CH Pro button modes ([#551](https://github.com/xoseperez/espurna/issues/551))
- Prevent resending messages from rfin in RF Bridge ([#561](https://github.com/xoseperez/espurna/issues/561))
- Fix AnalogSensor description ([#601](https://github.com/xoseperez/espurna/issues/601))
- Fixed missing setting in HASS WS callback (thanks to @mcspr)
- ECH1560 call sync from tick method
- Fixed several issues reported by codacy

### Added
- UART to MQTT module (thanks to Albert Weterings, [#529](https://github.com/xoseperez/espurna/issues/529))
- Added option to show HASS configuration code in ESPurna web UI ([#616](https://github.com/xoseperez/espurna/issues/616))
- OTA upgrade via terminal (using 'ota' command, with SSL support)
- Added I2C scan and clear commands to terminal (only when I2C enabled)
- Added new relay & wifi led mode ([#604](https://github.com/xoseperez/espurna/issues/604))
- Option to enable/disable web auth from web UI
- Added "Reset to factory settings" in web UI (thanks to Teo Pavel, [#569](https://github.com/xoseperez/espurna/issues/569))
- Added {magnitude} placeholder to MQTT root topic
- Option to report energy in kWh and power in kW ([#523](https://github.com/xoseperez/espurna/issues/523))
- Check upgrade file size and signature in web UI
- Automatically dump info on telnet connection if TERMINAL_SUPPORT is disabled
- Two different ESPURNA_CORE images for 1MB and 4MB boards, freeing GPIOs ([#557](https://github.com/xoseperez/espurna/issues/557))
- Initial support for PZEM004T sensor (still beta)
- Support for STM_RELAY board (thanks to Maciej Czerniak)
- Support for KMC 70011 energy monitor (thanks to Wayne Manion, [#598](https://github.com/xoseperez/espurna/issues/598))
- Support for Wifi Stecker Shuko device (thanks to @Geitde, [#622](https://github.com/xoseperez/espurna/issues/622))
- Support for GizWits Witty Cloud device (thanks to Theonedemon)

### Changed
- BMX280 changes to allow for hot-plug ([#353](https://github.com/xoseperez/espurna/issues/353))
- Increase the initial check interval for NTP ([#452](https://github.com/xoseperez/espurna/issues/452))
- Force turning relays off before turning others on when synced ([#491](https://github.com/xoseperez/espurna/issues/491))
- Publish slampher as light to Home Assistant ([#494](https://github.com/xoseperez/espurna/issues/494))
- Force API to return the target status of the relay ([#548](https://github.com/xoseperez/espurna/issues/548))
- Increasing max number of messages in JSON payload to 20 ([#588](https://github.com/xoseperez/espurna/issues/588))
- Change copy from 'Use colorpicker' to 'Use color'. Better hint. ([#590](https://github.com/xoseperez/espurna/issues/590))
- Completely reworked the RF module to use the same web UI as the RFBridge module to learn new codes ([#594](https://github.com/xoseperez/espurna/issues/594))
- Several spelling and grammar changes by Lee Marlow
- Always enabled telnet access in ESPURNA_CORE image
- Updated ESPSoftwareSerial, ESPAsyncTCP and ESPAsyncWebServer libraries

### Removed
- Remove dependency from gulp-util ([#493](https://github.com/xoseperez/espurna/issues/493))
- Removed specific support for Magic Home LED Controller 2.3 ([#512](https://github.com/xoseperez/espurna/issues/512))
- Disabled floating point support when building against Arduino Core 2.4.0 with PIO
- Removed WiFi distance calculation

## [1.12.3] 2018-01-29
### Fixed
- Fix telnet crash due to local reference ([#487](https://github.com/xoseperez/espurna/issues/487))

## [1.12.2] 2018-01-29
### Added
- Repository migrated over to GitHub
- Travis CI build test and deploy
- Pre-commit hook to change README.md file depending on the branch
- {hostname} and {mac} placeholders for MQTT root topic
- Added support for timezones with minutes ([#265](https://github.com/xoseperez/espurna/issues/265))
- SSDP support ([#282](https://github.com/xoseperez/espurna/issues/282), [#423](https://github.com/xoseperez/espurna/issues/423), disabled by default since current implementation is not compatible with Alexa [#479](https://github.com/xoseperez/espurna/issues/479))
- HA auto-discover for multi-relay boards and sensors ([#392](https://github.com/xoseperez/espurna/issues/392), [#465](https://github.com/xoseperez/espurna/issues/465))
- Reset the pulse timeout every time an MQTT message is sent with the non-normal payload value ([#454](https://github.com/xoseperez/espurna/issues/454))
- Option to disable schedules without deleting them ([#453](https://github.com/xoseperez/espurna/issues/453))
- Added LED_MODE_STATUS ([#458](https://github.com/xoseperez/espurna/issues/458))
- Support to set on/off state per channel using switches ([#457](https://github.com/xoseperez/espurna/issues/457))
- Added support for MagicHome LED Controller 2.3
- Alexa message queue (thanks to Qubeck)
- Secondary Serial RX port for H801 and H802 boards ([#386](https://github.com/xoseperez/espurna/issues/386), thanks to Pablo Pousada Rial)
- Added compatibility with https://github.com/rhx/RF-Bridge-EFM8BB1 to RF Bridge (Thanks to Rene Hexel)
- Added message queue to RF Bridge
- Added MAC to mDNS text fields
- Added wifi.ap command to go into AP mode
- Added message id on MQTT JSON payloads
- Added hooks for 3rd party code (custom modules)
- Local broker to broadcast messages internally
- Added timestamp to debug output
- Common I2C interface to abstract backend library (Wire or Brzo I2C)
- Added espurnaLoopRegister

### Fixed
- Fixed support for 4CH Pro different modes ([#333](https://github.com/xoseperez/espurna/issues/333))
- Fixed several sensor modules to enable hot-unplug-plug ([#398](https://github.com/xoseperez/espurna/issues/398))
- Fixed crash when calling idbSend from an MQTT callback ([#410](https://github.com/xoseperez/espurna/issues/410))
- Checking trailing slash in mqttTopic ([#422](https://github.com/xoseperez/espurna/issues/422))
- Fixed pulse and pulse_ms order in relay_t structure ([#424](https://github.com/xoseperez/espurna/issues/424))
- Use same buffer size across all terminal-realted classes/methods. Set to 128 chars ([#477](https://github.com/xoseperez/espurna/issues/477), [#478](https://github.com/xoseperez/espurna/issues/478))
- Fix WiFi scan status in web UI
- Several code quality fixes (thanks to @lobradov)
- Fixed error message on first command over telnet

### Changed
- BMX280 sensor module now doesn't depend on third party libraries
- Changed time management in ntp, mqtt and scheduler modules

## Deprecated
- {identifier} placeholder for MQTT root topic

## [1.12.1] 2018-01-14
### Added
- Option to perform a WiFi network scan from web UI
- Added hostname to web UI side menu ([#404](https://github.com/xoseperez/espurna/issues/404))
- Option to flash multiple devices with ESPurna OTA Manager

### Fixed
- Fix web UI layout so signature does not overlay buttons ([#396](https://github.com/xoseperez/espurna/issues/396))
- Option to disable network scan and allow connecting to hidden SSID ([#392](https://github.com/xoseperez/espurna/issues/392), [#399](https://github.com/xoseperez/espurna/issues/399))
- Fix crash caused by a delay in UDP debugging code ([#397](https://github.com/xoseperez/espurna/issues/397))
- Fix memory leak in influxDB module ([#410](https://github.com/xoseperez/espurna/issues/410))
- Fix typos in web UI ([#394](https://github.com/xoseperez/espurna/issues/394), [#421](https://github.com/xoseperez/espurna/issues/421))

### Changed
- Updated to fauxmoESP 2.4.2
- Changed default I2C GPIO for Wemos D1 ([#420](https://github.com/xoseperez/espurna/issues/420))
- Some terminal commands have changed. See docs or type "help".

## [1.12.0] 2018-01-11
### Added
- Scheduler (contributed by Stefano Cotterli, thank you!, [#131](https://github.com/xoseperez/espurna/issues/131))
- Added "wifi.scan" command to terminal
- Added ESPurna Switch board support
- Added support for python3 in memanalyzer and ota scripts (thanks to @Cabalist)
- Added BSSID, RSSI, channels and distance to web UI status tab
- Added mDNS name resolving to MQTT, InfluxDB and NoFUSS modules ([#129](https://github.com/xoseperez/espurna/issues/129), disabled by default)

### Fixed
- Update FauxmoESP library to 2.4.1, solves dependency issue ([#388](https://github.com/xoseperez/espurna/issues/388))
- Fixed hardware definition in Sonoff Basic and Dual R2 causing wrong relay state on boot ([#365](https://github.com/xoseperez/espurna/issues/365))

### Changed
- Removed auto-recursion check in Domoticz module ([#379](https://github.com/xoseperez/espurna/issues/379))
- Rename terminal commands: reset.wifi to wifi.reset, reset.mqtt to mqtt.reset.
- Update JustWifi library to 1.1.6 (support for multiple SSIDs with the same name)
- Changed the way Home Assistant module handles disabling auto-discovery ([#383](https://github.com/xoseperez/espurna/issues/383))

## [1.11.4] 2018-01-09
### Fixed
- Fix bug in RF Bridge when RF code contains the stop byte. Check overflow ([#357](https://github.com/xoseperez/espurna/issues/357))
- Fixed typos in code and wiki (Thanks to @Cabalist)
- Fix bug in magnitude topic and units ([#355](https://github.com/xoseperez/espurna/issues/355))

### Added
- Small core build to allow two-step flashing method for big binaries
- Thingspeak support ([#371](https://github.com/xoseperez/espurna/issues/371), disabled by default)
- Color synchronization between lights using MQTT ([#362](https://github.com/xoseperez/espurna/issues/362))
- Support for Arilux AL-LC02 ([#347](https://github.com/xoseperez/espurna/issues/347))
- Support for Tarpuna Shield for Wemos D1
- Build option to disable password checking ([#373](https://github.com/xoseperez/espurna/issues/373))
- Option to report sensor address via MQTT ([#377](https://github.com/xoseperez/espurna/issues/377), I2C address, GPIO, Dallas address,...)
- Added binary size to memanalyzer script
- Option to specify custom client ID for MQTT connection ([#368](https://github.com/xoseperez/espurna/issues/368))
- Cross-platform ESPurna OTA Manager implemented in python (untested)
- Terminal command to get or set digital GPIO

### Changed
- Using 2.3.0 for prebuilt binaries
- Fix delay in DHT sensor
- Allow MQTT keep alive value of up to 3600s
- Changed Sonoff 4CH Pro definitions to support built-in interlock mode ([#333](https://github.com/xoseperez/espurna/issues/333))

## [1.11.3] 2018-01-02
### Fixed
- Fix uninitialized PWM channels bug ([#356](https://github.com/xoseperez/espurna/issues/356))

### Added
- Added memory analyzer

## [1.11.2] 2017-12-30
### Fixed
- Fix my92xx and pwm references for Arduino IDE ([#346](https://github.com/xoseperez/espurna/issues/346))
- Fix SHT3X I2C sensor magnitude count ([#337](https://github.com/xoseperez/espurna/issues/337))
- Fix timing for DHT11 sensors ([#294](https://github.com/xoseperez/espurna/issues/294))
- Fix overflow in relayParsePayload with long MQTT messages ([#344](https://github.com/xoseperez/espurna/issues/344))
- Fix loading of Dallas and DHT sensors for Sonoff TH images ([#352](https://github.com/xoseperez/espurna/issues/352))
- Subscribe to Domoticz MQTT topics only if Domotic< is enabled

### Added
- Added option to change MQTT retain flag, QoS and keepalive time from webUI ([#321](https://github.com/xoseperez/espurna/issues/321))
- Added LED modes "always off" and "always on" ([#348](https://github.com/xoseperez/espurna/issues/348))
- Defined new ESPurna switch (no HLW8012 support & touch button ready)

### Changed
- Stop requiring definition of boards in migrate module

## [1.11.1] 2017-12-29
### Fixed
- Fixed relay status on reboot

### Added
- Added support for Arilux AL-LC01 and AL-LC11
- Added support for BH1750 luminosity sensor
- Added automatic memory size identification in ota_flash script

## [1.11.0] 2017-12-28
### Fixed
- Fixed Arduino IDE compilation issues ([#330](https://github.com/xoseperez/espurna/issues/330))
- Fixed issues with IE
- Fixed websocket auth issue with Safari (temporary)
- Fixed MQTT group sync when different switches share same group
- Fixed casting issue in buttonStore ([#327](https://github.com/xoseperez/espurna/issues/327))
- Fixed crash in InfluxDB initial heartbeat ([#318](https://github.com/xoseperez/espurna/issues/318))
- Fixed LED logic for ESPurna H08 board

### Added
- New sensors module (major change)
  + Existing sensor have been migrated: EMON*, ECH1560, V9261F, HLW8012, DHT, DALLAS, ANALOG, DIGITAL and EVENTS
  + New sensor have bee added: BMP280/BME280, EMON over ADS1115, MHZ19, PMSX003 (thanks to Òscar Rovira), SHT3X over I2C and SI7021
- Option to change boot and pulse modes per relay from the web UI
- Option to select sensor read interval and report interval from web UI
- Itead RF Bridge
  + Match MQTT RFOUT codes to relays
  + Force RFBridge to send messages even if switch is already in requested state ([#324](https://github.com/xoseperez/espurna/issues/324))
  + Implemented RFbridge message queue asynchronously
- Added option to load config via HTTP POST & reset ([#335](https://github.com/xoseperez/espurna/issues/335))
- Added option to define behaviour of the first LED between WIFI, MQTT, FIND-ME ([#317](https://github.com/xoseperez/espurna/issues/317))
- Added HTML linter to gulp builder
- Added Help command on terminal ([#338](https://github.com/xoseperez/espurna/issues/338))
- Added preliminary support for SSDP (untested, disabled by default) ([#282](https://github.com/xoseperez/espurna/issues/282))
- Reporting NTP datetime on MQTT heartbeat (thanks to Eldon R. Brown)
- Added version tracking and migration code
- I2C and GPIO locking features
- Changed default button action for touch button devices (TOUCH and T1) ([#327](https://github.com/xoseperez/espurna/issues/327))
- Generic 8 channel board ([#336](https://github.com/xoseperez/espurna/issues/336))

### Changed
- Added more sensor data filters (Max, MobileAverage)
- Changed max pulse time to 1h ([#316](https://github.com/xoseperez/espurna/issues/316))
- Renamed "reset" to "reboot" for clarity ([#315](https://github.com/xoseperez/espurna/issues/315))
- UI refactor
- Change apiRegister signature

## [1.10.1] 2017-12-05
### Fixed
- Fix Sonoff RFBridge learn message from web UI ([#287](https://github.com/xoseperez/espurna/issues/287))
- Fix unstability in "one and just one" sync mode ([#290](https://github.com/xoseperez/espurna/issues/290))
- Fix unnecessary inclusion of my92xx library ([#293](https://github.com/xoseperez/espurna/issues/293))
- Limit the MQTT queue to 10 messages when "Use JSON payload" enabled ([#296](https://github.com/xoseperez/espurna/issues/296))
- Fix Sonoff RFBridge OFF button toggling switch ([#303](https://github.com/xoseperez/espurna/issues/303))
- Allow defining only ON or OFF codes in Sonoff RFBridge ([#304](https://github.com/xoseperez/espurna/issues/304))
- Disabled terminal support for Sonoff Dual ([#310](https://github.com/xoseperez/espurna/issues/310))

### Added
- Support for SI7021-based sensor by Itead Studio compatible with Sonoff TH ([#216](https://github.com/xoseperez/espurna/issues/216))
- Support for Sonoff Dual R2 ([#286](https://github.com/xoseperez/espurna/issues/286))
- MQTT group topics (sync two or more switches from different devices, [#300](https://github.com/xoseperez/espurna/issues/300))
- Color transitions (enabled by default, can be disabled from web UI)
- Option to disable MQTT support at build time

### Changed
- Decreased PWM frequency for dimmer lights
- Changed password policy ([#297](https://github.com/xoseperez/espurna/issues/297))

## [1.10.0] 2017-11-26
### Fixed
- Temperatures with 1 decimal resolution
- Issues with Sonoff B1 due to bad driver management (using my92xx library now)
- Avoid recursive messages on Domoticz ([#272](https://github.com/xoseperez/espurna/issues/272))
- Fixed Sonoff T1 configuration
- Simplify and fix web auth ([#284](https://github.com/xoseperez/espurna/issues/284))
- Fix Embedis custom parser

### Added
- Added option to define a temperature correction factor (thanks to Pawel Raszewski)
- Option to disable system check on build time
- Power saving features (loopDelay and wifi sleep)
- Added Sonoff TH build environment
- Send Home Assistant auto discover messages on connect ([#279](https://github.com/xoseperez/espurna/issues/279))
- Implemented Home Assistant availability topic ([#280](https://github.com/xoseperez/espurna/issues/280))
- Update time, uptime and heap on webUI every heartbeat
- Support for LLMNR and NetBIOS ([#282](https://github.com/xoseperez/espurna/issues/282))
- Added I2C clean bus code
- Added realm to auth challenge

### Changed
- Changed default hostname to "ESPURNA_XXXXXX"
- Binaries built against stable core (~40Kb less, [#274](https://github.com/xoseperez/espurna/issues/274))
- Enabled TERMINAL_SUPPORT for Sonoff Dual (only available via TELNET)
- Dinamically resize debug strings (now messages are not cropped)
- MQTT: unsubscribe to '#' before subscribing
- Updated ESPAsyncWebServer and ESPAsyncTCP libraries
- Removed InfluxDB support by default
- Using stock slider in webUI to reduce size
- Unify DHT and DS18B20 code, show NOT CONNECTED on webUI

## [1.9.9] 2017-11-09
### Fixed
- Fixed bug in MY9291-based light bulbs at full brightness

### Added
- RFBridge: toggle when RF codes for ON and OFF are the same ([#270](https://github.com/xoseperez/espurna/issues/270))
- Support for HSV color schema (MQTT, API and webUI via a selector)

### Changed
- "COLOR" entry point deprecated, use "RGB" instead (MQTT and API, ex. topic "light/rgb/set" instead of "light/color/set")

## [1.9.8] 2017-11-08
### Fixed
- Removed dimmer lights flicker when saving to EEPROM ([#191](https://github.com/xoseperez/espurna/issues/191))
- Fixed low brightness in dimmer lights ([#157](https://github.com/xoseperez/espurna/issues/157))
- Fixed blank fields in energy ([#258](https://github.com/xoseperez/espurna/issues/258), [#259](https://github.com/xoseperez/espurna/issues/259))
- Fixed support for Arilux AL-LC06
- Updated fauxmoESP library with support for GetBinaryState actions

### Added
- Support for IR remotes
- Option to select power read and report interval from webUI
- Option to report real-time values in API, configurable via webUI
- Support for ESPurna-H Board v0.8
- Support for Arilux E27 light bulb (untested)
- Support for YJZK 2-gang switch

### Changed
- PWM using ESP8266_new_pwm by Stephan Bruens (https://github.com/StefanBruens/ESP8266_new_pwm)
- Using own DHT implementation (removed dependency on Adafruit libraries)
- Disabled serial debug for Sonoff RFBridge

## [1.9.7] 2017-10-25
### Fixed
- Fix Alexa interface switching on all lights ([#256](https://github.com/xoseperez/espurna/issues/256))

## [1.9.6] 2017-10-23
### Fixed
- Fix power report in Domoticz ([#236](https://github.com/xoseperez/espurna/issues/236))
- Fix Sonoff POW in AP mode ([#241](https://github.com/xoseperez/espurna/issues/241))
- Fix Home Automation auto-discovery (support for single relay switches and RGB lights, [#235](https://github.com/xoseperez/espurna/issues/235))
- Check WS authentication only on start event

### Added
- Support for 2.4.0 RC2 Arduino Core that fixes KRACK vulnerablity (pre-built images are compiled against this, [#242](https://github.com/xoseperez/espurna/issues/242))
- Support for ManCaveMade ESPLive board (thanks to Michael A. Cox)
- Support for InterMIT Tech QuinLED 2.6 (thanks to Colin Shorts)
- Support for Magic Home LED Controller 2.0 (thanks to users @gimi87 and @soif, [#231](https://github.com/xoseperez/espurna/issues/231))
- Support for Arilux AL-LC06 (thanks to Martijn Kruissen)
- Support for Xenon SM-PW702U Wifi boards (thanks to Joshua Harden, [#212](https://github.com/xoseperez/espurna/issues/212))
- Support for Authometion LYT8266 (testing, thanks to Joe Blellik, [#213](https://github.com/xoseperez/espurna/issues/213))
- Support for an external button for D1 Mini boards (thanks to user @PieBru, [#239](https://github.com/xoseperez/espurna/issues/239))
- Option to query relay status via MQTT or WS (thanks to Wesley Tuzza)
- Automatically install dependencies for web interface builder (thanks to Hermann Kraus)
- Support for HSV and IR for Magic Home LED Controller (optional, disabled by default, thanks to Wesley Tuzza)
- Added option to report DS18B20 temperatures based on changes (thanks to Michael A. Cox)
- Safer buffer handling for websocket data (thanks to Hermann Kraus & Björn Bergman)
- Updates HL8012 library with energy counting support (thanks to Hermann Kraus)
- Added option to disable light color persistence to avoid flickering ([#191](https://github.com/xoseperez/espurna/issues/191))
- Option to enable TELNET in STA mode from web UI ([#203](https://github.com/xoseperez/espurna/issues/203))

### Changed
- Changed default MQTT base topic to "{identifier}" (no leading slashes, [#208](https://github.com/xoseperez/espurna/issues/208))
- Prevent reconnecting when in AP mode if a web session or a telnet session is active ([#244](https://github.com/xoseperez/espurna/issues/244))
- Web UI checks for pending changes before reset/reconnect options ([#226](https://github.com/xoseperez/espurna/issues/226))
- Increase WIFI connect timeout and reconnect interval

## [1.9.5] 2017-09-28
### Fixed
- Revert to JustWifi 1.1.4 ([#228](https://github.com/xoseperez/espurna/issues/228))

## [1.9.4] 2017-09-22
### Added
- Added ESPurna specific mDNS text registers (app_name, app_version, device_name)
- Crash dump info is stored in EEPROM and retrieved via terminal ("crash" command)
- Support for Huacanxing H802
- Support for powermeters based on V9261F IC
- Support for powermeters based on ECH1560 IC (beta, untested)

### Changed
- Changed behaviour on MQTT connection failure ([#215](https://github.com/xoseperez/espurna/issues/215))
- Removed boot delay
- Refactor power modules
- Updated JustWifi library

### Fixed
- Set all esp8285 devices to use esp01_1m ([#210](https://github.com/xoseperez/espurna/issues/210), [#225](https://github.com/xoseperez/espurna/issues/225))
- Removed wifi gain option since it prevents some devices to connect ([#204](https://github.com/xoseperez/espurna/issues/204))

## [1.9.3] 2017-09-04
### Added
- New "erase.config" option in terminal to delete SDK settings
- Added error code to error message when updating from web UI
- Fixed Web UI to be behind a proxy (http://tinkerman.cat/secure-remote-access-to-your-iot-devices/)
- Support "ON", "OFF" and "TOGGLE" (also lowercase) as payload in relay MQTT, API and WS (http://tinkerman.cat/using-google-assistant-control-your-esp8266-devices/)

### Changed
- Updated fauxmoESP library to 2.2.0

### Fixed
- Fix HLW8012 calibration ([#194](https://github.com/xoseperez/espurna/issues/194))
- Fix telnet dropping connection
- Fix WiFiSecureClient connection with PubSubClient ([#64](https://github.com/xoseperez/espurna/issues/64))

## [1.9.2] 2017-08-31
### Added
- System stability check (turns off everything except WIFI AP, OTA and telnet if there is a boot crash loop) ([#196](https://github.com/xoseperez/espurna/issues/196))
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
- Crash in settings saving ([#190](https://github.com/xoseperez/espurna/issues/190)) and fixed UDP debug conditional build clauses

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
- Fixed light status on boot ([#157](https://github.com/xoseperez/espurna/issues/157))
- Fixed CSS bug cause by a bad merge

## [1.9.0] 2017-08-25
### Added
- Support for IteadStudio BN-SZ01 Ceiling Light ([#132](https://github.com/xoseperez/espurna/issues/132))
- Support for IteadStudio Sonoff RF Bridge ([#173](https://github.com/xoseperez/espurna/issues/173))
- Support for IteadStudio Sonoff 4CH Pro ([#174](https://github.com/xoseperez/espurna/issues/174))
- Support for IteadStudio Sonoff B1
- Support for IteadStudio Sonoff LED
- Support for IteadStudio Sonoff T1 wall switches (1, 2 and 4 channels)
- Support for WiOn 50055 WiFi Wall Outlet & Tap
- Support for EXS WiFi Relay v3.1 (and other future latching relay boards) ([#152](https://github.com/xoseperez/espurna/issues/152))
- TLS/SSL support for MQTT (caution: eats a lot of memory, do not use with web interface) ([#64](https://github.com/xoseperez/espurna/issues/64))
- Add support for delayed ON/OFF switches ([#123](https://github.com/xoseperez/espurna/issues/123), [#161](https://github.com/xoseperez/espurna/issues/161), [#188](https://github.com/xoseperez/espurna/issues/188))
- Added ON and OFF actions for button events (previously only TOGGLE available) ([#182](https://github.com/xoseperez/espurna/issues/182))
- Sliders in web interface to control dimmer channels independently (also for brightness)
- Debug info about MQTT disconnect reason

### Changed
- MQTT setters ending with "/set" by default
- Using DOUT flash mode on all devices ([#167](https://github.com/xoseperez/espurna/issues/167))
- Longer timeout for WiFi connection (better chances for Sonoff Basic to connect)
- Changed MQTT topics for light devices (COLOR, BRIGHTNESS, MIRED, KELVIN, CHANNEL) ([#144](https://github.com/xoseperez/espurna/issues/144))
- Changed the way light devices are defined (see LIGHT_PROVIDER_DIMMER)
- Allow to disable color picker in web interface
- API returns processed values for HLW8012 sensor (not raw values anymore) ([#176](https://github.com/xoseperez/espurna/issues/176))
- Major refactoring of settings

### Fixed
- Discard MQTT messages with empty payload ([#185](https://github.com/xoseperez/espurna/issues/185))
- Wifi connection issue (https://github.com/esp8266/Arduino/issues/2186)
- Alexa connection issue

## [1.8.3] 2017-07-23
### Added
- Issue [#85](https://github.com/xoseperez/espurna/issues/85) and [#90](https://github.com/xoseperez/espurna/issues/90). Option to report MQTT messages with JSON payloads
- Issue [#170](https://github.com/xoseperez/espurna/issues/170). Updated DebouceEvent library to allow disabling double click and get faster click responses
- Using memory layout with no SPIFFS for 1Mb devices

### Changed
- Rename settings s/POW/HLW8012/
- Return times in ISO8601 format

### Fixed
- Issue [#168](https://github.com/xoseperez/espurna/issues/168). Added H801 to arduino.h file
- Issue [#171](https://github.com/xoseperez/espurna/issues/171). Fix corrupted will message

## [1.8.2] 2017-07-16
### Added
- InfluxDB support via HTTP API
- Added custom reset reason to debug log
- Enable WIFI debug on hardware reset (button long click)

### Changed
- Issue [#159](https://github.com/xoseperez/espurna/issues/159). Allow decimals in relay pulse interval
- Updated HLW8012 library

### Fixed
- Issue [#148](https://github.com/xoseperez/espurna/issues/148). Fix bug in conditional compilation check
- Issue [#149](https://github.com/xoseperez/espurna/issues/149). Using different pulse counters for each relay (thanks to Lauris Ieviņš)
- Issue [#141](https://github.com/xoseperez/espurna/issues/141). Limit relay pulse interval to 60s
- Fixed units for apparent & reactive power (thanks to Lauris Ieviņš)
- Fixed mDNS setup when using custom HTTP port for web interface

## [1.8.1] 2017-05-22
### Fixed
- Issue [#140](https://github.com/xoseperez/espurna/issues/140). Fix no relay control bug in Sonoff Dual

## [1.8.0] 2017-05-21
### Added
- Added gamma correction to RGB strips. Thanks to Chris Ward.
- Added support for Huacanxing H801 WiFi LED Controller. Thanks to Minh Phuong Ly.
- Issue [#138](https://github.com/xoseperez/espurna/issues/138). Added NTP configuration from web interface
- Issue [#128](https://github.com/xoseperez/espurna/issues/128). Report color when booting and in heartbeat stream.
- Issue [#126](https://github.com/xoseperez/espurna/issues/126). Show NTP status in web interface.
- Added filter limits on POW readings.
- Added color temperature to RGB calculation. Thanks to Sacha Telgenhof.
- Issue [#120](https://github.com/xoseperez/espurna/issues/120). Added relay flood protection. Thanks to Izik Dubnov.
- Support for "#RRGGBB", "RRR,GGG,BBB" and "WWW" color formats.
- Issue [#117](https://github.com/xoseperez/espurna/issues/117). Added build date & time to web interface.

### Fixed
- Fix MQTT_RELAY board conifugration. Thanks to Denis French.
- Issue [#125](https://github.com/xoseperez/espurna/issues/125). Fix bug in relay status reading from EEPROM
- Issue [#127](https://github.com/xoseperez/espurna/issues/127). Fix button action in DUAL.
- Fix bug in Sonoff POW current reading. Thanks to Emmanuel Tatto.
- Minimizing my9291 flickering when booting.
- Fix conditional flags in hardware.ino to support Arduino IDE.

## [1.7.1] 2017-03-28
### Fixed
- Issue [#113](https://github.com/xoseperez/espurna/issues/113). Fix restoring color from EEPROM upon reboot
- Issue [#113](https://github.com/xoseperez/espurna/issues/113). Fix bug in API handlers

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
- Issue [#113](https://github.com/xoseperez/espurna/issues/113). Fix light bulb state to OFF in library prevented the bulb from turning on
- Issue [#58](https://github.com/xoseperez/espurna/issues/58). Added code to handle spurious readings
- Fix bug in HLW8012 calibration current parameter casting to int instead of float
- Issue [#115](https://github.com/xoseperez/espurna/issues/115). Removed local declaration of _mqttForward variable. Thanks to Paweł Fiedor
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
- Issue [#85](https://github.com/xoseperez/espurna/issues/85). Heartbeat reports now free heap, uptime and VCC every 5 minutes

### Changed
- Wait two minutes instead of one in AP mode before trying to reconnect to the router
- Issue [#92](https://github.com/xoseperez/espurna/issues/92). Debug log enabled by default in Arduino IDE
- Issue [#91](https://github.com/xoseperez/espurna/issues/91). Using AsyncMqttClient as default MQTT client again

### Fixed
- Report data from all sensors via websocket even if no MQTT connection
- Issue [#92](https://github.com/xoseperez/espurna/issues/92). Fix unknown reference in Arduino IDE
- Split data.h contents into 1k lines, otherwise Arduino IDE chokes on them
- Discard empty MQTT topic while subscribing

## [1.6.7] 2017-02-25
### Added
- Support for OpenLight / AI-Light by AI-Thinker based on MY9291 LED driver
- Issue [#87](https://github.com/xoseperez/espurna/issues/87). Factory reset when physical button pressed for >10 seconds

## [1.6.6] 2017-02-23
### Fixed
- Issue [#82](https://github.com/xoseperez/espurna/issues/82). Fix critical bug on Sonoff Dual

## [1.6.5] 2017-02-22
### Added
- Option to backup and restore settings from the web interface
- Footer in the web interface

### Changed
- Using PubSubClient as MQTT client by default (please read the documentation)
- Double & long clicks do nothing except for the first defined button

### Fixed
- Issue [#79](https://github.com/xoseperez/espurna/issues/79). Fix bug in WiFi led notification & MQTT connectivity (using PubSubClient)
- Issue [#73](https://github.com/xoseperez/espurna/issues/73). Fix bug when building without Domoticz support
- Fix Gulp tasks dependencies

## [1.6.4] 2017-02-20
### Added
- Option to embed the web interface in the firmware, disabled by default
- Change relay status with a GET request (browser friendly)
- Support for PROGMEM debug messages (only wifi module has been changed)
- Option to disable mDNS, enabled by default
- Show current web server port in debug log
- Issue [#75](https://github.com/xoseperez/espurna/issues/75). Link relays to LEDs
- Issue [#76](https://github.com/xoseperez/espurna/issues/76). Using http://espurna.local when in AP mode

### Changed
- Images and favicon is now embedded in the HTML
- Authentication challenge only in /auth request. All static contents are un-authenticated
- HTTP response code when out of websocket slots changed from 423 to 429

### Fixed
- Memory leak in MQTT connection method
- Wait 60 seconds before retrying to connect when in AP mode
- Issue [#24](https://github.com/xoseperez/espurna/issues/24) & [#74](https://github.com/xoseperez/espurna/issues/74). Update ESPAsyncTCP and ESPAsyncWebServer to latest GIT version that supports MSS defragmenting
- Issue [#73](https://github.com/xoseperez/espurna/issues/73). Fixes for windows machines

### Removed
- Captive portal removed, mDNS resolution for AP mode too

## [1.6.3] 2017-02-15
### Added
- Issue [#69](https://github.com/xoseperez/espurna/issues/69). Temperature unit configuration from the web interface
- Issue [#55](https://github.com/xoseperez/espurna/issues/55). WebServer port configurable from the web interface, defaults to 80
- Expand network configuration when adding a new network

### Changed
- Merged web contents except images in a single compressed file for reliability
- Update support for Itead Motor Clockwise/Anticlockwise board
- Scan for strongest network only if more than 1 network configured

### Fixed
- Issue [#71](https://github.com/xoseperez/espurna/issues/71). Added default values for netmask and DNS in web configuration
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
- Issue [#50](https://github.com/xoseperez/espurna/issues/50). Fix type bug in window variable when calculating energy for HLW8012 devices (Sonoff POW)

## [1.5.3] 2017-02-02
### Fixed
- Issue [#50](https://github.com/xoseperez/espurna/issues/50) and [#54](https://github.com/xoseperez/espurna/issues/54). Fixed domoticz MQTT message format

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
- Issue [#20](https://github.com/xoseperez/espurna/issues/20) Better inline documentation for Alexa and Domoticz default settings
- Issue [#39](https://github.com/xoseperez/espurna/issues/39) Fixed autoconnect issue with static IP (fixed in JustWifi library)
- Issue [#41](https://github.com/xoseperez/espurna/issues/41) Added password requirements to initial password change page

### Changed
- Changed LED pattern for WIFI notifications (shorter pulses)

## [1.4.4] 2017-01-13
### Added
- Adding current, voltage, apparent and reactive power reports to Sonoff POW (Web & MQTT)

### Fixed
- Issue [#35](https://github.com/xoseperez/espurna/issues/35) Fixed frequent MQTT connection drops after WIFI reconnect
- Defer wifi disconnection from web interface to allow request to return

### Changed
- Move all Arduino IDE configuration values to their own file
- Using latest HLW8012 library in interrupt mode

## [1.4.3] 2017-01-11
### Fixed
- Issue [#6](https://github.com/xoseperez/espurna/issues/6) Using forked Time library to prevent conflict with Arduino Core for ESP8266 time.h file in windows machines

## [1.4.2] 2017-01-09
### Added
- Support for inverse logic relays

### Fixed
- Issue [#31](https://github.com/xoseperez/espurna/issues/31). Fixed error in relay identification from MQTT messages

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
- Issue [#16](https://github.com/xoseperez/espurna/issues/16). Enforce minimum password strength in web interface

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
- Issue [#11](https://github.com/xoseperez/espurna/issues/11) Compile error when building sonoff-dual-debug
- Issue [#14](https://github.com/xoseperez/espurna/issues/14) MQTT Connection with Username an Password not working
- Issue [#17](https://github.com/xoseperez/espurna/issues/17) Moved static variable 'pending' to class variable

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
