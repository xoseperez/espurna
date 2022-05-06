# coding=utf-8
# pylint: dummy-variables-rgx='(_+[a-zA-Z0-9]*?$)|dummy|env'
#
# Original extra_scripts.py
# Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
#
# ldscripts, lwip patching, updated postmortem flags and git support
# Copyright (C) 2019-2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

# Run this script every time building an env BEFORE platform-specific code is loaded

import logging
import os
import shutil
import sys

from SCons.Script import Delete, Move, Import, ARGUMENTS

from espurna_utils import check_env
from espurna_utils.build import app_add_builder_single_source, app_add_target_build_re2c

from platformio.package.manager.library import LibraryPackageManager
from platformio.package.meta import PackageSpec


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


def get_shared_libdeps(config, section="common", name="shared_lib_deps"):
    raw = config.getraw(section, name)
    return config.parse_multi_values(raw)


def get_shared_libdir(config, section="common", name="shared_lib_dir"):
    return config.getraw(section, name)


def migrate_libraries(storage):
    target = env.Dir(f"$PROJECT_DIR/{storage}")
    if target.exists():
        return

    old_lib_deps = env.Dir("$PROJECT_LIBDEPS_DIR/$PIOENV")
    if not old_lib_deps.exists() or old_lib_deps.islink():
        return

    env.Execute(env.VerboseAction(Move(target, old_lib_deps), "Migrating $TARGET"))


def install_libraries(specs, storage, verbose=False):
    lm = LibraryPackageManager(storage)
    lm.set_log_level(logging.DEBUG if verbose else logging.INFO)

    for spec in specs:
        pkg = lm.get_package(spec)
        if not pkg:
            lm.install(spec, skip_dependencies=True)


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
# b/c lib_extra_dirs = ... was deprecated and global libs are not an option, just re-use the local custom lib storage
# (...while it still works :/...)
SHARED_LIBDIR = get_shared_libdir(CONFIG)
migrate_libraries(SHARED_LIBDIR)
install_libraries(get_shared_libdeps(CONFIG), SHARED_LIBDIR, verbose=VERBOSE)


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
