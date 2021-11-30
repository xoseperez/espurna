# coding=utf-8
# pylint: dummy-variables-rgx='(_+[a-zA-Z0-9]*?$)|dummy|env'
#
# Original extra_scripts.py
# Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
#
# ldscripts, lwip patching, updated postmortem flags and git support
# Copyright (C) 2019-2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

# Run this script every time building an env BEFORE platform-specific code is loaded

import os
import sys

from SCons.Script import Import, ARGUMENTS

from espurna_utils import check_env
from espurna_utils.build import app_add_builder_single_source, app_add_target_build_re2c

Import("env")
env = globals()["env"]


CI = check_env("CI", "false")
PIO_PLATFORM = env.PioPlatform()
CONFIG = env.GetProjectConfig()
VERBOSE = "1" == ARGUMENTS.get("PIOVERBOSE", "0")


class ExtraScriptError(Exception):
    pass


def log(message, verbose=False, file=sys.stderr):
    if verbose or VERBOSE:
        print(message, file=file)


# Most portable way, without depending on platformio internals
def subprocess_libdeps(lib_deps, storage, verbose=False):
    import subprocess

    args = [env.subst("$PYTHONEXE"), "-mplatformio", "lib", "-d", storage, "install"]
    if not verbose:
        args.append("-s")

    args.extend(lib_deps)
    subprocess.check_call(args)


def get_shared_libdeps_dir(section, name):
    if not CONFIG.has_option(section, name):
        raise ExtraScriptError("{}.{} is required to be set".format(section, name))

    opt = CONFIG.get(section, name)

    if opt not in env.GetProjectOption("lib_extra_dirs"):
        raise ExtraScriptError(
            "lib_extra_dirs must contain {}.{}".format(section, name)
        )

    return os.path.join(env["PROJECT_DIR"], opt)


def ensure_platform_updated():
    try:
        if PIO_PLATFORM.are_outdated_packages():
            log("updating platform packages")
            PIO_PLATFORM.update_packages()
    except Exception:
        log("Warning: no connection, cannot check for outdated packages", verbose=True)


# handle build flags through os environment.
# using env instead of ini to avoid platformio ini changing hash on every change
env.Append(
    ESPURNA_BOARD=os.environ.get("ESPURNA_BOARD", ""),
    ESPURNA_AUTH=os.environ.get("ESPURNA_AUTH", ""),
    ESPURNA_FLAGS=os.environ.get("ESPURNA_FLAGS", ""),
)

ESPURNA_OTA_PORT = os.environ.get("ESPURNA_IP")
if ESPURNA_OTA_PORT:
    env.Replace(UPLOAD_PROTOCOL="espota")
    env.Replace(UPLOAD_PORT=ESPURNA_OTA_PORT)
    env.Replace(UPLOAD_FLAGS="--auth=$ESPURNA_AUTH")
else:
    env.Replace(UPLOAD_PROTOCOL="esptool")

# handle `-t build-and-copy` parameters
env.Append(
    # what is the name suffix of the .bin
    ESPURNA_BUILD_NAME=os.environ.get("ESPURNA_BUILD_NAME", ""),
    # where to copy the resulting .bin
    ESPURNA_BUILD_DESTINATION=os.environ.get("ESPURNA_BUILD_DESTINATION", ""),
    # set the full string for the build, no need to change individual parts
    ESPURNA_BUILD_FULL_VERSION=os.environ.get("ESPURNA_BUILD_FULL_VERSION", ""),
    # or, replace parts of the version string that would've been auto-detected
    ESPURNA_BUILD_VERSION=os.environ.get("ESPURNA_BUILD_VERSION", ""),
    ESPURNA_BUILD_REVISION=os.environ.get("ESPURNA_BUILD_REVISION", ""),
    ESPURNA_BUILD_VERSION_SUFFIX=os.environ.get("ESPURNA_BUILD_VERSION_SUFFIX", ""),
)

# updates arduino core git to the latest master commit
if CI:
    package_overrides = env.GetProjectOption("platform_packages")
    for package in package_overrides:
        if "https://github.com/esp8266/Arduino.git" in package:
            ensure_platform_updated()
            break

# to speed-up build process, install libraries in a way they are shared between our envs
if check_env("ESPURNA_PIO_SHARED_LIBRARIES", "0"):
    storage = get_shared_libdeps_dir("common", "shared_libdeps_dir")
    subprocess_libdeps(env.GetProjectOption("lib_deps"), storage, verbose=VERBOSE)


# tweak build system to ignore espurna.ino, but include user code
# ref: platformio-core/platformio/tools/piomisc.py::ConvertInoToCpp()
def ConvertInoToCpp(env):
    pass


ino = env.Glob("$PROJECT_DIR/espurna/*.ino") + env.Glob("$PROJECT_DIR/espurna/*.pde")
if len(ino) == 1 and ino[0].name == "espurna.ino":
    env.AddMethod(ConvertInoToCpp)

# merge every .cpp into a single file and **only** build that single file
if check_env("ESPURNA_BUILD_SINGLE_SOURCE", "0"):
    app_add_builder_single_source(env)

# handle explicit targets that are used to build .re files, and before falling into the next sconsfile
app_add_target_build_re2c(env)
