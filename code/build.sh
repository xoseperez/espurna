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
script_build_environments=true
script_build_webui=true

release_mode=false

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

# Available environments
list_envs() {
    grep -E '^\[env:' platformio.ini | sed 's/\[env:\(.*\)\]/\1/g'
}

available=$(list_envs | grep -Ev -- '-ota$|-ssl$|-secure-client.*$|^esp8266-.*base$' | sort)

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

build_release() {
    echo "--------------------------------------------------------------"
    echo "Building release images..."
    python scripts/generate_release_sh.py \
        --ignore secure-client \
        --version $version \
        --destination $destination/espurna-$version > release.sh
    bash release.sh
    echo "--------------------------------------------------------------"
}

build_environments() {
    echo "--------------------------------------------------------------"
    echo "Building firmware images..."
    mkdir -p $destination/espurna-$version

    for environment in $environments; do
        echo "* espurna-$version-$environment.bin"
        platformio run --silent --environment $environment || exit 1
        echo -n "SIZE:    "
        stat_bytes .pio/build/$environment/firmware.bin
        [[ "${TRAVIS_BUILD_STAGE_NAME}" = "Test" ]] || \
            mv .pio/build/$environment/firmware.bin $destination/espurna-$version/espurna-$version-$environment.bin
    done
    echo "--------------------------------------------------------------"
}

# Parameters
print_getopts_help() {
    cat <<EOF
Usage: $(basename "$0") [OPTION] <ENVIRONMENT>...

  Where ENVIRONMENT is environment name(s) from platformio.ini

Options:

  -f VALUE    Filter build stage by name to skip it
              Supported VALUEs are "environments" and "webui"
              Can be specified multiple times. 
  -r          Release mode
              Generate build list through an external script.
  -l          Print available environments
  -d VALUE    Destination to move .bin files after building environments
  -h          Display this message
EOF
}

while getopts "f:lrpd:h" opt; do
  case $opt in
    f)
        case "$OPTARG" in
            webui) script_build_webui=false ;;
            environments) script_build_environments=false ;;
        esac
        ;;
    l)
        print_available
        exit
        ;;
    d)
        destination=$OPTARG
        ;;
    r)
        release_mode=true
        ;;
    h)
        print_getopts_help
        exit
    esac
done

shift $((OPTIND-1))

# Welcome
echo "--------------------------------------------------------------"
echo "ESPURNA FIRMWARE BUILDER"
echo "Building for version ${version}" ${git_revision:+($git_revision)}

# Environments to build
environments=$@

if $script_build_webui ; then
    build_webui
fi

if $script_build_environments ; then
    if [ $# -eq 0 ]; then
        set_default_environments
    fi

    if $release_mode ; then
        build_release
    else
        build_environments
    fi
fi

