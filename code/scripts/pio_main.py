# Run this script every time building an env:

from espurna_utils import (
    check_printsize,
    remove_float_support,
    ldscripts_inject_libpath,
    lwip_inject_patcher,
    app_inject_revision,
    dummy_ets_printf
)

Import("env", "projenv")

# Always show warnings for project code
projenv.ProcessUnFlags("-w")

# XXX: note that this will also break %d format with floats and print raw memory contents as int
# Cores after 2.3.0 can disable %f in the printf / scanf to reduce .bin size
remove_float_support(env)
ldscripts_inject_libpath(env)

# two-step update hint when using 1MB boards
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
