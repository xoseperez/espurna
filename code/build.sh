#!/bin/bash
set -e

# defaults

script_build_environments=true
script_build_webui=true
destination=../firmware

# ALL env: sections from the .ini, even including those we don't want to build
list_all_envs() {
    grep -E '^\[env:' platformio.ini | sed 's/\[env:\(.*\)\]/\1/g'
}

# -base is only supposed to be used through `extends = ...-base` for another env:
# -ssl / -secure-client hard-code certificates, so not very useful to distribute those
# -ota is a legacy option to conditionally load upload args, no longer needed
list_available_envs() {
    list_all_envs | grep -Ev -- '-ota$|-ssl$|-secure-client.*$|^esp8266-.*base$' | sort
}

print_available() {
    echo "--------------------------------------------------------------"
    echo "Available environments:"
    for environment in $(list_available_envs); do
        echo "* $environment"
    done
}

set_default_environments() {
    environments=$(list_available_envs)
}

build_webui() {
    # Build system uses gulpscript.js to build web interface
    if [ ! -e node_modules/gulp/bin/gulp.js ]; then
        echo "--------------------------------------------------------------"
        echo "Installing dependencies..."
        npm ci
    fi

    # Recreate web interface (espurna/data/index.html.*.gz.h)
    echo "--------------------------------------------------------------"
    echo "Building web interface..."
    node node_modules/gulp/bin/gulp.js || exit
}

build_environments() {
    echo "--------------------------------------------------------------"
    echo "Building environment images..."

    local environments=$@
    if [ $# -eq 0 ]; then
        environments=$(list_available_envs)
    fi

    set -x
    for environment in $environments; do
        env ESPURNA_BUILD_NAME=$environment \
            ESPURNA_BUILD_DESTINATION="$destination" \
            ESPURNA_BUILD_SINGLE_SOURCE="1" \
            pio run --silent --environment $environment -t build-and-copy
    done
    set +x

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
  -l          Print available environments
  -d VALUE    Destination to move the .bin file after building the environment
  -h          Display this message
EOF
}

while getopts "f:ld:h" opt; do
  case $opt in
    f)
        case "$OPTARG" in
            webui) script_build_webui=false ;;
            environments) script_build_environments=false ;;
            *)
                echo Must be either webui or environments
                exit
                ;;
        esac
        ;;
    l)
        print_available
        exit
        ;;
    d)
        destination=$OPTARG
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

if $script_build_webui ; then
    build_webui
fi

if $script_build_environments ; then
    build_environments $@
fi
