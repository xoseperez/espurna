#!/bin/bash

set -x -e -v

cd code

case "$1" in
("host")
    cd test/ && pio test
    ;;
("webui")
    ./build.sh -f environments
    git --no-pager diff --stat
    ;;
("build")
    scripts/test_build.py -e $2
    ;;
("release")
    ./build.sh -r
    ;;
(*)
    echo -e "\e[1;33mUnknown stage name, exiting!\e[0m"
    exit 1
    ;;
esac
