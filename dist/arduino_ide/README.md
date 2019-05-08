# boards.local.txt for ESPurna

Additional flash layouts to support multiple EEPROM sectors

### Installation

Place boards.local.txt into Arduino hardware directory, in the same directory as the boards.txt file. Depending on platform and ESP8266 installation method, it is one of:

- Linux (boards manager): `~/.arduino15/packages/esp8266/hardware/esp8266/<version>`
- Linux (git): `~/Arduino/hardware/esp8266com/esp8266/`
- Windows (boards manager): `%LOCALAPPDATA%\Arduino15\packages\esp8266\hardware\esp8266\<version>`
- Windows (git): `~\Documents\Arduino\hardware\esp8266com\esp8266\`
- macOS (boards manager): `~/Library/Arduino15/packages/esp2866/hardware/esp8266/<version>`
- macOS (git): `<application-directory>/Arduino.app/Contents/Java/hardware/esp8266com/esp8266`

Use `2.3.0/boards.local.txt` for Core version 2.3.0  
Use `latest/boards.local.txt` for all the others


### Arduino documentation

https://arduino-esp8266.readthedocs.io/en/latest/installing.html
https://www.arduino.cc/en/Hacking/Preferences
https://github.com/arduino/Arduino/wiki/Arduino-IDE-1.5-3rd-party-Hardware-specification#boardslocaltxt 
