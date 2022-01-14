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
    # TODO: both can only parse one file at a time
    npm exec --no -- eslint html/custom.js
    npm exec --no -- html-validate html/index.html
    # checks whether the webui can be built
    ./build.sh -f environments
    # TODO: gzip inserts an OS-dependant byte in the header, ref.
    # - https://datatracker.ietf.org/doc/html/rfc1952
    # - https://github.com/nodejs/node/blob/e46c680bf2b211bbd52cf959ca17ee98c7f657f5/deps/zlib/deflate.c#L901
    # - windowBits description in the https://zlib.net/manual.html#Advanced
    git --no-pager diff --stat
    ;;
("build")
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
