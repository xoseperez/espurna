# Docker build image

This directory contains a docker file for an ESPurna build environment. All docker commands are intended to be run from your main source repository directory.

Two volumes can be used. 
* `/espurna` with ESPurna source code.
* `/firmware` target directory for complied firmware files.

Using the commands below, files would be output to `/tmp/firmware/` on your docker host system.

## Build

```bash
# from the main repo directory espurna/
sudo docker build -t espurna-build -f docker/Dockerfile .
```

## Examples

### Listing Firmware Images
```
sudo docker run --rm -it -v /tmp/firmware/:/firmware espurna-build -l
--------------------------------------------------------------
Available environments:
* aithinker-ai-light
* allnet-4duino-iot-wlan-relais
* allterco-shelly1
* allterco-shelly1pm
* allterco-shelly2
...
```

### Building All Firmware
This simple example will build all firmware files from dev branch to /tmp/firmware.

```bash
sudo docker run --rm -it -v /tmp/firmware/:/firmware espurna-build
```

### Building A Single Environment
This example will only build firmware for environment `intermittech-quinled` from local files in `/home/user/espurna`

```
user@host:~/$ sudo docker run --rm -it -v /tmp/firmware/:/firmware espurna-build intermittech-quinled
--------------------------------------------------------------
ESPURNA FIRMWARE BUILDER
Building for version 1.15.0-dev (83a49b19)
--------------------------------------------------------------
Building web interface...
[18:38:22] Using gulpfile /espurna/code/gulpfile.js
[18:38:22] Starting 'default'...
[18:38:22] Starting 'webui'...
...
...
RAM:   [=====     ]  50.4% (used 41284 bytes from 81920 bytes)
Flash: [======    ]  58.1% (used 595392 bytes from 1023984 bytes)
SIZE:    599552

user@host:~/$ tree /tmp/firmware/
/tmp/firmware/
└── espurna-1.15.0-dev
    └── espurna-1.15.0-dev-intermittech-quinled.bin

1 directory, 1 file
```

## Shell for Tinkering
You can also get a shell into the build environment that should let you play around with as much as your intersted in:

```
sudo docker run --rm -it -v /tmp/firmware/:/firmware -v `pwd ../..`:/mnt --entrypoint /bin/bash espurna-build 
root@2ba964297716:/espurna/code# pio lib install
Library Storage: /espurna/code/.pio/libdeps/esp8266-512k-base
Library Manager: ESPAsyncTCP @ 1.2.0+sha.7e9ed22 is already installed
Library Manager: ArduinoJson @ 5.13.4 is already installed
Library Manager: AsyncMqttClient @ 0.8.1+sha.ddbf4d1 is already installed
Library Manager: EEPROM_Rotate @ 0.9.2+sha.00b9d6c is already installed
Library Manager: EspSoftwareSerial @ 3.4.1+sha.5378868 is already installed
Library Manager: ESP Async WebServer @ 1.2.2+sha.b0c61
....
....
root@2ba964297716:/espurna/code#
```
