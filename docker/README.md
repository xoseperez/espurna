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
```bash
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

```bash
udo docker run --rm -it -v /tmp/firmware/:/firmware espurna-build intermittech-quinled
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
Cloning into '/root/.platformio/.cache/tmp/pkg-installing-rwukc302'...
Cloning into '/root/.platformio/.cache/tmp/pkg-installing-20osjkr2'...
RAM:   [=====     ]  50.4% (used 41284 bytes from 81920 bytes)
Flash: [======    ]  58.1% (used 595392 bytes from 1023984 bytes)
SIZE:    599552
```

