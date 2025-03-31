#!/bin/bash

set -x -e -v

zlib_test() {
    # via https://launchpad.net/~arter97/+archive/ubuntu/zlib-ng
    cat <<EOF | sudo tee /etc/apt/preferences.d/zlib-ng
Package: *
Pin: release o=LP-PPA-arter97-zlib-ng
Pin-Priority: 1000
EOF
    cat <<EOF | sudo tee /etc/apt/apt.conf.d/51unattended-upgrades-zlibng
Unattended-Upgrade::Origins-Pattern:: "o=LP-PPA-arter97-zlib-ng";
EOF

    sudo add-apt-repository ppa:arter97/zlib-ng
    sudo apt update

    sudo apt install --allow-downgrades zlib1g
}

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
    zlib_test
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
