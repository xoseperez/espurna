#!/bin/bash

set -x -e -v

cd code

case "$1" in
("host")
    # runs PIO unit tests, using the host compiler
    # (see https://github.com/ThrowTheSwitch/Unity)
    pushd test
    pio test
    popd
    ;;
("webui")
    # TODO: both can only parse one file at a time
    npx eslint html/custom.js
    npx html-validate html/index.html
    # checks whether the webui can be built
    ./build.sh -f environments
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
