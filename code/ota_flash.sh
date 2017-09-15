#!/bin/bash

ip=
board=
auth=
flags=

export boards=()
ips=""

exists() {
    command -v "$1" >/dev/null 2>&1
}

echo_pad() {
    string=$1
    pad=$2
    printf '%s' "$string"
    printf '%*s' $(( $pad - ${#string} ))
}

useAvahi() {

    echo_pad "#" 4
    echo_pad "HOSTNAME" 20
    echo_pad "IP" 20
    echo_pad "DEVICE" 30
    echo_pad "VERSION" 10
    echo

    printf -v line '%*s\n' 84
    echo ${line// /-}

    counter=0

    ip_file="/tmp/espurna.flash.ips"
    board_file="/tmp/espurna.flash.boards"
    count_file="/tmp/espurna.flash.count"
    echo -n "" > $ip_file
    echo -n "" > $board_file
    echo -n "$counter" > $count_file

    avahi-browse -t -r -p  "_arduino._tcp" 2>/dev/null | grep ^= | sort -t ';' -k 3 | while read line; do

        (( counter++ ))
        echo "$counter" > $count_file

        hostname=`echo $line | cut -d ';' -f4`
        ip=`echo $line | cut -d ';' -f8`
        txt=`echo $line | cut -d ';' -f10`
        board=`echo $txt | sed -n "s/.*espurna_board=\([^\"]*\).*/\1/p"`
        version=`echo $txt | sed -n "s/.*espurna_version=\([^\"]*\).*/\1/p"`

        echo -n "$ip;" >> $ip_file
        echo -n "$board;" >> $board_file

        echo_pad "$counter" 4
        echo_pad "$hostname" 20
        echo_pad "$ip" 20
        echo_pad "$board" 30
        echo_pad "$version" 10
        echo


    done

    echo
    read -p "Choose the board you want to flash (empty if none of these): " num

    # None of these
    if [ "$num" == "" ]; then
        return
    fi

    # Check boundaries
    counter=`cat $count_file`
    if [ $num -lt 1 ] || [ $num -gt $counter ]; then
        echo "Board number must be between 1 and $counter"
        exit 1
    fi

    # Fill the fields
    ip=`cat $ip_file | cut -d ';' -f$num`
    board=`cat $board_file | cut -d ';' -f$num`

}

getBoard() {

    boards=(`cat espurna/config/hardware.h | grep "defined" | sed "s/.*(\(.*\)).*/\1/" | sort`)

    echo_pad "#" 4
    echo_pad "DEVICE" 30
    echo

    printf -v line '%*s\n' 34
    echo ${line// /-}

    counter=0
    for board in "${boards[@]}"; do
        (( counter++ ))
        echo_pad "$counter" 4
        echo_pad "$board" 30
        echo
    done

    echo
    read -p "Choose the board you want to flash (empty if none of these): " num

    # None of these
    if [ "$num" == "" ]; then
        return
    fi

    # Check boundaries
    counter=${#boards[*]}
    if [ $num -lt 1 ] || [ $num -gt $counter ]; then
        echo "Board code must be between 1 and $counter"
        exit 1
    fi

    # Fill the fields
    (( num -- ))
    board=${boards[$num]}

}

# ------------------------------------------------------------------------------

# Welcome
echo
echo "--------------------------------------------------------------"
echo "ESPURNA FIRMWARE OTA FLASHER"

# Get current version
version=`cat espurna/config/version.h | grep APP_VERSION | awk '{print $3}' | sed 's/"//g'`
echo "Building for version $version"

echo "--------------------------------------------------------------"
echo

if exists avahi-browse; then
    useAvahi
fi

if [ "$board" == "" ]; then
    getBoard
fi

if [ "$board" == "" ]; then
    read -p "Board type of the device to flash: " -e -i "NODEMCU_LOLIN" board
fi

if [ "$board" == "" ]; then
    echo "You must define the board type"
    exit 2
fi

if [ "$ip" == "" ]; then
    read -p "IP of the device to flash: " -e -i 192.168.4.1 ip
fi

if [ "$ip" == "" ]; then
    echo "You must define the IP of the device"
    exit 2
fi

if [ "$auth" == "" ]; then
    read -p "Authorization key of the device to flash: " auth
fi

if [ "$flags" == "" ]; then
    read -p "Extra flags for the build: " -e -i "-DTELNET_ONLY_AP=0" flags
fi

read -p "Environment to build: " -e -i "esp8266-1m-ota" env

echo
echo "ESPURNA_IP    = $ip"
echo "ESPURNA_BOARD = $board"
echo "ESPURNA_AUTH  = $auth"
echo "ESPURNA_FLAGS = $flags"
echo "ESPURNA_ENV   = $env"

echo
echo -n "Are these values corrent [y/N]: "
read response

if [ "$response" != "y" ]; then
    exit
fi

export ESPURNA_IP=$ip
export ESPURNA_BOARD=$board
export ESPURNA_AUTH=$auth
export ESPURNA_FLAGS=$flags

pio run -e $env -t upload
