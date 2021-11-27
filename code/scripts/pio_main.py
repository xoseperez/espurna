# coding=utf-8
# pylint: dummy-variables-rgx='(_+[a-zA-Z0-9]*?$)|dummy|env|projenv'
#
# Original extra_scripts.py
# Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
#
# ldscripts, lwip patching, updated postmortem flags and git support
# Copyright (C) 2019-2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

# Run this script every time building an env AFTER platform-specific code is loaded

import os
from espurna_utils import (
    check_printsize,
    remove_float_support,
    ldscripts_inject_libpath,
    app_inject_version,
    dummy_ets_printf,
    app_inject_flags,
    app_add_target_build_and_copy,
)

from SCons.Script import Import

Import("env", "projenv")
env = globals()["env"]
projenv = globals()["projenv"]

CI = "true" == os.environ.get("CI")

# See what happens in-between linking .cpp.o + .a into the resulting .elf
env.ProcessFlags('-Wl,-Map -Wl,\\"${BUILD_DIR}/${PROGNAME}.map\\"')

# Always show warnings for project code
projenv.ProcessUnFlags("-w")

# XXX: note that this will also break %d format with floats and print raw memory contents as int
# Cores after 2.3.0 can disable %f in the printf / scanf to reduce .bin size
remove_float_support(env)
ldscripts_inject_libpath(env)

# two-step update hint when using 1MB boards
if not CI:
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_printsize)

# disable postmortem printing to the uart. another one is in eboot, but this is what causes the most harm
if "DISABLE_POSTMORTEM_STACKDUMP" in env["CPPFLAGS"]:
    env.AddPostAction(
        "$BUILD_DIR/FrameworkArduino/core_esp8266_postmortem.cpp.o", dummy_ets_printf
    )

# override static version flag from the espurna/config/version.h
# either completely, or change the version / revision / suffix part separately
app_inject_version(projenv)

# handle ESPURNA_BOARD and ESPURNA_FLAGS here, since projenv is not available in pre-scripts
# TODO: prefer PLATFORMIO_SRC_BUILD_FLAGS instead? both of these only allow -D...
app_inject_flags(projenv)

# handle when CI does a tagged build or user explicitly asked to store the firmware.bin
app_add_target_build_and_copy(projenv)
