# coding=utf-8
# pylint: dummy-variables-rgx='(_+[a-zA-Z0-9]*?$)|dummy|env|projenv'
#
# Original extra_scripts.py
# Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
#
# ldscripts, lwip patching, updated postmortem flags and git support
# Copyright (C) 2019-2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

# Run this script every time building an env AFTER platform-specific code is loaded

from espurna_utils import (
    check_printsize,
    remove_float_support,
    ldscripts_inject_libpath,
    lwip_inject_patcher,
    app_inject_revision,
    dummy_ets_printf,
    app_inject_flags,
    copy_release,
)


Import("env", "projenv")

import os

CI = any([os.environ.get("TRAVIS"), os.environ.get("CI")])

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
        "$BUILD_DIR/FrameworkArduino/core_esp8266_postmortem.c.o", dummy_ets_printf
    )
    env.AddPostAction(
        "$BUILD_DIR/FrameworkArduino/core_esp8266_postmortem.cpp.o", dummy_ets_printf
    )

# patch lwip1 sources conditionally:
# https://github.com/xoseperez/espurna/issues/1610
lwip_inject_patcher(env)

# when using git, add -DAPP_REVISION=(git-commit-hash)
app_inject_revision(projenv)

# handle OTA board and flags here, since projenv is not available in pre-scripts
app_inject_flags(projenv)

# handle `-t release` when CI does a tagged build
env.AlwaysBuild(env.Alias("release", "${BUILD_DIR}/${PROGNAME}.bin", copy_release))
