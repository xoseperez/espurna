#!/bin/bash

# ------------------------------------------------------------------------------
# CONFIGURATION
# ------------------------------------------------------------------------------

ENVIRONMENT="nodemcu-lolin"
ADDR2LINE=$HOME/.platformio/packages/toolchain-xtensa/bin/xtensa-lx106-elf-addr2line
DECODER=utils/EspStackTraceDecoder.jar
DECODER_ORIGIN=https://github.com/littleyoda/EspStackTraceDecoder/releases/download/untagged-83b6db3208da17a0f1fd/EspStackTraceDecoder.jar
FILE="/tmp/.trace"

# ------------------------------------------------------------------------------
# END CONFIGURATION - DO NOT EDIT FURTHER
# ------------------------------------------------------------------------------

# remove default trace file
rm -rf $FILE

function help {
    echo
    echo "Syntax: $0 [-e <environment>] [-f <elf_file>] [-d <dumpfile>]"
    echo
}

# get environment from command line
while [[ $# -gt 1 ]]; do

    key="$1"

    case $key in
        -f)
            ELF="$2"
            shift
        ;;
        -e)
            ENVIRONMENT="$2"
            shift
        ;;
        -d)
            FILE="$2"
            shift
        ;;
    esac

    shift # past argument or value

done

# check environment folder
if [ ! -f $ELF ]; then
    ELF=.pio/build/$ENVIRONMENT/firmware.elf
fi
if [ ! -f $ELF ]; then
    echo "Could not find ELF file for the selected environment: $ELF"
    exit 2
fi

# get decode
if [ ! -f $DECODER ]; then
    folder=$(dirname "$DECODER")
    if [ $folder != "." ]; then
        mkdir -p $folder
    fi
    echo "Downloading decoder..."
    wget -q $DECODER_ORIGIN -O "$DECODER"
fi

# get trace interactively
if [ ! -f $FILE ]; then
    echo "Paste stack trace and end with a blank line:"
    trace=$(sed '/^$/q')
    echo $trace > $FILE
fi

java -jar $DECODER $ADDR2LINE $ELF $FILE
