import os

def ldscripts_inject_libpath(env):
    platform = env.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif8266")

    libpath_base = os.path.join("$PROJECT_DIR", "..", "dist", "ld")

    # we depend on different ldscript formats for old Core version
    # TODO: 1.5.0 needs to be removed next release
    # TODO: ldscript can be kept in the root of the repo
    #       (as this was done originally, during 2.3.0...2.4.2 times)

    if platform.version == "1.5.0":
        libpath_sdk = os.path.join(framework_dir, "tools", "sdk", "ld")
        env.Append(LIBPATH=[libpath_sdk])
        env.Prepend(LIBPATH=[os.path.join(libpath_base, "pre_2.5.0")])
    else:
        local_ld = env.subst(
            os.path.join("$BUILD_DIR", "ld", "local.eagle.app.v6.common.ld")
        )
        env.Prepend(LIBPATH=[os.path.join(libpath_base, "latest")])
