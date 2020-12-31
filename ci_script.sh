#!/bin/bash

set -x -e -v

cd code

case "$1" in
("host")
    cd test/ && pio test
    ;;
("webui")
    ./build.sh -f environments
    ;;
("build")
    # shellcheck disable=SC2086
    scripts/test_build.py -e "$TEST_ENV" $TEST_EXTRA_ARGS
    ;;
("release")
    ./build.sh -r
    ;;
(*)
    echo -e "\e[1;33mUnknown stage name, exiting!\e[0m"
    exit 1
    ;;
esac
