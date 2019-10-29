import os


def lwip_inject_patcher(env):
    # ignore when building with lwip2
    if "lwip_gcc" not in env["LIBS"]:
        return

    platform = env.PioPlatform()
    framework_dir = platform.get_package_dir("framework-arduinoespressif8266")
    toolchain_prefix = os.path.join(
        platform.get_package_dir("toolchain-xtensa"), "bin", "xtensa-lx106-elf-"
    )

    patch_action = env.VerboseAction(
        " ".join(
            [
                "-patch",
                "-u",
                "-N",
                "-d",
                os.path.join(framework_dir, "tools", "sdk", "lwip"),
                os.path.join("src", "core", "tcp_out.c"),
                env.subst(
                    os.path.join(
                        "$PROJECT_DIR",
                        "..",
                        "dist",
                        "patches",
                        "lwip_mtu_issue_1610.patch",
                    )
                ),
            ]
        ),
        "Patching lwip source",
    )
    build_action = env.VerboseAction(
        " ".join(
            [
                "make",
                "-C",
                os.path.join(framework_dir, "tools", "sdk", "lwip", "src"),
                "install",
                "TOOLS_PATH={}".format(toolchain_prefix),
                "LWIP_LIB=liblwip_gcc.a",
            ]
        ),
        "Rebuilding lwip",
    )

    patcher = env.Alias("patch-lwip", None, patch_action)
    builder = env.Alias("build-lwip", patcher, build_action)
    if os.environ.get("ESPURNA_PIO_PATCH_ISSUE_1610"):
        env.Depends("$BUILD_DIR/${PROGNAME}.elf", builder)
    env.AlwaysBuild(patcher)
    env.AlwaysBuild(builder)
