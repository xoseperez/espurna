#!/bin/bash

set -e -v

cd code

if [ ${TRAVIS_BUILD_STAGE_NAME} = "Test" ]; then
    ./test_build.sh $BUILDER_ENV $BUILDER_EXTRA
else
    ./build.sh -p
fi
