#!/bin/bash

set -x -e -v

cd code

case "$1" in
("host")
    # runs unit tests, using the host compiler and the esp8266 mock framework
    # - https://github.com/esp8266/Arduino/blob/master/tests/host/Makefile
    # - https://github.com/ThrowTheSwitch/Unity
    pushd test/unit
    cmake -B build
    cmake --build build
    cmake --build build --target test
    popd
    ;;
("webui")
    # checks whether the webui can be built
    ./build.sh -f environments
    git --no-pager diff --stat
    ;;
("build")
    # simply build the given environment
    pio run -e $2
    ;;
("test")
    # run generic build test with the specified environment as base
    scripts/test_build.py -e $2
    ;;
("release")
    # TODO: pending removal in favour of code/scripts/generate_release_sh.py
    ./build.sh -r
    ;;
(*)
    echo -e "\e[1;33mUnknown stage name, exiting!\e[0m"
    exit 1
    ;;
esac
