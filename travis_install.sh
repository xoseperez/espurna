#!/bin/bash

set -x -e -v

npm_install() {
    npm install -g npm@latest
    npm ci
}

pio_install() {
    pip install -U platformio
    pio upgrade --dev
    pio platform update -p
}

cd code

if [ "${TRAVIS_BUILD_STAGE_NAME}" = "Test Host" ]; then
    pio_install
elif [ "${TRAVIS_BUILD_STAGE_NAME}" = "Test WebUI" ]; then
    npm_install
elif [ "${TRAVIS_BUILD_STAGE_NAME}" = "Test PlatformIO Build" ]; then
    pio_install
elif [ "${TRAVIS_BUILD_STAGE_NAME}" = "Release" ]; then
    npm_install
    pio_install
else
    echo -e "\e[1;33mUnknown stage name, exiting!\e[0m"
    exit 1
fi
