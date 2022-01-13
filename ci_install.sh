#!/bin/bash

set -x -e -v

npm_install() {
    npm install -g npm@latest
    npm ci
}

pio_install() {
    pip3 install -U platformio
    pio upgrade --dev
    pio platform update -p
}

host_install() {
    sudo apt install cmake
}

cd code

case "$1" in
("host")
    host_install
    ;;
("webui")
    npm_install
    ;;
("build")
    pio_install
    ;;
("release")
    npm_install
    pio_install
    ;;
(*)
    echo -e "\e[1;33mUnknown stage name, exiting!\e[0m"
    exit 1
    ;;
esac
