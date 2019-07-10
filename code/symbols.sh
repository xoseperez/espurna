#!/bin/bash

# ------------------------------------------------------------------------------
# CONFIGURATION
# ------------------------------------------------------------------------------

ENVIRONMENT="wemos-d1mini-relayshield"
READELF="xtensa-lx106-elf-readelf"
NUMBER=20

# ------------------------------------------------------------------------------
# END CONFIGURATION - DO NOT EDIT FURTHER
# ------------------------------------------------------------------------------

# remove default trace file
rm -rf $FILE

function help {
    echo
    echo "Syntax: $0 [-e <environment>] [-n <number>]"
    echo
}

# get environment from command line
while [[ $# -gt 1 ]]; do

    key="$1"

    case $key in
        -e)
            ENVIRONMENT="$2"
            shift
        ;;
        -n)
            NUMBER="$2"
            shift
        ;;
    esac

    shift # past argument or value

done

# check environment folder
if [ $ENVIRONMENT == "" ]; then
    echo "No environment defined"
    help
    exit 1
fi
ELF=.pio/build/$ENVIRONMENT/firmware.elf
if [ ! -f $ELF ]; then
    echo "Could not find ELF file for the selected environment: $ELF"
    exit 2
fi

$READELF -s $ELF | head -3 | tail -1
$READELF -s $ELF | sort -r -k3 -n  | head -$NUMBER
