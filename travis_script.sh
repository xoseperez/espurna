#!/bin/bash

set -x -e -v

cd code

if [ "${TRAVIS_BUILD_STAGE_NAME}" = "Test Host" ]; then
    cd test/ && pio test
elif [ "${TRAVIS_BUILD_STAGE_NAME}" = "Test WebUI" ]; then
    ./build.sh -f environments
elif [ "${TRAVIS_BUILD_STAGE_NAME}" = "Test PlatformIO Build" ]; then
    # shellcheck disable=SC2086
    scripts/test_build.py -e "$TEST_ENV" $TEST_EXTRA_ARGS
elif [ "${TRAVIS_BUILD_STAGE_NAME}" = "Release" ]; then
    ./build.sh -r
else
    echo -e "\e[1;33mUnknown stage name, exiting!\e[0m"
    exit 1
fi
