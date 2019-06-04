#!/bin/bash
set -e

# Utility
is_git() {
    command -v git >/dev/null 2>&1 || return 1
    command git rev-parse >/dev/null 2>&1 || return 1

    return 0
}

stat_bytes() {
    case "$(uname -s)" in
        Darwin) stat -f %z "$1";;
        *) stat -c %s "$1";;
    esac
}

# Script settings

destination=../firmware
version_file=espurna/config/version.h
version=$(grep -E '^#define APP_VERSION' $version_file | awk '{print $3}' | sed 's/"//g')

if ${TRAVIS:-false}; then
    git_revision=${TRAVIS_COMMIT::7}
    git_tag=${TRAVIS_TAG}
elif is_git; then
    git_revision=$(git rev-parse --short HEAD)
    git_tag=$(git tag --contains HEAD)
else
    git_revision=unknown
    git_tag=
fi

if [[ -n $git_tag ]]; then
    new_version=${version/-*}
    sed -i -e "s@$version@$new_version@" $version_file
    version=$new_version
    trap "git checkout -- $version_file" EXIT
fi

par_build=false
par_thread=${BUILDER_THREAD:-0}
par_total_threads=${BUILDER_TOTAL_THREADS:-4}
if [ ${par_thread} -ne ${par_thread} -o \
    ${par_total_threads} -ne ${par_total_threads} ]; then
    echo "Parallel threads should be a number."
    exit
fi
if [ ${par_thread} -ge ${par_total_threads} ]; then
    echo "Current thread is greater than total threads. Doesn't make sense"
    exit
fi

# Available environments
list_envs() {
    grep env: platformio.ini | sed 's/\[env:\(.*\)\]/\1/g'
}

travis=$(list_envs | grep travis | sort)
available=$(list_envs | grep -Ev -- '-ota$|-ssl$|^travis' | sort)

# Build tools settings
export PLATFORMIO_BUILD_FLAGS="${PLATFORMIO_BUILD_FLAGS} -DAPP_REVISION='\"$git_revision\"'"

# Functions
print_available() {
    echo "--------------------------------------------------------------"
    echo "Available environments:"
    for environment in $available; do
        echo "* $environment"
    done
}

print_environments() {
    echo "--------------------------------------------------------------"
    echo "Current environments:"
    for environment in $environments; do
        echo "* $environment"
    done
}

set_default_environments() {
    # Hook to build in parallel when using travis
    if [[ "${TRAVIS_BUILD_STAGE_NAME}" = "Release" ]] && ${par_build}; then
        environments=$(echo ${available} | \
            awk -v par_thread=${par_thread} -v par_total_threads=${par_total_threads} \
            '{ for (i = 1; i <= NF; i++) if (++j % par_total_threads == par_thread ) print $i; }')
        return
    fi

    # Only build travisN
    if [[ "${TRAVIS_BUILD_STAGE_NAME}" = "Test" ]]; then
        environments=$travis
        return
    fi

    # Fallback to all available environments
    environments=$available
}

build_webui() {
    # Build system uses gulpscript.js to build web interface
    if [ ! -e node_modules/gulp/bin/gulp.js ]; then
        echo "--------------------------------------------------------------"
        echo "Installing dependencies..."
        npm install --only=dev
    fi

    # Recreate web interface (espurna/data/index.html.*.gz.h)
    echo "--------------------------------------------------------------"
    echo "Building web interface..."
    node node_modules/gulp/bin/gulp.js || exit

    # TODO: do something if webui files are different
    # for now, just print in travis log
    if ${TRAVIS:-false}; then
        git --no-pager diff --stat
    fi
}

build_environments() {
    echo "--------------------------------------------------------------"
    echo "Building firmware images..."
    mkdir -p $destination/espurna-$version

    for environment in $environments; do
        echo -n "* espurna-$version-$environment.bin --- "
        platformio run --silent --environment $environment || exit 1
        stat_bytes .pioenvs/$environment/firmware.bin
        [[ "${TRAVIS_BUILD_STAGE_NAME}" = "Test" ]] || \
            mv .pioenvs/$environment/firmware.bin $destination/espurna-$version/espurna-$version-$environment.bin
    done
    echo "--------------------------------------------------------------"
}

# Parameters
while getopts "lpd:" opt; do
  case $opt in
    l)
        print_available
        exit
        ;;
    p)
        par_build=true
        ;;
    d)
        destination=$OPTARG
        ;;
    esac
done

shift $((OPTIND-1))

# Welcome
echo "--------------------------------------------------------------"
echo "ESPURNA FIRMWARE BUILDER"
echo "Building for version ${version}" ${git_revision:+($git_revision)}

# Environments to build
environments=$@

if [ $# -eq 0 ]; then
    set_default_environments
fi

if ${CI:-false}; then
    print_environments
fi

build_webui
build_environments
