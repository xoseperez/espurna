#!/bin/bash

set -x -e -v

cd code

if [ "${TRAVIS_BUILD_STAGE_NAME}" = "Test webui" ]; then
    ./build.sh -f environments
elif [ "${TRAVIS_BUILD_STAGE_NAME}" = "Test platformio build" ]; then
    # shellcheck disable=SC2086
    scripts/test_build.py -e "$TEST_ENV" $TEST_EXTRA_ARGS
elif [ "${TRAVIS_BUILD_STAGE_NAME}" = "Release" ]; then
    ./build.sh -p
else
    echo -e "\e[1;33mUnknown stage name, exiting!\e[0m"
    exit 1
fi
