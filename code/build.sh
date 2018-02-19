#!/bin/bash

# Welcome
echo "--------------------------------------------------------------"
echo "ESPURNA FIRMWARE BUILDER"

# Available environments
available=$(grep env: platformio.ini | grep -v ota  | grep -v ssl  | sed 's/\[env://' | sed 's/\]/ /' | sort)
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
version=$(grep APP_VERSION espurna/config/version.h | awk '{print $3}' | sed 's/"//g')
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

# Build all the required firmware images
echo "--------------------------------------------------------------"
echo "Building firmware images..."
mkdir -p ../firmware/espurna-$version
for environment in $environments; do
    echo "* espurna-$version-$environment.bin"
    platformio run --silent --environment $environment || exit
    mv .pioenvs/$environment/firmware.bin ../firmware/espurna-$version/espurna-$version-$environment.bin
done
echo "--------------------------------------------------------------"
