#!/bin/bash

environments=$@

# Environments to build
ALL_ENVIRONMENTS="
    tinkerman-espurna-h
    itead-sonoff-basic itead-sonoff-rf itead-sonoff-basic-dht22 itead-sonoff-basic-ds18b20
    itead-sonoff-pow itead-sonoff-dual itead-sonoff-4ch itead-sonoff-4ch-pro
    itead-sonoff-touch itead-sonoff-b1 itead-sonoff-led itead-sonoff-rfbridge
    itead-sonoff-t1-1ch itead-sonoff-t1-2ch itead-sonoff-t1-3ch
    itead-slampher itead-s20 itead-1ch-inching itead-motor itead-bnsz01
    electrodragon-wifi-iot
    workchoice-ecoplug
    jangoe-wifi-relay
    openenergymonitor-mqtt-relay
    jorgegarcia-wifi-relays
    aithinker-ai-light
    magichome-led-controller
    huacanxing-h801
    wion-50055
"
if [ $# -eq 0 ]; then
    environments=$ALL_ENVIRONMENTS
fi

# Get current version
version=`cat espurna/config/version.h | grep APP_VERSION | awk '{print $3}' | sed 's/"//g'`
echo "--------------------------------------------------------------"
echo "ESPURNA FIRMWARE BUILDER"
echo "Building for version $version"

# Create output folder
mkdir -p firmware

# Recreate web interface
echo "--------------------------------------------------------------"
echo "Building web interface..."
node node_modules/gulp/bin/gulp.js || exit

# Build all the required firmwares
echo "--------------------------------------------------------------"
echo "Building firmware images..."
for environment in $environments; do
    echo "* espurna-$version-$environment.bin"
    platformio run -s -e $environment || exit
    mv .pioenvs/$environment/firmware.bin firmware/espurna-$version-$environment.bin
done
echo "--------------------------------------------------------------"
