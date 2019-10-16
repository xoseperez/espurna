import os


def ldscripts_inject_libpath(env):

    platform = env.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif8266")

    # espressif8266@1.5.0 did not append this directory into the LIBPATH
    libpath_sdk = os.path.join(framework_dir, "tools", "sdk", "ld")
    env.Append(LIBPATH=[libpath_sdk])

    libpath_base = os.path.join("$PROJECT_DIR", "..", "dist", "ld")
    env.Append(LIBPATH=[os.path.join(libpath_base, "pre_2.5.0")])

    # local.eagle.app.v6.common.ld exists only with Core >2.5.0
    def check_local_ld(target, source, env):
        local_ld = env.subst(
            os.path.join("$BUILD_DIR", "ld", "local.eagle.app.v6.common.ld")
        )
        if os.path.exists(local_ld):
            env.Prepend(LIBPATH=[os.path.join(libpath_base, "latest")])

    env.AddPreAction(os.path.join("$BUILD_DIR", "firmware.elf"), check_local_ld)
