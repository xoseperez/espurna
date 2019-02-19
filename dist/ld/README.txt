# ESP8266 ldscripts

## PlatformIO

LIBPATH automatically picked based on platform.json:version

## Arduino IDE

### Pre 2.5.0

cp pre_2.5.0/*.ld <esp8266-package>/tools/sdk/ld

### 2.5.0 and up

cp latest/*.ld <esp8266-package>/tools/sdk/ld
