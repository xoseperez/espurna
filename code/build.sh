#!/bin/bash

# Welcome
echo "--------------------------------------------------------------"
echo "ESPURNA FIRMWARE BUILDER"

# Available environments
travis=$(grep env: platformio.ini | grep travis | sed 's/\[env://' | sed 's/\]/ /' | sort)
available=$(grep env: platformio.ini | grep -v ota  | grep -v ssl  | grep -v travis | sed 's/\[env://' | sed 's/\]/ /' | sort)

# Parameters
while getopts "lp" opt; do
  case $opt in
    l)
        echo "--------------------------------------------------------------"
        echo "Available environments:"
        for environment in $available; do
            echo "* $environment"
        done
        exit
        ;;
    p)
        par_build=1
        par_thread=${BUILDER_THREAD:-0}
        par_total_threads=${BUILDER_TOTAL_THREADS:-4}
        if [ ${par_thread} -ne ${par_thread} -o \
            ${par_total_threads} -ne ${par_total_threads} ]; then
                echo "Parallel threads should be a number."
                exit
            fi
            if [ ${par_thread} -ge ${par_total_threads} ]; then
                echo "Current thread is greater than total threads. Doesn't make sense"
                exit
            fi
            ;;
    esac
done

shift $((OPTIND-1))

environments=$@

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
if [ ${par_build} ]; then
    to_build=$(echo ${environments} | awk -v par_thread=${par_thread} -v par_total_threads=${par_total_threads} '{ for (i = 1; i <= NF; i++) if (++j % par_total_threads == par_thread ) print $i; }')
else
    to_build=${environments}
fi

for environment in $to_build; do
    echo -n "* espurna-$version-$environment.bin --- "
    platformio run --silent --environment $environment || exit 1
    stat -c %s .pioenvs/$environment/firmware.bin
    mv .pioenvs/$environment/firmware.bin ../firmware/espurna-$version/espurna-$version-$environment.bin
done
echo "--------------------------------------------------------------"

mv espurna/config/version.h.original espurna/config/version.h
