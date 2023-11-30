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
import itertools

from espurna_utils import (
    app_add_target_build_and_copy,
    app_inject_flags,
    app_inject_version,
    app_add_gzip_file,
    app_patch_cachedir,
    check_binsize,
    check_env,
    disable_postmortem_output,
    ldscripts_inject_libpath,
    remove_float_support,
)

from SCons.Script import Import

Import("env", "projenv")
env = globals()["env"]
projenv = globals()["projenv"]

CI = check_env("CI", "false")

# See what happens in-between linking .cpp.o + .a into the resulting .elf
env.Append(ESPURNA_BUILD_FIRMWARE_MAP="${BUILD_DIR}/${PROGNAME}.map")
env.Append(LINKFLAGS="-Wl,-Map -Wl,$ESPURNA_BUILD_FIRMWARE_MAP -Wl,--cref")
env.SideEffect(env["ESPURNA_BUILD_FIRMWARE_MAP"], "${BUILD_DIR}/${PROGNAME}.elf")

# Always show warnings for project code
projenv.ProcessUnFlags("-w")

# XXX: note that this will also break %d format with floats and print raw memory contents as int
# Cores after 2.3.0 can disable %f in the printf / scanf to reduce .bin size
remove_float_support(env)
ldscripts_inject_libpath(env)

# compressed and two-step update hint when using 1MB boards
if not CI and env.get("UPLOAD_PROTOCOL") == "espota":
    env.AddPostAction(
        "${BUILD_DIR}/${PROGNAME}.bin",
        env.VerboseAction(check_binsize, "Checking maximum upload size $TARGET"),
    )

# disable postmortem printing to the uart. another one is in eboot, but this is what causes the most harm
if "DISABLE_POSTMORTEM_STACKDUMP" in itertools.chain(
    env["CPPFLAGS"], projenv["CPPFLAGS"]
):
    disable_postmortem_output(env)

# override static version flag from the espurna/config/version.h
# either completely, or change the version / revision / suffix part separately
app_inject_version(projenv)

# handle ESPURNA_BOARD and ESPURNA_FLAGS here, since projenv is not available in pre-scripts
# TODO: prefer PLATFORMIO_BUILD_SRC_FLAGS instead? both of these only allow -D...
app_inject_flags(projenv)

# handle when CI does a tagged build or user explicitly asked to store the firmware.bin
app_add_target_build_and_copy(projenv)

# handle special GzipFile builder
app_add_gzip_file(projenv)

# pre-processed single-source only makes sense from projenv
pre_process = env.get("ESPURNA_SINGLE_SOURCE_TARGET")
if pre_process:
    projenv.PreProcess(pre_process)

# hijack SCons signature generation to exclude BUILD_DIR, making our builds faster
# when using different environments (i.e. itead-sonoff-basic, itead-sonoff-pow, etc.)
cachedir_fix = check_env("ESPURNA_FIX_CACHEDIR_PATH", "n")
if cachedir_fix:
    app_patch_cachedir(projenv)
