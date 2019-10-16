#!/bin/bash

set -e -v

cd code

if [ ${TRAVIS_BUILD_STAGE_NAME} = "Test" ]; then
    scripts/test_build.py -e $BUILDER_ENV $BUILDER_EXTRA
else
    ./build.sh -p
fi
