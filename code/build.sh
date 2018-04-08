#!/bin/bash

# Welcome
echo "--------------------------------------------------------------"
echo "ESPURNA FIRMWARE BUILDER"

# Available environments
travis=$(grep env: platformio.ini | grep travis | sed 's/\[env://' | sed 's/\]/ /' | sort)
available=$(grep env: platformio.ini | grep -v ota  | grep -v ssl  | grep -v travis | sed 's/\[env://' | sed 's/\]/ /' | sort)

# Parameters
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

    # Hook to build travis test envs
    if [[ "${TRAVIS_BRANCH}" != "" ]]; then
        re='^[0-9]+\.[0-9]+\.[0-9]+$'
        if ! [[ ${TRAVIS_BRANCH} =~ $re ]]; then
            environments=$travis
        fi
    fi

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

echo "--------------------------------------------------------------"
echo "Get revision..."
revision=$(git rev-parse HEAD)
revision=${revision:0:7}
cp espurna/config/version.h espurna/config/version.h.original
sed -i -e "s/APP_REVISION            \".*\"/APP_REVISION            \"$revision\"/g" espurna/config/version.h

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
    platformio run --silent --environment $environment || break
    mv .pioenvs/$environment/firmware.bin ../firmware/espurna-$version/espurna-$version-$environment.bin
done
echo "--------------------------------------------------------------"

mv espurna/config/version.h.original espurna/config/version.h
