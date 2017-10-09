#!/bin/bash

# Welcome
echo "--------------------------------------------------------------"
echo "ESPURNA FIRMWARE BUILDER"

# Available environments
available=`cat platformio.ini | grep env: | grep -v ota  | sed 's/\[env://' | sed 's/\]/ /' | sort`
environments=$@
if [ "$environments" == "list" ]; then
    echo "--------------------------------------------------------------"
    echo "Available environments:"
    for environment in $available; do
        echo "* $environment"
    done
    exit
fi

# Environments to build
if [ $# -eq 0 ]; then
    environments=$available
fi

# Get current version
version=`cat espurna/config/version.h | grep APP_VERSION | awk '{print $3}' | sed 's/"//g'`
echo "Building for version $version"

# Create output folder
mkdir -p firmware

if [ ! -e node_modules/gulp/bin/gulp.js ]; then
    echo "--------------------------------------------------------------"
    echo "Installing dependencies..."
    npm install --only=dev
fi

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
