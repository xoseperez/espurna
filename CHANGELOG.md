# ESPurna change log

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [1.15.0] In the works

### Fixed
#### Alexa
- Fix device discovery / state callback ([#2388](https://github.com/xoseperez/espurna/issues/2388), thanks to **[@aL1aL7](https://github.com/aL1aL7)**)
- Display the device name in the WebUI ([6b2c34ea](https://github.com/xoseperez/espurna/commit/6b2c34eaae92f196deaaea82ae2864ff2fc6e4cc))
#### Debug
- Don't treat static PROGMEM and generic C-strings differently ([b167d616](https://github.com/xoseperez/espurna/commit/b167d61615f65b618999d8ed727851a236867b8a), [d9662bd6](https://github.com/xoseperez/espurna/commit/d9662bd66ae9f902707f393a607a07ba713e1199))
- Off-by-one error when formatting to allocated buffer ([efcb863c](https://github.com/xoseperez/espurna/commit/efcb863ca271d3afa2e9accd990bb6adaa3f9652))
#### Domoticz
- Add workaround for pressure sensors ([#2215](https://github.com/xoseperez/espurna/issues/2215))
- Do not put floats into nvalue ([#2230](https://github.com/xoseperez/espurna/issues/2230))
- Constrain pressure to 0...100 ([#2230](https://github.com/xoseperez/espurna/issues/2230))
- Fix idx truncation when reading from settings ([#2316](https://github.com/xoseperez/espurna/issues/2316), thanks to **[@m-kozlowski](https://github.com/m-kozlowski)**)
- Allow dimmer device to control the brightness ([#2317](https://github.com/xoseperez/espurna/issues/2317), thanks to **[@m-kozlowski](https://github.com/m-kozlowski)**)
- Send co2 ppm as nvalue [d04b85ac](https://github.com/xoseperez/espurna/commit/d04b85ac974f73a68e4e57bce494cda4ac5d6b87)
#### Hardware
- Fix GPIO16 support ([#2110](https://github.com/xoseperez/espurna/issues/2110), thanks to **[@foxman69](https://github.com/foxman69)**)
- Fix for button long click ([#2172](https://github.com/xoseperez/espurna/issues/2172), thanks to **[@ElderJoy](https://github.com/ElderJoy)**)
- Fix latched pulse always being HIGH ([#2145](https://github.com/xoseperez/espurna/issues/2145), thanks to **[@antonio-fiol](https://github.com/antonio-fiol)**)
- Fix ADC\_MODE\_VALUE use in preprocessor ([#2227](https://github.com/xoseperez/espurna/issues/2227), thanks to **[@vtochq](https://github.com/vtochq)**)
- Actually apply button pulldown ([#2239](https://github.com/xoseperez/espurna/issues/2239), thanks to **[@sigmafx](https://github.com/sigmafx)**)
- Fix general.h comment typo ([#2311](https://github.com/xoseperez/espurna/issues/2311), thanks to **[@ruimarinho](https://github.com/ruimarinho)**)
#### HomeAssistant
- Fix swapped device model and manufacturer fields in the discovery ([#2322](https://github.com/xoseperez/espurna/issues/2322), thanks to **[@alextircovnicu](https://github.com/alextircovnicu)**)
#### I2C
- Make brzo i2c library buildable again ([19f32145](https://github.com/xoseperez/espurna/commit/19f3214578ce3429e9140c6a42d1575e4b7fa498), ref. [pasko-zh/brzo\_i2c#44](https://github.com/pasko-zh/brzo_i2c/issues/44))
#### Influxdb
- Fix http response parsing, refactor module scope ([#2153](https://github.com/xoseperez/espurna/issues/2153))
#### IR
- Fixed build error in case IR TX is not used in raw mode ([#2322](https://github.com/xoseperez/espurna/issues/2322), thanks to **[@alextircovnicu](https://github.com/alextircovnicu)**)
- Queue outgoing messages and correctly handle both simple and RAW repeat value by storing it in a message-local state instead of the global one ([aad70881](https://github.com/xoseperez/espurna/commit/aad70881b2e006d6c9b8d2a1544d633a511d7974))
- Repeat using the method provided by the library, implement delayed repeats using an additional field in both protocols ([aad70881](https://github.com/xoseperez/espurna/commit/aad70881b2e006d6c9b8d2a1544d633a511d7974))
- Implement 'state' parser and sender for complex protocols (e.g. HVAC, or longer than 64bit). User-provided payload will be handled by the internal protocol sender. But, notice that the payload needs to be fully formed; things like checksum won't be generated and the payload string will be used as-is ([8bab956f](https://github.com/xoseperez/espurna/commit/8bab956fd095da601769c7d1df98a017fe761009))
#### Lights
- Don't crash when GPIO16 is specified as `LIGHT_PROVIDER_DIMMER` channel pin ([b6b7c28c](https://github.com/xoseperes/espurna/commit/b6b7c28c3fc19bf8fb9b87592ff56fd03f269ccc), [#2472](https://github.com/xoseperez/espurna/issues/2472), thanks to **[@hamed-ta](https://github.com/hamed-ta)**)
#### MQTT
- Set MQTT will topic after /get suffix initialization ([#2106](https://github.com/xoseperez/espurna/issues/2106), [#2115](https://github.com/xoseperez/espurna/issues/2115), thanks to **[@tomas-bara](https://github.com/tomas-bara)**)
#### Nofuss
- Fix nofuss.cpp typo ([#2251](https://github.com/xoseperez/espurna/issues/2251), thanks to **[@CmPi](https://github.com/CmPi)**)
- Bump to 0.4.0 (fork) to support the latest Core version ([0422d61c](https://github.com/xoseperez/espurna/commit/0422d61c6969be9963e83850e10b7b217b6e9190))
#### PlatformIO
- Check the return code of the git process and fail early, when trying to generate the version string [bdd821db](https://github.com/xoseperez/espurna/commit/bdd821db8609277fef827ce533570818a7614f55)
#### Relay
- Fix sync reentrancy lock ([94169dcb](https://github.com/xoseperez/espurna/commit/94169dcbb19b8b83118aaf6c18daf6064cbfa76f))
- Stable configuration IDs ([04569c6a](https://github.com/xoseperez/espurna/commit/04569c6a10afe2a662a22c77c7977746f72ea7e1))
- Don't cancel saving the `relayBootMask` when relays are processed in a certain order ([0c57f0bc](https://github.com/xoseperez/espurna/commit/0c57f0bcf9944e375869544fafe0fc0455964aa4))
#### RPN rules
- rpn $relayX variables were not populated on boot ([#2246](https://github.com/xoseperez/espurna/issues/2246), thanks to **[@pezinek](https://github.com/pezinek)**)
- add missing lights #include for rpn rules ([#2367](https://github.com/xoseperez/espurna/issues/2367), thanks to **[@ngilles](https://github.com/ngilles)**)
#### Scheduler
- Schedule restore no longer depends on relays [c945c239](https://github.com/xoseperez/espurna/commit/c945c239ea806f8926aefe6262a52177bd089aa5), [e22f67e5](https://github.com/xoseperez/espurna/commit/e22f67e5d61ce8c3c1eb1f9a50f2d6261b8b8d57)
#### Sensor
- Apparent, reactive power measurement unit corrections ([#2161](https://github.com/xoseperez/espurna/issues/2161), thanks to **[@irmishappy](https://github.com/irmishappy)**)
- Fixes and updates for thermostat and display ([#2173](https://github.com/xoseperez/espurna/issues/2173), thanks to **[@ElderJoy](https://github.com/ElderJoy)**)
- Properly dispatch emon sensor ratio defaults ([#2241](https://github.com/xoseperez/espurna/issues/2241))
- Set pH decimals to 3 ([#2306](https://github.com/xoseperez/espurna/issues/2306), thanks to **[@ruimarinho](https://github.com/ruimarinho)**)
- Fix setting up GPIO0 as INPUT when preparing to use analogRead(A0) ([ff11e581](https://github.com/xoseperez/espurna/commit/ff11e5814ff1fc938afa439c15f57fa9909b9e4a))
- Only change EventSensor counter from the ISR ([735e5c0e](https://github.com/xoseperez/espurna/commit/735e5c0ec22fabfdcfa55123b9bceaa3d8f917b8))
- Fix a typo when getting local index for Nth magnitude ([6ba5f95e](https://github.com/xoseperez/espurna/commit/6ba5f95e875468f2b7a93c69b45fc6f6c62f390f))
- SHT3X: add missing I2C address A (0x44) ([#2484](https://github.com/xoseperez/espurna/issues/2484), thanks to **[@drc38](https://github.com/drc38)**)
- Deprecate ...\_MIN\_CHANGE build flags in favour of runtime settings [1ef22e16](https://github.com/xoseperez/espurna/commit/1ef22e16f10818d893a0a8912d55b1dbce88fcdb)
- Reduce IRAM usage in sensors using attachInterrupt() [9db679f9](https://github.com/xoseperez/espurna/commit/9db679f93a61114dec8dad5f2953e59b7663c86a)
#### Settings
- Fix saving base2 integers ([71ddf350](https://github.com/xoseperez/espurna/commit/71ddf35022678667d0269ecc9c60c69bdab68079))
#### System
- Rework stability counter ([474f0e93](https://github.com/xoseperez/espurna/commit/474f0e93693387f2c85fa28d5df7e0c80716c85a))
- Refactor build configurations ([f9211634](https://github.com/xoseperez/espurna/commit/f92116341e141be50f946682404e2d0514fd11f3))
- Clean-up helper classes & functions ([ec220b7d](https://github.com/xoseperez/espurna/commit/ec220b7dd1f3b26e81138cec55beec8e37ab35f9))
#### TUYA
- Send lights channel value directly ([012c3818](https://github.com/xoseperez/espurna/commit/012c3818a59cd46cc89e2affc5ddcdea427a17c1))
- Always run the discovery ([2a08ccb2](https://github.com/xoseperez/espurna/commit/2a08ccb2113f8be4495a58aa7b95331591ebcd0b))
#### WebUI
- Fix scheduler panel tabindex= values ([#2096](https://github.com/xoseperez/espurna/issues/2096), thanks to **[@foxman69](https://github.com/foxman69)**)
- Directly iterate over internal callbacks array ([#2248](https://github.com/xoseperez/espurna/issues/2248), [#2261](https://github.com/xoseperez/espurna/issues/2261))
- Get rid of tabindex= property ([14c69a4a](https://github.com/xoseperez/espurna/commit/14c69a4a52cc842c0bf294786ea34904acf0aeec))
- External url clean-up ([d60fb47c](https://github.com/xoseperez/espurna/commit/d60fb47ca9be2f591b82f72678b36b02c1c79beb))
- Remove hard-coded group keys list ([1627e311](https://github.com/xoseperez/espurna/commit/1627e3119fe9084398b8e8c0ec794b8ed3a4f6b6))
- Send alert messages directly ([458fb7d9](https://github.com/xoseperez/espurna/commit/458fb7d936ec9d266cfce275f999dd629fb82e2f))
- Set websocket buffer to `nullptr` before returning control to the webserver, which will try to `free()` it ([256e790e](https://github.com/xoseperez/espurna/commit/256e790e4d7374e12430dad57e25ece7b880be25))

### Added
#### Buttons
- Runtime configuration. (`btnGpio#`, `btnProv#`, etc.), see `button` and `button <id>` terminal commands output.
- Custom action type ([ef194c9c](https://github.com/xoseperez/espurna/commit/ef194c9c2430ed14ddf5905214e2968c0f5f9980), [8ceeebdb](https://github.com/xoseperez/espurna/commit/8ceeebdb24aa9bd863fa28676a0364497ec193ae))
#### Garland
- New module for digital LED strips ([#2408](https://github.com/xoseperez/espurna/issues/2408), [c4d817c4](https://github.com/xoseperez/espurna/commit/c4d817c4fba05d70808b234eef3ac5d1ec2bf8c0), [46daa929](https://github.com/xoseperez/espurna/commit/46daa929f5e284877e105208c4e78f7844ae1b64), [d11f82d0](https://github.com/xoseperez/espurna/commit/d11f82d098a69a4a127a8db3218c5643f9831371), [4923377e](https://github.com/xoseperez/espurna/commit/4923377eacc5158896e8fd9ddbc993d1bb2653be), [6508f6bd](https://github.com/xoseperez/espurna/commit/6508f6bda8da2acef555fd2b909b7e2983b97e83), [24550a5b](https://github.com/xoseperez/espurna/commit/24550a5b80e1a626a7d8090746c0cfda2bfb4b23), [4efc417a](https://github.com/xoseperez/espurna/commit/4efc417a39220638079bdf060b0fc204d777f942), [f640cd8e](https://github.com/xoseperez/espurna/commit/f640cd8ecb3d170e9082d60461b89e29454bbbd4), [518d56b4](https://github.com/xoseperez/espurna/commit/518d56b442dda92c267fdb71e6b51b8a27638c0f), [0f73df7c](https://github.com/xoseperez/espurna/commit/0f73df7c36f940040c6a6416c15d39c7eee213be), [3fe68748](https://github.com/xoseperez/espurna/commit/3fe68748637099f08008fa5afc3650e09551285f), [dad8878c](https://github.com/xoseperez/espurna/commit/dad8878ccfcef68f616fed7296b0f07983d855c3), [660ae138](https://github.com/xoseperez/espurna/commit/660ae138d4a40bd3c48058f46d086d396fb217e0), thanks to **[@ElderJoy](https://github.com/ElderJoy)**)
#### Debug
- Optionally store boot log ([#2109](https://github.com/xoseperez/espurna/issues/2109))
- Log mode, allow to skip boot messages ([#2116](https://github.com/xoseperez/espurna/issues/2116))
#### Hardware
- KingArt WiFi Curtain Switch ([#2063](https://github.com/xoseperez/espurna/issues/2063), thanks to **[@AlbertWeterings](https://github.com/AlbertWeterings)**)
- Add support for Kogan Smarter Home Plug With Energy Meter ([#2086](https://github.com/xoseperez/espurna/issues/2086), thanks to **[@aureq](https://github.com/aureq)**)
- Add support for Teckin SB53 smart bulb ([#2090](https://github.com/xoseperez/espurna/issues/2090), thanks to **[@marcuswinkler](https://github.com/marcuswinkler)**)
- Add Shelly 1PM GPIO picture ([#2092](https://github.com/xoseperez/espurna/issues/2092), thanks to **[@lblabr](https://github.com/lblabr)**)
- Add MagicHome ZJ\_LB\_RGBWW\_L support ([#2100](https://github.com/xoseperez/espurna/issues/2100), thanks to **[@wwilsman](https://github.com/wwilsman)**)
- Deltaco smart home devices ([#2103](https://github.com/xoseperez/espurna/issues/2103), thanks to **[@orrpan](https://github.com/orrpan)**)
- Added hardware config for Avatto NAS-WR01W ([#2113](https://github.com/xoseperez/espurna/issues/2113), thanks to **[@blockmar](https://github.com/blockmar)**)
- Config for Teckin SP23 & Maxcio W-UK007S ([#2157](https://github.com/xoseperez/espurna/issues/2157), thanks to **[@julianwb](https://github.com/julianwb)**)
- Add support for read PIO-A of DS2406 ([#2174](https://github.com/xoseperez/espurna/issues/2174), thanks to **[@rmcbc](https://github.com/rmcbc)**)
- Example for Generic ESP01 boards with 512KiB flash ([#2185](https://github.com/xoseperez/espurna/issues/2185), thanks to **[@ziggurat29](https://github.com/ziggurat29)**)
- Board definition for the Gosund WP3 smart socket ([#2191](https://github.com/xoseperez/espurna/issues/2191), thanks to **[@ziggurat29](https://github.com/ziggurat29)**)
- correct Gosund WP3 LED documentation and provide reasonable default actions ([#2200](https://github.com/xoseperez/espurna/issues/2200), thanks to **[@ziggurat29](https://github.com/ziggurat29)**)
- Add support for HUGOAI smart socket plug. ([#2243](https://github.com/xoseperez/espurna/issues/2243), thanks to **[@estebanz01](https://github.com/estebanz01)**)
- Add support for Aoycocr X5P Plug. ([#2235](https://github.com/xoseperez/espurna/issues/2235), thanks to **[@estebanz01](https://github.com/estebanz01)**)
- Implement support for ProDino WIFI ([#2269](https://github.com/xoseperez/espurna/issues/2269), thanks to **[@dpeddi](https://github.com/dpeddi)**)
- Add support for AG-L4 v3 ([#2276](https://github.com/xoseperez/espurna/issues/2276)), thanks to **[@andrej-peterka](https://github.com/andrej-peterka)**
- Including support for Arlec PC190HA/PB89HA ([#2286](https://github.com/xoseperez/espurna/issues/2286), thanks to **[@mafrosis](https://github.com/mafrosis)**)
- Add support for the Zhilde ZLD-64EU-W ([#2342](https://github.com/xoseperez/espurna/issues/2342), thanks to **[@biot](https://github.com/biot)**)
- Add support for Fcmila E27 7W RGB+W light bulb ([#2353](https://github.com/xoseperez/espurna/issues/2353), thanks to **[@user176176](https://github.com/user176176)**)
- Resistor ladder / analog buttons support ([#2357](https://github.com/xoseperez/espurna/issues/2357))
- Add support for Gosund SP111 (hardware version 1.1 16A) ([#2360](https://github.com/xoseperez/espurna/issues/2360), [#2369](https://github.com/xoseperez/espurna/issues/2369), thanks to **[@alextircovnicu](https://github.com/alextircovnicu)**)
- Add support for LSC E27 10W white bulb ([#2375](https://github.com/xoseperez/espurna/issues/2375), thanks to **[@tom-kaltofen](https://github.com/tom-kaltofen)**)
- Add support for Benexmart GU5.3 RGBWW light ([#2381](https://github.com/xoseperez/espurna/issues/2381), thanks to **[@ngilles](https://github.com/ngilles)**)
- Add support for Gosund P1 Power Strip ([#2391](https://github.com/xoseperez/espurna/issues/2391), thanks to **[@alextircovnicu](https://github.com/alextircovnicu)**)
- Add support for Mirabella Genio White A60 globe ([#2439](https://github.com/xoseperez/espurna/issues/2439], [2fc559fa](https://github.com/xoseperez/espurna/commit/2fc559fa5596c6ae3f3cc906177e287c38c6333e), thanks to **[@andrewleech](https://github.com/andrewleech)**)
- Refactor iFan into a separate module ([a40eca30](https://github.com/xoseperez/espurna/commit/a40eca30ad79315afdb67afa0b0743d4c0087e93))
- Add support for Yagusmart switches ([#2488](https://github.com/xoseperez/espurna/pull/2488), thanks to **[@MelanieT](https://github.com/MelanieT)**)
#### HomeAssistant
- Advertise lights transition support in the discovery message ([4d157ccd5](https://github.com/xoseperez/espurna/commit/4d157ccd5bd5ffefa8b0bca79c4b2196c8a3e5dc))
- Reworked discovery, implement retries and queueing using the MQTT broker ACKs ([59269789](https://github.com/xoseperez/espurna/commit/59269789dc80308e9afc1e4b3051d9d33e13bf8f))
#### HTTP API
- Handle received data as terminal command [#2247](https://github.com/xoseperez/espurna/issues/2247))
- [Prometheus](https://prometheus.io/) metrics support ([#2332](https://github.com/xoseperez/espurna/issues/2332))
- Scheduler API ([#2431](https://github.com/xoseperez/espurna/issues/2431), thanks to **[@profawk](https://github.com/profawk)**)
#### IR
- Reworked module to support both simple and raw protocol at the same time ([aad70881](https://github.com/xoseperez/espurna/commit/aad70881b2e006d6c9b8d2a1544d633a511d7974))
- Support terminal commands execution when receiving IR code or changing relay state ([aad70881](https://github.com/xoseperez/espurna/commit/aad70881b2e006d6c9b8d2a1544d633a511d7974))
- More runtime configuration options ([308da556](https://github.com/xoseperez/espurna/commit/308da5563a3d701f0fc84662b93c3b5aa2c71995))
#### MQTT
- Handle received payload as terminal input (by default, `<root topic>/cmd`) ([#2247](https://github.com/xoseperez/espurna/issues/2247))
- Publish data with `MQTT.SEND <TOPIC> <PAYLOAD>` terminal command ([#2478](https://github.com/xoseperez/espurna/issues/2478), thanks to **[@pbek](https://github.com/pbek)**)
#### TUYA
- Updated build defaults based on [#2414](https://github.com/xoseperez/espurna/issues/2414) discussion ([92d5e7be](https://github.com/xoseperez/espurna/commit/92d5e7becba23552c836bda8404305a8dc8eb07d))
#### PlatformIO
- Use development version of PlatformIO Core in CI ([#2146](https://github.com/xoseperez/espurna/issues/2146), thanks to **[@ivankravets](https://github.com/ivankravets)**)
- Add '.example' files. ([#2257](https://github.com/xoseperez/espurna/issues/2257), thanks to **[@davebuk](https://github.com/davebuk)**)
- Create .map file for the resulting .elf to debug possible compilation issues ([21794b78](https://github.com/xoseperez/espurna/commit/21794b789296683b7ae00a209a42f35ab1023fa1), [1ed00f57](https://github.com/xoseperez/espurna/commit/1ed00f57683197608418a482f0b3b262991856f4))
#### Relay
- Runtime configuration. (`relayGpio#`, `relayProv#`, etc.). See `relay` and `relay <id>` terminal commands output.
- Support multiple provider types (GPIO, virtual, IO expanders, etc.)
- Separate MQTT group subscription and publish topics (per-relay `relayTopicSub#` and `relayTopicPub#` respectively)
- Support MQTT wildcards (`#` and `+`) in group subscription topic ([dcc423ec](https://github.com/xoseperez/espurna/commit/dcc423ecaf556082ea7d358b886167f6ad179a21))
- Remove internal pulse timer limit of 1 hour and 14 minutes, current limit should be around 47 days. ([#678](https://github.com/xoseperez/espurna/issues/678))
- Support extended time string with a unit suffix 'h' for hours, 'm' for minutes and 's' for seconds; such as, '5h' for 5 hours, '3m' for 3 minutes and '15s' for 15 seconds. For example, '1h35m'. Note that units are interpretted from the largest to the smallest, time string such as '1h2h' will be rejected and treated as 0 seconds instead of 1 hours plus 2 hours. ([#2139](https://github.com/xoseperez/espurna/issues/2139))
- Add `pulse <id> <time>` terminal command which accepts the same time string format or a floating point number of seconds. ([#2139](https://github.com/xoseperez/espurna/issues/2139))
#### RFBridge
- Keep serial disabled in the sonoff rfbridge hardware.h entry ([10519cc2](https://github.com/xoseperez/espurna/commit/10519cc276383b622222a457a19e55d7972d332f))
- Allow to use `<code>,<times>` in rfbON\# / rfbOFF\# settings keys, just like with the API payload ([19947c12](https://github.com/xoseperez/espurna/commit/19947c1231c067301427303c77316565b9163bb4))
- Terminal commands to send the code ([52a244db](https://github.com/xoseperez/espurna/commit/52a244db6e9fe4ad373b580ed4e504c0d84d6afd))
- Support all available relay providers, not just the DUMMY variant. Allow to control real GPIO relays with received RF codes.
#### RPN Rules
- `oneshot_ms`, `every_ms` timer support.
- rfbridge operators (`rfb_send`, `rfb_pop`, `rfb_info`, `rfb_sequence`, `rfb_match`, `rfb_match_wait`) and mqtt fixes ([#2302](https://github.com/xoseperez/espurna/issues/2302))
- system operators `sleep` and `rtcmem` ([#2366](https://github.com/xoseperez/espurna/issues/2366))
#### Sensors
- HLW8012: gpio runtime configuration ([#2142 (initial Pull Request)](https://github.com/xoseperez/espurna/issues/2142), [fa5e4f7d](https://github.com/xoseperez/espurna/commit/fa5e4f7d06db6bde01f3bebf4f2c8151893c97aa))
- Add SI1145 sensor ([#2216](https://github.com/xoseperez/espurna/issues/2216), thanks to **[@HilverinkJ](https://github.com/HilverinkJ)**)
- Add HDC1080 sensor ([#2227](https://github.com/xoseperez/espurna/issues/2227), thanks to **[@vtochq](https://github.com/vtochq)**)
- HLW8012: energy\_delta ([#2230](https://github.com/xoseperez/espurna/issues/2230))
- Load ratios after boot + show pwr defaults with `get` ([#2241](https://github.com/xoseperez/espurna/issues/2241))
- Default Emon ratios at compile time [12ae9d15](https://github.com/xoseperez/espurna/commit/12ae9d15be3f282c30bd5f6b39680d4de1e0ca85)
- Add BME680 sensor support ([#2429](https://github.com/xoseperez/espurna/issues/2429), [#2361](https://github.com/xoseperez/espurna/issues/2361), [#2295](https://github.com/xoseperez/espurna/issues/2295), thanks to **[@ruimarinho](https://github.com/ruimarinho)**)
- Add support for SmartMeasure SM300D2-VO2 air quality multi-sensor ([#2447](https://github.com/xoseperez/espurna/issues/2447), thanks to **[@xoseperez](https://github.com/xoseperez)**)
- Shared ADS1X115 I2CPort, support common gain & data rate settings [c056c54d](https://github.com/xoseperez/espurna/commit/c056c54db4a528d038584fbfacb8fb410c7c7a2e)
- Terminal commands to set expected ratio (`EXPECTED`) and total energy recorded by the sensor (`ENERGY`) [8f7f1c96](https://github.com/xoseperez/espurna/commit/8f7f1c968f92c42f4f80c53ddfb617af18b68a85)
#### Settings
- Led and button GPIO runtime settings ([#2117](https://github.com/xoseperez/espurna/issues/2117), [#2162](https://github.com/xoseperez/espurna/issues/2162), [#2170](https://github.com/xoseperez/espurna/issues/2170), [#2177](https://github.com/xoseperez/espurna/issues/2177))
- Configure light dimmer pins from settings ([#2129](https://github.com/xoseperez/espurna/issues/2129))
#### System
- Detect Tasmota magic numbers when booting, and do a preventive factory reset ([#2370](https://github.com/xoseperez/espurna/issues/2370))
#### Terminal
- Show pretty uptime with NTP\_SUPPORT ([#2137](https://github.com/xoseperez/espurna/issues/2137))
- Change command-line parser ([#2245](https://github.com/xoseperez/espurna/issues/2245), [#2247](https://github.com/xoseperez/espurna/issues/2247))
#### Thingspeak
- Configure Thingspeak URL at runtime ([#2124](https://github.com/xoseperez/espurna/issues/2124), thanks to **[@sametflo](https://github.com/sametflo)**)
- Refactor deprecated WiFiClientSecure ([#2140](https://github.com/xoseperez/espurna/issues/2140), [#2144](https://github.com/xoseperez/espurna/issues/2144))
#### WebUI
- WebUI: alert when WS closes ([#2131](https://github.com/xoseperez/espurna/issues/2131), thanks to **[@foxman69](https://github.com/foxman69)**)
- Allow to disable Web(UI) OTA support ([#2190](https://github.com/xoseperez/espurna/issues/2190))
- Kingart curtain switch UI support ([#2250](https://github.com/xoseperez/espurna/issues/2250), thanks to **[@echauvet](https://github.com/echauvet)**)
- Refactor WS implementation, add some comments to the header ([#2261](https://github.com/xoseperez/espurna/issues/2261))
- Support Web(UI) OTA upgrades when the default web server is disabled ([3ff460db](https://github.com/xoseperez/espurna/commit/3ff460db4af8e0b3df07ed04bb736941d47ca1ae))
#### WiFi
- Try to connect to a better AP, when the current RSSI is below -73dBm (only when WiFi scanning is enabled) ([f0f7dcc8](https://github.com/xoseperez/espurna/commit/f0f7dcc874d6f6f4b095b6cb89e69cdb65219150), [dde5f374](https://github.com/xoseperez/espurna/commit/dde5f374dd038afe1fb966d31e16bdac1be581fb), [5a973298 (initial commit)](https://github.com/xoseperez/espurna/commit/5a97329832816219a919c4669e22ad6af0c8d228))
- Allow to set bssid and channel, when scanning is disabled ([c5f70286](https://github.com/xoseperez/espurna/commit/c5f70286d1e63972446c2148914352d9f6acf345))

### Changed
#### Build
- Convert .ino -> .cpp ([#1306](https://github.com/xoseperez/espurna/issues/1306), [#2228](https://github.com/xoseperez/espurna/issues/2228), [#2234](https://github.com/xoseperez/espurna/issues/2234), [#2236](https://github.com/xoseperez/espurna/issues/2236))
- Rework build.sh & new release script generator ([75b51f1e](https://github.com/xoseperez/espurna/commit/75b51f1e80260e2325709e7426fc5b2ebd88ada9), [74e18a59](https://github.com/xoseperez/espurna/commit/74e18a59bcbe7a2ea72fccb6d4e5e484bf348bb9))
- Use python 3.x in CI and move to Github Actions.
- Use eslint and html-validate in CI ([433f399d](https://github.com/xoseperez/espurna/commit/433f399d9ce769e57ce660d93161649f6287e054))
- Simplify version + revision into just version ([f0f6f1b8](https://github.com/xoseperez/espurna/commit/f0f6f1b8c907fbf188704e3055210d8202a12f21))
- Remove Core 2.3.0 support from .ld scripts ([a1e7941f](https://github.com/xoseperez/espurna/commit/a1e7941fa60339fed84f259033523e1e17e3f17d))
#### Domoticz
- Separate lights IDX from relays, migrate existing configuration from `dczRelayIdx0` to `dczLightIdx` ([94f31241](https://github.com/xoseperez/espurna/commit/94f31241dc42508791d6a582cd163bec33a40a56))
#### Hardware
- lightfox relay provider & buttonAdd ([bd3a5889](https://github.com/xoseperez/espurna/commit/bd3a588977fb8b195f2bba40618839b617767485))
#### HTTP API
- Rework plain and JSON implementations ([#2405](https://github.com/xoseperez/espurna/issues/2405))
#### IR
- Simple and RAW text protocols reworked ([aad70881](https://github.com/xoseperez/espurna/commit/aad70881b2e006d6c9b8d2a1544d633a511d7974))
- IR\_BUTTON\_SET >=0 depends on TERMINAL\_SUPPORT ([aad70881](https://github.com/xoseperez/espurna/commit/aad70881b2e006d6c9b8d2a1544d633a511d7974))
#### Libraries
- Bump RFM69 version ([#2148](https://github.com/xoseperez/espurna/issues/2148))
- Pin arduino-mqtt version ([#2154](https://github.com/xoseperez/espurna/issues/2154))
- Update IRremoteESP8266 to 2.8.0 ([c33d9960](https://github.com/xoseperez/espurna/commit/c33d9960b4a7a04b8bce516be152d5df96ec9cc2))
- Use [fork of fauxmoesp](https://github.com/vintlabs/fauxmoESP), thanks to **[@m-kozlowski](https://github.com/m-kozlowski)**
- Use [fork of rc-switch](https://github.com/1technophile/rc-switch) ([7a24806a](https://github.com/xoseperez/espurna/commit/7a24806adb2c3e2357171e004b5b760daf3bdca4))
#### Lights
- Controlling global state no longer requires `RELAY_SUPPORT` or specifying a virtual relay in the configuration. Updated modules and APIs to use light controls directly ([2f39d0db](https://github.com/xoseperez/espurna/commit/2f39d0db8a71533dac0cf7c27a719d0097a001d2))
- Do not call the provider or run any transitions when channel values remain unchanged ([2f39d0db](https://github.com/xoseperez/espurna/commit/2f39d0db8a71533dac0cf7c27a719d0097a001d2))
- Color mode white factor calculations no longer ignore the fractional part of the number ([32aae703](https://github.com/xoseperez/espurna/commit/32aae70374833bf47a47fd56e89f8416dc28700f))
#### MQTT
- Set keepalive to be less than heartbeat interval ([#2154](https://github.com/xoseperez/espurna/issues/2154))
- Always buffer incoming data ([#2181](https://github.com/xoseperez/espurna/issues/2181))
- Set default heartbeat mode to repeat ([f4726d99](https://github.com/xoseperez/espurna/commit/f4726d996636aeaff2e1b62383e2bc5dc00e4a59))
- MDNS auto-connect only works when MQTT is enabled ([06fa5b1c](https://github.com/xoseperez/espurna/commit/06fa5b1c6d3705df48130ad4fe4d946227d4b08e))
#### NTP
- Use sntp app from lwip on latest Cores, replace NtpClient ([#2132](https://github.com/xoseperez/espurna/issues/2132))
- Simplify NTP tick callback, dont use broker ([13cbc031](https://github.com/xoseperez/espurna/commit/13cbc0310a054309db595451a787bc10f0ab5ca2))
- Remove legacy module based on [NtpClient](https://github.com/gmag11/NtpClient) ([2de44ed5](https://github.com/xoseperez/espurna/commit/2de44ed5d94cc88378b261cebd53c9aa8c4a992e))
- Updates to support 64bit time\_t.
#### PlatformIO
- Update latest Arduino Core platform to 3.0.2 ([1ca8d5e7](https://github.com/xoseperez/espurna/commit/1ca8d5e7a0130c2c23e958208b176bb8e8312d7c))
- Use SoftwareSerial library from the Core ([23da0b74](https://github.com/xoseperez/espurna/commit/23da0b74d403cebc27b6ae0ca520da3218bf7a47))
- Remove -ota envs, handle OTA condition in extra script ([#2099](https://github.com/xoseperez/espurna/issues/2099))
- platformio.ini refactoring ([#2212](https://github.com/xoseperez/espurna/issues/2212))
- Rename generic environments ([#2214](https://github.com/xoseperez/espurna/issues/2214))
    - esp8266-\<flavour\>-\<size\>-base to esp8266-\<size\>-\<flavour\>-base
    - espurna-base to espurna-core-webui
- Consistent shared libs location with CI and local install, prefer $repo/code/libraries ([f18f128e](https://github.com/xoseperez/espurna/commit/f18f128e4bb718f448ca460cdb0e39545187d7fe))
- Pin libraries versions as \<owner\>/\<name\> to fix possible issues with Trusted Package Registry ([a9220ec2b](https://github.com/xoseperez/espurna/commit/a9220ec2b27224b2da79880945f6f58450ba53e8))
- Add `pio run -e $env -t build-and-copy`, more configuration options for the version string ([4c33cacf](https://github.com/xoseperez/espurna/commit/4c33cacfdbe4c51ff52ffb9f530006dfa7037a6b))
#### RPN Rules
- Set MQTT variables just before running the rules ([32b864c5](https://github.com/xoseperez/espurna/commit/32b864c56394016666b716c1623aaf9c85432ed3), [658ce105](https://github.com/xoseperez/espurna/commit/658ce1056e3f11832bce3457c91c0c325c24f509))
#### Sensors
- Emon refactoring ([#2213](https://github.com/xoseperez/espurna/issues/2213))
- Further EmonSensor fixes and refactoring ([b19905a3](https://github.com/xoseperez/espurna/commit/b19905a3065672412351c38d859fc3f6cd7ad5cd))
- Rename generic pwr keys with a typed prefix ([1a36efb8](https://github.com/xoseperez/espurna/commit/1a36efb8f2032ac81c5aaa51623a71234b1c4287))
- Tweak analogRead() frequency in Emon sensor ([c136678a](https://github.com/xoseperez/espurna/commit/c136678a4f02b7cae2e59fe843c3910a660f49d1))
- Remove `PZ.RESET` and `PZ.VALUE` commands in favour of `EXPECTED`, `ENERGY` and `MAGNITUDES` [8f7f1c96](https://github.com/xoseperez/espurna/commit/8f7f1c968f92c42f4f80c53ddfb617af18b68a85)
#### Settings
- Refactor get/set/del/hasSetting ([#2048](https://github.com/xoseperez/espurna/issues/2048))
- Update migrate configuration & conditions, allow each module to access the current & previous version ([#2176](https://github.com/xoseperez/espurna/issues/2176))
#### System
- Use direct status update functions instead of broker ([78b4007f](https://github.com/xoseperez/espurna/commit/78b4007f01e8df9334d16e9550a03443527176f2))
- Use 64bit microseconds time source for uptime, no need to count overflows manually ([1ca98880](https://github.com/xoseperez/espurna/commit/1ca98880d64db0865d02f009002bc22e32ae5076))
- Update load average and system stability check intervals to use seconds instead of milliseconds ([1ca98880](https://github.com/xoseperez/espurna/commit/1ca98880d64db0865d02f009002bc22e32ae5076))
- Update websocket client timeout and update message configuration to use seconds instead of milliseconds ([135c7b80](https://github.com/xoseperez/espurna/commit/135c7b80acbfd28136146f08188d81262afd795c))
#### Terminal
- Rework boot info and terminal commands ([7ea73554](https://github.com/xoseperez/espurna/commit/7ea735548bcd41742fac32e8733b2084c4c334cd))
#### WebUI
- Remove jquery dependencies and clean-up websocket API ([fa3deeff](https://github.com/xoseperez/espurna/commit/fa3deeffbfa622ecd1869af2563940fb3143e94e), [84a7f633](https://github.com/xoseperez/espurna/commit/84a7f6337f72b011512b3e95efe36f2d661e5065), [8e5ab5c9](https://github.com/xoseperez/espurna/commit/8e5ab5c902a23dfd774dd9e768963856d4f26bd3))
- Use [iro.js](https://github.com/jaames/iro.js) as color picker ([808981ca](https://github.com/xoseperez/espurna/commit/808981ca3938d11d4ddd87005e2881433cc7707b))
- Use [terser](https://github.com/terser/terser) as js minifier, webui is no longer limited to ES5 feature set ([cfd6e36d](https://github.com/xoseperez/espurna/commit/cfd6e36dbe94ee0e8098351357f903c060fd5dc9))

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
- Add GENERIC\_E14, e14 rgb+w 4,5w ([#2039](https://github.com/xoseperez/espurna/2039), thanks to **[@orrpan](https://github.com/orrpan)**)
- Add support for LinkSprite R4 ([#2042](https://github.com/xoseperez/espurna/issues/2042), thanks to **[@mpcusack](https://github.com/mpcusack)**)
- Add support for eHomeDIY devices. ([#2046](https://github.com/xoseperez/espurna/issues/2046), thanks to **[@user890104](https://github.com/user890104)**)
- Add support for MAGICHOME\_ZJ\_WFMN\_C\_11 ([#2051](https://github.com/xoseperez/espurna/issues/2051), thanks to **[@davebuk](https://github.com/davebuk)**)
- Add support for the LSC LED LIGHT STRIP from ACTION using a tuya chip. ([#2065](https://github.com/xoseperez/espurna/issues/2065), thanks to **[@sehraf](https://github.com/sehraf)**)
- Add LOHAS\_E26\_A19, rename LOHAS\_9W to LOHAS\_E27\_9W ([#2068](https://github.com/xoseperez/espurna/issues/2068), thanks to **[@cro](https://github.com/cro)** for providing A19 configuration)
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
- Force get\_device\_size to return an int in OTA manager
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
