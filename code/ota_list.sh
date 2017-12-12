#!/bin/bash

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
    echo_pad "IP" 25
    echo_pad "APP" 15
    echo_pad "VERSION" 15
    echo_pad "DEVICE" 30
    echo

    printf -v line '%*s\n' 104
    echo ${line// /-}

    counter=0

    avahi-browse -t -r -p  "_arduino._tcp" 2>/dev/null | grep ^= | sort -t ';' -k 3 | while read line; do

        (( counter++ ))

        hostname=`echo $line | cut -d ';' -f4`
        ip=`echo $line | cut -d ';' -f8`
        txt=`echo $line | cut -d ';' -f10`
        app_name=`echo $txt | sed -n "s/.*app_name=\([^\"]*\).*/\1/p"`
        app_version=`echo $txt | sed -n "s/.*app_version=\([^\"]*\).*/\1/p"`
        board=`echo $txt | sed -n "s/.*target_board=\([^\"]*\).*/\1/p"`

        echo_pad "$counter" 4
        echo_pad "$hostname" 20
        echo_pad "http://$ip" 25
        echo_pad "$app_name" 15
        echo_pad "$app_version" 15
        echo_pad "$board" 30
        echo

    done

    echo

}

# ------------------------------------------------------------------------------

# Welcome
echo
echo "--------------------------------------------------------------"
echo "OTA-UPDATABLE DEVICES"
echo "--------------------------------------------------------------"
echo

if exists avahi-browse; then
    useAvahi
else
    echo "Avahi not installed"
    exit 1
fi
