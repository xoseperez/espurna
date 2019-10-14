#!/bin/bash

set -eu -o pipefail

CUSTOM_HEADER="espurna/config/custom.h"
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

trap 'rm -f ${CUSTOM_HEADER}' EXIT

for cfg in ${DEFAULT_CONFIGURATIONS[@]} ${EXTRA_CONFIGURATIONS} ; do
    echo travis_fold:start:build_${cfg}
    echo "- building ${cfg}"

    printf "#define MANUFACTURER \"TEST_BUILD\"\n" | tee ${CUSTOM_HEADER}
    printf "#define DEVICE \"${cfg^^}\"\n" | tee --append ${CUSTOM_HEADER}
    cat test/build/${cfg}.h | tee --append ${CUSTOM_HEADER}

    export PLATFORMIO_SRC_BUILD_FLAGS="-DUSE_CUSTOM_H"
    export PLATFORMIO_BUILD_CACHE_DIR="test/pio_cache"
    
    time pio run -s -e $TARGET_ENVIRONMENT
    echo travis_fold:end:build_${cfg}
done
