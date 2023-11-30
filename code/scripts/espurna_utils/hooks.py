# Override functions after build b/c neither runtime (api) or build (flags, order, etc.) has any way to do so
def disable_postmortem_output(env):
    env.AddPostAction(
        "$BUILD_DIR/FrameworkArduino/core_esp8266_postmortem.cpp.o",
        env.VerboseAction(
            "$OBJCOPY"
            " --redefine-sym ets_printf=__stub_printf"
            " --redefine-sym ets_printf_P=__stub_printf_P"
            " --redefine-sym ets_putc=__stub_putc"
            " $TARGET",
            "Overriding POSTMORTEM print functions from $TARGET",
        ),
    )


# newlib internal printf implementation declares these as weak, and neither symbol
# is exported in a way that usual build picks them up. remove implicit definitions
# to exclude them from the build completely
def remove_float_support(env):
    flags = " ".join(env["LINKFLAGS"])
    flags = flags.replace("-u _printf_float", "")
    flags = flags.replace("-u _scanf_float", "")
    newflags = flags.split()

    env.Replace(LINKFLAGS=newflags)
