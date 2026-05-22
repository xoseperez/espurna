Import("env")
import os


def _quote(p):
    """Wrap a path in double quotes so spaces survive cmd.exe / shell."""
    return '"{}"'.format(p)


def merge_bin(source, target, env):
    print("\n[POST-BUILD] Creating merged production binary...")

    platform = env.PioPlatform()
    esptool = os.path.join(platform.get_package_dir("tool-esptoolpy") or "", "esptool.py")
    if not os.path.exists(esptool):
        esptool = "esptool.py"  # fall back to PATH

    build_dir = env.subst("$BUILD_DIR")
    board = env.get("BOARD_MCU") or env.BoardConfig().get("build.mcu", "esp32")
    pio_env = env["PIOENV"]

    # Derive flash size from board config so non-4MB targets aren't silently truncated.
    flash_size = env.BoardConfig().get("upload.flash_size", "4MB")

    output = os.path.join(build_dir, "espurna-{}.bin".format(pio_env))

    bootloader = os.path.join(build_dir, "bootloader.bin")
    if not os.path.exists(bootloader):
        framework_dir = platform.get_package_dir("framework-arduinoespressif32")
        bootloader = os.path.join(framework_dir, "tools", "sdk", "bin", "bootloader_dout_40m.bin")

    partitions = os.path.join(build_dir, "partitions.bin")
    boot_app0 = os.path.join(build_dir, "boot_app0.bin")
    if not os.path.exists(boot_app0):
        framework_dir = platform.get_package_dir("framework-arduinoespressif32")
        boot_app0 = os.path.join(framework_dir, "tools", "partitions", "boot_app0.bin")

    firmware = os.path.join(build_dir, "firmware.bin")

    missing = [f for f in (bootloader, partitions, boot_app0, firmware) if not os.path.exists(f)]
    if missing:
        # Fail the PIO build so CI doesn't publish an un-merged binary as success.
        raise Exception("[POST-BUILD] Missing components for merge: {}".format(", ".join(missing)))

    cmd = (
        "$PYTHONEXE {esptool} --chip {chip} merge_bin -o {out} "
        "--flash_mode dout --flash_size {fsize} "
        "0x1000 {bootloader} "
        "0x8000 {partitions} "
        "0xe000 {boot_app0} "
        "0x10000 {firmware}"
    ).format(
        esptool=_quote(esptool),
        chip=board,
        out=_quote(output),
        fsize=flash_size,
        bootloader=_quote(bootloader),
        partitions=_quote(partitions),
        boot_app0=_quote(boot_app0),
        firmware=_quote(firmware),
    )

    print("Running: " + cmd)
    rc = env.Execute(cmd)
    if rc:
        raise Exception("[POST-BUILD] merge_bin failed with exit code {}".format(rc))
    print("[DONE] File ready: {}\n".format(output))


env.AddPostAction("$BUILD_DIR/firmware.bin", merge_bin)
