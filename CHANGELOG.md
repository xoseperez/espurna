# ESPurna change log

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [1.14.2] Not yet released
\-

## [1.14.1] 2019-12-31

### Fixed
#### Devices
- Set button pullup on shpx-v23 boards ([#2074](https://github.com/xoseperez/espurna/issues/2074), thanks to **[@RDobrinov](https://github.com/RDobrinov)**)
#### Domoticz
- Fix unresponsive switches, use proper datastructure to track state ([#2049](https://github.com/xoseperez/espurna/issues/2049))
#### InfluxDB
- Use sensor "report" instead of sending data each reading. Use async client, send data in batches ([#2061](https://github.com/xoseperez/espurna/issues/2061))
- Don't queue any new data while connecting
#### IR
- Revert "ir: use proper methods" ([#2066](https://github.com/xoseperez/espurna/issues/2066), thanks to **[@sehraf](https://github.com/sehraf)** for reporting the issue)
#### OTA
- Verify data stream and properly handle errors. Do not write to flash when any error was encountered. ([#2067](https://github.com/xoseperez/espurna/issues/2067))
#### Sensor
- Use different Broker instances for read and report data ([#2061](https://github.com/xoseperez/espurna/issues/2061))
#### WebUI
- Fix LED mode title style ([#2038](https://github.com/xoseperez/espurna/issues/2038), thanks to **[@foxman69](https://github.com/foxman69)**)
- Properly handle websocket payloads when scheduler is disabled ([#2050](https://github.com/xoseperez/espurna/issues/2050))
- Fix colorpicker not working in RGB mode ([#2053](https://github.com/xoseperez/espurna/issues/2053))
- Remove some unused code from resulting image ([#2053](https://github.com/xoseperez/espurna/issues/2053))

### Added
#### Devices
- Add GENERIC_E14, e14 rgb+w 4,5w ([#2039](https://github.com/xoseperez/espurna/2039), thanks to **[@orrpan](https://github.com/orrpan)**)
- Add support for LinkSprite R4 ([#2042](https://github.com/xoseperez/espurna/issues/2042), thanks to **[@mpcusack](https://github.com/mpcusack)**)
- Add support for eHomeDIY devices. ([#2046](https://github.com/xoseperez/espurna/issues/2046), thanks to **[@user890104](https://github.com/user890104)**)
- Add support for MAGICHOME\_ZJ_WFMN\_C\_11 ([#2051](https://github.com/xoseperez/espurna/issues/2051), thanks to **[@davebuk](https://github.com/davebuk)**)
- Add support for the LSC LED LIGHT STRIP from ACTION using a tuya chip. ([#2065](https://github.com/xoseperez/espurna/issues/2065), thanks to **[@sehraf](https://github.com/sehraf)**)
- Add LOHAS_E26_A19, rename LOHAS_9W to LOHAS_E27_9W ([#2068](https://github.com/xoseperez/espurna/issues/2068), thanks to **[@cro](https://github.com/cro)** for providing A19 configuration)
#### Home Assistant
- Try to avoid conflict with useCSS ([#2075](https://github.com/xoseperez/espurna/issues/2075))
#### WebUI
- WebUI fix change detection of input elements ([#1986](https://github.com/xoseperez/espurna/issues/1986))

### Changed
#### Build
- Removed unneeded reference to Ticker library in RFBridge module
#### PlatformIO
- Update latest Arduino core platform to 2.6.3 ([#2069](https://github.com/xoseperez/espurna/issues/2069), thanks to **[@Niek](https://github.com/Niek)**)
- Test espurna-core feature build ([#2080](https://github.com/xoseperez/espurna/issues/2080))
#### Scripts
- Using python3 by default in OTA manager
- Force get_device_size to return an int in OTA manager
- Beautify build script size output
- ota.py version 0.4 ([#2020](https://github.com/xoseperez/espurna/issues/2020))
- Do not run test stages on tag in Travis CI ([#2052](https://github.com/xoseperez/espurna/issues/2052))

## [1.14.0] 2019-11-29
### Fixed
#### Security
- Web OTA: check authentication result before accepting payload ([#1812](https://github.com/xoseperez/espurna/issues/1812))
- Authenticate /reset endpoint ([#1858](https://github.com/xoseperez/espurna/issues/1858), thanks to **[@foxman69](https://github.com/foxman69)**)
#### General
- Avoid crashes when using NTPClientLib ([#1642](https://github.com/xoseperez/espurna/pull/1642))
- Safer loop delay default ([#1574](https://github.com/xoseperez/espurna/issues/1574), [#1631](https://github.com/xoseperez/espurna/issues/1631), [#1699](https://github.com/xoseperez/espurna/issues/1699))
- Patch lwip1 MTU bug in travis-ci releases ([#1723](https://github.com/xoseperez/espurna/issues/1723))
- Store default LED settings exactly once ([#1719](https://github.com/xoseperez/espurna/issues/1719), [#1724](https://github.com/xoseperez/espurna/issues/1724))
- Backup EEPROM before performing OTA ([#1808](https://github.com/xoseperez/espurna/issues/1808), [#1809](https://github.com/xoseperez/espurna/issues/1809), [#2028](https://github.com/xoseperez/espurna/issues/2028), thanks to **[@arihantdaga](https://github.com/arihantdaga)**)
- Properly handle telnet negotiation ([#1927](https://github.com/xoseperez/espurna/issues/1927), thanks to **[@Niek](https://github.com/Niek)**)
- Markdown Typo Fix ([#1926](https://github.com/xoseperez/espurna/issues/1926), thanks to **[@mx-web](https://github.com/mx-web)**)
- Port PROGMEM definition from Cores 2.5.0+ for Core 2.3.0 to fix flashstring use in classes ([#1374](https://github.com/xoseperez/espurna/issues/1374))
- Use relative time for relay scheduling ([#1962](https://github.com/xoseperez/espurna/pull/1962))
- Enable all heartbeat messages when `hbReport => "1"` ([#2003](https://github.com/xoseperez/espurna/pull/2003))
#### Devices
- Add config for push button of Arilux AL-LC06 ([#1794](https://github.com/xoseperez/espurna/issues/1794), thanks to **[@user890104](https://github.com/user890104)**)
#### WiFi
- Don't change softAP configuration while user is still connected ([#1881](https://github.com/xoseperez/espurna/issues/1881))
- Place WIFIN\_... strings in flash ([#1893](https://github.com/xoseperez/espurna/issues/1893))
#### MQTT
- Reset connection timer with PubSubClient / ArduinoMQTT ([#1702](https://github.com/xoseperez/espurna/issues/1702))
- Remove 'connecting' flag when connected ([#1757](https://github.com/xoseperez/espurna/issues/1757))
- Fix Arduino-MQTT setWill parameters order ([#1978](https://github.com/xoseperez/espurna/issues/1978), thanks to **[@Niek](https://github.com/Niek)**)
#### Domoticz
- Fix crashes with unknown idx values ([#1588](https://github.com/xoseperez/espurna/issues/1588))
- Treat nvalue >= 1 as true ([#1606](https://github.com/xoseperez/espurna/issuess/1606))
- Lights: handle cmode=2 ([#1880](https://github.com/xoseperez/espurna/issues/1880))
#### Thingspeak
- Several async client fixes to avoid crashes ([#1806](https://github.com/xoseperez/espurna/issues/1806))
#### Home Assistant
- Send brightness topic to HA regardless of color setting ([#1730](https://github.com/xoseperez/espurna/issues/1730), thanks to **[@copyrights](https://github.com/copyrights)**)
- Rework discovery mechanism to prevent loosing messages ([#1969](https://github.com/xoseperez/espurna/issues/1969))
- Send discovery messages after reconnecting ([#1637](https://github.com/xoseperez/espurna/issues/1637), [#1969](https://github.com/xoseperez/espurna/issues/1969))
#### Lights
- Fix relay provider setting of Xiaomi Smart Desk Lamp ([#1627](https://github.com/xoseperez/espurna/issues/1627), thanks to **[@Ctrl-F4](https://github.com/Ctrl-F4)**)
- Prepare for .cpp migration ([#1874](https://github.com/xoseperez/espurna/issues/1874))
- Fix inconsistent light transitions ([#1901](https://github.com/xoseperez/espurna/issues/1901), [#1923](https://github.com/xoseperez/espurna/issues/1923))
#### WebUI
- WebUI relayOnDisc selector typo ([#1643](https://github.com/xoseperez/espurna/pull/1643))
- Avoid memory leak when using captive portal ([#1768](https://github.com/xoseperez/espurna/issues/1768))
- Updated WS protocol structure to reduce memory usage ([#1843](https://github.com/xoseperez/espurna/issues/1843), [#1851](https://github.com/xoseperez/espurna/issues/1851), [#1857](https://github.com/xoseperez/espurna/issues/1857))
- Send debug messages in batches ([#1851](https://github.com/xoseperez/espurna/issues/1851))
- Wait for data to be saved before rebooting ([#1863](https://github.com/xoseperez/espurna/issues/1863), thanks to **[@foxman69](https://github.com/foxman69)**)
#### Sensors
- Fix cse7766 missing energy magnitude ([#1665](https://github.com/xoseperez/espurna/issues/1665), thanks to **[@lipoforall](https://github.com/lipoforall)**)
- Fix stored energy values when using kWh ([#1334](https://github.com/xoseperez/espurna/issues/1334)
- Remove pinMode(0, ...) from AnalogSensor ([#1777](https://github.com/xoseperez/espurna/issues/1777), [#1827](https://github.com/xoseperez/espurna/issues/1827))
- Check value range for PMSX005 and SenseAir CO2 sensor ([#1865](https://github.com/xoseperez/espurna/issues/1865), thanks to **[@Yonsm](https://github.com/Yonsm)**)
- DHT: Increase read delay to 1100 usec per datasheet value for `DHT_CHIP_DHT22` ([#1918](https://github.com/xoseperez/espurna/issues/1918), [#1979](https://github.com/xoseperez/espurna/issues/1979), thanks to **[@JavierAder](https://github.com/JavierAder)** and **[@structuralB](https://github.com/structuralB)**)
- DHT: Add `DHT_CHIP_SI7021` for `ITEAD_SONOFF_TH`, use 500 usec read delay ([#1918](https://github.com/xoseperez/espurna/issues/1918#issuecomment-555672628), [#2000](https://github.com/xoseperez/espurna/issues/2000), thanks to **[@icevoodoo](https://github.com/icevoodoo)**)
- DHT: Set pin mode before digitalWrite ([#1979](https://github.com/xoseperez/espurna/issues/1979))
- DHT: Wait DHT_MIN_INTERVAL after initialization ([#1979](https://github.com/xoseperez/espurna/issues/1979))
#### Build
- Fix Travis failing with INFLUXDB_SUPPORT ([#1565](https://github.com/xoseperez/espurna/issues/1565))
- Build with platformio 4 ([#1805](https://github.com/xoseperez/espurna/issues/1805))
- Update wrong pinout on ag-l4 ([#1746](https://github.com/xoseperez/espurna/issues/1746), thanks to **[@zerog2k](https://github.com/zerog2k)**)
- Core 2.6.0+ fixes ([#1852](https://github.com/xoseperez/espurna/issues/1852))
- Add missing DOMOTICZ\_... #ifndef guards ([#1839](https://github.com/xoseperez/espurna/issues/1839))
- Add #ifndef guard for BMX280\_... defines ([#1867](https://github.com/xoseperez/espurna/issues/1867), thanks to **[@0x3333](https://github.com/0x3333)**)
- MQTT: update MQTT_MAX_PACKET_SIZE to fit JSON payload ([#1888](https://github.com/xoseperez/espurna/issues/1888))

### Added
#### General
- [RPN Rules](https://github.com/xoseperez/espurna/wiki/RPN-Rules) - custom rules to execute actions (mostly changing relay and light statuses) based on different inputs ([#1984](https://github.com/xoseperez/espurna/issues/1984), thanks to **[@xoseperez](https://github.com/xoseperez)**)
- Initial implementation of RTCMEM storage to preserve state (relay status, stability counter, energy etc.) between reboots ([#1420](https://github.com/xoseperez/espurna/issues/1420), [#1770](https://github.com/xoseperez/espurna/issues/1770))
- Allow to configure all LEDs from UI ([#1429](https://github.com/xoseperez/espurna/issues/1429), thanks to **[@xoseperez](https://github.com/xoseperez)**)
- SYNC_FIRST relay sync mode ([#1609](https://github.com/xoseperez/espurna/issues/1609), thanks to **[@foxel](https://github.com/foxel)**)
- Fix ESP.eraseConfig() when using Core 2.3.0 ([#1595](https://github.com/xoseperez/espurna/issues/1595), [#1616](https://github.com/xoseperez/espurna/issues/1616))
- Add UNUSED macro to prevent warnings
- Log in travis-ci if WebUI files have changed
- Terminal: heap fragmentation stat ([#1740](https://github.com/xoseperez/espurna/issues/1740))
- Recommend using basic BearSSL ciphers with low memory boards ([#1810](https://github.com/xoseperez/espurna/issues/1810), thanks to **[@Niek](https://github.com/Niek)**)
- New boot mode to lock relay status on boot (`RELAY_BOOT_LOCKED_OFF` and `RELAY_BOOT_LOCKED_ON`) ([#1705](https://github.com/xoseperez/espurna/issues/1705))
- Add netstat and dns probing (Core 2.5.2+) ([#1907](https://github.com/xoseperez/espurna/issues/1907))
- Add setting for WiFi TX power (`wifiTxPwr`) ([#1915](https://github.com/xoseperez/espurna/issues/1915))
- SoftAP button action will now toggle back to STA mode ([#1942](https://github.com/xoseperez/espurna/pull/1942))
- Detect esp8285 chip to allow the use of GPIO9 and GPIO10 ([#1958](https://github.com/xoseperez/espurna/issues/1958), [#1964](https://github.com/xoseperez/espurna/pull/1964) and thanks to **[@Niek](https://github.com/Niek)** for bitset::test fix in [#1977](https://github.com/xoseperez/espurna/pull/1977))
- Restore last schedule after reboot ([#1948](https://github.com/xoseperez/espurna/issues/1948), thanks to **[@foxman69](https://github.com/foxman69)**)
- Add `relayDelayOnN`, `relayDelayOffN`, `relayFloodTime`, `relayFloodCount` as runtime settings ([#1594](https://github.com/xoseperez/espurna/issues/1594), [#1962](https://github.com/xoseperez/espurna/pull/1962))
- Add experimental `relayDelayInterlock` to add a pause after turning relay off in SYNC\_ONE or SYNC\_NONE\_OR\_ONE modes ([#1510](https://github.com/xoseperez/espurna/issues/1510), [#1962](https://github.com/xoseperez/espurna/pull/1962))
- Add experimental `TUYA_SUPPORT` and `LIGHT_PROVIDER_TUYA` ([#1729](https://github.com/xoseperez/espurna/issues/1729), [#1997](https://github.com/xoseperez/espurna/issues/1997))
- Add `bssid` as heartbeat option ([#1995](https://github.com/xoseperez/espurna/issues/1995))
- Generic feature checks through `<type_traits>` ([#1974](https://github.com/xoseperez/espurna/issues/1974), thanks to **[@Niek](https://github.com/Niek)**)
- Support [binary, octal and hex literal](https://en.wikipedia.org/wiki/Integer_literal) prefixes (`0b`, `0o` and `0x` respectively) for `hbReport` setting ([#2003](https://github.com/xoseperez/espurna/pull/2003))
#### MQTT
- Add option to disable relay reporting ([#1645](https://github.com/xoseperez/espurna/issues/1645), thanks to **[@Niek](https://github.com/Niek)**)
- Safer settings reload and change detection ([#1701](https://github.com/xoseperez/espurna/issues/1701))
- `mqtt.info` command ([#1757](https://github.com/xoseperez/espurna/issues/1757))
- MQTT rewrite with SSL fixes ([#1751](https://github.com/xoseperez/espurna/issues/1751), [#1829](https://github.com/xoseperez/espurna/issues/1829), thanks to **[@Niek](https://github.com/Niek)**)
- Generic secure client configuration ([#1873](https://github.com/xoseperez/espurna/issues/1873))
- Custom relay status payloads ([#1885](https://github.com/xoseperez/espurna/issues/1885), [#1889](https://github.com/xoseperez/espurna/issues/1889))
- Log size instead of message itself when size is > 128 bytes ([#1969](https://github.com/xoseperez/espurna/issues/1969))
#### Home Assistant
- Add color_temp_state_topic ([#1891](https://github.com/xoseperez/espurna/issues/1891), thanks to **[@l3d00m](https://github.com/l3d00m)**)
#### Devices
- Add espurna-base for [tuya-convert](https://github.com/ct-Open-Source/tuya-convert), thanks to **[@xoseperez](https://github.com/xoseperez)**
- Add espurna-core-wps
- Add espurna-core-smartconfig
- LightFox dual support ([#1468](https://github.com/xoseperez/espurna/issues/1468), thanks to **[@foxel](https://github.com/foxel)**)
- Support for Digoo/Oxaoxe NX-SP202 ([#1502](https://github.com/xoseperez/espurna/issues/1502), thanks to **[@kobuki](https://github.com/kobuki)**)
- HAMA outlet model number 00176552 ([#1598](https://github.com/xoseperez/espurna/issues/1598), thanks to **[@markusrudolf](https://github.com/markusrudolf)**)
- Added LITESUN LA-WF3 support. ([#1618](https://github.com/xoseperez/espurna/issues/1618), thanks to **[@Zebble](https://github.com/Zebble)**)
- Added Teckin SP20 Power Plug. ([#1611](https://github.com/xoseperez/espurna/issues/1611), thanks to **[@brmo](https://github.com/brmo)**)
- Adding support for Generic GU10 from [#1549](https://github.com/xoseperez/espurna/issues/1549) ([#1650](https://github.com/xoseperez/espurna/issues/1650), thanks to **[@ColinShorts](https://github.com/ColinShorts)**)
- Add support for JINVOO VALVE SM-AW713 ([#1774](https://github.com/xoseperez/espurna/issues/1774), thanks to **[@reaper7](https://github.com/reaper7)**)
- Add support for MagicHome RGBWW LED strip ([#1787](https://github.com/xoseperez/espurna/issues/1787), thanks to **[@user890104](https://github.com/user890104)**)
- Nexete A19 RGBW bulb ([#1814](https://github.com/xoseperez/espurna/issues/1814), thanks to **[@konsgn](https://github.com/konsgn)**)
- Add Shelly 1PM ([#1822](https://github.com/xoseperez/espurna/issues/1822), thanks to **[@tonilopezmr](https://github.com/tonilopezmr)**)
- Support Shelly 2.5 ([#1827](https://github.com/xoseperez/espurna/issues/1827), thanks to **[@tonilopezmr](https://github.com/tonilopezmr)**)
- Add itead-sonoff-s31-lite ([#1830](https://github.com/xoseperez/espurna/issues/1830), thanks to **[@CrazyIvan359](https://github.com/CrazyIvan359)**)
- Add etekcity-esw01-usa ([#1836](https://github.com/xoseperez/espurna/issues/1836), thanks to **[@sirpatil](https://github.com/sirpatil)**)
- Add iselector-sm-pw702 ([#1837](https://github.com/xoseperez/espurna/issues/1837), thanks to **[@lwalkera](https://github.com/lwalkera)**)
- Itead Sonoff Mini support ([#1886](https://github.com/xoseperez/espurna/issues/1886), thanks to **[@xoseperez](https://github.com/xoseperez)**)
- ESP8266 FS-UAP1 Control board ([#1925](https://github.com/xoseperez/espurna/issues/1925), thanks to **[@NemoN](https://github.com/NemoN)**)
- Support for Teckin SP21 (Thanks to **[@xoseperez](https://github.com/xoseperez)**)
- Support for TFlag NX-SM100 and NX-SM200 power monitoring switches (Thanks to **[@xoseperez](https://github.com/xoseperez)**)
- Add tuya-generic-dimmer ([#1729](https://github.com/xoseperez/espurna/issues/1729), [#1997](https://github.com/xoseperez/espurna/issues/1997))
- Add support for Hykker Smart Plug with power meter ([#2010](https://github.com/xoseperez/espurna/issues/2010), thanks to **[@reaper7](https://github.com/reaper7)**)
- Add support for BW-SHP5 ([#2029](https://github.com/xoseperez/espurna/issues/2029), thanks to **[@aligator](https://github.com/aligator)**)
#### WebUI
- Configure WEB_REMOTE_DOMAIN at runtime ([#1789](https://github.com/xoseperez/espurna/issues/1789))
#### Modules
- Add Thermostat module ([#1603](https://github.com/xoseperez/espurna/issues/1603), thanks to **[@ElderJoy](https://github.com/ElderJoy)**)
- RF/RFBRIDGE Update ([#1693](https://github.com/xoseperez/espurna/issues/1693))
- Send Vcc, LoadAvg and Ssid to influxdb ([#1714](https://github.com/xoseperez/espurna/issues/1714), thanks to **[@lucciano](https://github.com/lucciano)**)
- Separate device name for alexa integration ([#1727](https://github.com/xoseperez/espurna/issues/1727), thanks to **[@m-kozlowski](https://github.com/m-kozlowski)**)
- Telnet/WiFiServer: alternative to ESPAsyncTCP ([#1799](https://github.com/xoseperez/espurna/issues/1799))  (Thanks to **[@Niek](https://github.com/Niek)**)
- Periodically call etharp_gratuitous ([#1877](https://github.com/xoseperez/espurna/issues/1877))
- Added support for reverse telnet ([#1920](https://github.com/xoseperez/espurna/pull/1920), thanks to **[@Niek](https://github.com/Niek)**)
#### Sensors
- MH-Z19 auto-calibration setting ([#1580](https://github.com/xoseperez/espurna/issues/1580) , [#1592](https://github.com/xoseperez/espurna/issues/1592), thanks to **[@eschava](https://github.com/eschava)**)
- CSE7766: Add reactive power calculation ([#1591](https://github.com/xoseperez/espurna/issues/1591), thanks to **[@AlbertWeterings](https://github.com/AlbertWeterings)**)
- MAX6675: default pins and SENSOR_SUPPORT dependency ([#1646](https://github.com/xoseperez/espurna/issues/1646), [#1666](https://github.com/xoseperez/espurna/issues/1666))
- Add `bmx280Number` and `bmx280Address` settings ([#1690](https://github.com/xoseperez/espurna/issues/1690))
- PZEM004T: settings & dev board ([#1712](https://github.com/xoseperez/espurna/issues/1712))
- Added LDR sensor (Thanks to Altan Altay)
- ADE9753 Support ([#1827](https://github.com/xoseperez/espurna/issues/1827), thanks to **[@tonilopezmr](https://github.com/tonilopezmr)**)
- Telaire T6613 Support ([#1956](https://github.com/xoseperez/espurna/issues/1956), thanks to **[@james-coder](https://github.com/james-coder)**)
- Adding support for miobulb001 ([#1973](https://github.com/xoseperez/espurna/issues/1973), thanks to **[@ealfaroc](https://github.com/ealfaroc)**)
#### Lights
- Allow to set relative brightness, channel value and color in mireds using +N and -N notation ([#1607](https://github.com/xoseperez/espurna/issues/1607), [#1938](https://github.com/xoseperez/espurna/pull/1938), thanks to **[@tsymbaliuk](https://github.com/tsymbaliuk)**)
- Two channel CCT ([#1732](https://github.com/xoseperez/espurna/issues/1732), thanks to **[@copyrights](https://github.com/copyrights)**)
- Send to mired topic if CCT is in use ([#1732](https://github.com/xoseperez/espurna/issues/1732), [#1742](https://github.com/xoseperez/espurna/issues/1742), thanks to **[@copyrights](https://github.com/copyrights)**)
- Add description for each channel when using terminal commands ([#1826](https://github.com/xoseperez/espurna/issues/1826))
- Channel value change detection to reduce light provider updates ([#1914](https://github.com/xoseperez/espurna/issues/1914))
#### Build
- special dummy printf to disable Serial using some boards ([#1664](https://github.com/xoseperez/espurna/issues/1664))
- Experimental support of HTTPUpdate for OTA ([#1751](https://github.com/xoseperez/espurna/issues/1751))
- Add ability to print all GPIO values at once ([#1798](https://github.com/xoseperez/espurna/issues/1798), thanks to **[@Niek](https://github.com/Niek)**)
- DEBUG_SUPPORT check for crashSetup ([#1807](https://github.com/xoseperez/espurna/issues/1807))
- Build date in ota.py (mDNS) ([#1736](https://github.com/xoseperez/espurna/issues/1736), thanks to **[@m-kozlowski](https://github.com/m-kozlowski)**)

### Changed
#### General
- Updated Copyright notice to 2019
- Use espurna.io as CORS domain
- Small design change in memanalyzer output and fix out-of-range error
- Update PlatformIO Core versions ([#1734](https://github.com/xoseperez/espurna/pull/1734))
- Show Core revision as hex string ([#1786](https://github.com/xoseperez/espurna/issues/1786))
- Update debugSend / debugSend_P ([#1788](https://github.com/xoseperez/espurna/issues/1788))
- Send RF MQTT message with retain off ([#1679](https://github.com/xoseperez/espurna/issues/1679), thanks to **[@Niek](https://github.com/Niek)**)
- Thermostat upgrade ([#1711](https://github.com/xoseperez/espurna/issues/1711), thanks to **[@ElderJoy](https://github.com/ElderJoy)**)
- Update crash handler ([#1796](https://github.com/xoseperez/espurna/issues/1796),  [#1947](https://github.com/xoseperez/espurna/pull/1947))
- Drop legacy relay settings migration ([#1797](https://github.com/xoseperez/espurna/issues/1797))
- MQTT & OTA Fingerprint setting capitalization fix ([#1952](https://github.com/xoseperez/espurna/pull/1952), thanks to **[@Niek](https://github.com/Niek)**)
#### WiFi
- Let JustWifi fallback mode handle AP ([#1784](https://github.com/xoseperez/espurna/issues/1784))
- Start SmartConfig without any networks configured ([#1785](https://github.com/xoseperez/espurna/issues/1785))
#### Domoticz
- Less debugging for lights code ([#1588](https://github.com/xoseperez/espurna/issues/1588))
#### WebUI
- Send status right after boot to avoid some empty fields on Status page ([#1700](https://github.com/xoseperez/espurna/issues/1700))
- Parse host query via browser api, add console logging ([#1901](https://github.com/xoseperez/espurna/issues/1901))
- Update to jquery 3.4.1 and wheelcolorpicker 3.0.8 ([#1901](https://github.com/xoseperez/espurna/issues/1901))
#### Lights
- Import Encoder library ([#1769](https://github.com/xoseperez/espurna/issues/1769))
- Apply brightness to all channels only when `useWhite` is disabled ([#1826](https://github.com/xoseperez/espurna/issues/1826))
- Move gamma table to PROGMEM ([#1826](https://github.com/xoseperez/espurna/issues/1826))
- Don't show v for hsv in wheelcolorpicker, use brightness instead ([#1901](https://github.com/xoseperez/espurna/issues/1901))
- Fix kelvin/mired constants naming ([#1902](https://github.com/xoseperez/espurna/issues/1902))
- Store pwm constants as PROGMEM data ([#1906](https://github.com/xoseperez/espurna/issues/1906))
- Schedule provider update in CONT instead of SYS context ([#1901](https://github.com/xoseperez/espurna/issues/1901), [#1923](https://github.com/xoseperez/espurna/issues/1923))
- Configurable cold and warm mired values  ([#1945](https://github.com/xoseperez/espurna/pull/1945))
- Update `XIAOMI_SMART_DESK_LAMP` warm mired value ([#1945](https://github.com/xoseperez/espurna/pull/1945))
#### Sensors
- Change from BMX280_ADDRESS2 notation to BMX280_NUMBER for number of sensors ([#1647](https://github.com/xoseperez/espurna/issues/1647), thanks to **[@CraigMarkwardt](https://github.com/CraigMarkwardt)**)
- Allow sensor class can specify the number of decimals to represent its magnitude types  ([#1648](https://github.com/xoseperez/espurna/issues/1648), thanks to **[@CraigMarkwardt](https://github.com/CraigMarkwardt)**)
- Default to one BMX280 sensor ([#1690](https://github.com/xoseperez/espurna/issues/1690))
- Rename NTC/LDR_SENSOR to NTC/LDR_SUPPORT ([#1758](https://github.com/xoseperez/espurna/issues/1758))
- Faster event handling for EventsSensor ([#1771](https://github.com/xoseperez/espurna/issues/1771))
- Use indexed keys for energy saving ([#1875](https://github.com/xoseperez/espurna/issues/1875))
- Enable multiple Digital & Events sensors ([#1832](https://github.com/xoseperez/espurna/issues/1832), thanks to **[@pilotak](https://github.com/pilotak)**)
#### Build
- .ld scripts refactoring to allow building with Cores 2.3.0 ... 2.6.0 ([#1559](https://github.com/xoseperez/espurna/issues/1559))
- Pin ArduinoJson version ([#1613](https://github.com/xoseperez/espurna/issues/1613))
- Remove gosund-sp1-v23 env in favour of blitzwolf-shpx-v23 ([#1703](https://github.com/xoseperez/espurna/issues/1703))
- Update [ESPAsyncTCP to 7e9ed22](https://github.com/me-no-dev/ESPAsyncTCP/commit/7e9ed22) ([#1752](https://github.com/xoseperez/espurna/issues/1752), [#1806](https://github.com/xoseperez/espurna/issues/1806))
- PIO4: default_envs ([#1793](https://github.com/xoseperez/espurna/issues/1793), thanks to **[@Niek](https://github.com/Niek)**)
- PIO: Use up-to-date platforms, update comments ([#1811](https://github.com/xoseperez/espurna/issues/1811), [#2023](https://github.com/xoseperez/espurna/pull/2023))
- Move DEVICE and MANUFACTURER check to the top of hardware.h ([#1816](https://github.com/xoseperez/espurna/issues/1816), thanks to **[@rmcbc](https://github.com/rmcbc)**)

## [1.13.5] 2019-02-27
### Fixed
- Revert loopDelay dependency on wifi sleep mode (#1574)
- Fix hardcoded serial objects in \_debugSendSerial, terminalLoop and PZEM sensor (#1573)
- Fix RFBridge not showing codes in web UI as per @mcspr suggested change (#1571)
- Fix BSSIDs in scan output (#1567)
- Fix PZEM004TSensor pointer use
- RFBridge: fix webui codes parsing
- Avoid websocket ping back on fw upgrade via web UI form (#1574)
- Removing line break before templated variable to fix issue with Windows Arduino IDE (#1579, thanks to @AlbertWeterings)
- Send brightness to websocket

### Added
- Relay MQTT group receive-only sync mode setting
- Set wifi sleep mode from settings
- Add unique id and device support for better HA UI integration (#1547, thanks to @abmantis)
- Improved inline documentation of BMX280 settings (#1585, thanks to CraigMarkwardt)

## [1.13.4] 2019-02-21
### Fixed
- Travis fixes
- IR results on raw mode (thanks to @vtochq)
- Missing configuration in HTTP API (#1288)
- NTP  sync changes (#1342)
- Proper buffer size to fit two digit rfbOFF key (#1348)
- Use correct arguments for stat on macOS (#1355, thanks to @jackwilson)
- Enable `reload` command when no web support (#1383)
- Wrong GPIO value for dummy relay (#1386)
- Wait until mqtt client has finished trying to connect
- Disable EEPROM Rotate before NoFUSS update (#1398, thanks to @arihantdaga)
- Only check domoticz state in broker callback (#1562)
- Fix upload_port and upload_args
- Fix heartbeat dropdown size
- Setup settings before using them in system module (#1542)
- Fix HEARTBEAT_REPORT_DESCRIPTION typo (#1539)
- Fix wsDebugSend prototype
- Fix pulse for dummy relays (#1496, thanks to @Niek)
- Fix RFBridge websocket data
- Only process Domoticz RGB MQTT Messages for the current idx (#1489, thanks to @soif)
- Fix pulse for dummy relays
- Fix compile error when both RF_SUPPORT and API_SUPPORT are enabled (#1479, thanks to @Niek)
- Fix compile error when TERMINAL_SUPPORT is disabled (#1426)
- Fix compile error when RF_SUPPORT is enabled (#1475)
- Fix CodingStyle link (#1473)
- Fix: Add Debug flag for compilation of wifiDebug() function (#1454)
- Fix bug in RFM69 that counted packets twice
- Escape hyphens in img.shields.io urls
- Fix travis builds based on latest core
- Increase buffer size to fit B0 code (#1423)
- Fix function call typo in RF code (#1421)
- Fix RF code conversion to long (#1410)

### Added
- Support for MAXCIO W-DE003 device (thanks to @kerk1v)
- Support for Tonbux XS-SSA01 device (thanks to @StevenWolfe)
- Support for Blitzwolf BW-SHP2 v2.3 (#1351)
- Support for Tecking SP22 v1.4+
- Support for Lombez Lux Nova 2 smart bulbs (thanks to @kcghost)
- Support for Orvibo B25 (#1402, thanks to @plutec)
- Support for GBLife RGBW Socket (#1305)
- Support for Generic Relay ESP01 V4.0 in inverse relay version (#1504, #1554)
- Support for Gosund WS1 aka KS-602S (#1551, thanks to @nsvrana)
- Support for Oukitel P1 smart switch (#1553, thanks to @quinnsam)
- Support for Lyasi light bulb (#1533, thanks to Eichhoernchen)
- Support for RGB(WW) controlled using Domoticz MQTT messages (#1459, thanks to @sq5gvm)
- Support for newer AL-LC02 boards with different pinout (#1469, thanks to @sq5gvm)
- Support for SmartLife Mini Smart Socket RGB (thanks to @kuppe234, #1411)
- Support for Gosund SP1 v2.3 (#1448)
- Support for OBI Wifi Schuko Plug V2 (#1408, thanks to @arthurf1969)
- Support for pulse meter power sensor for new-generation smart-meters
- Support for VL53L1X ToF sensor (thanks to @ruimarinho)
- Support for VEML6075 UV sensor (thanks to @ruimarinho)
- Support for EZO pH Circuit sensor (thanks to @ruimarinho)
- Support for MAX6675 temperature sensor (#1375, thanks to @lucciano)
- Support for MagicHome ZJ WFMN A/B v1.1 (#1339)
- Support for multiple PZEM004T sensors (thanks to @0x3333)
- Support for Support PMS5003S (#1511, thanks to @Yonsm)
- Support for pulse meter power sensor for new-generation smart-meters (including debouncing and energy ratio support by @jackwilson)
- Support for BMP085 and BMP180 sensors (#1082)
- Add dim up and down actions to button handler (#1250)
- Compact WS data (#1387)
- Improved analog sensor (#1326, thanks to @cconde)
- Report SSID in heartbeat messages
- Option to send full data to thinkgspeak on every message (#1369)
- Added RSSI to InfluxDB heartbeat (#1400, tahnks to @BuildTheRobots)
- Option to report time even if no NTP sync (#1310)
- Support for mixed combination of real and dummy relays (#1305)
- Report target color values on MQTT and API
- Note on WiFi tab about hostname (#1555)
- Allow saving heartbeat settings from web (#1538)
- Build images for Sonoff Basic R2 with DHT and DALLAS support
- Add warning about TELNET_PASSWORD
- Domoticz: track last relay state (#1536)
- Adding description field to web UI, reporting it via MQTT (#1523)
- ESP-01 + 2ch 5v relay LC tech Exclusive relay on (#1519, thanks to @clabnet)
- Add OTA support over MQTT (#1424, thanks to @Niek)
- Configure Heartbeat from WebUI & option HEARTBEAT_REPEAT_STATUS (#1474, thanks to martiera)
- Delay light comms (mqtt, ws, broker) to avoid jamming
- Added message type to broker
- Yield() after handling OTA request
- Disconnect websocket when auth fails
- Manage relay changes in third party modules via broker
- Added API entry points for RFBridge module (#1407)
- Domoticz over MQTT to Espurna RGB/RGBW/RGBWW
- Debug check position to make sure definition is not nullified to avoid putting checks in all places
- MQTT reconnect delay based on last disconnection
- Add terminal support for wifiDebug
- Created contribute.md and support.md files
- Created issue templates
- Runtime heartbeat configuration (#1406)
- APP_VERSION suffix (#1418)
- Allow {hostname} and {mac} placeholder for mqtt user and client_id fields (#1338)
- Split ws messages for relays and rf codes (#262)
- Added learn and forget terminal commands to RFBridge and RF modules (#1253)
- Change light transition time via MQTT or API (#1412)

### Changed
- Telnet password requirements (#1382)
- Separate tab for NoFUSS options (#1404)
- Updated to use gulp4 (#1403)
- Updated to EEPROM_Rotate 0.9.2
- Show proper switches names in web UI
- Removing loop delay if WIFI is not set to sleep, reducing it to 1ms otherwise (#1541)
- Change naming for BlitzWolf SHP2 and SHP6 (now SHPX) boards
- Print each HA config entry separately (#1535)
- Updated DebounceEvent to 2.0.5 (#1527, #1254)
- Python cleanup (@1526, thanks to Cabalist)
- Normalize naming for Arilux AL LC02 v14
- Increase version field size in OTA manager
- Merge RF and RFBridge code (#1435, thanks to @Niek)
- Update to fauxmoESP 3.1.0
- Move crash code to it's own module

## [1.13.3] 2018-10-08
### Fixed
- Honour build time settings for MQTT on fresh install (#719)
- Fix custom_crash_callback declaration for Arduino IDE 1.8.6 (#1169)
- Fix eneUnits key in web UI (#1177)
- Fix HA names (#1183)
- API is now restful (issue a PUT to change a relay status). It can be disabled from web UI (#1192)
- Remove static array to prevent out of bound in relay.ino (#1217)
- Remove duplicate call to EEPROMr.begin (#1214)
- Fix issue when SPIFFS_SUPPORT is enabled (#1225)
- Fix quoting units_of_measurement in HA config output (#1227)
- Fix "Clear counts" on rfm69 does not reset node count properly (thanks to @Trickx, #1239)
- Fix homecube 3rd led setting (thanks to @mcspr)
- Fix typo in static IP hint text (@thanks to @zafrirron)
- Fix hostname/password length requirements (thanks to @mcspr and @djwmarcx)
- Do not quote numbers in MQTT JSON payloads
- Fix telnet client object deletion (thanks to @mcspr)
- Call wakeUp PMS on first reading cycle to avoid not data in a long period (thanks to @Yonsm)
- Small fixes and windows support for ESPurna OTA Manager (thanks to @mcspr)
- Fix for YiDian XS-SSA05 configs (thanks to @ducky64)
- Send MQTT messages only for button events with assigned actions (thanks to @Valcob)
- Avoid EEPROM commits on callbacks (#1214)

### Added
- Option to report energy based on delta since last report (#369)
- Support for IR-MQTT bridge, also in RAW mode (#556, #907)
- Allow faster sensor reading intervals, down to 1 second (#848)
- Support for Xiaomi Smart Desk Lamp (#884)
- Retry up to 3 times on bad response to Thingspeak server (#1213)
- Support for apparent power and power factor on CSE7/XX sensor (#1215)
- Support for encoders
- Support for Allterco Shelly2
- Added SDS011 sensor support (thanks to @derlucas)
- Added password check to telnet (option to disable it)
- Added PHYX support (thanks to @whitebird)
- Added config command that outputs the configuration in JSON
- Support for MICS-2710, MICS-5525 and MICS-4514, gas sensors
- Support for iWoole LED Table Lamp (thanks to @CollinShorts)
- Command to output free stack
- Password management from web UI (thanks to @mcspr)
- Added BESTEK MRJ1011 support (thanks to @InduPrakash)
- Support for EXS WiFi Relay 5.0 (thanks to @cheise, #1218)
- Allowing disabling or single heartbeat on MQTT connect or repeat (default) (#1196)
- Command to save settings when SETTINGS_AUTOSAVE is off

### Changed
- Upgraded to JustWifi 2.0.2
- Upgraded to FauxmoESP 3.0.1
- Upgraded to DebounceEvent 2.0.4 to properly support BUTTON_SWITCH
- Split `info` command output into `info` and `wifi`. Refactor output.
- Custom HA payloads (thanks to @Yonsm)

## [1.13.2] 2018-08-27
### Fixed
- Fix relay overflow window length
- Fix TravisCI release condition (thanks to @mcspr, [#1042](https://github.com/xoseperez/espurna/issues/1042))
- Fix Sonoff RFBridge build in Arduino IDE ([#1043](https://github.com/xoseperez/espurna/issues/1043))
- Using corrent path separator in gulpfile.js (thanks to @InduPrakash, [#1045](https://github.com/xoseperez/espurna/issues/1045))
- Fix KMC70011 LED logic (thanks to @zerog2k, [#1056](https://github.com/xoseperez/espurna/issues/1056))
- Fix Luani HVIO to use 1MB flash size and toggle switch (thanks to @BauerPh, [#1065](https://github.com/xoseperez/espurna/issues/1065) and [#1068](https://github.com/xoseperez/espurna/issues/1068))
- Fix switches in Microsoft Edge (thanks to @Valcob, [#1066](https://github.com/xoseperez/espurna/issues/1066))
- Fix build.sh error handling (thanks to @mcspr, [#1075](https://github.com/xoseperez/espurna/issues/1075))
- Correctly init Serial on RELAY_PROVIDER_STM ([#1130](https://github.com/xoseperez/espurna/issues/1130))
- Disconnect before running WPS and SmartConfig discovery ([#1146](https://github.com/xoseperez/espurna/issues/1146))
- Fix sort fields in OTA manager

### Added
- Support for YJZK 1Ch and 3CH switches (thanks to @CollinShorts and @q32103940, [#1047](https://github.com/xoseperez/espurna/issues/1047))
- Support for AG-L4 color desk lamp (thanks to @zerog2k, [#1050](https://github.com/xoseperez/espurna/issues/1050))
- Option to cofigure ON/OFF payload at build time ([#1085](https://github.com/xoseperez/espurna/issues/1085))
- Option to change default payload for HA ([#1085](https://github.com/xoseperez/espurna/issues/1085))
- Support for Allterco Shelly1 (thanks to @abmantis, [#1128](https://github.com/xoseperez/espurna/issues/1128))
- Support for HomeCube 16A (thanks to @hyteoo, [#1106](https://github.com/xoseperez/espurna/issues/1106))
- Support for multiple sonar sensors (thanks to @ruimarinho, [#1116](https://github.com/xoseperez/espurna/issues/1116))
- Support for hardware serial on PMSX003 device (thanks to @ruimarinho, [#1122](https://github.com/xoseperez/espurna/issues/1122))
- Support for Lohas 9W bulbs (thanks to @steveway, [#1135](https://github.com/xoseperez/espurna/issues/1135))
- Show literal for webUI image in info ([#1142](https://github.com/xoseperez/espurna/issues/1142))
- Add RFBRIDGE code to full webUI image ([#1157](https://github.com/xoseperez/espurna/issues/1157))
- Handle events in EventSensor
- Option to remove API_SUPPORT at build time
- Option to save total energy in EEPROM after X reports, disabled by default
- Support for DHT12 sensor (thanks to Altan Altay)
- Support for 2MB flash boards

### Changed
- Update PlatformIO support to 3.6.X branch
- Explicitly disable ATC on RFM69 gateway ([#938](https://github.com/xoseperez/espurna/issues/938))
- Reduce memory footprint of API calls ([#1133](https://github.com/xoseperez/espurna/issues/1133))
- Init relay GPIO when in inverse mode to be OFF ([#1078](https://github.com/xoseperez/espurna/issues/1078))


## [1.13.1] 2018-07-10
### Fixed
- Build issues with Arduino IDE ([#975](https://github.com/xoseperez/espurna/issues/975))
- Right web interface image for with RF Bridge
- Full web interface image if light and sensor together ([#981](https://github.com/xoseperez/espurna/issues/981))
- Some devices still not using DOUT flash mode
- Crash on loading malformed configuration file
- Mismatch between memory size and layout size for some boards (this might require reflashing)
- Wrong settings report after factory reset
- Memory leak in JustWifi library
- New buttons not rendering right in Safari ([#1028](https://github.com/xoseperez/espurna/issues/1028))

### Added
- Support for RFM69GW board (see http://tinkerman.cat/rfm69-wifi-gateway/)
- Support for Sonoff IFAN02
- Support for NTC sensors ([#1001](https://github.com/xoseperez/espurna/issues/1001))
- Support for single-pin latched relays ([#1039](https://github.com/xoseperez/espurna/issues/1039))
- Check binary flash mode in web upgrade
- Sampling to AnalogSensor
- Parallel builds in Travis (thanks to @lobradov)

### Changed
- Reworked platformio.ini, build.sh files (thanks to @gn0st1c and @mcspr)

## [1.13.0] 2018-06-22
### Fixed
- Fixed PZEM004T compilation issues, working when using hardware serial ([#837](https://github.com/xoseperez/espurna/issues/837))
- Fixed per channel state on/off for lights ([#830](https://github.com/xoseperez/espurna/issues/830))
- Fixed overflow in CSE7766 energy calculation ([#856](https://github.com/xoseperez/espurna/issues/856))
- Fixed On MQTT disconnect in web UI ([#845](https://github.com/xoseperez/espurna/issues/845))
- Check valid hostnames ([#874](https://github.com/xoseperez/espurna/issues/874), [#879](https://github.com/xoseperez/espurna/issues/879))
- Fix Sonoff POW R2 configuration
- Fixed InfluxDB sensor by id ([#882](https://github.com/xoseperez/espurna/issues/882))
- Fix build when disabling WEB_SUPPORT ([#923](https://github.com/xoseperez/espurna/issues/923))
- Fix calibration error in EmonSensor ([#876](https://github.com/xoseperez/espurna/issues/876))
- Fix telnet and web debug responsiveness ([#896](https://github.com/xoseperez/espurna/issues/896))
- Use double quotes in JSON for non-numeric values ([#929](https://github.com/xoseperez/espurna/issues/929))
- Support connections over HTTPS via proxy ([#937](https://github.com/xoseperez/espurna/issues/937))

### Added
- EEPROM sector rotation using EEPROM_Rotate library
- Code filtering when building web UI images
- Added pulsing a relay via MQTT and REST API ([#896](https://github.com/xoseperez/espurna/issues/896), [#902](https://github.com/xoseperez/espurna/issues/902))
- Support for WPS (not available in pre-built binaries)
- Support for Smart Config (not available in pre-built binaries)
- Support for CCT lights (thanks to @Skaronator)
- Allow RELAYx_DELAY_ON/OFF also for none GPIO relay types (thanks to @zafrirron)
- Added relay status to Domoticz on MQTT connection ([#872](https://github.com/xoseperez/espurna/issues/872))
- Added configurable UART-to-MQTT terminator
- Added telnet link to web UI
- Reload terminal command to force all modules to reload settings from config ([#816](https://github.com/xoseperez/espurna/issues/816))
- Added security headers to each HTTP response (thanks to @ITNerdBox)
- Customized GET terminal command (thanks to @mcspr)
- More RC codes supported on TX for RF Bridge (thanks to @wildwiz)
- Support for BL0937 power monitoring chip with unmodified HLW8012 library ([#737](https://github.com/xoseperez/espurna/issues/737))
- Enable CORS
- Support for Allnet ESP8266 UP Relay (thanks to @bajo)
- Support for Tonbux Mosquito Killer (thanks to @gn0st1c)
- Support for Neo Coolcam NAS-WR01W WiFi Smart Power Plug
- Support for TYWE3S-based Estink WiFi Power Strip (thanks to @sandman, [#852](https://github.com/xoseperez/espurna/issues/852))
- Support for Pilotak ESP DIN V1
- Support for DIY Geiger counter (thanks to @Trickx)
- Support for HomeCube / Blitzwolf BW-SHP2
* Support for Vanzavanzu Smart Wifi Plug Mini
- Support for Bruno Horta's OnOfre board

### Changed
- Updated PlatformIO to use Core 3.5.3
- Updated to JustWifi 2.0
- CSS optimizations ([#870](https://github.com/xoseperez/espurna/issues/870), [#871](https://github.com/xoseperez/espurna/issues/871))
- Several changes in OTA Manager
- Better memory layout info
- MQTT keep alive time increased to 300s
- Using ticket-based authentication for WS
- Refactor module and sensor listings ([#896](https://github.com/xoseperez/espurna/issues/896))
- Using alternative math methods to save ~8Kb with lights
- Simpligying mired/kelvin methods
- Changed web UI checkboxes with pure CSS versions

### Deprecated
- {identifier} place-holder in MQTT base topic

## [1.12.6] 2018-05-02
### Fixed
- Check NTP_SUPPORT for sensors (thanks to @mcspr)
- Fix AM2302 sensor
- Fix hostname truncated to 20 chars when advertised to DHCP ([#774](https://github.com/xoseperez/espurna/issues/774))
- Decouple Serial object from Terminal, Debug modules ([#787](https://github.com/xoseperez/espurna/issues/787))
- Fix Arilux LC-01 definitions ([#797](https://github.com/xoseperez/espurna/issues/797))
- Do not uppercase hostname in web interface ([#799](https://github.com/xoseperez/espurna/issues/799))
- Ensure scheduler has access to all channels independently of the color mode ([#807](https://github.com/xoseperez/espurna/issues/807))

### Added
- Support for IteadStudio Sonoff S31 ([#497](https://github.com/xoseperez/espurna/issues/497))
- Option to ignore daylight saving in scheduler ([#783](https://github.com/xoseperez/espurna/issues/783))
- Report last energy reset datetime in web interface ([#784](https://github.com/xoseperez/espurna/issues/784))
- Added captive portal in AP mode
- Support for IR toggle mode (thanks to @darshkpatel)
- Support for IteadStudio Sonoff POW R2 (thanks to @ColinShorts)
- Support for Luani HVIO (thanks to @wildwiz)
- Support for Zhilde ZLD-EU55-W power strip (thanks to @wildwiz)
- Support for RFB_DIRECT Sonoff Bridge EFM8BB1 bypass hack (thanks to @wildwiz)
- Support for SenseAir S8 CO2 sensor (thanks to @Yonsm)
- Support for PMS5003T/ST sensors (thanks to @Yonsm)

### Changed
- Updated JustWifi Library
- Some cleanup in the web interface
- Refactored configuration files (thanks to @lobradov, @mcspr)
- Changes pre-commit hook (thanks to @mcspr)

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
