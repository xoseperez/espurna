#!/bin/bash

set -eu -o pipefail

TARGET_ENVIRONMENT=${1:?"pio env name"}
shift 1

EXTRA_CONFIGURATIONS=$@
DEFAULT_CONFIGURATIONS=(
    basic
    sensor
    emon
    light_my92xx
    light_dimmer
    nondefault
)

trap 'rm -f espurna/config/custom.h' EXIT

for cfg in ${DEFAULT_CONFIGURATIONS[@]} ${EXTRA_CONFIGURATIONS} ; do
    echo travis_fold:start:build_${cfg}
    echo "- building ${cfg}"

    printf "#define MANUFACTURER \"TEST_BUILD\"\n" | tee espurna/config/custom.h
    printf "#define DEVICE \"${cfg^^}\"\n" | tee --append espurna/config/custom.h
    cat test/build/${cfg}.h | tee --append espurna/config/custom.h

    export PLATFORMIO_SRC_BUILD_FLAGS="-DUSE_CUSTOM_H"
    export PLATFORMIO_BUILD_CACHE_DIR="test/pio_cache"
    
    time pio run -s -e $TARGET_ENVIRONMENT
    echo travis_fold:end:build_${cfg}
done
