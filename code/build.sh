#!/bin/bash

# Environments to build
environments=$@
if [ $# -eq 0 ]; then
    environments=`cat platformio.ini | grep env: | grep -v ota  | sed 's/\[env://' | sed 's/\]/ /'`
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
